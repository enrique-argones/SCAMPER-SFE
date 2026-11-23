# SCAMPER SFE

**Strengthened Fuzzy Extractor software for biometric template protection.**

SCAMPER SFE is a set of C++17 libraries and command-line programs that bind a random cryptographic key to a biometric
template, and reproduce that key from a fresh biometric sample of the same subject, with the *Hybrid Strengthened Turbo-Code
Fuzzy Extractor* [1]–[3]. It was developed at COSIC (KU Leuven) in SCAMPER, a Research and Innovation ICON project funded by
VLAIO, whose objective is to integrate biometric template protection into self-sovereign identity platforms using the
Strengthened Fuzzy Extractor.

- Unimodal and multimodal (feature-level fusion) fuzzy extractors.
- Reliable features protected by **code-offset key binding** on PSK constellations; unreliable features fused into more
  reliable combinations and protected by **fuzzy-commitment key generation** on a 2-PAM constellation.
- One **soft-decoded Turbo Code** per subject, with user-specific interleavers and puncturing, that uses the noise model of
  every feature.
- Evaluation programs that enroll whole databases in parallel and report the authentication decision of every comparison.
- A Rust binding of the unimodal extractor that keeps templates in memory.

## Contents

- [Repository layout](#repository-layout)
- [Technical description](#technical-description)
- [Installation](#installation)
- [Usage](#usage)
- [Rust binding](#rust-binding)
- [Documentation](#documentation)
- [Security notice](#security-notice)
- [License](#license)
- [References](#references)

## Repository layout

| Path | Content |
|------|---------|
| `libraries/GMMLite` | Minimal Gaussian Mixture Models (`gmmlite::GMM`), the noise models of the decoder. C++ standard library only. |
| `libraries/ThreadPool` | Fixed-size thread pool (`ThreadPool`, `SafeQueue`, `PendingTasksGuard`). |
| `libraries/TurboCodes` | Soft-decoded Turbo Codes (`TurboCode`), PSK and 2-PAM constellations, file readers, Turbo Code configurations. |
| `libraries/HSTCFE` | Unimodal Hybrid Strengthened Turbo-Code Fuzzy Extractor (`HybridStrengthenedTurboCodeFuzzyExtractor`). |
| `libraries/MHSTCFE` | Multimodal extractor (`MultimodalHybridStrengthenedTurboCodeFuzzyExtractor`). |
| `applications/HybridTurboCodeStrengthenedFuzzyExtractor` | Unimodal enrollment and evaluation program. |
| `applications/MultimodalHybridStrengthenedTurboCodeFuzzyExtractor` | Multimodal enrollment and evaluation program (chimeric subjects). |
| `rust-binding`, `Cargo.toml`, `build.rs` | Rust binding (crate `sfe`) of the unimodal extractor, and its `minimal` example. |
| `flake.nix`, `flake.lock` | Optional Nix development shell. |
| `tests` | End-to-end test of both programs on synthetic data. |
| `cmake` | CMake helpers shared by all modules. |
| `scripts/install.sh` | Builds, tests and installs everything in Release and Debug. |
| `doc` | Complete manual (`Doxyfile`, pages, bibliography), standalone integrators guide (`integrators/Doxyfile`), common Doxygen settings and `build_docs.sh`. |

Every module has its own `CMakeLists.txt`, its own `Doxyfile` and a `doxygen` folder with the detailed technical description
of the module.

## Technical description

### Fuzzy extractors and the Strengthened Fuzzy Extractor

A fuzzy extractor [4] is a pair of procedures. *Gen* takes a biometric reading and outputs a uniformly random key and public
helper data; *Rep* takes the helper data and a new reading, and outputs the same key if the new reading is close enough to the
enrollment one. Fuzzy extractors are a family of biometric cryptosystems [5] for biometric template protection [6]. Their
classical realizations, the fuzzy commitment [7] and the code-offset or helper-data schemes [8], bind the codeword of an
error-correcting code to the biometric data; their helper data leak information about the template [9], and hard-decision
codes operate far from the capacity of the biometric channel.

The Strengthened Fuzzy Extractor (SFE) [1] uses a model-based *uncoupling* construction that maps every continuous feature to
a phase uniformly distributed over the population, so that the values bound to it are independent of the biometric data, and
a fixed second authentication factor that protects the helper data and makes the protected templates of a subject unlinkable
and renewable. The SFE was realized with soft-decoded Turbo Codes and user-specific helper data [2], and extended into the
*hybrid* SFE implemented here [3], which also exploits the features too unreliable to carry a bit on their own.

### Enrollment (Gen)

1. **Uncoupling.** Every feature is mapped to the phase θ = 2π F̂(x), where F̂ is the empirical CDF of the feature on a
   training population [1]. The mean and standard deviation of the phase over the enrollment samples describe the subject and
   its intra-subject noise.
2. **Bit loading.** The effective noise α·σ + β of each feature (α, β: safety margins) is compared with thresholds derived for
   the PSK channel and the rate of the code: reliable features carry 1 to 3 bits, unreliable features none [3].
3. **Feature fusion.** Unreliable features are greedily grouped into linear combinations of maximal signal-to-noise ratio
   until each combination reaches a minimum reliability; each combination carries one bit [3].
4. **Turbo Code dimensioning.** For a template of capacity C bits, a Turbo Code [10] of rate 1/r with key length ⌈C/r⌉ is
   built with random, user-specific interleavers, and punctured [11] so that its codeword has exactly C bits.
5. **Key binding.** A random key is Turbo encoded and its codeword bits are assigned to the symbols by a random permutation.
   Reliable features bind their bits with the code-offset construction: the bits are Gray-mapped [12] to a PSK phase φ [13],
   and the helper data stores (φ + μ) mod 2π. Each combination of unreliable features commits its bit through the sign of its
   weights, so that a genuine probe reproduces the 2-PAM symbol ±1.

### Reproduction (Rep)

A probe is turned into noisy symbols: one PSK phase per reliable feature, and one 2-PAM value per combination of unreliable
features. The soft Turbo decoder computes the channel likelihood of every codeword bit from the conditional pdf of the symbol
that carries it, modelled with Gaussian mixtures [14] (with a wrapped-normal approximation for phases [15]), and runs the
iterative MAP algorithm [10], [16], [17] until its component decoders agree. The key is reproduced when the probe is close
enough to the enrollment samples. Because each feature is a channel with its own noise level, no feature is discarded and
reliable features weigh more than unreliable ones, which yields longer effective keys than constructions that discard the
features with insufficient capacity [3]. Turbo Codes combined with modulation constellations had been used for template
protection in [18].

### Multimodal fusion and evaluation

The multimodal extractor fuses several modalities at feature level [19]: the uncoupled features of all modalities, each with
the normalizer of its own modality, are concatenated, and one key is bound to the concatenated vector. The evaluation programs
enroll every subject (or chimeric multimodal subject) in parallel, compare every probe with every template, and report for
each comparison whether the key was reproduced and whether it was a genuine comparison, from which the false rejection and
false acceptance rates are computed [2], [3].

Linear algebra is done with Armadillo [20], parallel execution with a thread pool [21], and the random keys, interleavers and
permutations are drawn from a Mersenne Twister [22]. The module pages of the manual describe every step in detail, with links
to the code that implements it.

## Installation

### Requirements

| Dependency | Needed for | Debian / Ubuntu package |
|------------|------------|-------------------------|
| C++17 compiler (GCC ≥ 9 or Clang ≥ 10) | everything | `build-essential` |
| CMake ≥ 3.23 | everything | `cmake` |
| Armadillo | TurboCodes and above | `libarmadillo-dev` |
| POSIX threads | ThreadPool and above | (part of the C library) |
| [argagg](https://github.com/vietjtnguyen/argagg) (header only) | the programs | not packaged, see below |
| Python 3 | the end-to-end test | `python3` |

```sh
sudo apt install build-essential cmake libarmadillo-dev python3
# argagg: a single header, installed into PREFIX/include/argagg (here PREFIX = /usr/local)
sudo mkdir -p /usr/local/include/argagg
sudo curl -L -o /usr/local/include/argagg/argagg.hpp \
     https://raw.githubusercontent.com/vietjtnguyen/argagg/0.4.7/include/argagg/argagg.hpp
```

On Fedora, use `dnf install gcc-c++ cmake armadillo-devel python3`.

### Build, test and install

The top-level CMake project builds every library and program in dependency order:

```sh
git clone git@gitlab.esat.kuleuven.be:fuzzy-extractors/scamper-sfe.git
cd scamper-sfe
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$HOME/sfe
cmake --build build/release -j
(cd build/release && ctest --output-on-failure)
cmake --install build/release
```

| CMake option | Default | Meaning |
|--------------|---------|---------|
| `CMAKE_BUILD_TYPE` | `Release` | `Release` or `Debug`. Debug libraries and programs get the `_Debug` suffix and print verbose traces. |
| `CMAKE_INSTALL_PREFIX` | `/usr/local` | Install prefix. |
| `SFE_BUILD_TESTS` | `ON` | Build the tests (`test_gmmlite`, `end_to_end`). |

`scripts/install.sh` builds, optionally tests, and installs both the Release and the Debug configurations (they can share a
prefix); it uses `sudo` only for the installation step and only when the prefix is not writable:

```sh
scripts/install.sh -p $HOME/sfe -t        # options: -p PREFIX -b BUILD_DIR -c Release|Debug|both -t (tests) -y (no prompt)
```

Installed files:

| What | Location |
|------|----------|
| Libraries `libGMMLite`, `libThreadPool`, `libTurboCodes`, `libHSTCFE`, `libMHSTCFE` (and `_Debug`) | `PREFIX/lib` |
| Headers, included as `<Module/header.h>` | `PREFIX/include/{GMMLite,ThreadPool,TurboCodes,HSTCFE,MHSTCFE}` |
| Programs (and `_Debug`) | `PREFIX/bin` |
| Turbo Code configurations (rates 1/3 to 1/6) | `PREFIX/share/TurboCodes/configFiles` |
| Example configuration files | `PREFIX/share/HSTCFE/configFiles`, `PREFIX/share/MultimodalHybridStrengthenedTurboCodeFuzzyExtractor/configFiles` |
| License and README | `PREFIX/share/doc/SCAMPER_SFE` |

The installed programs find their libraries through a relative run path, so no `LD_LIBRARY_PATH` is needed. Every module can
also be built on its own against the modules already installed (`cmake -S libraries/HSTCFE ... -DCMAKE_PREFIX_PATH=PREFIX`);
see *Building, testing and installing* in the manual.

## Usage

```sh
# Unimodal: enroll every subject of enrollment/ and test every probe of probe/ (output folders must exist)
mkdir -p HSTCFEs recovered
HybridTurboCodeStrengthenedFuzzyExtractor -m both -G generalConfigFile.txt -H HSTCFEs/ \
    -e enrollment/ -p probe/ -r recovered/ -R

# Multimodal (chimeric subjects): after the unimodal enrollment of every modality
MultimodalHybridStrengthenedTurboCodeFuzzyExtractor -c multimodalConfig.txt -m both -R
```

`recovered/authenticationResults.txt` contains one line `PROBE_ID TEMPLATE_ID AUTHENTICATED GENUINE` per comparison. The
formats of every input and output file are described in the *File formats* page of the manual, and `tests/end_to_end.py` is a
complete working example on synthetic data.

## Rust binding

The Cargo package `sfe` at the root of the repository exposes the unimodal extractor to Rust through
[cxx](https://cxx.rs). `build.rs` builds the needed C++ libraries with CMake, so they do not have to be installed first.
It needs Rust ≥ 1.85, CMake and Armadillo (`nix develop` provides them with the included flake).

```rust
use sfe::HSTCFE;
use std::path::Path;

let mut general = HSTCFE::new();
general.load_general_data(Path::new("generalConfigFile.txt"));
general.set_user_id("subject_0002");
general.generate(sfe::turbocodes::parse_samples_file(Path::new("enrollment/subject_0002.txt")));
let template: Vec<u8> = general.get_user_specific_data();          // helper data, kept in memory

let probes = sfe::turbocodes::parse_samples_file(Path::new("probe/subject_0002.txt"));
let mut user = HSTCFE::load_user_specific_extractor(&general, template);
let accepted = user.reproduce(probes[0].clone()) == user.turbocode_input_message();
```

`cargo run --bin minimal` runs this example (it expects `generalConfigFile.txt` in the current folder and the samples in
`../fingerVeinData_196/testSet/{enrollment,probe}/subject_0002.txt`, relative to the repository). The template is the text
of a `.userSpecificHSTCFE` file, produced and read in C++ by `getUserSpecificData` and `loadUserSpecificDataFromBlob`.

Current limitations: `build.rs` expects the libraries in `lib64` (Fedora-like systems; on Debian/Ubuntu the link fails) and
links the `_Debug` libraries (so `cargo build --release` fails), and C++ exceptions abort the process instead of becoming Rust
errors. See the *Rust binding* page of the manual for details.

## Documentation

The documentation is written for [Doxygen](https://www.doxygen.nl): one `Doxyfile` per module (in the module folder) and one
for the complete manual (`doc/Doxyfile`), all sharing the settings of `doc/Doxyfile.common`. The bibliography
(`doc/bibliography.bib`) is formatted in the PDF manuals with `IEEEtran.bst`, the IEEE Transactions style.

### Tools

| Tool | Debian / Ubuntu package |
|------|-------------------------|
| Doxygen ≥ 1.15 (tested with 1.15.0 and 1.16.1) | `doxygen`, or the [binary release](https://www.doxygen.nl/download.html) if the distribution's is older |
| Graphviz (`dot`) | `graphviz` |
| Ghostscript (formula images) | `ghostscript` |
| `pdflatex`, `bibtex`, `makeindex` | `texlive-latex-base` (and `texlive-binaries`) |
| `latexmk` | `latexmk` |
| LaTeX packages used by Doxygen (`tabularray`, `etoc`, `listofitems`, ...) | `texlive-latex-recommended`, `texlive-latex-extra`, `texlive-fonts-recommended`, `texlive-plain-generic` |
| `IEEEtran.bst` | `texlive-publishers` |

```sh
sudo apt install doxygen graphviz ghostscript latexmk texlive-latex-base texlive-latex-recommended \
                 texlive-latex-extra texlive-fonts-recommended texlive-plain-generic texlive-publishers
```

### Generating the manuals

```sh
sh doc/build_docs.sh          # HTML (and LaTeX sources) of every manual
sh doc/build_docs.sh --pdf    # also every PDF manual, built with latexmk
```

The script builds the module manuals in dependency order (they are linked through tag files), then the complete manual and
the standalone guide for cryptographic integrators, and stops with an error if Doxygen, LaTeX, BibTeX or latexmk report any
warning. The results are:

| Manual | HTML | PDF |
|--------|------|-----|
| Complete manual | `doc/html/index.html` | `doc/latex/refman.pdf`, copied to `doc/SCAMPER_SFE_Manual.pdf` |
| Guide for cryptographic integrators | `doc/integrators/html/index.html` | `doc/integrators/latex/refman.pdf`, copied to `doc/SCAMPER_SFE_Integrators_Guide.pdf` |
| One module | `<module>/doc/html/index.html` | `<module>/doc/latex/refman.pdf` |

A single manual can also be built by hand from the folder of its `Doxyfile`:

```sh
cd doc && doxygen Doxyfile && cd latex && latexmk -pdf refman.tex
```

The complete PDF manual takes several minutes. The generated documentation is not tracked by git.

## Security notice

SCAMPER SFE is a research implementation, designed to measure the recognition performance and the key length of the SFE. It
uses a fixed-seed Mersenne Twister, stores the key in the helper data and in `.Key` files for evaluation, and does not encrypt
the helper data with the second authentication factor of the SFE [1] (the experiments correspond to the scenario in which the
second factor has been disclosed [2]). It must not be used to protect real biometric data without addressing these points.

## Author

Enrique Argones Rúa, COSIC – KU Leuven (enrique.argonesrua@esat.kuleuven.be).

Developed in the VLAIO Research and Innovation ICON project SCAMPER.

## License

SCAMPER SFE is released under the [MIT License](LICENSE). Copyright (c) 2026 KU Leuven.

## References

[1] E. Argones Rúa, A. Abidin, R. Peeters, and J. Romme, "Efficient and privacy-preserving cryptographic key derivation from
continuous sources," *IEEE Trans. Inf. Forensics Security*, vol. 14, no. 11, pp. 2834–2847, Nov. 2019,
doi: 10.1109/TIFS.2019.2907459.

[2] E. Argones Rúa, E. Maiorana, and P. Campisi, "Strengthened fuzzy extractors using Turbo-codes: Case study on finger vein
authentication," in *Proc. IEEE Int. Workshop Inf. Forensics Secur. (WIFS)*, 2024, pp. 1–6,
doi: 10.1109/WIFS61860.2024.10810678.

[3] E. Argones Rúa, E. Maiorana, G. Garofalo, T. Van hamme, D. Preuveneers, and P. Campisi, "Exploiting reliable and
unreliable features in hybrid strengthened fuzzy extractors for biometric template protection," in *Proc. IEEE Int. Joint
Conf. Biometrics (IJCB)*, Rome, Italy, 2026, to be published.

[4] Y. Dodis, R. Ostrovsky, L. Reyzin, and A. Smith, "Fuzzy extractors: How to generate strong keys from biometrics and other
noisy data," *SIAM J. Comput.*, vol. 38, no. 1, pp. 97–139, Jan. 2008, doi: 10.1137/060651380.

[5] C. Rathgeb and A. Uhl, "A survey on biometric cryptosystems and cancelable biometrics," *EURASIP J. Inf. Secur.*,
vol. 2011, pp. 1–25, 2011, doi: 10.1186/1687-417X-2011-3.

[6] *Information Security, Cybersecurity and Privacy Protection — Biometric Information Protection*, ISO/IEC Standard 24745:2022,
Geneva, Switzerland, 2022.

[7] A. Juels and M. Wattenberg, "A fuzzy commitment scheme," in *Proc. 6th ACM Conf. Comput. Commun. Secur. (CCS)*,
Singapore, Nov. 1999, pp. 28–36, doi: 10.1145/319709.319714.

[8] J.-P. Linnartz and P. Tuyls, "New shielding functions to enhance privacy and prevent misuse of biometric templates," in
*Audio- and Video-Based Biometric Person Authentication (AVBPA)* (Lecture Notes in Computer Science, vol. 2688). Berlin,
Germany: Springer, 2003, pp. 393–402.

[9] T. Ignatenko and P. Tuyls, "Information leakage in fuzzy commitment schemes," *IEEE Trans. Inf. Forensics Security*,
vol. 7, no. 2, pp. 481–493, Apr. 2012.

[10] C. Berrou, A. Glavieux, and P. Thitimajshima, "Near Shannon limit error-correcting coding and decoding: Turbo-codes. 1,"
in *Proc. IEEE Int. Conf. Commun. (ICC)*, Geneva, Switzerland, May 1993, vol. 2, pp. 1064–1070,
doi: 10.1109/ICC.1993.397441.

[11] J. Hagenauer, "Rate-compatible punctured convolutional codes (RCPC codes) and their applications," *IEEE Trans.
Commun.*, vol. 36, no. 4, pp. 389–400, Apr. 1988, doi: 10.1109/26.2763.

[12] F. Gray, "Pulse code communication," U.S. Patent 2 632 058, Mar. 17, 1953.

[13] J. G. Proakis and M. Salehi, *Digital Communications*, 5th ed. New York, NY, USA: McGraw-Hill, 2008.

[14] D. A. Reynolds, "Gaussian mixture models," in *Encyclopedia of Biometrics*, S. Z. Li and A. K. Jain, Eds. Boston, MA,
USA: Springer, 2009, pp. 659–663.

[15] K. V. Mardia and P. E. Jupp, *Directional Statistics*. Chichester, U.K.: Wiley, 2000.

[16] L. R. Bahl, J. Cocke, F. Jelinek, and J. Raviv, "Optimal decoding of linear codes for minimizing symbol error rate,"
*IEEE Trans. Inf. Theory*, vol. 20, no. 2, pp. 284–287, Mar. 1974, doi: 10.1109/TIT.1974.1055186.

[17] J. Hagenauer, E. Offer, and L. Papke, "Iterative decoding of binary block and convolutional codes," *IEEE Trans. Inf.
Theory*, vol. 42, no. 2, pp. 429–445, Mar. 1996, doi: 10.1109/18.485714.

[18] E. Maiorana, D. Blasi, and P. Campisi, "Biometric template protection using Turbo codes and modulation constellations,"
in *Proc. IEEE Int. Workshop Inf. Forensics Secur. (WIFS)*, Tenerife, Spain, 2012, pp. 25–30.

[19] A. Nagar, K. Nandakumar, and A. K. Jain, "Multibiometric cryptosystems based on feature-level fusion," *IEEE Trans. Inf.
Forensics Security*, vol. 7, no. 1, pp. 255–268, Feb. 2012.

[20] C. Sanderson and R. Curtin, "Armadillo: A template-based C++ library for linear algebra," *J. Open Source Softw.*, vol. 1,
no. 2, p. 26, 2016, doi: 10.21105/joss.00026.

[21] A. Williams, *C++ Concurrency in Action*, 2nd ed. Shelter Island, NY, USA: Manning, 2019.

[22] M. Matsumoto and T. Nishimura, "Mersenne twister: A 623-dimensionally equidistributed uniform pseudo-random number
generator," *ACM Trans. Model. Comput. Simul.*, vol. 8, no. 1, pp. 3–30, Jan. 1998, doi: 10.1145/272991.272995.
