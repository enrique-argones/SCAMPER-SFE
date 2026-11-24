#!/bin/sh
# Builds the manual of the multimodal program, MultimodalHybridStrengthenedTurboCodeFuzzyExtractor (HTML and LaTeX, and the
# PDF with --pdf), and checks that no warning is produced.
#
# Usage (from any folder):  sh doc/build_multimodal_program.sh [--pdf] [--keep] [--deps]
# See doc/build_lib.sh for the options; doc/build_docs.sh builds every manual.
. "$(dirname "$0")/build_lib.sh"

needs GMMLite ThreadPool TurboCodes HSTCFE MHSTCFE
build applications/MultimodalHybridStrengthenedTurboCodeFuzzyExtractor doc
finish
