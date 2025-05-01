use std::collections::HashMap;

use serde::Deserialize;

#[derive(Deserialize, Debug)]
pub struct NpmVersionList {
    versions: HashMap<String, NpmPackage>,
}

#[derive(Deserialize, Debug)]
pub struct NpmPackage {
    name: String,
    version: String,
    dist: Option<NpmRegistryDist>,
}

#[derive(Deserialize, Debug)]
pub struct NpmRegistryDist {
    shasum: String,
    tarball: String,
    size: usize,
}
