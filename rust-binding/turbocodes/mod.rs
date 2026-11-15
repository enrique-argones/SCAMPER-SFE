use std::path::Path;

#[cxx::bridge]
pub mod ffi {
    struct ParsedSamples {
        values: Vec<f64>,
        lengths: Vec<usize>,
    }

    unsafe extern "C++" {
        include!("sfe/libraries/TurboCodes/turbocode.h");
        include!("sfe/libraries/TurboCodes/vectorfiles.h");
        include!("sfe/rust-binding/turbocodes/shim.h");

        type real_number_vector_vector;
        type real_number_vector;

        fn parse_samples_file_wrapper(
            samples_file_name: &CxxString,
        ) -> ParsedSamples;
    }
}

pub fn parse_samples_file(
    file_path: &Path,
) -> Vec<Vec<f64>> {
    let file_path = file_path
        .to_str()
        .expect("configuration path is not valid UTF-8");

    cxx::let_cxx_string!(filename = file_path);
    let parsed = ffi::parse_samples_file_wrapper(&filename);

    let mut pos = 0;
    let mut result = Vec::new();

    for len in parsed.lengths {
        result.push(
            parsed.values[pos..pos + len].to_vec()
        );

        pos += len;
    }

    result
}



