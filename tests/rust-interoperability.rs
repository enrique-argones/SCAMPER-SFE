use std::error::Error;
use std::path::PathBuf;

use sfe::turbocodes::parse_samples_file;
use sfe::GeneralHSTCFE;

const SUBJECT: &str = "subject_0002";

fn load_general() -> Result<GeneralHSTCFE, Box<dyn Error>> {
    Ok(GeneralHSTCFE::from_config_file(
        PathBuf::from(env!("CARGO_MANIFEST_DIR"))
            .join("generalConfigFile.txt"))?)
}

fn load_samples(folder: &str) -> Result<Vec<Vec<f64>>, Box<dyn Error>> {
    let path = PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("..")
        .join("fingerVeinData_196")
        .join("testSet")
        .join(folder)
        .join(format!("{SUBJECT}.txt"));
    Ok(parse_samples_file(&path)?)
}

#[test]
fn reproduces_key_from_stored_helper_data() -> Result<(), Box<dyn Error>> {
    let general = load_general()?;
    let enrollment = load_samples("enrollment")?;
    let probes = load_samples("probe")?;
    let probe = probes.first().ok_or("probe file contains no samples")?;

    let (_, enrolled_key, helper_data) = general.enroll(SUBJECT, &enrollment)?;
    //helper_data.save("subject_0002.userSpecificHSTCFE")?;

    //let helper_data = UserPublicData::load("subject_0002.userSpecificHSTCFE")?;
    let mut user = GeneralHSTCFE::load_user_specific_extractor(&general, helper_data);

    let reproduced = user.reproduce(probe).ok_or("reproduce returned None")?;
    assert!(reproduced == enrolled_key, "reproduced key differs from the enrolled key");
    Ok(())
}

#[test]
fn reproduces_key_with_enrolled_extractor() -> Result<(), Box<dyn Error>> {
    let general = load_general()?;
    let enrollment = load_samples("enrollment")?;
    let probes = load_samples("probe")?;
    let probe = probes.first().ok_or("probe file contains no samples")?;

    let (mut user, enrolled_key, _) = general.enroll(SUBJECT, &enrollment)?;

    let reproduced = user.reproduce(probe).ok_or("reproduce returned None")?;
    assert!(reproduced == enrolled_key, "reproduced key differs from the enrolled key");
    Ok(())
}
