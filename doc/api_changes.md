# Why the Rust API differs from the C++ one

This crate wraps the C++ class HybridStrengthenedTurboCodeFuzzyExtractor.
Instead of copying the C++ interface method by method,
the Rust API is reshaped so that rules the C++ code only documents
are enforced by the types.

## Quick mapping
| C++ |	Rust |
| --- | --- | 
| Constructor + loadGeneralData	| GeneralHSTCFE::from_config_file|
| setUserId + generate + validForReproduce + getters |GeneralHSTCFE::enroll|
| Copy + loadUserSpecificDataFromBlob | GeneralHSTCFE::load_user_specific_extractor|
| reproduce	| UserHSTCFE::reproduce |
| Exceptions |Result<_, ApiError>|

## The main changes

### Two types instead of one

The C++ class plays two roles, depending on which methods have been called on it:
- General extractor: holds only the configuration 
    (Turbo Code parameters and the normalizers built from the training set).
    This is what you have after loadGeneralData.
- User extractor: 
    the configuration plus one subject's helper data. 
    This is what you have after setUserId and generate,
    or after loadUserSpecificDataFromBlob.

In C++, every method is available in both roles, because it's a single class.
Calling reproduce on a general extractor compiles,
even though it makes no sense,
because there's no enrolled template to reproduce the key from.

In Rust, each role is its own type: GeneralHSTCFE and UserHSTCFE.
reproduce is only available on UserHSTCFE,
because only a user extractor holds the helper data needed to reproduce a key.
enroll is only available on GeneralHSTCFE,
because enrollment starts from the configuration and creates a new user extractor,
rather than modifying an existing one.

Both types wrap the same C++ object, so the split costs nothing at runtime.

### Configuration is loaded on construction

from_config_file is the only constructor, because an empty, unconfigured extractor is never useful.
Every GeneralHSTCFE is ready to use, because there's no way to create one without a configuration.

### Enrollment is one call

enroll replaces five C++ calls, because their order matters and is easy to get wrong.
It checks template validity itself, because forgetting that check silently produces a template that can't reproduce the key.
It returns the user extractor, key and helper data directly, because results should come back from the operation instead of being fetched afterwards.
It takes &self and works on a copy, because one general extractor should serve any number of enrollments.

### Errors instead of exceptions

Fallible functions return Result, because an uncaught C++ exception terminates the whole program.
C++ error messages are kept inside ApiError, because they explain what went wrong.

> Note that there might still be functions that can return uncaught exceptions


Hidden FFI, opt-in raw access

The cxx bindings are private,
because then the C++ side can change without breaking users.
The `sys` feature exposes them anyway,
because some use cases need direct access.

## Example
```rust
let general = GeneralHSTCFE::from_config_file("generalConfigFile.txt")?;

// Enroll and store the helper data
let (_enrolled, key, helper_data) = general.enroll("subject_0002", &enrollment_samples)?;
helper_data.save("subject_0002.userSpecificHSTCFE")?;

// Later: rebuild the user extractor and reproduce the key
let helper_data = UserPublicData::load("subject_0002.userSpecificHSTCFE")?;
let mut user = GeneralHSTCFE::load_user_specific_extractor(&general, helper_data);

match user.reproduce(&probe) {
    Some(reproduced) if reproduced == key => println!("key reproduced"),
    Some(_) => println!("probe does not match"),
    None => println!("reproduction failed"),
}
```

## Known limitations
- Loading malformed helper data still terminates the process.
- reproduce always returns Some for now.
- Sample lengths aren't validated on the Rust side.
- Paths must be valid UTF-8.
- The types aren't Send or Sync yet.
- No encryption of the UserPublicData yet
