Rust binding {#rust_binding_page}
============

[TOC]

The Cargo package \c sfe (version 0.2), at the root of the repository, exposes the unimodal extractor
(\ref HybridStrengthenedTurboCodeFuzzyExtractor) to Rust through the [cxx](https://cxx.rs) bridge. It enrolls a subject,
returns the key and the user-specific helper data, and reproduces the key from a probe sample. The helper data can be kept in
memory as bytes or written to a \c .userSpecificHSTCFE file. The multimodal extractor and the evaluation programs are not
exposed.

The Rust API does not copy the C++ class method by method: it uses separate types for the general and the user extractors,
enrolls in one call and returns Rust errors where the C++ code throws. The reasons are given in \c doc/api_changes.md. The
\ref integrators_guide_page describes how to use the API to protect real keys.

The binding was written by João Azevedo (merge request \c rust-bindings, November 2026).

Layout {#rust_layout}
======

| Path | Content |
|------|---------|
| \c Cargo.toml, \c Cargo.lock | Package \c sfe: library \c rust-binding/lib.rs and example program \c minimal (\c rust-binding/main.rs). Feature \c sys. |
| \c build.rs | Builds the C++ libraries with CMake and compiles the cxx bridges and the C++ shims. |
| \c rust-binding/lib.rs | Public types \c GeneralHSTCFE, \c UserHSTCFE, \c Key and \c UserPublicData. |
| \c rust-binding/errors.rs | Error type \c sfe::errors::ApiError. |
| \c rust-binding/hstcfe/mod.rs | cxx bridge to the C++ extractor class (private; public as \c sfe::sys with the feature \c sys). |
| \c rust-binding/hstcfe/shim.h, \c shim.cc | C++ adapters between Rust slices and vectors and the extractor types. |
| \c rust-binding/turbocodes/mod.rs | Module \c sfe::turbocodes: \c parse_samples_file, a wrapper of \ref parseSamplesFile. |
| \c rust-binding/turbocodes/shim.h, \c shim.cc | C++ adapter of \ref parseSamplesFile. |
| \c rust-binding/main.rs | Minimal example: enrollment and reproduction. |
| \c tests/rust-interoperability.rs | Integration tests of the binding (\ref rust_tests). |
| \c doc/api_changes.md | Differences between the Rust API and the C++ class, and known limitations. |
| \c flake.nix, \c flake.lock | Optional Nix development shell (Rust, CMake, Armadillo, argagg, gdb). |

Requirements {#rust_requirements}
============

- Rust 1.85 or newer with Cargo (the package uses edition 2024).
- The dependencies of the C++ libraries (see \ref build_deps): a C++17 compiler, CMake 3.23 or newer and Armadillo.
  argagg is not needed, since the programs are not built.

The crates \c cxx, \c cxx-build and \c cmake are downloaded by Cargo. Nix users can enter the development shell with
\c nix \c develop.

Building {#rust_build}
========

    cargo build                  # library and the "minimal" example
    cargo run --bin minimal      # runs the example (see below)
    cargo test                   # integration tests (see below)
    cargo doc --open             # API documentation of the crate

\c build.rs builds ThreadPool, GMMLite, TurboCodes and HSTCFE with the \c cmake crate, always in the CMake \c Release
configuration, each in its own folder of the Cargo build directory (<tt>target/PROFILE/build/sfe-HASH/out</tt>). It then
compiles the cxx bridges of \c hstcfe/mod.rs and \c turbocodes/mod.rs together with the two shims (with \c -O3 and \c NDEBUG),
links the four libraries and Armadillo dynamically, and adds the library folders to the run path of the executable. The C++
libraries do not have to be installed beforehand. Because the Release libraries are used, the binding prints none of the
verbose traces of the Debug libraries, only the progress messages of \c GeneralHSTCFE::from_config_file.

\warning In the current version of \c build.rs:
- the library folders are hard-coded as \c lib64, which is where CMake installs libraries on Fedora-like systems. On
  Debian and Ubuntu CMake uses \c lib, and the link fails with <tt>unable to find library -lTurboCodes</tt>; replacing
  \c lib64 with \c lib in \c build.rs works around it;
- the run path points to the Cargo build folder, so the executable cannot be moved to another machine.

Usage {#rust_usage}
=====

\code{.unparsed}
use sfe::{GeneralHSTCFE, UserPublicData};
use sfe::turbocodes::parse_samples_file;

// Shared configuration: Turbo Code, bit-loading parameters and training-set normalizers.
let general = GeneralHSTCFE::from_config_file("generalConfigFile.txt")?;

// Enrollment: one Vec<f64> per sample. Err if the template cannot carry a key.
let enrollment = parse_samples_file("enrollment/subject_0002.txt")?;
let (_user, enrolled_key, helper_data) = general.enroll("subject_0002", &enrollment)?;
helper_data.save("subject_0002.userSpecificHSTCFE")?;          // or helper_data.into_bytes()

// Reproduction, possibly in another program: a user extractor rebuilt from the helper data.
let helper_data = UserPublicData::load("subject_0002.userSpecificHSTCFE")?;
let mut user = GeneralHSTCFE::load_user_specific_extractor(&general, helper_data);
let probes = parse_samples_file("probe/subject_0002.txt")?;
let Some(key) = user.reproduce(&probes[0]) else { return Err("reproduction failed".into()) };
let accepted = key == enrolled_key;                              // evaluation only
\endcode

| Rust | C++ |
|------|-----|
| \c GeneralHSTCFE::from_config_file(path) | Default constructor, then \ref HybridStrengthenedTurboCodeFuzzyExtractor::loadGeneralData "loadGeneralData" |
| \c GeneralHSTCFE::clone() | Copy constructor (the normalizers are shared) |
| \c GeneralHSTCFE::enroll(id, samples) | Copy constructor, \ref HybridStrengthenedTurboCodeFuzzyExtractor::setUserId "setUserId", \ref HybridStrengthenedTurboCodeFuzzyExtractor::generate "generate", \ref HybridStrengthenedTurboCodeFuzzyExtractor::turboCodeInputMessage "turboCodeInputMessage", \ref HybridStrengthenedTurboCodeFuzzyExtractor::getUserSpecificData "getUserSpecificData" and \ref HybridStrengthenedTurboCodeFuzzyExtractor::validForReproduce "validForReproduce" |
| \c GeneralHSTCFE::load_user_specific_extractor(general, data) | Copy constructor, then \ref HybridStrengthenedTurboCodeFuzzyExtractor::loadUserSpecificDataFromBlob "loadUserSpecificDataFromBlob" |
| \c UserHSTCFE::reproduce(probe) | \ref HybridStrengthenedTurboCodeFuzzyExtractor::reproduceBatch "reproduceBatch" with one probe |
| \c UserHSTCFE::get_key() | \ref HybridStrengthenedTurboCodeFuzzyExtractor::turboCodeInputMessage "turboCodeInputMessage" |
| \c UserHSTCFE::get_user_specific_data() | \ref HybridStrengthenedTurboCodeFuzzyExtractor::getUserSpecificData "getUserSpecificData" |
| \c GeneralHSTCFE::raw(), \c UserHSTCFE::raw() | Reference to the underlying C++ object (feature \c sys) |
| \c turbocodes::parse_samples_file(path) | \ref parseSamplesFile |

- \c GeneralHSTCFE holds the general configuration. Neither \c enroll nor \c load_user_specific_extractor modifies it,
  so one instance serves any number of subjects.
- \c UserHSTCFE holds the helper data of one subject. It is returned by \c enroll and by
  \c load_user_specific_extractor.
- \c Key holds the bits of a key; \c as_bits returns them as \c Vec<bool>, one element per bit, and \c len gives their
  number. Two keys are compared with \c ==.
- \c UserPublicData holds the helper data, in the text format of a \c .userSpecificHSTCFE file:
  \c save and \c load write and read such a file, and \c from_bytes, \c as_bytes and \c into_bytes convert it from and to
  bytes. Its contents are only parsed by \c load_user_specific_extractor. Files written by \c save can also be read by the C++
  programs; the format is described in \ref ff_userspecific.

\c reproduce returns an \c Option: \c None is reserved for reproductions known to have failed, but the current version always
returns \c Some. A wrong key is only detected by comparing it with a reference; see \ref ig_verify.

The extractor types are neither \c Send nor \c Sync: they cannot be moved or shared between threads.

Errors {#rust_errors}
------

The fallible functions return a \c Result whose error type is \c sfe::errors::ApiError:

| Variant | Returned by | Cause |
|---------|-------------|-------|
| \c FailedToLoadGeneralConfig | \c GeneralHSTCFE::from_config_file | Path not valid UTF-8, file missing or malformed (with the C++ message). |
| \c InvalidExtractor | \c GeneralHSTCFE::enroll | The template has no capacity (\ref HybridStrengthenedTurboCodeFuzzyExtractor::validForReproduce "validForReproduce" is false): enroll again with better samples. |
| \c FailedToParseSamplesFile | \c turbocodes::parse_samples_file | Path not valid UTF-8, file missing or malformed (with the path and the C++ message). |
| \c IOError | \c UserPublicData::save, \c UserPublicData::load | File cannot be written or read. |
| \c InvalidEnrollment | — | Not returned by the current version. |

\note The other bridged calls are not declared with a \c Result return type. A C++ exception thrown by them therefore
terminates the process, as cxx specifies: in particular, \c load_user_specific_extractor with malformed helper data, and
\c enroll or \c reproduce with samples of the wrong dimensionality.

Minimal example {#rust_example}
===============

\c cargo \c run \c --bin \c minimal enrolls \c subject_0002 and reproduces its key from the first probe sample with the user
extractor returned by \c enroll, and prints \c true when the reproduced key equals the enrolled one. It expects:

- \c generalConfigFile.txt in the current folder (\ref ff_general);
- the samples files <tt>subject_0002.txt</tt> in <tt>../fingerVeinData_196/testSet/enrollment</tt> and
  <tt>../fingerVeinData_196/testSet/probe</tt>, relative to the repository folder (\ref ff_samples).

Tests {#rust_tests}
=====

\c cargo \c test runs the two tests of \c tests/rust-interoperability.rs, which use the same files as the minimal example
(\c generalConfigFile.txt in the repository folder):

- \c reproduces_key_from_stored_helper_data enrolls \c subject_0002, rebuilds a user extractor from the helper data with
  \c load_user_specific_extractor and reproduces the key from the first probe sample;
- \c reproduces_key_with_enrolled_extractor reproduces the key with the user extractor returned by \c enroll.

Security {#rust_security}
========

The binding inherits the research-only caveats of the C++ code (\ref td_security): the helper data returned by \c enroll and
written by \c UserPublicData::save contains the secret key (label \c TurboCodeInputMessage), and \c UserHSTCFE::get_key returns
it. A deployed system must remove the key from the stored helper data and must not keep it in memory longer than needed;
\c Key and \c UserPublicData are not erased when they are dropped. The \ref integrators_guide_page describes how to derive and
verify application keys and lists the issues that block a deployment.
