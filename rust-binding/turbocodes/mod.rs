use std::path::Path;
use crate::errors::ApiError;

/// Raw cxx bridge to the C++ samples reader.
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
        ) -> Result<ParsedSamples>;
    }
}

/// Reads a samples file (header `N D`, then `N` lines of `D` values; see the "File formats" page of the manual) and
/// returns one `Vec<f64>` per sample.
///
/// # Panics
/// If the path is not valid UTF-8. A C++ error (missing or malformed file) terminates the process.
pub fn parse_samples_file(
    file_path: impl AsRef<Path>,
) -> Result<Vec<Vec<f64>>, ApiError> {
    let file_path = match file_path.as_ref().to_str() {
        Some(p) => p,
        None => {
            return Err(ApiError::FailedToParseSamplesFile(format!(
                "couldnt parse config path",
            )))
        }
    };

    cxx::let_cxx_string!(filename = file_path);
    let parsed = match ffi::parse_samples_file_wrapper(&filename) {
        Ok(p) => p,
        Err(e) => {
            return Err(ApiError::FailedToParseSamplesFile(format!(
                "file {} : {}", &file_path, e.what()
            )))
        }
    };

    let mut pos = 0;
    let mut result = Vec::new();

    for len in parsed.lengths {
        result.push(
            parsed.values[pos..pos + len].to_vec()
        );

        pos += len;
    }

    Ok(result)
}



