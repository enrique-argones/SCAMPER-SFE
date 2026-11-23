//! Minimal example of the Rust binding: enrolls `subject_0002`, exports its template to memory, rebuilds a user
//! extractor from it and reproduces the key from the first probe sample. Prints `true` if the key is reproduced.
//!
//! Run with `cargo run --bin minimal`. Expects `generalConfigFile.txt` in the current folder and the samples files
//! `../fingerVeinData_196/testSet/{enrollment,probe}/subject_0002.txt`, relative to the repository folder.

use sfe::GeneralHSTCFE;
use sfe::UserPublicData;
use std::path::PathBuf;

pub fn main() -> Result<(), Box<dyn std::error::Error>> {
    let general = GeneralHSTCFE::from_config_file("generalConfigFile.txt")?;

    let samples_path = PathBuf::from(env!("CARGO_MANIFEST_DIR"))
            .join("..")
            .join("fingerVeinData_196")
            .join("testSet");

    let enrollment_path = samples_path.join("ienrollment/subject_0002.txt");
    let enrollment = sfe::turbocodes::parse_samples_file(&enrollment_path)?;
    let (mut user, key_gen, _upd) = general.enroll("subject_0002", &enrollment)?;

    let probe_path = samples_path.join("probe/subject_0002.txt");
    let probes = sfe::turbocodes::parse_samples_file(&probe_path)?;
    let Some(key_rep) = user.reproduce(&probes[0]) else {
        return Err("Key was not reproduced correctly".into());
    };

    println!("{:#}", key_gen == key_rep);
    Ok(())
}

