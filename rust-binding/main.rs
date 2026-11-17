//! Minimal example of the Rust binding: enrolls `subject_0002`, exports its template to memory, rebuilds a user
//! extractor from it and reproduces the key from the first probe sample. Prints `true` if the key is reproduced.
//!
//! Run with `cargo run --bin minimal`. Expects `generalConfigFile.txt` in the current folder and the samples files
//! `../fingerVeinData_196/testSet/{enrollment,probe}/subject_0002.txt`, relative to the repository folder.

use sfe::HSTCFE;
use std::path::PathBuf;

pub fn main() {
    let mut general = HSTCFE::new();
    let config_path = PathBuf::from("generalConfigFile.txt");
    general.load_general_data(&config_path);


    let save_dir = PathBuf::from(".");
    let samples_path = PathBuf::from(env!("CARGO_MANIFEST_DIR"))
            .join("..")
            .join("fingerVeinData_196")
            .join("testSet");

    let enrollment_path = samples_path.join("enrollment/subject_0002.txt");
    let enrollment = sfe::turbocodes::parse_samples_file(&enrollment_path);

    let probe_path = samples_path.join("probe/subject_0002.txt");
    let probes = sfe::turbocodes::parse_samples_file(&probe_path);

    general.set_user_id("subject_0002");
    general.generate(enrollment);
    let key_gen = general.turbocode_input_message();

    let user_specific_data = general.get_user_specific_data();

    let mut user = HSTCFE::load_user_specific_extractor(&general, user_specific_data);
    let key_gen_loaded = user.turbocode_input_message();

    if !general.valid_for_reproduce() { 
        println!("Not Valid");
        return;
    }

    let key_rep = user.reproduce(probes[0].clone());

    println!("{:#}", key_gen == key_rep);
}

