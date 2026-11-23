# Input filter of the standalone guide (doc/integrators/Doxyfile), applied to doc/pages/integrators_guide.md.
# The guide is the main page here, so its page label is dropped, and its references to pages that only exist in the
# complete manual are replaced with plain text. A new reference of that kind makes Doxygen warn in the standalone build
# ("unable to resolve reference"), and build_docs.sh fails until a line is added here.
s/[[:space:]]{#integrators_guide_page}//
s/\\ref rust_binding_page/the "Rust binding" page of the complete SCAMPER SFE manual/
s/(\\ref td_security)/(see "Security considerations" in the complete SCAMPER SFE manual)/
s/(\\ref ff_general)/(see "HSTCFE general configuration file" in the complete SCAMPER SFE manual)/
s/in the format of \\ref ff_samples/in the samples-file format described in the complete SCAMPER SFE manual/
