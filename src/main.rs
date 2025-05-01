use anyhow::Result;
use clap::Parser;
use cli::Cli;
use cli_options::CliOptions;
use fatal::fatal;

mod cli;
mod cli_options;
mod common;
mod install_context;
mod models;

#[tokio::main]
async fn main() {
    if let Err(e) = run_app().await {
        fatal!("zepo: {}", e);
    }
}

async fn run_app() -> Result<()> {
    let options = CliOptions::parse();
    Cli::new(options)?.execute().await
}
