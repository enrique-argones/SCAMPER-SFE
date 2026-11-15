pub mod turbocodes;
use std::path::Path;
use cxx::UniquePtr;


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

        fn load_user_extractor(
            general: &HybridStrengthenedTurboCodeFuzzyExtractor,
            folder: &str,
            user_id: &str,
        ) -> Result<UniquePtr<HybridStrengthenedTurboCodeFuzzyExtractor>>;    
    }
}

type Raw = ffi::HybridStrengthenedTurboCodeFuzzyExtractor;

pub struct HSTCFE {
    inner: UniquePtr<Raw>,
}

impl HSTCFE {

    pub fn new() -> Self {
        Self { inner: ffi::make_extractor(), }
    }

    pub fn valid_for_reproduce(&self) -> bool{
        self.inner.validForReproduce()
    }

    pub fn raw(&self) -> &Raw {
        self.inner.as_ref().expect("null extractor")
    }

    pub fn load_general_data(&mut self, config_path: &Path) {
        let config_path = config_path
            .to_str()
            .expect("configuration path is not valid UTF-8");

        cxx::let_cxx_string!(config_path_cpp = config_path);

        self.inner.pin_mut().loadGeneralData(&config_path_cpp);
    }

    pub fn set_user_id(&mut self, user_id: &str) {
        cxx::let_cxx_string!(user_id_cpp = user_id);

        self.inner.pin_mut().setUserId(&user_id_cpp);
    }

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

    pub fn reproduce(&mut self, samples: Vec<f64>) -> Vec<bool>{
        ffi::reproduce_wrapper(self.inner.pin_mut(), &samples)
    }

    pub fn turbocode_input_message(&mut self) -> Vec<bool>{
        ffi::turboCodeInputMessage_wrapper(self.inner.pin_mut())
    }

    pub fn save_user_specific_data(&mut self, folder: &Path, user_id: &str) -> Result<(), cxx::Exception> {
        std::fs::create_dir_all(folder).expect("could not create template folder");
        // The loader takes the user ID from the file name
        let file = folder.join(format!("{user_id}.userSpecificHSTCFE"));
        let file = file.to_str().expect("file path is not valid UTF-8");
        cxx::let_cxx_string!(file_cpp = file);
        self.inner.pin_mut().saveUserSpecificData(&file_cpp)
    }

    pub fn load_user_extractor(
        general: &Self,
        folder: &Path,
        user_id: &str,
    ) -> Result<Self, cxx::Exception> {
        let folder = folder.to_str().expect("folder path is not valid UTF-8");
        let inner = ffi::load_user_extractor(&general.inner, folder, user_id)
            .expect("it should work");
        Ok(Self {
            inner,
        })
    }
}

