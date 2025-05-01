use std::{
    collections::HashMap,
    path::{Path, PathBuf},
    time::Duration,
};

use anyhow::{Ok, Result, anyhow};
use async_trait::async_trait;
use serde::de::DeserializeOwned;
use tokio::{
    fs::{self, read_to_string},
    sync::RwLock,
};

fn to_safe_filename(val: impl AsRef<str>) -> String {
    val.as_ref().replace('/', "__")
}

#[async_trait]
pub trait Fetcher {
    async fn fetch(&self, x: &str) -> Result<String>;
}

pub struct Cache {
    cache_dir: PathBuf,
    expire: Option<Duration>,
    fetcher: Option<Box<dyn Fetcher>>,
    memory_cache: Option<RwLock<HashMap<String, String>>>,
}

impl Cache {
    pub fn set_fetcher(&mut self, fetcher: Option<Box<dyn Fetcher>>) {
        self.fetcher = fetcher;
    }

    pub fn with_fetcher(mut self, fetcher: Option<Box<dyn Fetcher>>) -> Self {
        self.set_fetcher(fetcher);
        self
    }

    pub fn set_expire(&mut self, expire: Option<Duration>) {
        self.expire = expire;
    }

    pub fn with_expire(mut self, expire: Option<Duration>) -> Self {
        self.set_expire(expire);
        self
    }

    pub fn set_memory_cache_enabled(&mut self, enabled: bool) {
        self.memory_cache = if enabled {
            Some(RwLock::new(HashMap::new()))
        } else {
            None
        };
    }

    pub fn with_memory_cache_enabled(mut self, enabled: bool) -> Self {
        self.set_memory_cache_enabled(enabled);
        self
    }

    pub fn new(cache_path: impl AsRef<Path>) -> Self {
        Self {
            cache_dir: cache_path.as_ref().to_owned(),
            expire: Some(Duration::from_secs(3600 * 24 * 7)), // 7 days
            fetcher: None,
            memory_cache: None,
        }
    }

    async fn put_memory_cache(&self, name: &str, val: String) {
        if let Some(memory_cache) = &self.memory_cache {
            memory_cache.write().await.insert(name.to_owned(), val);
        }
    }

    async fn get_memory_cache(&self, name: &str) -> Option<String> {
        if let Some(memory_cache) = &self.memory_cache {
            let map = memory_cache.read().await;
            let cached = map.get(name);
            cached.cloned()
        } else {
            None
        }
    }

    pub async fn get_cache(&self, name: impl AsRef<str>) -> Result<String> {
        // from memory
        if let Some(cached) = self.get_memory_cache(name.as_ref()).await {
            return Ok(cached);
        }

        // from file
        let cached_file = self.cache_dir.join(to_safe_filename(&name));

        let expired = if !cached_file.exists() {
            true
        } else if self.expire.is_some() {
            let mod_time = cached_file.metadata()?.modified()?;
            mod_time.elapsed()? > self.expire.unwrap()
        } else {
            false
        };

        if !expired {
            let result = read_to_string(cached_file).await?;
            self.put_memory_cache(name.as_ref(), result.clone()).await;
            return Ok(result);
        }

        // fetch
        let Some(fetcher) = &self.fetcher else {
            return Err(anyhow!("No fetcher available"));
        };

        let result = fetcher.fetch(name.as_ref()).await?;
        fs::write(cached_file, &result).await?;
        self.put_memory_cache(name.as_ref(), result.clone()).await;

        Ok(result)
    }

    pub async fn get_cache_json<T: DeserializeOwned>(&self, name: impl AsRef<str>) -> Result<T> {
        let result = self.get_cache(name).await?;
        Ok(serde_json::from_str(result.as_str())?)
    }
}
