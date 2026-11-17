Guide for cryptographic integrators {#integrators_guide_page}
===================================

[TOC]

This page is for the engineers who use SCAMPER SFE as a cryptographic building block, for example to unlock or derive the
holder key of a self-sovereign identity wallet from a biometric sample. It describes the contract of the Rust API exposed by
the crate \c sfe (\ref rust_binding_page), which is the API of the minimal example \c rust-binding/main.rs, how to turn its
output into key material, and what is still missing before the extractor can protect real keys.

\warning SCAMPER SFE 1.0.0 is a research implementation (\ref td_security). The section \ref ig_gaps lists the issues that
**block a deployment**. Several of them, in particular the fixed-seed random generator and the key stored in the helper data,
cannot be worked around by the integrator: they must be fixed in the library.

Primitive {#ig_primitive}
=========

The extractor is a fuzzy extractor \cite dodis2008fuzzy with user-specific helper data
\cite argonesrua2024strengthened, \cite argonesrua2026exploiting. Its two procedures depend on public parameters \f$\Pi\f$
shared by all subjects: the general configuration file (\ref ff_general), the Turbo Code configuration file it names, and the
training set whose empirical CDFs uncouple the features.

- \f$\mathrm{Gen}(\mathbf{X}; \Pi) \to (\mathbf{b}, P)\f$ takes several enrollment samples \f$\mathbf{X}\f$ of one subject and outputs a random
  key \f$\mathbf{b} \in \{0,1\}^{N_I}\f$ and helper data \f$P\f$;
- \f$\mathrm{Rep}(\mathbf{x}', P; \Pi) \to \mathbf{b}^\prime\f$ takes one probe sample and returns a candidate key, equal to \f$\mathbf{b}\f$ when the probe
  is close enough to the enrollment samples.

Rep has **no failure output**. It always returns \f$N_I\f$ bits, whether the probe is genuine or not. Deciding whether
\f$\mathbf{b}^\prime = \mathbf{b}\f$ is the job of the integrator (\ref ig_verify).

The three objects have different roles:

| Object | API | Confidentiality | Integrity | Lifetime |
|--------|-----|-----------------|-----------|----------|
| Public parameters \f$\Pi\f$ | \c load_general_data | Public | Must be authentic and identical at Gen and Rep | System version |
| Helper data \f$P\f$ | \c get_user_specific_data | Public in the SFE model only once protected by the second factor; <b>currently contains the key</b> | Must be authentic | One enrollment |
| Key \f$\mathbf{b}\f$ | \c turbocode_input_message, \c reproduce | Secret | — | Transient |

API contract {#ig_api}
============

All the functions belong to \c sfe::HSTCFE, except \c sfe::turbocodes::parse_samples_file.

| Rust call | Precondition | Effect and result | On error |
|-----------|--------------|-------------------|----------|
| \c HSTCFE::new() | — | Empty extractor | — |
| \c load_general_data | Argument: \c &Path | Reads \f$\Pi\f$: parses the Turbo Code file and reads the whole training-set folder to build the normalizers. The most expensive call: do it once per process. | Missing or malformed file: **process abort**. Non-UTF-8 path: panic. |
| \c set_user_id(&str) | — | Identifier written in \f$P\f$ (label \c UserId). | — |
| \c generate | Argument: one \c Vec<f64> per sample. \f$\Pi\f$ loaded. Every sample has the training-set dimensionality \f$D\f$. | Gen: draws \f$\mathbf{b}\f$, builds \f$P\f$ inside the object. Overwrites any previous enrollment held by the object. | Inconsistent input: **process abort**. |
| \c valid_for_reproduce() | After \c generate or a load | \c false when the template has no capacity (\f$C = 0\f$): there is no key, the enrollment must be rejected. | — |
| \c get_user_specific_data() | After \c generate | \f$P\f$ as ASCII text (see the data types below). Includes \f$\mathbf{b}\f$ (\ref ig_gap_key). | — |
| \c turbocode_input_message() | After \c generate or a load | \f$\mathbf{b}\f$ as stored in the object. Use it <b>only right after</b> \c generate, to bind \f$\mathbf{b}\f$ to the application key. | — |
| \c HSTCFE::load_user_specific_extractor | Arguments: a general extractor holding \f$\Pi\f$ and \f$P\f$ as \c Vec<u8> | New extractor: copy of the general extractor (cheap: the normalizers are shared) with \f$P\f$ loaded. | Malformed \f$P\f$: **process abort**. |
| \c reproduce | Argument: one \c Vec<f64>. Loaded extractor with \c valid_for_reproduce() | Rep: soft Turbo decoding of one probe; returns \f$\mathbf{b}^\prime\f$, \f$N_I\f$ bits. | Inconsistent \f$P\f$: **process abort**. |
| \c raw() | — | Reference to the C++ object, for diagnostics. | — |
| \c turbocodes::parse_samples_file | Argument: \c &Path | Samples of a file in the format of \ref ff_samples, one \c Vec<f64> per sample. | Missing or malformed file: **process abort**. |

Data types and sizes:

- **Samples.** \c Vec<f64> of length \f$D\f$, the dimensionality of the training set. The C++ code stores them as 32-bit
  \c float. The values must be finite: the shim does not check them.
- **Enrollment.** Several samples per subject. The intra-subject standard deviation of every feature is estimated from them;
  with a single sample, or with identical samples, it is zero (see \ref ig_gap_sigma).
- **Key.** \c Vec<bool>, index \f$i\f$ is bit \f$i\f$ of the Turbo Code input message. Its length
  \f$N_I = \lceil C\,k/n \rceil\f$ depends on the capacity \f$C\f$ of the template and on the code rate \f$k/n\f$ of
  \f$\Pi\f$ (1/3 to 1/6). It differs between subjects and between two enrollments of the same subject. \f$N_I\f$ is stored
  in \f$P\f$ and is therefore public.
- **Helper data.** \c Vec<u8> holding the text of a \c .userSpecificHSTCFE file (\ref ff_userspecific). The values are separated by whitespace, so
  the user identifier must not contain whitespace. \f$P\f$ can only be loaded by an extractor holding the same \f$\Pi\f$: a
  different \f$\Pi\f$ is not detected and makes Rep return wrong keys.

Integration pattern {#ig_flow}
===================

Enrollment:

1. Load \f$\Pi\f$ once per process into a *general* extractor, and compute a digest of \f$\Pi\f$ (configuration file, Turbo Code
   file and training-set files) to identify the parameter version.
2. Capture several samples, call \c set_user_id with an opaque identifier (random, not a name or a DID: it is stored in
   \f$P\f$), then \c generate. If \c valid_for_reproduce() is \c false, reject the enrollment.
3. Read \f$\mathbf{b}\f$ with \c turbocode_input_message() and derive the application key from it (\ref ig_kdf). Register the
   public part of the derived key (\ref ig_verify).
4. Store \f$P\f$, the KDF salt and the digest of \f$\Pi\f$ together, protected as in \ref ig_gaps.
5. Erase \f$\mathbf{b}\f$, the derived secret, the enrollment samples and the enrolling object as soon as possible.

Reproduction:

1. Check that the stored digest equals the digest of the loaded \f$\Pi\f$.
2. Build the extractor with \c HSTCFE::load_user_specific_extractor(&general, P) and call \c reproduce(probe).
3. Derive the application key from \f$\mathbf{b}^\prime\f$ with the stored salt and accept only if it matches the registered public
   key.
4. **Never** call \c turbocode_input_message() on an extractor built from \f$P\f$ in a deployed flow: it returns the key read from
   \f$P\f$, which is how the minimal example checks its result, and it bypasses the biometric entirely.

Key derivation {#ig_kdf}
==============

Do not use \f$\mathbf{b}\f$ directly as a key. Its length varies, it is a bit vector rather than bytes, and its effective
strength is bounded by the biometric (\ref ig_verify), not by \f$N_I\f$. Encode it canonically and feed it to a KDF such as
HKDF \cite rfc5869, with a random salt stored next to \f$P\f$ and a context string that binds the use and the version of
\f$\Pi\f$. The encoding below (4-byte big-endian \f$N_I\f$, then the bits most-significant first, zero-padded) is a
recommendation of this guide, not part of the crate: whichever encoding is chosen must be fixed for the lifetime of the
templates.

\code{.unparsed}
// Cargo.toml: hkdf = "0.12", sha2 = "0.10", ed25519-dalek = "2", zeroize = "1", getrandom = "0.2"
use ed25519_dalek::{SigningKey, VerifyingKey};
use hkdf::Hkdf;
use sha2::Sha256;
use zeroize::Zeroizing;

/// Canonical encoding of an SFE key: 4-byte big-endian bit count, then the bits MSB-first, zero-padded.
fn encode_key(bits: &[bool]) -> Zeroizing<Vec<u8>> {
    let mut out = Zeroizing::new(Vec::with_capacity(4 + bits.len().div_ceil(8)));
    out.extend_from_slice(&(bits.len() as u32).to_be_bytes());
    for chunk in bits.chunks(8) {
        let mut byte = 0u8;
        for (i, &bit) in chunk.iter().enumerate() {
            if bit { byte |= 0x80 >> i; }
        }
        out.push(byte);
    }
    out
}

/// Ed25519 signing key derived from an SFE key. `params` is the digest of the public parameters.
fn derive_signing_key(sfe_key: &[bool], params: &[u8; 32], salt: &[u8; 32]) -> SigningKey {
    let ikm = encode_key(sfe_key);
    let hk = Hkdf::<Sha256>::new(Some(salt), &ikm);
    let mut info = b"SCAMPER-SFE/v1/ed25519".to_vec();
    info.extend_from_slice(params);
    let mut seed = Zeroizing::new([0u8; 32]);
    hk.expand(&info, seed.as_mut()).expect("valid HKDF-SHA256 output length");
    SigningKey::from_bytes(&seed)
}

/// Enrollment: fresh salt and public key to register.
fn bind(sfe_key: &[bool], params: &[u8; 32]) -> ([u8; 32], VerifyingKey) {
    let mut salt = [0u8; 32];
    getrandom::getrandom(&mut salt).expect("OS random source unavailable");
    (salt, derive_signing_key(sfe_key, params, &salt).verifying_key())
}

/// Reproduction: the signing key if the candidate SFE key matches the registered public key.
fn unlock(candidate: &[bool], params: &[u8; 32], salt: &[u8; 32], registered: &VerifyingKey) -> Option<SigningKey> {
    let sk = derive_signing_key(candidate, params, salt);
    (sk.verifying_key() == *registered).then_some(sk)
}
\endcode

The calls of the extractor around it:

\code{.unparsed}
// Enrollment
general.set_user_id(&opaque_id);
general.generate(enrollment_samples);
if !general.valid_for_reproduce() { /* reject, capture again */ }
let (salt, vk) = bind(&general.turbocode_input_message(), &params);
let helper_data = general.get_user_specific_data();       // store with salt and params; register vk

// Reproduction
let mut user = HSTCFE::load_user_specific_extractor(&general, helper_data);
let candidate = user.reproduce(probe);
match unlock(&candidate, &params, &salt, &vk) { Some(sk) => { /* sign */ }, None => { /* reject */ } }
\endcode

The salt separates the keys derived from different enrollments, but it does not add entropy: if \f$\mathbf{b}\f$ is predictable
(\ref ig_gap_rng), so is the derived key.

Verification and strength {#ig_verify}
=========================

Rep cannot tell whether it succeeded, so the integrator needs a verifier. The pattern above stores a public key derived from
\f$\mathbf{b}\f$ and compares public keys, which needs no constant-time comparison. A hash of \f$\mathbf{b}\f$ stored with
\f$P\f$ works the same way. A comparison with secret values (for example the stored key in the minimal example) must use a
constant-time comparison.

Any such verifier, together with \f$P\f$, is an **offline oracle**. An attacker holding both can run Rep on biometric samples of
other people (or synthetic ones) and test each result locally.

- Each attempt succeeds with probability close to the false acceptance rate (FAR) measured for the configuration. The work of an
  offline attack is therefore of the order of \f$1/\mathrm{FAR}\f$ Rep calls, which is far below \f$2^{N_I}\f$. The FAR, not
  \f$N_I\f$, is the figure to report as the strength of the binding.
- The SFE design removes this oracle by protecting \f$P\f$ with a second authentication factor \cite argonesrua2019efficient,
  which is not implemented (\ref ig_gap_factor). Until it is, keep \f$P\f$ and the verifier where an attacker cannot read
  them together, or verify online with rate limiting.
- Without the second factor, the helper data of two enrollments of the same subject are linkable, and \f$P\f$ reveals which
  features of the subject are reliable.

- **Integrity.** \f$P\f$ is not authenticated: a modified \f$P\f$ is either rejected by an abort or silently changes what Rep
  computes. Sign or MAC \f$P\f$, the salt and the parameter digest as one record at enrollment, and check them before calling
  \c load_user_specific_extractor.

Operational constraints {#ig_operations}
=======================

- **Threads.** \c HSTCFE is neither \c Send nor \c Sync (an opaque C++ type behind \c cxx::UniquePtr). Create the extractors in
  the thread that uses them. \c reproduce modifies the decoder state of its object, so one object must not be shared between
  concurrent calls. \c generate draws from a random generator shared by the whole process and protected by a mutex.
- **No clone.** The binding has no way to copy a general extractor without loading \f$P\f$. Enrolling with the general object
  itself (as the minimal example does) leaves \f$\mathbf{b}\f$ of the last enrolled subject inside it, and that state is copied
  into every extractor built from it until it is overwritten.
- **Errors.** None of the bridged functions returns a \c Result: every C++ exception aborts the process. Validate the inputs
  (paths, dimensionality, finite values, number of enrollment samples) before the call, and treat \f$P\f$ that has not been
  authenticated as untrusted input that can crash the process.
- **Timing.** The Turbo decoder stops as soon as its component decoders agree, so a genuine probe usually takes fewer iterations
  than an impostor probe. The running time of Rep therefore leaks whether the probe was close to the template.
- **Memory hygiene.** Keys and helper data are returned in ordinary \c Vec, and the C++ objects keep \f$\mathbf{b}\f$ in memory;
  nothing is zeroized. Wrap copies in \c zeroize::Zeroizing and drop the extractors as soon as possible.
- **Console output.** \c build.rs links the Debug libraries, which are compiled with \c DEBUG defined. In that configuration the
  libraries print to standard output, among others, the PSK symbols committed at enrollment (which encode codeword bits), the
  normalized biometric values and the decoder state. Never run a Debug build on real biometric data, and do not collect its
  standard output.

Deployment blockers {#ig_gaps}
===================

| Id | Issue | Consequence | Fix |
|----|-------|-------------|-----|
| G1 | Fixed-seed Mersenne Twister (\ref ig_gap_rng) | Keys, interleavers and permutations are predictable | Library |
| G2 | Key stored in the helper data (\ref ig_gap_key) | Whoever reads \f$P\f$ gets \f$\mathbf{b}\f$ | Library |
| G3 | Second factor not implemented (\ref ig_gap_factor) | Offline oracle, linkable templates | Library and integrator |
| G4 | Exceptions abort the process | Denial of service from bad inputs or \f$P\f$ | Binding |
| G5 | Debug build prints secrets | Leakage through logs | Binding (\c build.rs) |
| G6 | Zero-variance features (\ref ig_gap_sigma) | Unstable decoding for some templates | Library |
| G7 | No zeroization, no clone, no timing protection | Residual secrets, side channel | Binding and integrator |
| G8 | \c build.rs portability (\ref rust_build) | Build fails on Debian/Ubuntu and in release mode | Binding |

Random generator (G1) {#ig_gap_rng}
---------------------

All the random values of Gen (the key bits, the interleavers and the symbol permutation) are drawn from one
\c std::mt19937 seeded with the compile-time constant \c RAND_SEED. Every process therefore draws the same sequence, and the randomness of an
enrollment depends only on the enrollments made before it in the same process: for example, the first subjects enrolled by two
processes get the same key whenever their key lengths \f$N_I\f$ are equal.
The Mersenne Twister is also not a cryptographic generator \cite matsumoto1998mersenne. No integrator-side KDF can repair this.
The library must draw these values from the operating system's cryptographic random source.

Key in the helper data (G2) {#ig_gap_key}
---------------------------

\c get_user_specific_data writes \f$\mathbf{b}\f$ under the label \c TurboCodeInputMessage, and the parser requires that label.
The library needs a serialization of \f$P\f$ without the key. Until then, \f$P\f$ must be handled as a secret, which defeats the
purpose of a fuzzy extractor.

Second factor (G3) {#ig_gap_factor}
------------------

The Strengthened Fuzzy Extractor protects \f$P\f$ with a fixed second authentication factor \cite argonesrua2019efficient,
which makes the protected templates of a subject unlinkable and renewable and prevents the offline attack of \ref ig_verify.
The evaluations of this software correspond to a disclosed second factor \cite argonesrua2024strengthened. The protection of
\f$P\f$ is not part of the library yet.

Zero-variance features (G6) {#ig_gap_sigma}
---------------------------

When all the enrollment samples of a subject give the same normalized value for a feature (for example because they all fall
beyond the range of the training set), its standard deviation is zero. Such a feature gets the maximum capacity and a
zero-variance noise model, and the soft decoding of its bits becomes degenerate. Use several distinct enrollment samples, and a
training set that covers the range of the features. A variance floor in the library is under consideration.

Testing checklist {#ig_testing}
=================

- Enroll and reproduce in **different** extractors, built from the serialized \f$P\f$, as in production (the C++ test
  \c inmemory_reproduce checks that the enrolling object, its copy and a reloaded \f$P\f$ decode identically).
- Measure the false rejection and false acceptance rates of the integrated flow on a held-out set, with the production
  \f$\Pi\f$; report the FAR as the strength of the binding.
- Check the distribution of \f$N_I\f$ and the rate of \c valid_for_reproduce() \c == \c false enrollments on the target
  population.
- Check that a changed \f$\Pi\f$ is detected through the parameter digest before Rep is called.
- Build in release mode and check that nothing is printed to standard output.
