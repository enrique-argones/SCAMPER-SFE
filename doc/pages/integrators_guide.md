Guide for cryptographic integrators {#integrators_guide_page}
===================================

[TOC]

This page is for the engineers who use SCAMPER SFE as a cryptographic building block, for example to unlock or derive the
holder key of a self-sovereign identity wallet from a biometric sample. It describes the contract of the Rust API of the crate
\c sfe 0.2 (types \c GeneralHSTCFE, \c UserHSTCFE, \c Key, \c UserPublicData and \c ApiError), how to turn its output into key
material, and what is still missing before the extractor can protect real keys. The reasons for the shape of this API, compared
with the C++ class, are explained in \c doc/api_changes.md; the build of the crate is described in \ref rust_binding_page.

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

Rep has **no failure output**. It always returns \f$N_I\f$ bits, whether the probe is genuine or not: in \c sfe 0.2,
\c UserHSTCFE::reproduce returns an \c Option, but it is always \c Some. Deciding whether \f$\mathbf{b}^\prime = \mathbf{b}\f$ is the job of
the integrator (\ref ig_verify).

The three objects have different roles:

| Object | Rust type | Confidentiality | Integrity | Lifetime |
|--------|-----------|-----------------|-----------|----------|
| Public parameters \f$\Pi\f$ | \c GeneralHSTCFE | Public | Must be authentic and identical at Gen and Rep | System version |
| Helper data \f$P\f$ | \c UserPublicData | Public in the SFE model only once protected by the second factor; <b>currently contains the key</b>, despite the name of the type | Must be authentic | One enrollment |
| Key \f$\mathbf{b}\f$ | \c Key | Secret | — | Transient |

API contract {#ig_api}
============

The types are exported at the root of the crate, except \c sfe::turbocodes::parse_samples_file and \c sfe::errors::ApiError.
"Abort" means that a C++ exception crosses the bridge without being converted into an \c ApiError, which terminates the
process.

| Rust call | Arguments and preconditions | Effect and result | On error |
|-----------|-----------------------------|-------------------|----------|
| \c GeneralHSTCFE::from_config_file | Path of the general configuration file. | Reads \f$\Pi\f$: parses the Turbo Code file and reads the whole training-set folder to build the normalizers. The most expensive call: do it once per thread (\ref ig_operations). | \c Err(FailedToLoadGeneralConfig): missing or malformed file, non-UTF-8 path. |
| \c GeneralHSTCFE::clone | — | Copy of the general extractor. Cheap: the normalizers are shared. | — |
| \c GeneralHSTCFE::enroll | User identifier, enrollment samples (one \c [f64] slice each, of dimensionality \f$D\f$). | Gen on a copy of the general extractor, which is not modified. Returns the enrolled \c UserHSTCFE, the \c Key and the \c UserPublicData. | \c Err(InvalidExtractor) if the template has no capacity (\f$C = 0\f$): reject the enrollment. Wrong dimensionality: <b>abort</b>. |
| \c GeneralHSTCFE::load_user_specific_extractor | General extractor, \c UserPublicData. | New \c UserHSTCFE: copy of the general extractor with \f$P\f$ loaded. | Malformed \f$P\f$: <b>abort</b>. |
| \c UserHSTCFE::reproduce | One probe sample of dimensionality \f$D\f$. | Rep: soft Turbo decoding; returns \c Some(Key), \f$N_I\f$ bits, whatever the probe. | Wrong dimensionality: <b>abort</b>. |
| \c UserHSTCFE::get_key | — | \f$\mathbf{b}\f$ as stored in the object, that is, read from \f$P\f$. **Never use it in a deployed flow.** | — |
| \c UserHSTCFE::get_user_specific_data | — | \f$P\f$ of the object, as \c UserPublicData. | — |
| \c UserHSTCFE::raw, \c GeneralHSTCFE::raw | \c GeneralHSTCFE::raw needs the feature \c sys. | Reference to the C++ object, for diagnostics. | — |
| \c Key::as_bits, \c Key::len | — | Copy of the bits as \c Vec<bool>; \f$N_I\f$. | — |
| \c UserPublicData::from_bytes, \c as_bytes, \c into_bytes | — | Conversion from and to bytes, without any check: \f$P\f$ is only parsed by \c load_user_specific_extractor. | — |
| \c UserPublicData::save, \c load | Path. | Writes or reads the bytes of \f$P\f$ to or from a file (the \c .userSpecificHSTCFE format). | \c Err(IOError). |
| \c turbocodes::parse_samples_file | Path. | Samples of a file in the format of \ref ff_samples, one \c Vec<f64> per sample. | \c Err(FailedToParseSamplesFile). |

The feature \c sys exposes the raw cxx bridge as \c sfe::sys, without stability guarantees. An integration should not need it.

Data types and sizes:

- **Samples.** \c f64 slices of length \f$D\f$, the dimensionality of the training set. The C++ code stores them as 32-bit
  \c float. The binding does not check the length (a wrong one aborts the process) nor that the values are finite.
- **Enrollment.** Several distinct samples per subject. The intra-subject standard deviation of every feature is estimated
  from them; with a single sample, or with identical samples, it is zero (\ref ig_gap_sigma). \c enroll accepts a single
  sample, and the template then gets the largest possible capacity, which is not a sign of a stronger key.
- **Key.** \c Key holds the bits of the Turbo Code input message; index \f$i\f$ of \c as_bits() is bit \f$i\f$. Its length
  \f$N_I = \lceil C\,k/n \rceil\f$ depends on the capacity \f$C\f$ of the template and on the code rate \f$k/n\f$ of
  \f$\Pi\f$ (1/3 to 1/6). It differs between subjects and between two enrollments of the same subject. \f$N_I\f$ is stored
  in \f$P\f$ and is therefore public. \c Key implements \c PartialEq with an ordinary, not constant-time, comparison, and has
  no \c Debug implementation.
- **Helper data.** \c UserPublicData holds the text of a \c .userSpecificHSTCFE file. The values are
  separated by whitespace, so the user identifier must not contain whitespace. \f$P\f$ can only be loaded by an extractor
  holding the same \f$\Pi\f$: a different \f$\Pi\f$ is not detected and makes Rep return wrong keys.

Integration pattern {#ig_flow}
===================

Enrollment:

1. Load \f$\Pi\f$ with \c GeneralHSTCFE::from_config_file, and compute a digest of \f$\Pi\f$ (configuration file, Turbo Code
   file and training-set files) that identifies the parameter version.
2. Capture several samples and call \c enroll with an opaque identifier (random, not a name or a DID: it is stored in
   \f$P\f$). An \c Err means that the enrollment must be rejected and the samples captured again.
3. Derive the application key from the returned \c Key (\ref ig_kdf) and register the public part of the derived key
   (\ref ig_verify). Drop the returned \c UserHSTCFE: it holds \f$\mathbf{b}\f$.
4. Store the bytes of \c UserPublicData, the KDF salt and the digest of \f$\Pi\f$ together, protected as in \ref ig_gaps.
   \c UserPublicData::save writes the bytes alone, with the default file permissions of the process.
5. Erase \f$\mathbf{b}\f$, the derived secret and the enrollment samples as soon as possible.

Reproduction:

1. Check the authenticity of the stored record and that its digest equals the digest of the loaded \f$\Pi\f$.
2. Build the extractor with \c GeneralHSTCFE::load_user_specific_extractor and call \c reproduce(probe).
3. Derive the application key from the candidate with the stored salt, and accept only if it matches the registered public
   key. Do not take \c Some as a success.
4. **Never** call \c UserHSTCFE::get_key, and never compare the candidate with the key enrolled by the same process, in a
   deployed flow. \c get_key returns the key read from \f$P\f$, and the comparison with the enrolled key is how the minimal
   example and the test \c tests/rust-interoperability.rs check their results: both bypass the biometric.

Key derivation {#ig_kdf}
==============

Do not use \f$\mathbf{b}\f$ directly as a key. Its length varies, it is a bit vector rather than bytes, and its effective
strength is bounded by the biometric (\ref ig_verify), not by \f$N_I\f$. Encode it canonically and feed it to a KDF such as
HKDF \cite rfc5869, with a random salt stored next to \f$P\f$ and a context string that binds the use and the version of
\f$\Pi\f$. The encoding below (4-byte big-endian \f$N_I\f$, then the bits most-significant first, zero-padded) is a
recommendation of this guide, not part of the crate: whichever encoding is chosen must stay fixed for the lifetime of the
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
fn unlock(candidate: &[bool], params: &[u8; 32], salt: &[u8; 32],
          registered: &VerifyingKey) -> Option<SigningKey> {
    let sk = derive_signing_key(candidate, params, salt);
    (sk.verifying_key() == *registered).then_some(sk)
}
\endcode

The calls of the extractor around it:

\code{.unparsed}
use sfe::{GeneralHSTCFE, UserPublicData};

let general = GeneralHSTCFE::from_config_file("generalConfigFile.txt")?;

// Enrollment
// Err: reject the enrollment and capture again
let (user, key, helper_data) = general.enroll(&opaque_id, &enrollment_samples)?;
drop(user);                                   // holds the key
let bits = Zeroizing::new(key.as_bits());
let (salt, vk) = bind(&bits, &params);
let record = helper_data.into_bytes();
// store record, salt and params as one authenticated record; register vk

// Reproduction (record authenticated and params checked beforehand)
let helper_data = UserPublicData::from_bytes(record);
let mut user = GeneralHSTCFE::load_user_specific_extractor(&general, helper_data);
// always Some in sfe 0.2: Some is not a success
let Some(candidate) = user.reproduce(&probe) else { return Err("rejected".into()) };
match unlock(&Zeroizing::new(candidate.as_bits()), &params, &salt, &vk) {
    Some(sk) => { /* sign with sk */ }
    None => { /* reject */ }
}
\endcode

This flow was checked against \c sfe 0.2 on synthetic data: 30 subjects with 24 features, keys of 14 to 19 bits, 8 of 10
genuine probes accepted and none of 290 impostor probes.

The salt separates the keys derived from different enrollments, but it does not add entropy: if \f$\mathbf{b}\f$ is predictable
(\ref ig_gap_rng), so is the derived key.

Verification and strength {#ig_verify}
=========================

Rep cannot tell whether it succeeded, so the integrator needs a verifier. The pattern above stores a public key derived from
\f$\mathbf{b}\f$ and compares public keys, which needs no constant-time comparison. A hash of \f$\mathbf{b}\f$ stored with
\f$P\f$ works the same way. A comparison with secret values, such as \c == between two \c Key values, is not constant-time.

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

- **Threads.** The extractor types are neither \c Send nor \c Sync (opaque C++ types behind \c cxx::UniquePtr): a clone cannot
  be moved to another thread, so each thread must call \c from_config_file itself. \c reproduce modifies the decoder state of
  its object, hence \c &mut \c self. \c enroll draws from a random generator shared by the whole process and protected by a
  mutex.
- **Errors.** Loading \f$\Pi\f$ and samples files returns \c Result. Malformed \f$P\f$ and samples of the wrong dimensionality
  still abort the process. Validate the inputs before the calls, and treat \f$P\f$ that has not been authenticated as untrusted
  input that can crash the process.
- **Timing.** The Turbo decoder stops as soon as its component decoders agree, so a genuine probe usually takes fewer iterations
  than an impostor probe. On the synthetic data above, Rep took 0.4 ms on average for genuine probes and 1.5 ms for impostor
  probes: the running time leaks whether the probe was close to the template.
- **Memory hygiene.** \c Key, \c UserPublicData and the C++ objects keep \f$\mathbf{b}\f$ in memory and are not zeroized on drop;
  \c Key::as_bits returns a new copy each time. Wrap the copies in \c zeroize::Zeroizing and drop the extractors as soon as
  possible.
- **Console output.** The build script of \c sfe 0.2 builds and links the Release libraries, which print only progress messages
  and the paths of the training-set files while \f$\Pi\f$ is loaded. Never link the Debug libraries (compiled with \c DEBUG):
  they print the PSK symbols committed at enrollment (which encode codeword bits), the normalized biometric values and the
  decoder state.

Deployment blockers {#ig_gaps}
===================

| Id | Issue | Consequence | Fix | Status in \c sfe 0.2 |
|----|-------|-------------|-----|----------------------|
| G1 | Fixed-seed Mersenne Twister | Keys, interleavers and permutations are predictable | Library | Open |
| G2 | Key stored in the helper data | Whoever reads \f$P\f$ gets \f$\mathbf{b}\f$ | Library | Open |
| G3 | Second factor not implemented | Offline oracle, linkable templates | Library and integrator | Open |
| G4 | C++ exceptions abort the process | Denial of service from bad inputs or \f$P\f$ | Binding | Partly fixed: \f$\Pi\f$ and samples files return \c Result |
| G5 | Debug build prints secrets | Leakage through logs | Binding | Fixed: Release libraries |
| G6 | Zero-variance features | Unstable decoding for some templates | Library | Open |
| G7 | No zeroization, timing side channel | Residual secrets, leakage of the decision | Binding and integrator | Open; the general extractor can now be cloned and is not modified by \c enroll |
| G8 | Build script portability | Build fails where CMake installs to \c lib (Debian, Ubuntu); run path into the Cargo build folder | Binding | Partly fixed: library names; \c lib64 still hard-coded |

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

The bytes of \c UserPublicData contain \f$\mathbf{b}\f$ under the label \c TurboCodeInputMessage, and the parser requires that
label. The library needs a serialization of \f$P\f$ without the key. Until then, \f$P\f$ (including the files written by
\c UserPublicData::save) must be handled as a secret, which defeats the purpose of a fuzzy extractor.

Second factor (G3) {#ig_gap_factor}
------------------

The Strengthened Fuzzy Extractor protects \f$P\f$ with a fixed second authentication factor \cite argonesrua2019efficient,
which makes the protected templates of a subject unlinkable and renewable and prevents the offline attack of \ref ig_verify.
The evaluations of this software correspond to a disclosed second factor \cite argonesrua2024strengthened. The protection of
\f$P\f$ is not part of the library yet (\c doc/api_changes.md lists the encryption of \c UserPublicData as a known limitation).

Zero-variance features (G6) {#ig_gap_sigma}
---------------------------

When all the enrollment samples of a subject give the same normalized value for a feature (for example because they all fall
beyond the range of the training set), its standard deviation is zero. Such a feature gets the maximum capacity and a
zero-variance noise model, and the soft decoding of its bits becomes degenerate. Use several distinct enrollment samples, and a
training set that covers the range of the features. A variance floor in the library is under consideration.

Testing checklist {#ig_testing}
=================

- Enroll and reproduce in **different** extractors, built from the serialized \f$P\f$, as in production. The Rust test
  \c reproduces_key_from_stored_helper_data in \c tests/rust-interoperability.rs does this, and the C++ test
  \c inmemory_reproduce checks that the enrolling object, its copy and a reloaded \f$P\f$ decode identically.
- Measure the false rejection and false acceptance rates of the integrated flow on a held-out set, with the production
  \f$\Pi\f$; report the FAR as the strength of the binding.
- Check the distribution of \f$N_I\f$ and the rate of rejected enrollments on the target population.
- Check that a changed \f$\Pi\f$ is detected through the parameter digest before Rep is called.
- Check that nothing but the progress messages of \c from_config_file is printed to standard output.
