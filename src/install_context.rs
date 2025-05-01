use std::{collections::HashMap, fs, path::Path, sync::Arc};

use anyhow::{Ok, Result, anyhow};
use async_trait::async_trait;
use pubgrub::{Range, SemanticVersion};
use reqwest::Client;

use crate::{
    cli::Config,
    common::{Cache, Fetcher},
    models::NpmVersionList,
};

const METADATAS_CACHE_NAME: &str = "metadatas";

pub struct InstallContext {
    requirements: HashMap<String, Range<SemanticVersion>>,
    metadata_cache: Cache,
}

impl InstallContext {
    pub fn new(cache_path: impl AsRef<Path>, config: Arc<Config>) -> Result<Self> {
        let metadatas_cache_dir = cache_path.as_ref().join(METADATAS_CACHE_NAME);
        if !metadatas_cache_dir.exists() {
            fs::create_dir(&metadatas_cache_dir)?;
        }

        let cache = Cache::new(metadatas_cache_dir)
            .with_memory_cache_enabled(true)
            .with_fetcher(Some(Box::new(NpmFetcher::new(config))));

        Ok(Self {
            requirements: HashMap::new(),
            metadata_cache: cache,
        })
    }

    pub fn add_requirement(&mut self, package_name: String, version_range: Range<SemanticVersion>) {
        self.requirements.insert(package_name, version_range);
    }

    pub async fn resolve(&self) -> Result<()> {
        let a = self
            .metadata_cache
            .get_cache_json::<NpmVersionList>("is-even")
            .await?;

        println!("{:?}", a);

        Ok(())
    }
}

struct NpmFetcher {
    config: Arc<Config>,
    http_client: Client,
}

impl NpmFetcher {
    fn new(config: Arc<Config>) -> Self {
        Self {
            config,
            http_client: Client::new(),
        }
    }
}

impl NpmFetcher {
    /// find suitable registry
    fn get_registry(&self, name: &str) -> Option<&String> {
        let scope = if name.starts_with('@') && name.contains('/') {
            // scoped
            Some(name.split_at(name.find('/').unwrap()).0)
        } else {
            None
        };

        if let Some(scope_name) = scope {
            let config_key = format!("{}:registry", scope_name);
            self.config.0.get(&config_key)
        } else {
            self.config.0.get("registry")
        }
    }
}

#[async_trait]
impl Fetcher for NpmFetcher {
    async fn fetch(&self, name: &str) -> Result<String> {
        let registry = self
            .get_registry(name)
            .ok_or(anyhow!("No registry available for {}", name))?;

        let response = self
            .http_client
            .get(format!("{}/{}", registry, name))
            .send()
            .await?;

        Ok(response.text().await?)
    }
}
