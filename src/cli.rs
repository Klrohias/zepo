use std::{
    collections::HashMap,
    env::{current_dir, current_exe},
    fs::{self, read_to_string},
    path::{Path, PathBuf},
    sync::Arc,
};

use anyhow::{Ok, Result, anyhow};
use pubgrub::Range;
use serde::{Deserialize, Serialize};

use crate::{
    cli_options::{CliCommands, CliOptions, ConfigSubcommands},
    install_context::InstallContext,
};

const PACKAGE_JSON_FILE: &str = "package.json";

fn find_root_dir() -> Result<PathBuf> {
    let mut cwd = current_dir()?;
    while cwd.join(PACKAGE_JSON_FILE).is_file() {
        cwd = cwd
            .parent()
            .ok_or(anyhow!("Cannot find package.json"))?
            .to_path_buf();
    }

    Ok(cwd)
}

pub struct AppPaths {
    home_dir: PathBuf,
    config_file: PathBuf,
    cache_dir: PathBuf,
    source_tree_dir: PathBuf,
}

impl AppPaths {
    fn default() -> Result<Self> {
        let home_dir = current_exe()?
            .parent()
            .ok_or(anyhow!("Cannot find config path"))?
            .to_path_buf();

        let config_file = home_dir.join("zepo-config.json");
        let cache_dir = home_dir.join("caches");
        let source_tree_dir = home_dir.join("sources");

        Ok(Self {
            home_dir,
            config_file,
            cache_dir,
            source_tree_dir,
        })
    }

    fn default_and_created() -> Result<Self> {
        let s = Self::default()?;

        if !s.source_tree_dir.exists() {
            fs::create_dir(&s.source_tree_dir)?;
        }
        if !s.cache_dir.exists() {
            fs::create_dir(&s.cache_dir)?;
        }

        Ok(s)
    }
}

#[derive(Serialize, Deserialize, Clone)]
pub struct Config(pub HashMap<String, String>);

impl Config {
    fn load(path: impl AsRef<Path>) -> Result<Config> {
        let path = path.as_ref();
        let file_content = read_to_string(&path)?;
        Ok(serde_json::from_str(&file_content)?)
    }
}

impl Default for Config {
    fn default() -> Self {
        let mut result = HashMap::new();
        result.insert(
            "registry".to_string(),
            "https://npm.pkg.github.com".to_string(),
        );

        Config(result)
    }
}

pub struct Cli {
    options: CliOptions,
    root_dir: Option<PathBuf>,
    paths: AppPaths,
    config: Arc<Config>,
}

impl Cli {
    pub fn new(options: CliOptions) -> Result<Cli> {
        let paths = AppPaths::default_and_created()?;

        // load config
        let config_path = &paths.config_file;
        if !config_path.is_file() {
            fs::write(config_path, serde_json::to_string(&Config::default())?)?;
        }
        let config = Arc::new(Config::load(config_path)?);

        // construct cli
        Ok(Cli {
            options,
            root_dir: find_root_dir().ok(),
            paths,
            config,
        })
    }

    pub async fn execute(&self) -> Result<()> {
        let options = &self.options;
        match &options.command {
            CliCommands::Install { .. } => self.execute_install().await,
            CliCommands::Remove { .. } => todo!(),
            CliCommands::Search { .. } => todo!(),
            CliCommands::Config(subcommand) => self.execute_config(subcommand).await,
        }
    }

    async fn execute_config(&self, subcommand: &ConfigSubcommands) -> Result<()> {
        match subcommand {
            ConfigSubcommands::Get { key } => {
                // get value from config HashMap
                if let Some(value) = self.config.0.get(key) {
                    println!("{}", value);
                } else {
                    return Err(anyhow!("Configuration key '{}' not found", key));
                }
            }
            ConfigSubcommands::Set { key, value } => {
                // create config file path
                let config_path = self.paths.home_dir.join("zepo-config.json");

                // update config in memory
                let mut config = self.config.0.clone();
                config.insert(key.clone(), value.clone());

                // write updated config to file
                fs::write(&config_path, serde_json::to_string(&Config(config))?)?;
                println!("Successfully set {} = {}", key, value);
            }
            ConfigSubcommands::List => {
                // list all config key-value pairs
                for (key, value) in &self.config.0 {
                    println!("{} = {}", key, value);
                }
            }
        }
        Ok(())
    }

    async fn execute_install(&self) -> Result<()> {
        let CliCommands::Install { packages, global } = &self.options.command else {
            return Err(anyhow!("CliCommands mismatch"));
        };

        // check options
        let is_restore = packages.is_empty();

        if is_restore && *global {
            return Err(anyhow!("Cannot restore globally"));
        }

        if self.root_dir == None && !*global {
            return Err(anyhow!(
                "Project not found, is there any package.json in current dir or parents?"
            ));
        }

        // install
        let mut ctx = InstallContext::new(&self.paths.cache_dir, self.config.clone())?;
        for package in packages {
            ctx.add_requirement(package.to_owned(), Range::full());
        }
        ctx.resolve().await?;

        Ok(())
    }
}
