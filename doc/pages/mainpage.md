SCAMPER SFE {#mainpage}
===========

C++ libraries and command-line programs for **biometric template protection** with the **Strengthened Fuzzy Extractor
(SFE)** \cite argonesrua2019efficient, \cite argonesrua2024strengthened, \cite argonesrua2026exploiting. A random secret key
is bound to the biometric template of a subject so that:

- the key can be reproduced from a fresh biometric sample of the same subject;
- the stored helper data does not contain the biometric template in the clear.

The implementation is a *hybrid* construction. Reliable features are protected with a **code-offset** key-binding scheme on
PSK constellations. Unreliable features are combined into more reliable ones and protected with a **fuzzy-commitment**
key-generation scheme on a 2-PAM constellation. All the bits are protected by one **soft-decoded Turbo Code**, which uses the
noise model of every feature. A multimodal extractor fuses several biometric modalities at feature level.

The software was developed at COSIC (KU Leuven) in the SCAMPER project, a Research and Innovation ICON project funded by
VLAIO, whose objective is to integrate biometric template protection into self-sovereign identity platforms using the
Strengthened Fuzzy Extractor.

\author Enrique Argones Rúa (COSIC, KU Leuven)
\version 1.0.0
\date 2026
\copyright Copyright (c) 2026 KU Leuven. Released under the MIT License (file \c LICENSE of the repository).

Components {#main_components}
==========

| Component | Type | Depends on | Documentation |
|-----------|------|------------|---------------|
| GMMLite | Library | C++ standard library | \ref gmmlite_page |
| ThreadPool | Library | POSIX threads | \ref threadpool_page |
| TurboCodes | Library | GMMLite, Armadillo | \ref turbocodes_page |
| HSTCFE | Library | TurboCodes, ThreadPool | \ref hstcfe_page |
| MHSTCFE | Library | HSTCFE | \ref mhstcfe_page |
| HybridTurboCodeStrengthenedFuzzyExtractor | Program | HSTCFE, argagg | \ref unimodal_program_page |
| MultimodalHybridStrengthenedTurboCodeFuzzyExtractor | Program | MHSTCFE, argagg | \ref multimodal_program_page |
| sfe (Rust crate) | Rust binding | HSTCFE, cxx | \ref rust_binding_page |

\dot
digraph components {
  rankdir=BT;
  node [shape=box, fontname=Helvetica, fontsize=10, style=filled, fillcolor="#eef3fb", color="#4a6fa5"];
  edge [color="#555555", arrowsize=0.7];
  GL   [label="GMMLite\ngmmlite::GMM", URL="\ref gmmlite::GMM"];
  ARMA [label="Armadillo", fillcolor="#f4f4f4", color="#999999"];
  TC   [label="TurboCodes\nTurboCode, PSK, PAM2", URL="\ref TurboCode"];
  TP   [label="ThreadPool", URL="\ref ThreadPool"];
  HS   [label="HSTCFE\nHybridStrengthenedTurboCodeFuzzyExtractor", URL="\ref HybridStrengthenedTurboCodeFuzzyExtractor"];
  MH   [label="MHSTCFE\nMultimodalHybridStrengthenedTurboCodeFuzzyExtractor", URL="\ref MultimodalHybridStrengthenedTurboCodeFuzzyExtractor"];
  P1   [label="HybridTurboCodeStrengthenedFuzzyExtractor\n(program)", shape=component, fillcolor="#fdf3e3", color="#b7791f"];
  P2   [label="MultimodalHybridStrengthenedTurboCodeFuzzyExtractor\n(program)", shape=component, fillcolor="#fdf3e3", color="#b7791f"];
  TC -> GL; TC -> ARMA;
  HS -> TC; HS -> TP;
  MH -> HS;
  RS   [label="sfe\n(Rust binding)", URL="\ref rust_binding_page", fillcolor="#eaf6ee", color="#3b8a57"];
  P1 -> HS; P2 -> MH; RS -> HS;
}
\enddot

Where to start {#main_start}
==============

- \subpage technical_description describes the complete system: the fuzzy-extractor background, the hybrid SFE scheme, the
  multimodal fusion, the evaluation methodology and the security considerations.
- \subpage modules_page gathers the detailed description of every library and program.
- \subpage fileformats describes every input and output file (samples, configurations, helper data, keys, results).
- \subpage building lists the dependencies and explains how to compile, test and install the software and how to generate
  this documentation.
- \subpage rust_binding_page explains how to build and use the extractor from Rust.
- \subpage integrators_guide_page is for the engineers who use the extractor as a cryptographic building block: the contract of
  the Rust API, key derivation and verification, and the issues that block a deployment.

Minimal example {#main_example}
===============

\code{.cpp}
#include <HSTCFE/hybridstrengthenedturbocodefuzzyextractor.h>
#include <TurboCodes/vectorfiles.h>

int main() {
    // Shared configuration: Turbo Code, bit-loading parameters and training-set normalizers.
    HybridStrengthenedTurboCodeFuzzyExtractor general;
    general.loadGeneralData("generalConfigFile.txt");

    real_number_vector_vector enrollment, probe;
    parseSamplesFile("subject_001_enrollment.txt", enrollment);
    parseSamplesFile("subject_001_probe.txt", probe);

    // Enrollment: random key + helper data.
    HybridStrengthenedTurboCodeFuzzyExtractor user(general);
    user.setUserId("subject_001");
    user.generate(enrollment);
    if (!user.validForReproduce()) return 1;   // not enough reliable information in the template

    // Reproduction from a probe sample.
    binary_polynomial key = user.reproduce(probe[0]);
    bool accepted = (key == user.turboCodeInputMessage());

    return accepted ? 0 : 2;
}
\endcode

History {#main_history}
=======

Version 1.0.0 (November 2026) is the first release of SCAMPER SFE as a single repository. It gathers the GMMLite library, the
ThreadPool library, and the TurboCodes, HSTCFE and MHSTCFE libraries and the two evaluation programs, which were previously
developed in separate repositories. The libraries use GMMLite for their Gaussian mixture models; the earlier versions, based on
the CHMM library, produced bit-identical results (see \ref gmmlite_equivalence). The whole software is built by one CMake
project and tested end to end on synthetic data.

After 1.0.0 (November 2026): Rust binding of the unimodal extractor (\ref rust_binding_page), and in-memory storage of the
user-specific helper data (\ref HybridStrengthenedTurboCodeFuzzyExtractor::getUserSpecificData,
\ref HybridStrengthenedTurboCodeFuzzyExtractor::loadUserSpecificDataFromBlob), and guide for cryptographic integrators
(\ref integrators_guide_page).
