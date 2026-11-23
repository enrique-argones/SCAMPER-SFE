//! Rust bindings for the Hybrid Strengthened Turbo Code Fuzzy Extractor (HSTCFE).
//!
//! A fuzzy extractor derives a stable key from noisy biometric data. At enrollment, it binds a
//! random key to a subject's samples and produces *helper data*. Later, a new sample from the same
//! subject (a *probe*), together with that helper data, reproduces the same key, even though the
//! probe is never exactly identical to the enrollment samples.
//!
//! This crate wraps the C++ class `HybridStrengthenedTurboCodeFuzzyExtractor`.
//!
//! # Overview
//!
//! - [`GeneralHSTCFE`] holds the general configuration. Create one with
//!   [`GeneralHSTCFE::from_config_file`], then use it to enroll subjects and to build user extractors.
//! - [`UserHSTCFE`] holds one subject's helper data and reproduces their key.
//! - [`Key`] is a key produced by enrollment or reproduction.
//! - [`UserPublicData`] is a subject's helper data, which you store after enrollment and load
//!   before reproduction.
//!
//! # Example
//!
//! ```no_run
//! use std::path::PathBuf;
//! use sfe::{GeneralHSTCFE, UserPublicData};
//! use sfe::turbocodes::parse_samples_file;
//!
//! # fn main() -> Result<(), Box<dyn std::error::Error>> {
//! let general = GeneralHSTCFE::from_config_file("generalConfigFile.txt")?;
//!
//! // Enrollment: bind a new key to the subject's samples and store the helper data.
//! let enrollment = parse_samples_file(&PathBuf::from("enrollment/subject_0002.txt"))?;
//! let (_, enrolled_key, helper_data) = general.enroll("subject_0002", &enrollment)?;
//! helper_data.save("subject_0002.userSpecificHSTCFE")?;
//!
//! // Verification, possibly later or in another program.
//! let helper_data = UserPublicData::load("subject_0002.userSpecificHSTCFE")?;
//! let mut user = GeneralHSTCFE::load_user_specific_extractor(&general, helper_data);
//!
//! let probes = parse_samples_file(&PathBuf::from("probe/subject_0002.txt"))?;
//! match user.reproduce(&probes[0]) {
//!     Some(key) if key == enrolled_key => println!("key reproduced"),
//!     Some(_) => println!("probe does not match"),
//!     None => println!("reproduction failed"),
//! }
//! # Ok(())
//! # }
//! ```
//!
//! # Errors
//!
//! Fallible functions return [`ApiError`]. When an error comes from the C++ code, the C++
//! exception message is included.
//!
//! # Security notes
//!
//! - A key returned by [`UserHSTCFE::reproduce`] is not guaranteed to be correct; see its
//!   documentation.
//! - For evaluation purposes, the helper data produced by the C++ code also contains the key.
//!   Do not store or share it as-is in a deployed system.
//! - [`UserHSTCFE::get_key`] exposes the enrolled key and is intended for evaluation only.
//!
//! # Feature flags
//!
//! - `sys`: exposes the raw C++ bindings as the `sys` module and enables
//!   [`GeneralHSTCFE::raw`]. Not covered by semver guarantees.
//!
//! # Threads
//!
//! The extractor types do not implement `Send` or `Sync`, so they cannot be moved or shared
//! between threads.
pub mod turbocodes;

mod hstcfe; 
/// Raw access to the C++ bindings.
#[cfg(feature = "sys")]
pub mod sys {
    pub use crate::hstcfe::ffi::*;
    pub use crate::turbocodes::ffi::*;
}

pub mod errors;
use errors::ApiError;

use std::path::Path;
use cxx::UniquePtr;
use std::fs;

type Raw = hstcfe::ffi::HybridStrengthenedTurboCodeFuzzyExtractor;

/// A key produced by [`GeneralHSTCFE::enroll`] or [`UserHSTCFE::reproduce`].
///
/// Keys can be compared with `==` to check whether a reproduction succeeded. The comparison is
/// not constant-time.
#[derive(Eq, PartialEq)]
pub struct Key {
    inner: Vec<bool>,
}
impl Key {
    /// Returns a copy of the key's bits.
    pub fn as_bits(&self) -> Vec<bool> {
        return self.inner.clone();
    }

    /// Returns the length of the key.
    pub fn len(&self) -> usize{
        return self.inner.len();
    }
}
/// A subject's helper data, produced at enrollment and needed to reproduce their key.
///
/// Store it after [`GeneralHSTCFE::enroll`], and load it into a user extractor with
/// [`GeneralHSTCFE::load_user_specific_extractor`].
pub struct UserPublicData {
    inner: Vec<u8>,
}
impl UserPublicData {
    /// Wraps bytes previously obtained from [`UserPublicData::as_bytes`] or a saved file.
    /// The contents are checked when the data is loaded into a user extractor.
    pub fn from_bytes(bytes: Vec<u8>) -> Self {
        Self { inner: bytes }
    }

    /// Returns the helper data as bytes, for example to store it in a database.
    pub fn as_bytes(&self) -> &[u8] {
        &self.inner
    }

    /// Returns the helper data as bytes, consuming `self`.
    pub fn into_bytes(self) -> Vec<u8> {
        self.inner
    }

    /// Writes the helper data to a file, replacing it if it exists.
    ///
    /// # Errors
    ///
    /// Returns an error if the file cannot be written.
    pub fn save(&self, path: impl AsRef<Path>) -> Result<(), ApiError> {
        fs::write(path, &self.inner)?;
        Ok(())
    }

    /// Reads helper data from a file written by [`UserPublicData::save`].
    ///
    /// The contents are not checked here; they are parsed when the data is loaded into a user
    /// extractor.
    ///
    /// # Errors
    ///
    /// Returns an error if the file cannot be read.
    pub fn load(path: impl AsRef<Path>) -> Result<Self, ApiError> {
        Ok(Self { inner: fs::read(path)? })
    }
}


/// A general extractor: holds the configuration shared by all subjects.
///
/// Use it to enroll subjects with [`GeneralHSTCFE::enroll`] and to build user extractors from
/// stored helper data with [`GeneralHSTCFE::load_user_specific_extractor`]. Neither operation
/// modifies the general extractor, so one instance can serve any number of subjects.
///
/// Cloning copies the loaded configuration, which is much faster than loading it again.
pub struct GeneralHSTCFE {
    inner: UniquePtr<Raw>,
}

/// A user extractor: holds one subject's helper data and reproduces their key.
///
/// Obtained from [`GeneralHSTCFE::enroll`] or [`GeneralHSTCFE::load_user_specific_extractor`].
pub struct UserHSTCFE {
    inner: UniquePtr<Raw>,
}

impl GeneralHSTCFE {
    /// Returns a reference to the underlying C++ object.
    ///
    /// Only available with the `sys` feature.
    #[cfg(feature = "sys")]
    pub fn raw(&self) -> &Raw {
        self.inner.as_ref().expect("null extractor")
    }


    /// Creates a general extractor by loading a configuration file.
    ///
    /// The file contains the following entries, in this order, separated by whitespace:
    ///
    /// ```text
    /// TurboCodeConfigFile             <path to the Turbo Code configuration file>
    /// TurboCodeMaxDecodingIterations  <integer>
    /// TrainSetSamplesFilesFolder      <path to the training set folder>
    /// CodeOffsetStdDevFactor          <number>
    /// CodeOffsetStdDevOffset          <number>
    /// FuzzyCommitmentMinReliability   <number>
    /// ```
    ///
    /// Because entries are separated by whitespace, the paths cannot contain spaces. Relative
    /// paths in the file are resolved from the current working directory.
    ///
    /// Loading builds the normalizers from the training set, which can take a while. The C++
    /// code prints progress messages to standard output.
    ///
    /// # Errors
    ///
    /// Returns [`ApiError::FailedToLoadGeneralConfig`] if the path is not valid UTF-8, if the
    /// file cannot be opened, or if its contents are invalid.
    pub fn from_config_file(config_path: impl AsRef<Path>) 
        -> Result<Self, ApiError> 
    {
        let mut inner = hstcfe::ffi::make_extractor();
        let config_path = match config_path.as_ref().to_str() {
            Some(p) => p,
            None => {
                return Err(ApiError::FailedToLoadGeneralConfig(format!(
                    "couldnt parse config path",
                )))
            }
        };
        
        cxx::let_cxx_string!(config_path_cpp = config_path);

        let res = inner.pin_mut().loadGeneralData(&config_path_cpp);
        match res {
            Ok(_) => Ok(Self {inner}),
            Err(e) => Err(ApiError::FailedToLoadGeneralConfig(format!(
                    "Failed to load general config file: {}",
                    e.what()
                )))
        }
    }

    /// Enrolls a subject: binds a new random key to the subject's samples.
    ///
    /// The general extractor is copied, not modified.
    ///
    /// Returns, in order:
    ///
    /// - a user extractor for this subject, ready to reproduce the key;
    /// - the enrolled key;
    /// - the helper data, to store for later reproduction.
    ///
    /// # Errors
    ///
    /// Returns [`ApiError::InvalidExtractor`] if the samples are not reliable enough to
    /// reproduce a key later. Enrolling again with better samples may succeed.
    pub fn enroll(&self, user_id: &str, samples: &[ impl AsRef<[f64]>]) 
        -> Result<(UserHSTCFE, Key, UserPublicData), ApiError>
    {
        cxx::let_cxx_string!(user_id_cpp = user_id);
        let mut new_inner = hstcfe::ffi::copy_extractor(&self.inner);

        new_inner.pin_mut().setUserId(&user_id_cpp);
        
        let total_values: usize =
            samples.iter().map(|s| s.as_ref().len()).sum();

        let mut values = Vec::with_capacity(total_values);
        let mut lengths = Vec::with_capacity(samples.len());

        for sample in samples {
            let s = sample.as_ref();
            lengths.push(s.len());
            values.extend_from_slice(s);
        }


        hstcfe::ffi::generate_wrapper(new_inner.pin_mut(), &values, &lengths);
        let key = hstcfe::ffi::turboCodeInputMessage_wrapper(new_inner.pin_mut());
        let user_data = hstcfe::ffi::get_user_specific_data_wrapper(new_inner.pin_mut());

        if !new_inner.validForReproduce() {
            return Err(ApiError::InvalidExtractor("While generating new UserExtractor".to_string()));
        }

        Ok((UserHSTCFE {inner: new_inner},
            Key{ inner:key}, 
            UserPublicData{inner: user_data}))
    }

    /// Builds a user extractor from a general extractor and a subject's helper data.
    ///
    /// The general extractor is copied, not modified. `data` should come from an extractor
    /// using the same configuration.
    pub fn load_user_specific_extractor(general: &Self, data: UserPublicData) -> UserHSTCFE{
        let mut user_extractor = hstcfe::ffi::copy_extractor(&general.inner);
        hstcfe::ffi::load_user_specific_data_wrapper(user_extractor.pin_mut(), data.inner);
        UserHSTCFE {
            inner: user_extractor,
        }
    }
}

impl UserHSTCFE {
    /// Returns a reference to the underlying C++ object.
    ///
    /// Only available with the `sys` feature.
    #[cfg(feature = "sys")]
    pub fn raw(&self) -> &Raw {
        self.inner.as_ref().expect("null extractor")
    }

    /// Reproduces the key from one probe sample.
    ///
    /// # Return value
    ///
    /// `None` is reserved for reproductions that are known to have failed. 
    pub fn reproduce(&mut self, samples: &[f64]) -> Option<Key> {
        let inner = hstcfe::ffi::reproduce_wrapper(self.inner.pin_mut(), samples);
        Some(Key { inner })
    }

    /// Returns the key stored in this extractor.
    pub fn get_key(&mut self) -> Key {
        let inner = hstcfe::ffi::turboCodeInputMessage_wrapper(self.inner.pin_mut());
        Key { inner } 
    }

    /// Returns this subject's helper data, the same as returned by [`GeneralHSTCFE::enroll`].
    pub fn get_user_specific_data(&mut self) -> UserPublicData {
        let inner = hstcfe::ffi::get_user_specific_data_wrapper(self.inner.pin_mut());
        UserPublicData {inner}
    }
}

impl std::clone::Clone for GeneralHSTCFE {
    fn clone(&self) -> Self {
        Self {
            inner: hstcfe::ffi::copy_extractor(&self.inner),
        }
    }
}

