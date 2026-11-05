Multimodal program {#multimodal_program_page}
==================

[TOC]

\c MultimodalHybridStrengthenedTurboCodeFuzzyExtractor is the command-line program that enrolls multimodal subjects with the
\ref MultimodalHybridStrengthenedTurboCodeFuzzyExtractor and evaluates the recognition performance of the protected
templates. It is the program used for the multimodal experiments of \cite argonesrua2026exploiting.

\author Enrique Argones Rúa (COSIC, KU Leuven)
\date 2025

Chimeric subjects {#multimodal_program_chimeric}
=================

Multimodal databases in which every subject provides every modality are rare. The program therefore builds **chimeric**
subjects, a usual practice in multibiometrics: the subjects of each modality are shuffled independently and paired position by
position, so the number of chimeric subjects is the size of the smallest modality. A chimeric subject is identified as
\c ID1_ID2_..._IDM, where \c IDm is the identifier of its subject in modality \c m. Unimodal identifiers must therefore contain
exactly one underscore (for example \c face_012).

What it does {#multimodal_program_what}
============

- **Enrollment**: the unimodal helper data of every modality (written beforehand by the unimodal program) provide the
  per-modality normalizers. For every chimeric subject, a multimodal extractor is enrolled from its per-modality enrollment
  samples, and \c ID.userSpecificHSTCFE and \c ID.Key are written to the multimodal folder, together with
  \c generalConfigFile.generalHSTCFE.
- **Test**: for every chimeric subject, the multimodal probe samples are the Cartesian product of its per-modality probe
  samples. Every chimeric probe set is compared against every enrolled chimeric template, and \c authenticationResults.txt
  receives one line per probe sample, in the same format as the unimodal program.

All the parameters are given in an experiment configuration file (\c -c); its complete layout is given in the description of
MultimodalHybridStrengthenedTurboCodeFuzzyExtractor/main.cpp, and an example is installed to
\c PREFIX/share/MultimodalHybridStrengthenedTurboCodeFuzzyExtractor/configFiles.

Example {#multimodal_program_example}
=======

\verbatim
# 1. unimodal enrollment of every modality (provides the normalizers)
HybridTurboCodeStrengthenedFuzzyExtractor -m enrollment -G face/generalConfigFile.txt -H face/HSTCFEs/ -e face/enrollment/
HybridTurboCodeStrengthenedFuzzyExtractor -m enrollment -G vein/generalConfigFile.txt -H vein/HSTCFEs/ -e vein/enrollment/
# 2. multimodal enrollment and test (the folders named in config.txt must exist)
MultimodalHybridStrengthenedTurboCodeFuzzyExtractor -c config.txt -m both -R
\endverbatim

The end-to-end test of the repository (\c tests/end_to_end.py) runs both programs on two synthetic modalities and can be used as
a complete working example.

Building {#multimodal_program_build}
========

The program needs the MHSTCFE library and its dependencies, and the header-only argagg command-line parser. It is built by the
top-level CMake project of the repository, or on its own:

    cmake -S applications/MultimodalHybridStrengthenedTurboCodeFuzzyExtractor \
          -B build/multimodal -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_PREFIX_PATH=PREFIX -DCMAKE_INSTALL_PREFIX=PREFIX
    cmake --build build/multimodal -j
    cmake --install build/multimodal

It is installed to \c PREFIX/bin (\c MultimodalHybridStrengthenedTurboCodeFuzzyExtractor_Debug in Debug builds).
