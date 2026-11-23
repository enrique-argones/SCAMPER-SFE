use std::fmt;

#[derive(Debug)]
pub enum ApiError {
    InvalidExtractor(String),
    FailedToLoadGeneralConfig(String),
    FailedToParseSamplesFile(String),
    InvalidEnrollment,
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
