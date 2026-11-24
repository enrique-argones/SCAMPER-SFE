#!/bin/sh
# Builds the standalone guide for cryptographic integrators, doc/integrators/Doxyfile (HTML and LaTeX, and the PDF with
# --pdf), and checks that no warning is produced. The PDF is copied to doc/SCAMPER_SFE_Integrators_Guide.pdf.
#
# Usage (from any folder):  sh doc/build_integrators.sh [--pdf] [--keep] [--deps]
# See doc/build_lib.sh for the options; doc/build_docs.sh builds every manual.
. "$(dirname "$0")/build_lib.sh"

build doc/integrators .
copy_pdf doc/integrators/latex doc/SCAMPER_SFE_Integrators_Guide.pdf "Guide for cryptographic integrators"
finish
