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

/// Reads a samples file and returns one `Vec<f64>` per sample.
///
/// The file starts with a header line containing the number of samples `N` and the number of
/// values per sample `D`, followed by `N` lines of `D` values each, separated by whitespace.
///
/// See the "File formats" page of the manual for details.
///
/// The result can be passed directly to [`GeneralHSTCFE::enroll`](crate::GeneralHSTCFE::enroll),
/// and each of its elements to [`UserHSTCFE::reproduce`](crate::UserHSTCFE::reproduce).
///
/// # Errors
///
/// Returns [`ApiError::FailedToParseSamplesFile`] 
///     - if the path is not valid UTF-8, 
///     - if the file cannot be opened, or
///     - if its contents do not match the format above.
/// When the error comes from the C++ parser,
/// the message includes the file path and the C++ error message.
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



