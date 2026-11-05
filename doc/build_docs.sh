#!/bin/sh
# Builds every Doxygen manual of SCAMPER SFE (HTML and LaTeX, and the PDFs with --pdf) and checks that no warning is produced.
#
# Usage (from any folder):  sh doc/build_docs.sh [--pdf] [--keep]
#   --pdf   also build the PDF manuals with latexmk (refman.pdf in every latex folder); the complete manual is copied to
#           doc/SCAMPER_SFE_Manual.pdf
#   --keep  do not delete the previous output before building
#
# The per-module manuals are linked to each other through tag files, so they are built in dependency order, and the
# complete manual (doc/Doxyfile) is built last. The script stops with an error if Doxygen writes anything to a
# doxygen_warnings.log, or if a LaTeX, BibTeX or latexmk run reports an error or a warning.
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PDF=no
CLEAN=yes
for argument in "$@"; do
    case "$argument" in
        --pdf) PDF=yes ;;
        --keep) CLEAN=no ;;
        *) echo "Usage: sh doc/build_docs.sh [--pdf] [--keep]" >&2; exit 1 ;;
    esac
done

for tool in doxygen dot; do
    command -v "$tool" > /dev/null || { echo "error: $tool not found (see README.md, Documentation)" >&2; exit 1; }
done
[ "$PDF" = no ] || command -v latexmk > /dev/null || { echo "error: latexmk not found (see README.md, Documentation)" >&2; exit 1; }

FAILED=""

# build <folder with the Doxyfile> <output folder relative to it>
build() {
    folder="$ROOT/$1"
    output="$folder/$2"
    shown="$1/$2"; [ "$2" = . ] && shown="$1"
    echo "== $1"
    if [ "$CLEAN" = yes ]; then
        rm -rf "$output/html" "$output/latex"
    fi
    (cd "$folder" && doxygen Doxyfile)
    if [ -s "$folder/doxygen_warnings.log" ]; then
        echo "   Doxygen warnings ($(wc -l < "$folder/doxygen_warnings.log")): see $1/doxygen_warnings.log"
        FAILED="$FAILED $1(doxygen)"
    else
        echo "   HTML: $shown/html/index.html (no Doxygen warnings)"
    fi
    if [ "$PDF" = yes ]; then
        latex="$output/latex"
        if (cd "$latex" && latexmk -pdf -interaction=nonstopmode -halt-on-error refman.tex > latexmk.out 2>&1); then
            # LaTeX, package, font and BibTeX warnings, and latexmk's own warnings about the run.
            warnings="$(cat "$latex/refman.log" "$latex/latexmk.out" "$latex/refman.blg" 2>/dev/null \
                        | grep -E -i "warning" | grep -v -E "^warning\\$ -- 0$|[(][0-9]+ lines written, 0 warnings[)]" || true)"
            if [ -n "$warnings" ]; then
                echo "$warnings" | sed 's/^/   /' | head -20
                FAILED="$FAILED $1(latex)"
            else
                echo "   PDF:  $shown/latex/refman.pdf (no LaTeX warnings)"
            fi
        else
            echo "   PDF build failed: see $shown/latex/refman.log and latexmk.out"
            FAILED="$FAILED $1(latex)"
        fi
    fi
}

build libraries/GMMLite doc
build libraries/ThreadPool doc
build libraries/TurboCodes doc
build libraries/HSTCFE doc
build libraries/MHSTCFE doc
build applications/HybridTurboCodeStrengthenedFuzzyExtractor doc
build applications/MultimodalHybridStrengthenedTurboCodeFuzzyExtractor doc
build doc .

if [ "$PDF" = yes ] && [ -f "$ROOT/doc/latex/refman.pdf" ]; then
    cp "$ROOT/doc/latex/refman.pdf" "$ROOT/doc/SCAMPER_SFE_Manual.pdf"
    echo "Complete manual: doc/SCAMPER_SFE_Manual.pdf"
fi

if [ -n "$FAILED" ]; then
    echo "FAILED:$FAILED" >&2
    exit 1
fi
echo "All manuals generated without warnings."
