pub mod turbocodes;

mod hstcfe; 
/// Raw access to the C++ bindings. Not covered by semver guarantees.
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

#[derive(Eq, PartialEq)]
pub struct Key {
    inner: Vec<bool>,
}
impl Key {
    pub fn as_bits(&self) -> Vec<bool> {
        return self.inner.clone();
    }

    pub fn len(&self) -> usize{
        return self.inner.len();
    }
}

pub struct UserPublicData {
    inner: Vec<u8>,
}
impl UserPublicData {
    /// Wraps bytes previously obtained from [`UserPublicData::as_bytes`] or a saved file.
    /// The contents are checked when the data is loaded into a user extractor.
    pub fn from_bytes(bytes: Vec<u8>) -> Self {
        Self { inner: bytes }
    }

    pub fn as_bytes(&self) -> &[u8] {
        &self.inner
    }

    pub fn into_bytes(self) -> Vec<u8> {
        self.inner
    }

    pub fn save(&self, path: impl AsRef<Path>) -> Result<(), ApiError> {
        fs::write(path, &self.inner)?;
        Ok(())
    }

    pub fn load(path: impl AsRef<Path>) -> Result<Self, ApiError> {
        Ok(Self { inner: fs::read(path)? })
    }
}

pub struct GeneralHSTCFE {
    inner: UniquePtr<Raw>,
}

pub struct UserHSTCFE {
    inner: UniquePtr<Raw>,
}

impl GeneralHSTCFE {
    /// Reference to the underlying C++ object.
    #[cfg(feature = "sys")]
    pub fn raw(&self) -> &Raw {
        self.inner.as_ref().expect("null extractor")
    }

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

    /// Sets the subject identifier stored in the helper data (C++ `setUserId`).
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

    pub fn load_user_specific_extractor(general: &Self, data: UserPublicData) -> UserHSTCFE{
        let mut user_extractor = hstcfe::ffi::copy_extractor(&general.inner);
        hstcfe::ffi::load_user_specific_data_wrapper(user_extractor.pin_mut(), data.inner);
        UserHSTCFE {
            inner: user_extractor,
        }
    }
}

impl UserHSTCFE {
    /// Reference to the underlying C++ object.
    pub fn raw(&self) -> &Raw {
        self.inner.as_ref().expect("null extractor")
    }

    pub fn reproduce(&mut self, samples: &[f64]) -> Option<Key> {
        let inner = hstcfe::ffi::reproduce_wrapper(self.inner.pin_mut(), samples);
        Some(Key { inner })
    }

    pub fn get_key(&mut self) -> Key {
        let inner = hstcfe::ffi::turboCodeInputMessage_wrapper(self.inner.pin_mut());
        Key { inner } 
    }

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

