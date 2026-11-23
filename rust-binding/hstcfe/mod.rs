


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
        ) -> Result<()>;

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
