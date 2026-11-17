Rust binding {#rust_binding_page}
============

[TOC]

The Cargo package \c sfe, at the root of the repository, exposes the unimodal extractor
(\ref HybridStrengthenedTurboCodeFuzzyExtractor) to Rust through the [cxx](https://cxx.rs) bridge. It enrolls a subject, keeps
the user-specific helper data in memory as a byte buffer, and reproduces the key from a probe sample, without writing any
template file. The multimodal extractor and the evaluation programs are not exposed.

The binding was contributed by João Azevedo (merge request \c rust-bindings, November 2026).

Layout {#rust_layout}
======

| Path | Content |
|------|---------|
| \c Cargo.toml, \c Cargo.lock | Package \c sfe: library \c rust-binding/lib.rs and example program \c minimal (\c rust-binding/main.rs). |
| \c build.rs | Builds the C++ libraries with CMake and compiles the cxx bridges and the C++ shims. |
| \c rust-binding/lib.rs | Bridge to \ref HybridStrengthenedTurboCodeFuzzyExtractor and the safe wrapper \c sfe::HSTCFE. |
| \c rust-binding/hstcfe/shim.h, \c shim.cc | C++ adapters between Rust slices and vectors and the extractor types. |
| \c rust-binding/turbocodes/mod.rs | Module \c sfe::turbocodes: \c parse_samples_file, a wrapper of \ref parseSamplesFile. |
| \c rust-binding/turbocodes/shim.h, \c shim.cc | C++ adapter of \ref parseSamplesFile. |
| \c rust-binding/main.rs | Minimal example: enrollment, in-memory template, reproduction. |
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

    cargo build                  # library and the "minimal" example, in target/debug
    cargo run --bin minimal      # runs the example (see below)

\c build.rs builds ThreadPool, GMMLite, TurboCodes and HSTCFE with the \c cmake crate, each in its own folder of
<tt>target/PROFILE/build/sfe-HASH/out</tt>, then compiles the cxx bridges of \c lib.rs and \c turbocodes/mod.rs together with
the two shims, links the four libraries and Armadillo dynamically, and adds the library folders to the run path of the
executable. The C++ libraries do not have to be installed beforehand.

\warning In the current version of \c build.rs:
- the library folders are hard-coded as \c lib64, which is where CMake installs libraries on Fedora-like systems. On
  Debian and Ubuntu CMake uses \c lib, and the link fails with <tt>unable to find library -lTurboCodes_Debug</tt>;
- the library names are hard-coded with the \c _Debug suffix, which CMake only adds in Debug builds, so
  <tt>cargo build --release</tt> fails at link time on every system;
- the run path points to the Cargo build folder, so the executable cannot be moved to another machine.

Usage {#rust_usage}
=====

\code{.unparsed}
use sfe::HSTCFE;
use std::path::Path;

// Shared configuration: Turbo Code, bit-loading parameters and training-set normalizers.
let mut general = HSTCFE::new();
general.load_general_data(Path::new("generalConfigFile.txt"));

// Enrollment: one Vec<f64> per enrollment sample.
let enrollment = sfe::turbocodes::parse_samples_file(Path::new("enrollment/subject_0002.txt"));
general.set_user_id("subject_0002");
general.generate(enrollment);
if !general.valid_for_reproduce() {
    return; // not enough reliable information in the template
}
let template: Vec<u8> = general.get_user_specific_data();   // helper data, in memory

// Reproduction: a new extractor built from the general data and the stored template.
let probes = sfe::turbocodes::parse_samples_file(Path::new("probe/subject_0002.txt"));
let mut user = HSTCFE::load_user_specific_extractor(&general, template);
let key: Vec<bool> = user.reproduce(probes[0].clone());
let accepted = key == user.turbocode_input_message();
\endcode

| Rust (\c sfe::HSTCFE) | C++ |
|-----------------------|-----|
| \c new() | Default constructor |
| \c load_general_data(path) | \ref HybridStrengthenedTurboCodeFuzzyExtractor::loadGeneralData |
| \c set_user_id(id) | \ref HybridStrengthenedTurboCodeFuzzyExtractor::setUserId |
| \c generate(samples) | \ref HybridStrengthenedTurboCodeFuzzyExtractor::generate |
| \c valid_for_reproduce() | \ref HybridStrengthenedTurboCodeFuzzyExtractor::validForReproduce |
| \c reproduce(probe) | \ref HybridStrengthenedTurboCodeFuzzyExtractor::reproduceBatch with one probe |
| \c turbocode_input_message() | \ref HybridStrengthenedTurboCodeFuzzyExtractor::turboCodeInputMessage |
| \c get_user_specific_data() | \ref HybridStrengthenedTurboCodeFuzzyExtractor::getUserSpecificData |
| \c load_user_specific_extractor(general, data) | Copy constructor, then \ref HybridStrengthenedTurboCodeFuzzyExtractor::loadUserSpecificDataFromBlob |
| \c raw() | Reference to the underlying C++ object |

Keys are returned as \c Vec<bool>, one element per bit. The template returned by \c get_user_specific_data has the text format
of a \c .userSpecificHSTCFE file (\ref ff_userspecific), so it can also be written to disk and read by the C++ programs.

\note None of the bridged functions is declared with a \c Result return type, except the unused \c saveUserSpecificData. A C++
exception thrown by them (for example a missing configuration file, or a malformed template) therefore terminates the process,
as cxx specifies, instead of being returned as a Rust error.

Minimal example {#rust_example}
===============

\c cargo \c run \c --bin \c minimal enrolls \c subject_0002 and reproduces its key from the first probe sample, through an
in-memory template, and prints \c true when the reproduced key equals the enrolled one. It expects:

- \c generalConfigFile.txt in the current folder (\ref ff_general);
- the samples files <tt>subject_0002.txt</tt> in <tt>../fingerVeinData_196/testSet/enrollment</tt> and
  <tt>../fingerVeinData_196/testSet/probe</tt>, relative to the repository folder (\ref ff_samples).

Because the example links the Debug libraries, it also prints the verbose traces of the decoder.

Security {#rust_security}
========

The binding inherits the research-only caveats of the C++ code (\ref td_security): the template returned by
\c get_user_specific_data contains the secret key (label \c TurboCodeInputMessage), and \c turbocode_input_message returns it.
A deployed system must remove the key from the stored template and must not keep it in memory longer than needed.
The \ref integrators_guide_page describes how to derive and verify application keys and lists the issues that block a
deployment.
