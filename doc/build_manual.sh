#!/bin/sh
# Builds the complete manual, doc/Doxyfile (HTML and LaTeX, and the PDF with --pdf), and checks that no warning is
# produced. The PDF is copied to doc/SCAMPER_SFE_Manual.pdf.
#
# Usage (from any folder):  sh doc/build_manual.sh [--pdf] [--keep] [--deps]
# See doc/build_lib.sh for the options; doc/build_docs.sh builds every manual.
. "$(dirname "$0")/build_lib.sh"

build doc .
copy_pdf doc/latex doc/SCAMPER_SFE_Manual.pdf "Complete manual"
finish
