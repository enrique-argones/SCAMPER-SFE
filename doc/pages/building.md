Building, testing and installing {#building}
================================

[TOC]

Dependencies {#build_deps}
============

| Dependency | Needed for | Notes |
|------------|------------|-------|
| C++17 compiler | everything | GCC 9 or newer, or Clang 10 or newer, with \c std::filesystem. |
| CMake 3.23 or newer | everything | The \c FILE_SET HEADERS install syntax needs 3.23. |
| Armadillo | TurboCodes and above | Linear algebra \cite sanderson2016armadillo. Found with CMake's \c FindArmadillo module. |
| POSIX threads | ThreadPool and above | Found with CMake's \c Threads package. |
| argagg | the programs | Header-only command-line parser (\c argagg/argagg.hpp). Found with \c find_path. |
| Python 3 | tests | Only for the end-to-end test. |

On Debian or Ubuntu:

    sudo apt install build-essential cmake libarmadillo-dev python3
    # argagg (header only, MIT licence): copy argagg.hpp to PREFIX/include/argagg
    sudo mkdir -p /usr/local/include/argagg
    sudo curl -L -o /usr/local/include/argagg/argagg.hpp \
         https://raw.githubusercontent.com/vietjtnguyen/argagg/0.4.7/include/argagg/argagg.hpp

On Fedora, Armadillo is \c armadillo-devel.

Compiling, testing and installing {#build_compile}
=================================

The top-level \c CMakeLists.txt builds every library and program in one build tree, in dependency order:

    cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release [-DCMAKE_INSTALL_PREFIX=PREFIX]
    cmake --build build/release -j
    (cd build/release && ctest --output-on-failure)
    cmake --install build/release

| CMake option | Default | Meaning |
|--------------|---------|---------|
| \c CMAKE_BUILD_TYPE | \c Release | \c Release or \c Debug. Debug libraries and programs get the \c _Debug suffix and are compiled with \c DEBUG defined, which turns on verbose tracing. |
| \c CMAKE_INSTALL_PREFIX | \c /usr/local | Install prefix. |
| \c SFE_BUILD_TESTS | \c ON | Builds the tests (run them with \c ctest). |

The script \c scripts/install.sh does the same for the Release and Debug configurations, which can be installed in the same
prefix:

    scripts/install.sh [-p PREFIX] [-b BUILD_DIR] [-c Release|Debug|both] [-t] [-y]

It asks for confirmation (unless \c -y is given), runs the tests when \c -t is given, and uses \c sudo for the installation only
when the prefix is not writable.

Tests {#build_tests}
-----

| Test | What it checks |
|------|----------------|
| \c test_gmmlite | GMMLite: closed-form log-likelihoods, flat models, flooring, copies and sampler moments. |
| \c end_to_end | Generates two synthetic modalities, runs both programs in enrollment and test mode, and checks the false rejection and false acceptance rates (\c tests/end_to_end.py). |
| \c compare_with_chmm | Optional: bit-exact agreement of GMMLite with the CHMM library. Only built when CHMM is installed. |

Install locations {#build_install}
-----------------

No installation path is hard-coded. Everything is installed relative to \c CMAKE_INSTALL_PREFIX with the GNU directory
variables:

| What | Installed to |
|------|--------------|
| Libraries \c libGMMLite, \c libThreadPool, \c libTurboCodes, \c libHSTCFE, \c libMHSTCFE (and their \c _Debug versions) | \c PREFIX/lib |
| Headers | \c PREFIX/include/{GMMLite,ThreadPool,TurboCodes,HSTCFE,MHSTCFE} |
| Programs (and their \c _Debug versions) | \c PREFIX/bin |
| Turbo Code configuration files | \c PREFIX/share/TurboCodes/configFiles |
| Example configuration files | \c PREFIX/share/HSTCFE/configFiles, \c PREFIX/share/MultimodalHybridStrengthenedTurboCodeFuzzyExtractor/configFiles |
| License and README (top-level build only) | \c PREFIX/share/doc/SCAMPER_SFE |

The headers are installed in per-module folders, so they are included as <tt>\<TurboCodes/turbocode.h\></tt> and so on. The
installed libraries and programs find each other through a relative run path (\c $ORIGIN/../lib), so they run from any prefix
without setting \c LD_LIBRARY_PATH.

Building one module {#build_by_hand}
-------------------

Every module can also be configured on its own from its folder. It then uses the modules it depends on from an install prefix,
which is searched through \c CMAKE_PREFIX_PATH, the install prefix and the system folders:

    cmake -S libraries/HSTCFE -B build/HSTCFE -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_PREFIX_PATH=$HOME/sfe -DCMAKE_INSTALL_PREFIX=$HOME/sfe
    cmake --build build/HSTCFE -j
    cmake --install build/HSTCFE

The dependency order is GMMLite, ThreadPool, TurboCodes, HSTCFE, MHSTCFE, and then the two programs. A Debug build looks for
the \c _Debug libraries. If CMake reports that a library or header was not found, the module that provides it has not been
installed under the prefixes it searches.

The Rust binding is built with Cargo instead; see \ref rust_build.

Building the documentation {#build_docs}
==========================

The documentation is written in the Doxygen sources and in the Markdown pages of the \c doxygen folder of every module and of
\c doc/pages. There is one \c Doxyfile per module and one for the complete manual; all of them include the common settings of
\c doc/Doxyfile.common.

| Doxyfile | Output | Content |
|----------|--------|---------|
| \c doc/Doxyfile | \c doc/html, \c doc/latex | **Complete manual**: system pages, every library and program. |
| \c doc/integrators/Doxyfile | \c doc/integrators/html, \c doc/integrators/latex | **Guide for cryptographic integrators** alone (\ref integrators_guide_page). |
| \c libraries/GMMLite/Doxyfile | \c libraries/GMMLite/doc | GMMLite library. |
| \c libraries/ThreadPool/Doxyfile | \c libraries/ThreadPool/doc | ThreadPool library. |
| \c libraries/TurboCodes/Doxyfile | \c libraries/TurboCodes/doc | TurboCodes library. |
| \c libraries/HSTCFE/Doxyfile | \c libraries/HSTCFE/doc | HSTCFE library. |
| \c libraries/MHSTCFE/Doxyfile | \c libraries/MHSTCFE/doc | MHSTCFE library. |
| \c applications/HybridTurboCodeStrengthenedFuzzyExtractor/Doxyfile | \c .../doc | Unimodal program. |
| \c applications/MultimodalHybridStrengthenedTurboCodeFuzzyExtractor/Doxyfile | \c .../doc | Multimodal program. |

Tools {#build_docs_tools}
-----

| Tool | Notes |
|------|-------|
| Doxygen 1.15 or newer | Tested with 1.15.0 and 1.16.1. |
| Graphviz (\c dot) | Class, include and component diagrams. |
| Ghostscript | Formula images of the HTML manuals. |
| TeX Live with \c pdflatex, \c makeindex, \c bibtex and \c latexmk | PDF manuals. |
| LaTeX packages | Those of the TeX Live collections \c latex-base, \c latex-recommended, \c latex-extra (\c tabularray, \c etoc, ...), \c fonts-recommended, \c plain-generic (\c listofitems) and \c publishers (\c IEEEtran.bst, the IEEE Transactions bibliography style). |

On Debian or Ubuntu:

    sudo apt install doxygen graphviz ghostscript latexmk texlive-latex-base texlive-latex-recommended \
                     texlive-latex-extra texlive-fonts-recommended texlive-plain-generic texlive-publishers

If the distribution provides an older Doxygen, install the binary release of Doxygen 1.15 or newer from
<https://www.doxygen.nl/download.html>.

Generating the manuals {#build_docs_generate}
----------------------

The per-module manuals are linked through tag files, so they must be built in dependency order. The script
\c doc/build_docs.sh does it, builds the complete manual and then the standalone integrators guide, and fails if Doxygen,
LaTeX, BibTeX or latexmk report any warning:

    sh doc/build_docs.sh          # HTML and LaTeX sources of every manual
    sh doc/build_docs.sh --pdf    # also the PDF manuals, with latexmk; complete manual in doc/SCAMPER_SFE_Manual.pdf,
                                  # integrators guide in doc/SCAMPER_SFE_Integrators_Guide.pdf

The standalone guide is built from the same page as the complete manual. The input filter \c doc/integrators/filter.sed
turns its references to the other pages of the complete manual into plain text; a new reference of that kind needs a line in
that file, or the standalone build fails with a Doxygen warning.

To build one manual by hand, run Doxygen in the folder of its \c Doxyfile and latexmk in its \c latex output folder:

    cd doc
    doxygen Doxyfile                                  # HTML in html/index.html, LaTeX in latex
    (cd latex && latexmk -pdf refman.tex)             # PDF manual: latex/refman.pdf

Doxygen writes its warnings to \c doxygen_warnings.log next to each \c Doxyfile. The bibliography is \c doc/bibliography.bib;
the PDF manuals format it with \c IEEEtran.bst. \c doc/doxygen_sfe.sty, loaded by every \c Doxyfile, corrects two defects of the
LaTeX output of Doxygen that would otherwise produce LaTeX warnings (the off-screen measurement of tables and repeated section
labels).
