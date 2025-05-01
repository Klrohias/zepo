use clap::{Parser, Subcommand};

#[derive(Parser)]
#[command(author, version, about, long_about = None)]
pub struct CliOptions {
    /// Specify the target
    #[clap(long, global = true, short = 'T')]
    pub(crate) target: Option<String>,

    #[command(subcommand)]
    pub(crate) command: CliCommands,
}

#[derive(Subcommand)]
pub enum CliCommands {
    /// Install specified packages
    #[clap(aliases = ["i", "add"])]
    Install {
        /// Install globally
        #[arg(long = "global", short = 'G')]
        global: bool,

        /// Package names
        packages: Vec<String>,
    },
    /// Remove specified packages
    Remove {
        /// Package names
        #[clap(required = true)]
        name: Vec<String>,
    },
    /// Search for packages
    Search {
        /// Search keyword
        #[clap(required = true)]
        keyword: String,
    },
    /// Manage zepo config
    #[command(subcommand)]
    Config(ConfigSubcommands),
}

#[derive(Subcommand)]
pub enum ConfigSubcommands {
    Get { key: String },
    Set { key: String, value: String },
    List,
}
