# Shared code of the documentation build scripts (doc/build_docs.sh and doc/build_<manual>.sh). Not meant to be run on its
# own: each script sources it with  . "$(dirname "$0")/build_lib.sh"  and then calls build, needs and finish.
#
# Options accepted by every script:
#   --pdf   also build the PDF manual(s) with latexmk (refman.pdf in the latex output folder)
#   --keep  do not delete the previous output before building
#   --deps  rebuild the manuals this one links to (HTML only), even if their tag files already exist
#
# A manual stops the script with an error if Doxygen writes anything to its doxygen_warnings.log, or if a LaTeX, BibTeX or
# latexmk run reports an error or a warning.
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SCRIPT="sh doc/$(basename "$0")"
PDF=no
CLEAN=yes
DEPS=no
for argument in "$@"; do
    case "$argument" in
        --pdf) PDF=yes ;;
        --keep) CLEAN=no ;;
        --deps) DEPS=yes ;;
        *) echo "Usage: $SCRIPT [--pdf] [--keep] [--deps]" >&2; exit 1 ;;
    esac
done

for tool in doxygen dot; do
    command -v "$tool" > /dev/null || { echo "error: $tool not found (see README.md, Documentation)" >&2; exit 1; }
done
[ "$PDF" = no ] || command -v latexmk > /dev/null || { echo "error: latexmk not found (see README.md, Documentation)" >&2; exit 1; }

FAILED=""

# build <folder with the Doxyfile> <output folder relative to it> [nopdf]
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
    if [ "$PDF" = yes ] && [ "$3" != nopdf ]; then
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

# needs <library>...: builds (HTML only) the library manuals whose tag files the next manual reads, when the tag file is
# missing or with --deps. List them in dependency order.
needs() {
    for library in "$@"; do
        if [ "$DEPS" = yes ] || [ ! -f "$ROOT/libraries/$library/doc/$library.tag" ]; then
            build "libraries/$library" doc nopdf
        fi
    done
}

# copy_pdf <latex folder relative to ROOT> <destination relative to ROOT> <description>
copy_pdf() {
    if [ "$PDF" = yes ] && [ -f "$ROOT/$1/refman.pdf" ]; then
        cp "$ROOT/$1/refman.pdf" "$ROOT/$2"
        echo "$3: $2"
    fi
}

# finish: reports the result and sets the exit status.
finish() {
    if [ -n "$FAILED" ]; then
        echo "FAILED:$FAILED" >&2
        exit 1
    fi
    echo "Documentation generated without warnings."
}
