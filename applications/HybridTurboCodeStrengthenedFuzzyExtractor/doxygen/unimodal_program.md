Unimodal program {#unimodal_program_page}
================

[TOC]

\c HybridTurboCodeStrengthenedFuzzyExtractor is the command-line program that enrolls the subjects of one biometric modality
with the unimodal \ref HybridStrengthenedTurboCodeFuzzyExtractor and evaluates the recognition performance of the protected
templates. It is the program used for the unimodal experiments of \cite argonesrua2024strengthened and
\cite argonesrua2026exploiting.

\author Enrique Argones Rúa (COSIC, KU Leuven)
\date 2025

What it does {#unimodal_program_what}
============

- **Enrollment** (\c -m \c enrollment): for every samples file \c ID.txt of the enrollment folder, a copy of the general fuzzy
  extractor runs Gen in a task of the global \ref threadPool and writes the helper data \c ID.userSpecificHSTCFE and the key
  \c ID.Key to the HSTCFE folder. Templates without enough reliable information (\f$N_I = 0\f$) are reported as not valid.
- **Test** (\c -m \c test): every probe samples file is compared against every valid template. For each comparison, Rep is run
  on each probe sample, and one line is appended to \c authenticationResults.txt in the recovered folder:
  \verbatim
  PROBE_ID TEMPLATE_ID AUTHENTICATED GENUINE
  \endverbatim
  \c AUTHENTICATED is 1 when the decoded message equals the enrolled key, and \c GENUINE is 1 for same-subject comparisons.
- **Both** (\c -m \c both): enrollment followed by test.

The false rejection rate (FRR) is the fraction of genuine comparisons with \c AUTHENTICATED = 0, and the false acceptance rate
(FAR) the fraction of impostor comparisons with \c AUTHENTICATED = 1. The key length \f$N_I\f$ of each template, stored in its
helper data, measures the security level. The complete list of options, and the naming of the output files, are given in the
description of HybridTurboCodeStrengthenedFuzzyExtractor/main.cpp.

Example {#unimodal_program_example}
=======

\verbatim
mkdir -p HSTCFEs recovered            # the output folders must exist
HybridTurboCodeStrengthenedFuzzyExtractor -m both -G generalConfigFile.txt -H HSTCFEs/ \
    -e enrollment/ -p probe/ -r recovered/ -R
\endverbatim

\c generalConfigFile.txt is the general configuration of the HybridStrengthenedTurboCodeFuzzyExtractor (Turbo Code file,
training set and bit-loading parameters); an example is installed to \c PREFIX/share/HSTCFE/configFiles. The samples files
contain the number of samples and the number of features on the first line, followed by one sample per line. The enrollment
and probe files of a subject must have the same name.

The end-to-end test of the repository (\c tests/end_to_end.py) runs this program on synthetic data and can be used as a
complete working example.

Building {#unimodal_program_build}
========

The program needs the HSTCFE library and its dependencies, and the header-only argagg command-line parser. It is built by the
top-level CMake project of the repository, or on its own:

    cmake -S applications/HybridTurboCodeStrengthenedFuzzyExtractor \
          -B build/unimodal -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_PREFIX_PATH=PREFIX -DCMAKE_INSTALL_PREFIX=PREFIX
    cmake --build build/unimodal -j
    cmake --install build/unimodal

It is installed to \c PREFIX/bin (\c HybridTurboCodeStrengthenedFuzzyExtractor_Debug in Debug builds, which prints verbose
traces).
