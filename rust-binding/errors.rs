//! The error type returned by the fallible functions of this crate.
use std::fmt;

/// Errors returned by this crate.
#[derive(Debug)]
pub enum ApiError {
    /// The enrollment samples are not reliable enough to reproduce a key later.
    ///
    /// Returned by [`GeneralHSTCFE::enroll`](crate::GeneralHSTCFE::enroll). Enrolling again
    /// with better samples may succeed. The string describes where the error occurred.
    InvalidExtractor(String),

    /// The general configuration could not be loaded.
    ///
    /// Returned by [`GeneralHSTCFE::from_config_file`](crate::GeneralHSTCFE::from_config_file)
    /// when the path is not valid UTF-8, the file cannot be opened, or its contents are invalid.
    FailedToLoadGeneralConfig(String),

    /// A samples file could not be read.
    ///
    /// Returned by [`parse_samples_file`](crate::turbocodes::parse_samples_file) when the path
    /// is not valid UTF-8, the file cannot be opened, or its contents do not match the expected
    /// format. The string includes the file path when the error comes from the C++ parser.
    FailedToParseSamplesFile(String),

    /// The enrollment is invalid.
    ///
    /// Not currently returned by any function in this crate.
    InvalidEnrollment,

    /// An I/O operation failed, such as reading or writing a helper data file.
    ///
    /// Returned by [`UserPublicData::save`](crate::UserPublicData::save) and
    /// [`UserPublicData::load`](crate::UserPublicData::load). The string is the message of the
    /// underlying [`std::io::Error`].
    IOError(String),
}

impl From<std::io::Error> for ApiError {
    fn from(e: std::io::Error) -> Self {
        ApiError::IOError(e.to_string())
    }
}
impl fmt::Display for ApiError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            ApiError::InvalidExtractor(msg) => write!(f, "{msg}"),
            ApiError::FailedToLoadGeneralConfig(msg) => write!(f, "{msg}"),
            ApiError::FailedToParseSamplesFile(msg) => write!(f, "{msg}"),
            ApiError::InvalidEnrollment => write!(f, "Invalid Enrollment"),
            ApiError::IOError(msg) => write!(f, "{msg}"),
        }
    }
}

impl std::error::Error for ApiError {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        match self {
            ApiError::InvalidExtractor(_) => None,
            ApiError::FailedToLoadGeneralConfig(_) => None,
            ApiError::FailedToParseSamplesFile(_) => None,
            ApiError::InvalidEnrollment => None,
            ApiError::IOError(_) => None,
        }
    }
}
