#!/bin/sh
# Builds the manual of the GMMLite library (HTML and LaTeX, and the PDF with --pdf) and checks that no warning is produced.
#
# Usage (from any folder):  sh doc/build_gmmlite.sh [--pdf] [--keep] [--deps]
# See doc/build_lib.sh for the options; doc/build_docs.sh builds every manual.
. "$(dirname "$0")/build_lib.sh"

build libraries/GMMLite doc
finish
