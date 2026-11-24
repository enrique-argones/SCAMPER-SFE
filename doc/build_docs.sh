#!/bin/sh
# Builds every Doxygen manual of SCAMPER SFE (HTML and LaTeX, and the PDFs with --pdf) and checks that no warning is produced.
#
# Usage (from any folder):  sh doc/build_docs.sh [--pdf] [--keep]
#   --pdf   also build the PDF manuals with latexmk (refman.pdf in every latex folder); the complete manual is copied to
#           doc/SCAMPER_SFE_Manual.pdf and the standalone integrators guide to doc/SCAMPER_SFE_Integrators_Guide.pdf
#   --keep  do not delete the previous output before building
#
# The per-module manuals are linked to each other through tag files, so they are built in dependency order. The
# complete manual (doc/Doxyfile) is built after them, followed by the standalone integrators guide
# (doc/integrators/Doxyfile). The script stops with an error if Doxygen writes anything to a doxygen_warnings.log, or if a
# LaTeX, BibTeX or latexmk run reports an error or a warning. One manual is built with doc/build_<manual>.sh; the shared
# code is in doc/build_lib.sh.
. "$(dirname "$0")/build_lib.sh"

build libraries/GMMLite doc
build libraries/ThreadPool doc
build libraries/TurboCodes doc
build libraries/HSTCFE doc
build libraries/MHSTCFE doc
build applications/HybridTurboCodeStrengthenedFuzzyExtractor doc
build applications/MultimodalHybridStrengthenedTurboCodeFuzzyExtractor doc
build doc .
build doc/integrators .

copy_pdf doc/latex doc/SCAMPER_SFE_Manual.pdf "Complete manual"
copy_pdf doc/integrators/latex doc/SCAMPER_SFE_Integrators_Guide.pdf "Guide for cryptographic integrators"
finish
