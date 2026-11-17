//! Rust binding of the SCAMPER SFE unimodal extractor (`HybridStrengthenedTurboCodeFuzzyExtractor`, library HSTCFE).
//!
//! The C++ classes are reached through the [cxx](https://cxx.rs) bridge declared in [`ffi`] and the C++ adapters in
//! `rust-binding/hstcfe/shim.cc`. [`HSTCFE`] is the safe wrapper to use: it loads the general configuration, enrolls a
//! subject, returns the user-specific helper data as a byte buffer, rebuilds a user extractor from that buffer and
//! reproduces the key from a probe sample. Keys are `Vec<bool>`, one element per bit.
//!
//! C++ exceptions thrown through the bridged functions terminate the process (none of them returns a cxx `Result`).
//!
//! **Research-only:** the helper data contains the secret key (label `TurboCodeInputMessage`); see the "Security notice"
//! of the README.
//!
//! See the "Rust binding" page of the SCAMPER SFE manual (`doc/pages/rust_binding.md`) for build instructions.

/// Reading of biometric samples files (wrapper of the C++ `parseSamplesFile`).
pub mod turbocodes;
use std::path::Path;
use cxx::UniquePtr;


/// Raw cxx bridge to the C++ extractor. Prefer the safe wrapper [`HSTCFE`].
#[cxx::bridge]
pub mod ffi {
    unsafe extern "C++" {
        include!("sfe/libraries/HSTCFE/hybridstrengthenedturbocodefuzzyextractor.h");
        include!("sfe/libraries/TurboCodes/turbocode.h");
        include!("sfe/libraries/TurboCodes/vectorfiles.h");

        type real_number_vector_vector;
        type real_number_vector;

        include!("sfe/rust-binding/hstcfe/shim.h");

        type HybridStrengthenedTurboCodeFuzzyExtractor;
        type binary_polynomial;
        
        pub fn make_extractor() 
            -> UniquePtr<HybridStrengthenedTurboCodeFuzzyExtractor>;

        fn loadGeneralData(
            self: Pin<&mut HybridStrengthenedTurboCodeFuzzyExtractor>,
            config_path: &CxxString,
        );

        fn setUserId(
            self: Pin<&mut HybridStrengthenedTurboCodeFuzzyExtractor>,
            user_id: &CxxString,
        );

        fn generate_wrapper(
            extractor: Pin<&mut HybridStrengthenedTurboCodeFuzzyExtractor>,
            enrollment_samples: &[f64],
            num_samples: &[usize],
        );

        fn validForReproduce(
            self: &HybridStrengthenedTurboCodeFuzzyExtractor,
        ) -> bool;

        fn reproduce_wrapper(
            extractor: Pin<&mut HybridStrengthenedTurboCodeFuzzyExtractor>,
            probe_sample:&[f64],
        ) -> Vec<bool>;

        fn turboCodeInputMessage_wrapper(
            extractor: Pin<&mut HybridStrengthenedTurboCodeFuzzyExtractor>,
        )  -> Vec<bool>;

        fn saveUserSpecificData(
            self: Pin<&mut HybridStrengthenedTurboCodeFuzzyExtractor>,
            file_name: &CxxString,
        ) -> Result<()>;

        pub fn copy_extractor(
            general: &HybridStrengthenedTurboCodeFuzzyExtractor,
        ) -> UniquePtr<HybridStrengthenedTurboCodeFuzzyExtractor>;

        fn get_user_specific_data_wrapper(
            fe: Pin<&mut HybridStrengthenedTurboCodeFuzzyExtractor>,
        ) -> Vec<u8>;


        fn load_user_specific_data_wrapper(
            fe: Pin<&mut HybridStrengthenedTurboCodeFuzzyExtractor>,
            data: Vec<u8>);
    }
}

type Raw = ffi::HybridStrengthenedTurboCodeFuzzyExtractor;

/// Owner of one C++ `HybridStrengthenedTurboCodeFuzzyExtractor`.
///
/// Typical use: one *general* extractor holds the configuration ([`HSTCFE::load_general_data`]); it enrolls a subject
/// ([`HSTCFE::set_user_id`], [`HSTCFE::generate`]) and exports its template ([`HSTCFE::get_user_specific_data`]); a *user*
/// extractor is later rebuilt from the general one and the template ([`HSTCFE::load_user_specific_extractor`]) to
/// reproduce the key ([`HSTCFE::reproduce`]).
pub struct HSTCFE {
    inner: UniquePtr<Raw>,
}

impl HSTCFE {

    /// Creates an empty extractor (C++ default constructor).
    pub fn new() -> Self {
        Self { inner: ffi::make_extractor(), }
    }

    /// Whether the enrolled template carries enough reliable information to reproduce a key (C++ `validForReproduce`).
    pub fn valid_for_reproduce(&self) -> bool{
        self.inner.validForReproduce()
    }

    /// Reference to the underlying C++ object.
    pub fn raw(&self) -> &Raw {
        self.inner.as_ref().expect("null extractor")
    }

    /// Loads the general configuration file (Turbo Code, bit-loading parameters, training set) and builds the
    /// normalizers (C++ `loadGeneralData`).
    ///
    /// # Panics
    /// If the path is not valid UTF-8. A C++ error (missing or malformed file) terminates the process.
    pub fn load_general_data(&mut self, config_path: &Path) {
        let config_path = config_path
            .to_str()
            .expect("configuration path is not valid UTF-8");

        cxx::let_cxx_string!(config_path_cpp = config_path);

        self.inner.pin_mut().loadGeneralData(&config_path_cpp);
    }

    /// Sets the subject identifier stored in the helper data (C++ `setUserId`).
    pub fn set_user_id(&mut self, user_id: &str) {
        cxx::let_cxx_string!(user_id_cpp = user_id);

        self.inner.pin_mut().setUserId(&user_id_cpp);
    }

    /// Enrolls a subject from its enrollment samples (one `Vec<f64>` per sample, all of the same dimensionality) and binds
    /// a new random key (C++ `generate`). Check [`HSTCFE::valid_for_reproduce`] afterwards.
    pub fn generate(&mut self, samples: Vec<Vec<f64>>) {
        let total_values: usize =
            samples.iter().map(Vec::len).sum();

        let mut values = Vec::with_capacity(total_values);
        let mut lengths = Vec::with_capacity(samples.len());

        for sample in samples {
            lengths.push(sample.len());
            values.extend_from_slice(&sample);
        }

        ffi::generate_wrapper(self.inner.pin_mut(), &values, &lengths);
    }

    /// Reproduces the key from one probe sample (C++ `reproduceBatch` with a single sample). The result equals
    /// [`HSTCFE::turbocode_input_message`] when the probe is close enough to the enrolled template.
    pub fn reproduce(&mut self, samples: Vec<f64>) -> Vec<bool>{
        ffi::reproduce_wrapper(self.inner.pin_mut(), &samples)
    }

    /// Returns the enrolled key (C++ `turboCodeInputMessage`). For evaluation only: a deployed system must not keep it.
    pub fn turbocode_input_message(&mut self) -> Vec<bool>{
        ffi::turboCodeInputMessage_wrapper(self.inner.pin_mut())
    }

    /*pub fn save_user_specific_data(&mut self, folder: &Path, user_id: &str) -> Result<(), cxx::Exception> {
        std::fs::create_dir_all(folder).expect("could not create template folder");
        // The loader takes the user ID from the file name
        let file = folder.join(format!("{user_id}.userSpecificHSTCFE"));
        let file = file.to_str().expect("file path is not valid UTF-8");
        cxx::let_cxx_string!(file_cpp = file);
        self.inner.pin_mut().saveUserSpecificData(&file_cpp)
    }*/

    /// Returns the user-specific helper data (C++ `getUserSpecificData`): the text of a `.userSpecificHSTCFE` file.
    ///
    /// The buffer also contains the secret key, for evaluation; a deployed system must not store it.
    pub fn get_user_specific_data(&mut self) -> Vec<u8>{
        ffi::get_user_specific_data_wrapper(self.inner.pin_mut())
 
    }
    /// Builds a user extractor: copies `general` (which must hold the general configuration) and loads the helper data
    /// returned by [`HSTCFE::get_user_specific_data`] (C++ `loadUserSpecificDataFromBlob`). A malformed buffer terminates
    /// the process.
    pub fn load_user_specific_extractor(general: &Self, data: Vec<u8>) -> Self{
        let mut user_extractor = ffi::copy_extractor(&general.inner);
        ffi::load_user_specific_data_wrapper(user_extractor.pin_mut(), data);
        Self {
            inner: user_extractor,
        }
    }
}

