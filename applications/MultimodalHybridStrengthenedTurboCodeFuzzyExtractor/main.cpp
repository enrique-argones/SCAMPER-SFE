/// \file MultimodalHybridStrengthenedTurboCodeFuzzyExtractor/main.cpp
/// \brief Command-line program to enroll chimeric multimodal subjects and evaluate the
/// \ref MultimodalHybridStrengthenedTurboCodeFuzzyExtractor.
///
/// \section mhstcfe_usage Usage
/// \code{.unparsed}
/// MultimodalHybridStrengthenedTurboCodeFuzzyExtractor -c CONFIG_FILE -m MODE [-R]
/// \endcode
///
/// | Option | Description |
/// |--------|-------------|
/// | \c -h, \c --help | Shows the help message. |
/// | \c -c, \c --configFile | Multimodal experiment configuration file (format below). Mandatory. |
/// | \c -m, \c --mode | Operation mode: \c enrollment, \c test, or \c both. Mandatory. |
/// | \c -R, \c --resultsOnly | In test mode, only write \c authenticationResults.txt (no \c .recovered files). |
///
/// \section mhstcfe_config Configuration file
/// Label/value pairs, in this order (\c MC = number of modalities; every path must exist):
/// \code{.unparsed}
/// codeOffsetStdDevFactor REAL
/// codeOffsetStdDevOffset REAL
/// fuzzyCommitmentMinReliability REAL
/// modalitiesCount MC
/// enrollmentSamplesFilesFolders
///     ENROLLMENT_FOLDER[1] ... ENROLLMENT_FOLDER[MC]
/// probeSamplesFilesFolders
///     PROBE_FOLDER[1] ... PROBE_FOLDER[MC]
/// hybridStrengthenedTurboCodeFuzzyExtractorFilesFolders
///     UNIMODAL_HSTCFE_FOLDER[1] ... UNIMODAL_HSTCFE_FOLDER[MC]
/// hybridStrengthenedTurboCodeFuzzyExtractorGeneralConfigFiles
///     UNIMODAL_GENERAL_CONFIG[1] ... UNIMODAL_GENERAL_CONFIG[MC]
/// multimodalHybridStrengthenedTurboCodeFuzzyExtractorFilesFolder MULTIMODAL_HSTCFE_FOLDER
/// recoveredMessagesFilesFolder RECOVERED_FOLDER
/// \endcode
/// The unimodal HSTCFE folders must already contain the user-specific files produced by the unimodal program, since they hold
/// the per-modality normalizers and user identifiers.
///
/// \section mhstcfe_enrollment Enrollment mode
/// The user identifiers of each modality are shuffled independently and paired position by position to form
/// \e chimeric subjects (the number of chimeric subjects is the size of the smallest modality). Each chimeric subject gets the
/// identifier \c ID1_ID2_..._IDMC, a \ref MultimodalHybridStrengthenedTurboCodeFuzzyExtractor is enrolled from the per-modality
/// enrollment samples, and \c ID.userSpecificHSTCFE and \c ID.Key are written to the multimodal folder, together with
/// \c generalConfigFile.generalHSTCFE.
///
/// \section mhstcfe_test Test mode
/// For each chimeric subject, multimodal probe samples are built as the Cartesian product of its per-modality probe samples
/// (\ref buildUserSamples), and every chimeric probe set is compared against every enrolled chimeric template.
/// \c authenticationResults.txt receives one line per probe sample:
/// \code{.unparsed}
/// PROBE_CHIMERIC_ID TEMPLATE_CHIMERIC_ID AUTHENTICATED GENUINE
/// \endcode
/// @note Unimodal user identifiers are assumed to contain exactly one underscore (e.g. \c face_012), because chimeric
/// identifiers are split at every second underscore.
/// \author Enrique Argones Rúa
/// \date 2025

#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <cstdlib>
#include <argagg/argagg.hpp>
#include <armadillo>
#include <HSTCFE/hybridstrengthenedturbocodefuzzyextractor.h>
#include <MHSTCFE/mhstce.h>
#include <TurboCodes/turbocode.h>
#include <TurboCodes/psk.h>
#include <TurboCodes/vectorfiles.h>
#include <ThreadPool/ThreadPool.hpp>
#include <ThreadPool/PendingTasksGuard.hpp>
#include <atomic>

/// Maximum number of modalities (currently not enforced).
#define MAX_MODALITIES_COUNT 10

/// Parameters of the enrollment mode.
typedef struct {
    /// Folder with one \c .txt enrollment samples file per subject.
    std::string enrollmentSamplesFilesFolderName;
    /// Bit-loading factor (not used).
    real_number stdDevFactor;
    /// Bit-loading offset (not used).
    real_number stdDevOffset;
    /// Minimum fuzzy-commitment reliability (not used).
    real_number fuzzyCommitmentMinimumReliability;
} EnrollmentModeParameters;

/// Parameters of the test mode.
typedef struct {
    /// Folder with one \c .txt probe samples file per subject.
    std::string probeSamplesFilesFolderName;
    /// Output folder for the recovered messages and the authentication results.
    std::string recoveredMessagesFilesFolderName;
} TestModeParameters;

/// Parameters common to both modes.
typedef struct {
    /// General HSTCFE configuration file.
    std::string generalHybridFuzzyExtractorConfigFileName;
    /// Folder for the user-specific helper data (\c .userSpecificHSTCFE) and key (\c .Key) files.
    std::string hybridFuzzyExtractorFilesFolderName;
} GeneralModeParameters;

/// Prints the structure of the configuration file to standard error.
void printConfigFileHelp() {
    std::cerr << "Structure of config files:" << std::endl;
    std::cerr << "---------------------------------------------------------------------------" << std::endl;
    std::cerr << "codeOffsetStdDevFactor REAL" << std::endl;
    std::cerr << "codeOffsetStdDevOffset REAL" << std::endl;
    std::cerr << "fuzzyCommitmentMinReliability REAL" << std::endl;
    std::cerr << "modalitiesCount INT_MC" << std::endl;
    std::cerr << "enrollmentSamplesFilesFolders" << std::endl;
    std::cerr << "\tSTR_ES_FF[1]" << std::endl;
    std::cerr << "\t..." << std::endl;
    std::cerr << "\tSTR_ES_FF[INT_MC]" << std::endl;
    std::cerr << "probeSamplesFilesFolders" << std::endl;
    std::cerr << "\tSTR_PS_FF[1]" << std::endl;
    std::cerr << "\t..." << std::endl;
    std::cerr << "\tSTR_PS_FF[INT_MC]" << std::endl;
    std::cerr << "hybridStrengthenedTurboCodeFuzzyExtractorFilesFolders" << std::endl;
    std::cerr << "\tSTR_HFE_FF[1]" << std::endl;
    std::cerr << "\t..." << std::endl;
    std::cerr << "\tSTR_HFE_FF[INT_MC]" << std::endl;
    std::cerr << "hybridStrengthenedTurboCodeFuzzyExtractorGeneralConfigFiles" << std::endl;
    std::cerr << "\tSTR_GCF[1]" << std::endl;
    std::cerr << "\t..." << std::endl;
    std::cerr << "\tSTR_GCF[INT_MC]" << std::endl;
    std::cerr << "multimodalHybridStrengthenedTurboCodeFuzzyExtractorFilesFolder STR_MHSFE_FF" << std::endl;
    std::cerr << "recoveredMessagesFilesFolder STR_RM_FF" << std::endl;
    std::cerr << "---------------------------------------------------------------------------" << std::endl;
}

/// Reads \p count paths from a stream, checking that each one exists.
/// @param[in,out] istr Input stream.
/// @param[in] count Number of paths to read.
/// @param[out] pathsVector Paths read.
/// @exception std::logic_error if a path does not exist.
void parsePathsVector(std::istream &istr, unsigned int const count, std::vector<std::string> &pathsVector) {
    pathsVector.clear();
    std::string path;
    for (unsigned int index = 0; index < count; index++) {
        istr >> path;
        if (!std::filesystem::exists(path)) {
            std::string exceptionMessage = std::string("Path for modality ") +
                                           std::to_string(index + 1) + std::string("\"") +
                                           path +
                                           std::string("\" does not exist.");
            throw std::logic_error(exceptionMessage);
        }
        pathsVector.push_back(path);
    }
}

/// Reads one path from a stream, checking that it exists.
/// @param[in,out] istr Input stream.
/// @param[out] path Path read.
/// @exception std::logic_error if the path does not exist.
void parsePath(std::istream &istr, std::string &path) {
    istr >> path;
    if (!std::filesystem::exists(path)) {
        std::string exceptionMessage = std::string("Path \"") +
                                       path +
                                       std::string("\" does not exist.");
        throw std::logic_error(exceptionMessage);
    }
}

/// Parses the multimodal experiment configuration file (see the file documentation for its layout).
/// @param[in,out] istr Stream opened on the configuration file.
/// @param[out] codeOffsetStdDevFactor Bit-loading factor.
/// @param[out] codeOffsetStdDevOffset Bit-loading offset.
/// @param[out] fuzzyCommitmentMinReliability Minimum reliability of the combined fuzzy-commitment features.
/// @param[out] modalitiesCount Number of modalities.
/// @param[out] enrollmentSamplesFilesFolders Enrollment samples folder of each modality.
/// @param[out] probeSamplesFilesFolders Probe samples folder of each modality.
/// @param[out] hybridStrengthenedTurboCodeFuzzyExtractorFilesFolders Unimodal HSTCFE folder of each modality.
/// @param[out] hybridStrengthenedTurboCodeFuzzyExtractorGeneralConfigFiles Unimodal general configuration file of each modality.
/// @param[out] multimodalHybridStrengthenedTurboCodeFuzzyExtractorFilesFolder Output folder for the multimodal helper data.
/// @param[out] recoveredMessagesFilesFolder Output folder for the recovered messages and authentication results.
/// @exception std::logic_error if a label is not the expected one or a path does not exist.
void parseConfigFile(std::istream &istr,
                     real_number &codeOffsetStdDevFactor,
                     real_number &codeOffsetStdDevOffset,
                     real_number &fuzzyCommitmentMinReliability,
                     unsigned int &modalitiesCount,
                     std::vector<std::string> &enrollmentSamplesFilesFolders,
                     std::vector<std::string> &probeSamplesFilesFolders,
                     std::vector<std::string> &hybridStrengthenedTurboCodeFuzzyExtractorFilesFolders,
                     std::vector<std::string> &hybridStrengthenedTurboCodeFuzzyExtractorGeneralConfigFiles,
                     std::string &multimodalHybridStrengthenedTurboCodeFuzzyExtractorFilesFolder,
                     std::string &recoveredMessagesFilesFolder) {
    std::string header;
    std::string path;
    std::string integerString;
    real_number floatingPointNumber;
    unsigned int integer;
    istr >> header;
    if (header != std::string("codeOffsetStdDevFactor")) {
        std::string exceptionMessage = std::string("Expected header \"codeOffsetStdDevFactor\", but \"") +
                                       header +
                                       std::string("\" was read instead.");
        throw std::logic_error(exceptionMessage);
    }
    istr >> codeOffsetStdDevFactor;
    istr >> header;
    if (header != std::string("codeOffsetStdDevOffset")) {
        std::string exceptionMessage = std::string("Expected header \"codeOffsetStdDevOffset\", but \"") +
                                       header +
                                       std::string("\" was read instead.");
        throw std::logic_error(exceptionMessage);
    }
    istr >> codeOffsetStdDevOffset;
    istr >> header;
    if (header != std::string("fuzzyCommitmentMinReliability")) {
        std::string exceptionMessage = std::string("Expected header \"fuzzyCommitmentMinReliability\", but \"") +
                                       header +
                                       std::string("\" was read instead.");
        throw std::logic_error(exceptionMessage);
    }
    istr >> fuzzyCommitmentMinReliability;
    istr >> header;
    if (header != std::string("modalitiesCount")) {
        std::string exceptionMessage = std::string("Expected header \"modalitiesCount\", but \"") +
                                       header +
                                       std::string("\" was read instead.");
        throw std::logic_error(exceptionMessage);
    }
    istr >> integerString;
    integer = std::stoi(integerString);
    if ((integer < 2) || (integer > MAX_MODALITIES_COUNT)) {
        std::string exceptionMessage = std::string("Expected value of \"modalitiesCount\" must be in the closed interval [2,10], but \"") +
                                       integerString +
                                       std::string("\" was read instead.");
        throw std::logic_error(exceptionMessage);
    }
    modalitiesCount = integer;

    istr >> header;
    if (header != std::string("enrollmentSamplesFilesFolders")) {
        std::string exceptionMessage = std::string("Expected header \"enrollmentSamplesFilesFolders\", but \"") +
                                       header +
                                       std::string("\" was read instead.");
        throw std::logic_error(exceptionMessage);
    }
    parsePathsVector(istr, modalitiesCount, enrollmentSamplesFilesFolders);

    istr >> header;
    if (header != std::string("probeSamplesFilesFolders")) {
        std::string exceptionMessage = std::string("Expected header \"probeSamplesFilesFolders\", but \"") +
                                       header +
                                       std::string("\" was read instead.");
        throw std::logic_error(exceptionMessage);
    }
    parsePathsVector(istr, modalitiesCount, probeSamplesFilesFolders);

    istr >> header;
    if (header != std::string("hybridStrengthenedTurboCodeFuzzyExtractorFilesFolders")) {
        std::string exceptionMessage = std::string("Expected header \"hybridStrengthenedTurboCodeFuzzyExtractorFilesFolders\", but \"") +
                                       header +
                                       std::string("\" was read instead.");
        throw std::logic_error(exceptionMessage);
    }
    parsePathsVector(istr, modalitiesCount, hybridStrengthenedTurboCodeFuzzyExtractorFilesFolders);

    istr >> header;
    if (header != std::string("hybridStrengthenedTurboCodeFuzzyExtractorGeneralConfigFiles")) {
        std::string exceptionMessage = std::string("Expected header \"hybridStrengthenedTurboCodeFuzzyExtractorFilesFolders\", but \"") +
                                       header +
                                       std::string("\" was read instead.");
        throw std::logic_error(exceptionMessage);
    }
    parsePathsVector(istr, modalitiesCount, hybridStrengthenedTurboCodeFuzzyExtractorGeneralConfigFiles);

    istr >> header;
    if (header != std::string("multimodalHybridStrengthenedTurboCodeFuzzyExtractorFilesFolder")) {
        std::string exceptionMessage = std::string("Expected header \"hybridStrengthenedTurboCodeFuzzyExtractorFilesFolders\", but \"") +
                                       header +
                                       std::string("\" was read instead.");
        throw std::logic_error(exceptionMessage);
    }
    parsePath(istr, multimodalHybridStrengthenedTurboCodeFuzzyExtractorFilesFolder);

    istr >> header;
    if (header != std::string("recoveredMessagesFilesFolder")) {
        std::string exceptionMessage = std::string("Expected header \"hybridStrengthenedTurboCodeFuzzyExtractorFilesFolders\", but \"") +
                                       header +
                                       std::string("\" was read instead.");
        throw std::logic_error(exceptionMessage);
    }
    parsePath(istr, recoveredMessagesFilesFolder);
}

/// Loads all the multimodal user-specific files of a folder.
/// @param[in] multimodalHybridStrengthenedTurboCodeFuzzyExtractorFilesFolder Folder with the multimodal user-specific files.
/// @param[in] generalMHSTCFE Object holding the multimodal general data; it is copied for every file.
/// @param[out] userMHSTCFEsVector Loaded multimodal fuzzy extractors.
/// @param[in] userSpecificMHSTEFEFilesExtension Extension of the files to load, including the leading dot.
/// @param[out] userSpecificMHSTEFEFilesNames Stems of the loaded files (chimeric identifiers).
/// @exception std::ifstream::failure if the folder does not exist or is not a directory.
/// @exception std::logic_error if a file cannot be loaded.
void loadMultimodalHybridStrengthenedTurboCodeFuzzyExtractors(const std::string multimodalHybridStrengthenedTurboCodeFuzzyExtractorFilesFolder,
                                                              const MultimodalHybridStrengthenedTurboCodeFuzzyExtractor &generalMHSTCFE,
                                                              std::vector<MultimodalHybridStrengthenedTurboCodeFuzzyExtractor> &userMHSTCFEsVector,
                                                              const std::string &userSpecificMHSTEFEFilesExtension,
                                                              std::vector<std::string> &userSpecificMHSTEFEFilesNames) {
    userMHSTCFEsVector.clear();
    userSpecificMHSTEFEFilesNames.clear();
    std::filesystem::path mhstcfeFilesPath(multimodalHybridStrengthenedTurboCodeFuzzyExtractorFilesFolder);
    if (!std::filesystem::exists(mhstcfeFilesPath)) {
        throw std::ifstream::failure(std::string("MultimodalHybridStrengthenedTurboCodeFuzzyExtractor files path ") +
                                     std::string(multimodalHybridStrengthenedTurboCodeFuzzyExtractorFilesFolder) +
                                     std::string(" does not exist."));
    }
    if (!std::filesystem::is_directory(mhstcfeFilesPath)) {
        throw std::ifstream::failure(std::string("Enrollment samples files path ") + std::string(mhstcfeFilesPath) + std::string(" is not a directory."));
    }
    for (auto const& dir_entry : std::filesystem::directory_iterator{mhstcfeFilesPath}) {
        if (dir_entry.is_regular_file()) {
            std::string extension = dir_entry.path().extension();
            std::string stem = dir_entry.path().stem();
            if ((extension == userSpecificMHSTEFEFilesExtension) && (dir_entry.path().has_stem())) {
                userMHSTCFEsVector.push_back(generalMHSTCFE);
                auto last = userMHSTCFEsVector.rbegin();
                try {
#ifdef DEBUG
                    std::cout << "Reading user-specific MHSTCFE configuration file from " << stem << " ... ";
                    std::cout.flush();
#endif
                    last->loadUserSpecificData(dir_entry.path());
#ifdef DEBUG
                    std::cout << std::endl;
#endif
                } catch (std::exception &e) {
                    throw std::logic_error(std::string("Exception caught while doing MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::loadUserSpecificData: ") + e.what());
                }
                userSpecificMHSTEFEFilesNames.push_back(stem);
            }
        }
    }

}

/// Loads, for every modality, the unimodal general data and all the unimodal user-specific files.
/// @param[in] hybridStrengthenedTurboCodeFuzzyExtractorFilesFolders Unimodal HSTCFE folder of each modality.
/// @param[in] hybridStrengthenedTurboCodeFuzzyExtractorGeneralConfigFiles Unimodal general configuration file of each modality.
/// @param[out] hstcfeVectorVector For each modality, the loaded unimodal fuzzy extractors.
/// @param[out] userIdsVectorVector For each modality, the identifiers of the loaded fuzzy extractors.
/// @exception std::logic_error if both vectors have different lengths or a file cannot be loaded.
void loadHybridStrengthenedTurboCodeFuzzyExtractors(const std::vector<std::string> hybridStrengthenedTurboCodeFuzzyExtractorFilesFolders,
                                                    const std::vector<std::string> hybridStrengthenedTurboCodeFuzzyExtractorGeneralConfigFiles,
                                                    std::vector<std::vector<HybridStrengthenedTurboCodeFuzzyExtractor>> &hstcfeVectorVector,
                                                    std::vector<std::vector<std::string>> &userIdsVectorVector) {
    unsigned int modalitiesCount = hybridStrengthenedTurboCodeFuzzyExtractorFilesFolders.size();
    if (modalitiesCount != hybridStrengthenedTurboCodeFuzzyExtractorGeneralConfigFiles.size()) {
        std::string exceptionMessage = std::string("Inconsistent number of modalities. HSTCFE folders vector has length ") +
                                       std::to_string(modalitiesCount) +
                                       std::string(", while HSTCFE general config files vector has length ") +
                                       std::to_string(hybridStrengthenedTurboCodeFuzzyExtractorGeneralConfigFiles.size()) +
                                       std::string(".");
        throw std::logic_error(exceptionMessage);
    }
    hstcfeVectorVector.clear();
    userIdsVectorVector.clear();
    for (unsigned int modalityI = 0; modalityI < modalitiesCount; modalityI++) {
        HybridStrengthenedTurboCodeFuzzyExtractor hstcfe;
        std::vector<HybridStrengthenedTurboCodeFuzzyExtractor> hstcfeVector;
        std::vector<std::string> userIds;
        hstcfe.loadGeneralData(hybridStrengthenedTurboCodeFuzzyExtractorGeneralConfigFiles[modalityI]);
        try {
            readHybridStrengthenedTurboCodeFuzzyExtractorsFromPath(
                hybridStrengthenedTurboCodeFuzzyExtractorFilesFolders[modalityI],
                ".userSpecificHSTCFE",
                hstcfe,
                userIds,
                hstcfeVector);
        } catch (std::exception &e) {
            throw std::logic_error(std::string("Exception caught when reading user-specific HSTCFE files: ") + std::string(e.what()));
        }
        hstcfeVectorVector.push_back(hstcfeVector);
        userIdsVectorVector.push_back(userIds);
    }
}

///
/// \brief matchUnimodalAndMultimodalHSTCFEs For each element of \p mhstcfeVector the correct elements from each modality in \p hstcfeVectorVector are added
/// \param[in] hstcfeVectorVector For each modality, the loaded unimodal fuzzy extractors.
/// \param[in,out] mhstcfeVector Multimodal fuzzy extractors; their unimodal extractors are set with
/// \ref MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::setHstcfesNoFurtherUpdates.
/// \exception std::logic_error if a modality identifier is not found.
///
void matchUnimodalAndMultimodalHSTCFEs(const std::vector<std::vector<HybridStrengthenedTurboCodeFuzzyExtractor>> &hstcfeVectorVector,
                                       std::vector<MultimodalHybridStrengthenedTurboCodeFuzzyExtractor> &mhstcfeVector) {
    unsigned int modalitiesCount = hstcfeVectorVector.size();
    for (unsigned int mhstcfeI = 0; mhstcfeI < mhstcfeVector.size(); mhstcfeI++) {
        std::string fullId = mhstcfeVector[mhstcfeI].userId();
        std::string remainingId = fullId;
        std::vector<HybridStrengthenedTurboCodeFuzzyExtractor> thisIdHSTCFEVector;
        for (unsigned int modalityI = 0; modalityI < modalitiesCount; modalityI++) {
            std::string modId;
            if (modalityI < modalitiesCount - 1) {
                std::size_t pos = remainingId.find("_");
                pos = remainingId.find("_", pos + 1);
                if (pos != std::string::npos) {
                    modId = remainingId.substr(0, pos);
                    remainingId = remainingId.substr(pos + 1, std::string::npos);
                }
            } else {
                modId = remainingId;
                remainingId = std::string();
            }
            bool found(false);
            for (unsigned int thisModalityHSTCFEI = 0;
                 thisModalityHSTCFEI < hstcfeVectorVector[modalityI].size();
                 thisModalityHSTCFEI++) {
                if (modId == hstcfeVectorVector[modalityI][thisModalityHSTCFEI].userId()) {
                    found = true;
                    thisIdHSTCFEVector.push_back(hstcfeVectorVector[modalityI][thisModalityHSTCFEI]);
                    break;
                }
            }
            if (!found) {
                std::string exceptionMessage = std::string("User ID ") + modId +
                        std::string(" not found for modality ") + std::to_string(modalityI);
                throw std::logic_error(exceptionMessage);
            }
        }
        mhstcfeVector[mhstcfeI].setHstcfesNoFurtherUpdates(thisIdHSTCFEVector);
    }
}

///
/// \brief buildUserSamples Builds the user samples combining each sample from each modality with all other samples from the other modalities
/// \param[in] thisUserModalitySamples For each modality, the samples of one subject.
/// \param[out] userSamples All \f$\prod_m n_m\f$ concatenated multimodal samples, \f$n_m\f$ being the number of samples of modality \f$m\f$.
///
void buildUserSamples(std::vector<real_number_vector_vector> &thisUserModalitySamples, real_number_vector_vector &userSamples) {
    unsigned int modalitiesCount = thisUserModalitySamples.size();
    unsigned int totalSamplesCount = 1;
    for (unsigned int modalityI = 0; modalityI < modalitiesCount; modalityI++) {
        totalSamplesCount *= thisUserModalitySamples[modalityI].size();
    }
    userSamples = real_number_vector_vector(totalSamplesCount);
    for (unsigned int sampleI = 0; sampleI < totalSamplesCount; sampleI++) {
        unsigned int sampleTempI = sampleI;
        real_number_vector sample;
        unsigned int modalitySampleCoordinate;
        for (unsigned int modalityI = 0; modalityI < modalitiesCount; modalityI++) {
            modalitySampleCoordinate = (sampleTempI % thisUserModalitySamples[modalityI].size());
            sampleTempI = sampleTempI / thisUserModalitySamples[modalityI].size();
            sample.insert(sample.end(),
                          thisUserModalitySamples[modalityI][modalitySampleCoordinate].begin(),
                          thisUserModalitySamples[modalityI][modalitySampleCoordinate].end());
        }
        userSamples[sampleI] = sample;
    }
}


/// Program entry point. See the file documentation for the command-line options, configuration file and output files.
/// @param argc Number of command-line arguments.
/// @param argv Command-line arguments.
/// @return \c EXIT_SUCCESS on success, \c EXIT_FAILURE on error.
int main(int argc, char *argv[])
{
    std::string configFileName;
    real_number
        codeOffsetStdDevFactor,
        codeOffsetStdDevOffset,
        fuzzyCommitmentMinReliability;
    unsigned int modalitiesCount;
    std::vector<std::string> enrollmentSamplesFilesFolders;
    std::vector<std::string> probeSamplesFilesFolders;
    std::vector<std::string> hybridStrengthenedTurboCodeFuzzyExtractorFilesFolders;
    std::vector<std::string> hybridStrengthenedTurboCodeFuzzyExtractorGeneralConfigFiles;
    std::string multimodalHybridStrengthenedTurboCodeFuzzyExtractorFilesFolder;
    std::string recoveredMessagesFilesFolder;

    std::vector<MultimodalHybridStrengthenedTurboCodeFuzzyExtractor> mhstcfeVector;
    std::vector<std::string> mhstcfeIds;

    bool resultsOnly(false);
    bool enrollmentMode(false);
    bool testMode(false);

    std::random_device randomDevice;
    std::mt19937 randomGenerator(randomDevice());

    argagg::parser argparser {{
        { "help",
         {"-h", "--help"},
         "shows this help message.",
         0},
        {"configFile",
         {"-c", "--configFile"},
         "Path of the configuration file",
         1},
        {"mode",
         {"-m","--mode"},
         "Operation mode {\"enrollment\", \"test\", or \"both\"}.",
         1},
        {"resultsOnly",
         {"-R","--resultsOnly"},
         "Results only. If given, only the authentication results are stored in the recovered messages folder.",
         0}
    }};

    argagg::parser_results args;

    try {
        args = argparser.parse(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    if (args["help"]) {
        argagg::fmt_ostream fmt(std::cerr);
        fmt << "Usage: " << argv[0] << " [OPTION]\n" << argparser;
        return EXIT_SUCCESS;
    }

    if (!args["configFile"]) {
        argagg::fmt_ostream fmt(std::cerr);
        fmt << "configFile is a mandatory argument." << std::endl;
        fmt << "Usage: " << argv[0] << " [OPTION]\n" << argparser;
        return EXIT_FAILURE;
    } else {
        configFileName = args["configFile"].as<std::string>("");
        try {
            std::ifstream configFile(configFileName);
            std::cout << "Parsing config file " << configFileName << "... ";
            parseConfigFile(configFile,
                            codeOffsetStdDevFactor,
                            codeOffsetStdDevOffset,
                            fuzzyCommitmentMinReliability,
                            modalitiesCount,
                            enrollmentSamplesFilesFolders,
                            probeSamplesFilesFolders,
                            hybridStrengthenedTurboCodeFuzzyExtractorFilesFolders,
                            hybridStrengthenedTurboCodeFuzzyExtractorGeneralConfigFiles,
                            multimodalHybridStrengthenedTurboCodeFuzzyExtractorFilesFolder,
                            recoveredMessagesFilesFolder);
            configFile.close();
            std::cout << "Done." << std::endl;
        } catch (std::exception &e) {
            std::cerr << "Exception raised while processing configuration file " << configFileName << ": "
                      << e.what() << std::endl;
            return EXIT_FAILURE;
        }
    }

    if (args["resultsOnly"]) {
        resultsOnly = true;
    }

    if (!args["mode"]) {
        argagg::fmt_ostream fmt(std::cerr);
        fmt << "mode is a mandatory argument." << std::endl;
        fmt << "Usage: " << argv[0] << " [OPTION]\n" << argparser;
        return EXIT_FAILURE;
    } else {
        if (args["mode"].as<std::string>("") == std::string("enrollment")) {
            std::cout << "Enrollment mode." << std::endl;
            enrollmentMode = true;
        }
        if (args["mode"].as<std::string>("") == std::string("test")) {
            std::cout << "Test mode." << std::endl;
            testMode = true;
        }
        if (args["mode"].as<std::string>("") == std::string("both")) {
            std::cout << "Enrollment and Test mode." << std::endl;
            enrollmentMode = true;
            testMode = true;
        }
    }

    if (enrollmentMode) {
        std::cout << "Starting enrollment stage..." << std::endl;
        std::vector<std::vector<HybridStrengthenedTurboCodeFuzzyExtractor>> hstcfeVectorVector;
        std::vector<std::vector<std::string>> userIdsVectorVector(modalitiesCount);
        std::vector<std::vector<std::string>> allEnrollmentSamplesFilesNames(modalitiesCount);
        std::vector<std::vector<real_number_vector_vector>> allEnrollmentSamples(modalitiesCount);
        std::cout << "Loading Hybrid Strengthened Turbo Code Fuzzy Extractors" << std::endl;
        for (unsigned int modalityI = 0; modalityI < modalitiesCount; modalityI++) {
            std::cout << "Modality " << modalityI + 1 << " general config file and user specific configuration files folder:";
            std::cout.flush();
            std::cout << "\n\t" << hybridStrengthenedTurboCodeFuzzyExtractorGeneralConfigFiles[modalityI]
                      << "\n\t" << hybridStrengthenedTurboCodeFuzzyExtractorFilesFolders[modalityI] << std::endl;
        }
        loadHybridStrengthenedTurboCodeFuzzyExtractors(hybridStrengthenedTurboCodeFuzzyExtractorFilesFolders,
                                                       hybridStrengthenedTurboCodeFuzzyExtractorGeneralConfigFiles,
                                                       hstcfeVectorVector,
                                                       userIdsVectorVector);
        for (unsigned int modalityI = 0; modalityI < modalitiesCount; modalityI++) {
            std::cout << "Reading samples from modality " << modalityI + 1 << "... ";
            std::cout.flush();
            readSamplesFilesFromPath(enrollmentSamplesFilesFolders[modalityI],
                                     std::string(".txt"),
                                     allEnrollmentSamplesFilesNames[modalityI],
                                     allEnrollmentSamples[modalityI]);
            std::cout << "Done reading samples from modality " << modalityI + 1 << std::endl;
        }
        std::cout << "All samples read. Building chimerical identities... ";
        std::cout.flush();
        std::vector<std::vector<unsigned int>> permutedIndexesVectorVector(modalitiesCount, std::vector<unsigned int>());
        unsigned int mhstcfeCount = userIdsVectorVector[0].size();
        for (unsigned int modalityI = 0; modalityI < modalitiesCount; modalityI++) {
            std::cout << "Total number of identities in modality " << modalityI + 1 << ": "
                      << userIdsVectorVector[modalityI].size() << std::endl;
            unsigned int modalityIdsCount = userIdsVectorVector[modalityI].size();
            if (mhstcfeCount > modalityIdsCount) {
                mhstcfeCount = modalityIdsCount;
            }
            for (unsigned int index = 0; index < modalityIdsCount; index++) {
                permutedIndexesVectorVector[modalityI].push_back(index);
            }
            std::shuffle(permutedIndexesVectorVector[modalityI].begin(), permutedIndexesVectorVector[modalityI].end(), randomGenerator);
        }
        std::cout << "Total number of chimerical identities: " << mhstcfeCount << std::endl;
        std::cout << "Done (building chimerical identities)." << std::endl;
        std::cout << "Building Multimodal hybrid strengthened turbo codes fuzzy extractor objects... " << std::endl;
        std::cout.flush();
        std::vector<MultimodalHybridStrengthenedTurboCodeFuzzyExtractor> mhstcfeVector(mhstcfeCount);
        std::vector< std::future<void> > futuresVector(mhstcfeCount);
        std::vector<std::vector<real_number_vector_vector>> enrollmentSamples(mhstcfeCount);
        // Declared after every object the generate tasks use, so that it is destroyed (and waits for them) first.
        PendingTasksGuard<void> pendingTasksGuard(futuresVector);
        for (unsigned int mhstcfeI = 0; mhstcfeI < mhstcfeCount; mhstcfeI++) {
            std::string mhstcfeId;
            std::vector<HybridStrengthenedTurboCodeFuzzyExtractor> hstcfeVector;
            for (unsigned int modalityI = 0; modalityI < modalitiesCount; modalityI++) {
                unsigned int modalityResolvedIndex = permutedIndexesVectorVector[modalityI][mhstcfeI];
                std::cout << "Modality " << modalityI + 1 << " resolved index " << modalityResolvedIndex << std::endl;
                std::string modalityUserId = userIdsVectorVector[modalityI][modalityResolvedIndex];
                if (modalityI > 0) {
                    mhstcfeId += std::string("_");
                }
                mhstcfeId += modalityUserId;
                unsigned int enrollmentSamplesIndex = std::find(allEnrollmentSamplesFilesNames[modalityI].begin(),
                                                                allEnrollmentSamplesFilesNames[modalityI].end(),
                                                                modalityUserId) -
                                                      allEnrollmentSamplesFilesNames[modalityI].begin();
                if (enrollmentSamplesIndex >= allEnrollmentSamplesFilesNames[modalityI].size()) {
                    std::string exceptionMessage = std::string("User ID ") + modalityUserId +
                                                   std::string(" not found in enrollment samples folder ") +
                                                   enrollmentSamplesFilesFolders[modalityI];
                    throw std::logic_error(exceptionMessage);
                }
                std::cout << "Pushing samples from modality " << modalityI + 1
                          << ", id " << modalityUserId
                          << " (index " << enrollmentSamplesIndex << "/" << userIdsVectorVector[modalityI].size() << "),"
                          << " into chimerical ID " << mhstcfeI << std::endl;
                enrollmentSamples[mhstcfeI].push_back(allEnrollmentSamples[modalityI][enrollmentSamplesIndex]);
                hstcfeVector.push_back(hstcfeVectorVector[modalityI][modalityResolvedIndex]);
                std::cout << "Done pushing samples." << std::endl;
            }
            std::cout << "Pushing chimerical id " << mhstcfeId << std::endl;
            mhstcfeIds.push_back(mhstcfeId);
            std::cout << "Setting attributes of Multimodal Hybrid Strengthened Fuzzy Extractor object with index "
                      << mhstcfeI << std::endl;
            std::cout << "Setting fuzzy extractor parameters." << std::endl;
            mhstcfeVector[mhstcfeI].setCodeOffsetStdDevFactor(codeOffsetStdDevFactor);
            mhstcfeVector[mhstcfeI].setCodeOffsetStdDevOffset(codeOffsetStdDevOffset);
            mhstcfeVector[mhstcfeI].setFuzzyCommitmentMinReliability(fuzzyCommitmentMinReliability);
            std::cout << "Setting hybrid strengthened fuzzy extractors vector." << std::endl;
            mhstcfeVector[mhstcfeI].setHstcfes(hstcfeVector);
            std::cout << "Setting Id to " << mhstcfeId << std::endl;
            mhstcfeVector[mhstcfeI].setUserId(mhstcfeId);
            if (mhstcfeI == 0) {
                mhstcfeVector[mhstcfeI].saveGeneralData(multimodalHybridStrengthenedTurboCodeFuzzyExtractorFilesFolder +
                                                        std::string("/generalConfigFile.generalHSTCFE"));
            }
            auto func = std::bind(&MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::generate,
                                  &mhstcfeVector[mhstcfeI],
                                  std::ref(enrollmentSamples[mhstcfeI]));
            futuresVector[mhstcfeI] = threadPool.submit(pendingTasksGuard.cancellable(func));
        }
        std::cout << "Done (building Multimodal hybrid strengthened turbo codes fuzzy extractor objects)." << std::endl;
        for (unsigned int mhstcfeI = 0; mhstcfeI < mhstcfeCount; mhstcfeI++) {
            std::cout << "\rGenerate processes progress: " << 100.0*(mhstcfeI + 1.0)/mhstcfeCount << "% ";
            std::cout.flush();
            try {
                if (!futuresVector[mhstcfeI].valid()) {
                    throw std::future_error(std::future_errc::no_state);
                } else {
                    futuresVector[mhstcfeI].get();
                }
            } catch (const std::exception &e) {
                std::clog << "Exception caught while executing the \"generate\" procedure for subject = "
                          << mhstcfeIds[mhstcfeI] << std::endl;
                std::clog << "\t" << e.what() << std::endl;
                return EXIT_FAILURE;
            }
            mhstcfeVector[mhstcfeI].saveUserSpecificData(multimodalHybridStrengthenedTurboCodeFuzzyExtractorFilesFolder +
                                                         std::string("/") +
                                                         mhstcfeIds[mhstcfeI] +
                                                         std::string(".userSpecificHSTCFE"));
            mhstcfeVector[mhstcfeI].saveKey(multimodalHybridStrengthenedTurboCodeFuzzyExtractorFilesFolder + std::string("/") +
                                           mhstcfeIds[mhstcfeI] + std::string(".Key"));
        }
        std::cout << "Enrollment stage Done." << std::endl;
    }
    if (testMode) {
        std::cout << "Starting test stage..." << std::endl;
        std::vector<std::vector<HybridStrengthenedTurboCodeFuzzyExtractor>> hstcfeVectorVector;
        std::vector<std::vector<std::string>> userIdsVectorVector;
        std::vector<std::vector<std::string>> allProbeSamplesFilesNames(modalitiesCount);
        std::vector<std::vector<real_number_vector_vector>> allProbeSamples(modalitiesCount);
        // First, we need to load the Hybrid Fuzzy Extractors for each modality
        std::cout << "Loading Hybrid Strengthened Turbo Code Fuzzy Extractors... " << std::endl;
        loadHybridStrengthenedTurboCodeFuzzyExtractors(hybridStrengthenedTurboCodeFuzzyExtractorFilesFolders,
                                                       hybridStrengthenedTurboCodeFuzzyExtractorGeneralConfigFiles,
                                                       hstcfeVectorVector,
                                                       userIdsVectorVector);
        std::cout << "Done loading Hybrid Strengthened Turbo Code Fuzzy Extractors." << std::endl;
        // Second, we need to load the Multifactor Hybrid Fuzzy Extractors and allocate the correct unimodal Hybrid Fuzzy Extractors to them
        std::cout << "Loading General Data for Multimodal Hybrid Strengthened Turbo Code Fuzzy Extractors... " << std::endl;
        MultimodalHybridStrengthenedTurboCodeFuzzyExtractor generalMHSTCFE;
        generalMHSTCFE.loadGeneralData(multimodalHybridStrengthenedTurboCodeFuzzyExtractorFilesFolder +
                                std::string("/generalConfigFile.generalHSTCFE"));
        std::cout << "Done loading General Data for Multimodal Hybrid Strengthened Turbo Code Fuzzy Extractors." << std::endl;
        std::vector<MultimodalHybridStrengthenedTurboCodeFuzzyExtractor> mhstcfeVector;
        std::vector<std::string> userSpecificMHSTEFEFilesNames;
        std::cout << "Loading User-specific data for Multimodal Hybrid Strengthened Turbo Code Fuzzy Extractors... " << std::endl;
        loadMultimodalHybridStrengthenedTurboCodeFuzzyExtractors(multimodalHybridStrengthenedTurboCodeFuzzyExtractorFilesFolder,
                                                                 generalMHSTCFE,
                                                                 mhstcfeVector,
                                                                 std::string(".userSpecificHSTCFE"),
                                                                 userSpecificMHSTEFEFilesNames);
        std::cout << "Done loading User-specific data for Multimodal Hybrid Strengthened Turbo Code Fuzzy Extractors... " << std::endl;

        std::cout << "Matching Unimodal and Multimodal Hybrid Strengthened Turbo Code Fuzzy Extractors... " << std::endl;
        matchUnimodalAndMultimodalHSTCFEs(hstcfeVectorVector, mhstcfeVector);

        /*
        for (unsigned int mhstcfeI = 0; mhstcfeI < mhstcfeVector.size(); mhstcfeI++) {
            for (unsigned int modalityI = 0; modalityI < modalitiesCount; modalityI++) {
                std::cout << "Size of normalizer for chimerical id index " << mhstcfeI << ", modality " << modalityI + 1
                          << ": ";
                std::cout.flush();
                std::cout << mhstcfeVector[mhstcfeI].hstcfes()[modalityI].uniformNormalizer()->size() << std::endl;
                if (mhstcfeVector[mhstcfeI].hstcfes()[modalityI].uniformNormalizer()->size() > 0) {
                    std::cout << "\tSize of first dimension tree: ";
                    std::cout.flush();
                    std::cout << mhstcfeVector[mhstcfeI].hstcfes()[modalityI].uniformNormalizer()->begin()->size() << std::endl;
                    std::cout << "\tSize of last dimension tree: ";
                    std::cout.flush();
                    std::cout << mhstcfeVector[mhstcfeI].hstcfes()[modalityI].uniformNormalizer()->rbegin()->size() << std::endl;
                }
            }
        }
        */
        std::cout << "Done matching Unimodal and Multimodal Hybrid Strengthened Turbo Code Fuzzy Extractors." << std::endl;
        // Now we need to load all the samples and create the multimodal samples for all the quimeric users
        std::cout << "Reading probe samples... " << std::endl;
        for (unsigned int modalityI = 0; modalityI < modalitiesCount; modalityI++) {
            readSamplesFilesFromPath(probeSamplesFilesFolders[modalityI],
                                     std::string(".txt"),
                                     allProbeSamplesFilesNames[modalityI],
                                     allProbeSamples[modalityI]);
        }
        std::cout << "Done reading probe samples... " << std::endl;
        // Build multimodal user samples
        std::cout << "Building multimodal probe samples... " << std::endl;
        unsigned int subjectsCount = mhstcfeVector.size();
        std::vector<real_number_vector_vector> allUsersProbeSamples;
        std::vector<std::string> allUsersProbeSamplesUsersIds;
        for (unsigned int idI = 0; idI < subjectsCount; idI++) {
            std::string fullId = mhstcfeVector[idI].userId();
            std::string remainingId = fullId;
            std::vector<real_number_vector_vector> thisUserModalitySamples;
            for (unsigned int modalityI = 0; modalityI < modalitiesCount; modalityI++) {
                std::string modId;
                if (modalityI < modalitiesCount - 1) {
                    std::size_t pos = remainingId.find("_");
                    pos = remainingId.find("_", pos + 1);
                    if (pos != std::string::npos) {
                        modId = remainingId.substr(0, pos);
                        remainingId = remainingId.substr(pos + 1, std::string::npos);
                    }
                } else {
                    modId = remainingId;
                    remainingId = std::string();
                }
                std::vector<std::string>::iterator it = std::find(allProbeSamplesFilesNames[modalityI].begin(),
                                                                  allProbeSamplesFilesNames[modalityI].end(),
                                                                  modId);
                if (it == allProbeSamplesFilesNames[modalityI].end()) {
                    throw std::logic_error(std::string("Id ") + modId +
                                           std::string(" not found in modality ") + std::to_string(modalityI) +
                                           std::string("."));
                }
                unsigned int index = it - allProbeSamplesFilesNames[modalityI].begin();
                thisUserModalitySamples.push_back(allProbeSamples[modalityI][index]);
            }
            real_number_vector_vector userSamples;
            buildUserSamples(thisUserModalitySamples, userSamples);
            allUsersProbeSamples.push_back(userSamples);
            allUsersProbeSamplesUsersIds.push_back(fullId);
        }
        std::cout << "Done building multimodal probe samples. " << std::endl;
        // Perform multimodal reproduce
        std::vector < std::future< binary_polynomial_vector > > futuresVector(subjectsCount);
        PendingTasksGuard<binary_polynomial_vector> pendingTasksGuard(futuresVector);
        for (unsigned int allUsersProbeSamplesI = 0; allUsersProbeSamplesI < allUsersProbeSamples.size(); allUsersProbeSamplesI++) {
            for (unsigned int idI = 0; idI < subjectsCount; idI++) {
                if (mhstcfeVector[idI].validForReproduce()) {
                    std::cout << "\rReproducing probe samples from " << allUsersProbeSamplesUsersIds[allUsersProbeSamplesI]
                              << " (" << allUsersProbeSamplesI + 1 << "/" << allUsersProbeSamples.size() << ") ... ";
                    std::cout << "Starting reproduce procedures " << 100.0*(1.0 + idI)/subjectsCount << " % ";
                    std::cout.flush();
                    auto func = std::bind(&MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::reproduceBatch,
                                          &mhstcfeVector[idI],
                                          std::ref(allUsersProbeSamples[allUsersProbeSamplesI]));
                    futuresVector[idI] = threadPool.submit(pendingTasksGuard.cancellable(func));
                }
            }
            for (unsigned int idI = 0; idI < subjectsCount; idI++) {
                if (mhstcfeVector[idI].validForReproduce()) {
                    binary_polynomial_vector recoveredMessages;
                    std::cout << "\rReproducing probe samples from " << allUsersProbeSamplesUsersIds[allUsersProbeSamplesI]
                              << " (" << allUsersProbeSamplesI + 1 << "/" << allUsersProbeSamples.size() << ") ... ";
                    std::cout << "Finishing reproduce procedures " << 100.0*(1.0 + idI)/subjectsCount << " % ";
                    std::cout.flush();
                    try {
                        if (!futuresVector[idI].valid()) {
                            throw std::future_error(std::future_errc::no_state);
                        } else {
                            // Store recovered messages
                            recoveredMessages = futuresVector[idI].get();
                            std::string recoveredMessagesFileName;
                            std::ofstream recoveredMessagesFile;
                            if (!resultsOnly) {
                                recoveredMessagesFileName = recoveredMessagesFilesFolder +
                                        "/probe_" + allUsersProbeSamplesUsersIds[allUsersProbeSamplesI] +
                                        "_mhstcfe_" + mhstcfeVector[idI].userId() +
                                        ".recovered";
                                recoveredMessagesFile.open(recoveredMessagesFileName);
                                if (!recoveredMessagesFile) {
                                    throw std::iostream::failure(std::string("Error opening recovered messages file ") +
                                                                 recoveredMessagesFileName + std::string(" for writing."));
                                }
                            }
                            std::string authenticationResultsFileName(recoveredMessagesFilesFolder +
                                                                      "/authenticationResults.txt");
                            std::ofstream authenticationResultsFile;
                            if ((allUsersProbeSamplesI == 0) && (idI == 0)) {
                                authenticationResultsFile.open(authenticationResultsFileName, std::ios_base::out);
                            } else {
                                authenticationResultsFile.open(authenticationResultsFileName, std::ios_base::app);
                            }
                            if (!authenticationResultsFile) {
                                throw std::iostream::failure(std::string("Error opening authentication results file ") +
                                                             authenticationResultsFileName + std::string(" for writing."));
                            }
                            binary_polynomial genuineMessage = mhstcfeVector[idI].turboCodeInputMessage();
                            bool isGenuine(true);
                            isGenuine = (allUsersProbeSamplesUsersIds[allUsersProbeSamplesI] == mhstcfeVector[idI].userId());
                            for (unsigned int messageI = 0; messageI < recoveredMessages.size(); messageI++) {
                                bool authenticationResult(true);
                                for (unsigned int bitI = 0; bitI < recoveredMessages[messageI].size(); bitI++) {
                                    if (recoveredMessages[messageI][bitI] != genuineMessage[bitI]) {
                                        authenticationResult = false;
                                        if (resultsOnly) break;
                                    }
                                    if (!resultsOnly) {
                                        recoveredMessagesFile << recoveredMessages[messageI][bitI];
                                        if (bitI < recoveredMessages[messageI].size() - 1) {
                                            recoveredMessagesFile << " ";
                                        }
                                    }
                                }
                                if (!resultsOnly) {
                                    recoveredMessagesFile << std::endl;
                                }
                                authenticationResultsFile << allUsersProbeSamplesUsersIds[allUsersProbeSamplesI]  << " "
                                                          << mhstcfeVector[idI].userId()
                                                          << (authenticationResult?std::string(" 1"):std::string(" 0"))
                                                          << (isGenuine?std::string(" 1"):std::string(" 0")) << std::endl;
                            }
                            if (!resultsOnly) {
                                recoveredMessagesFile.close();
                            }
                            authenticationResultsFile.close();
                        }
                    } catch (const std::exception &e) {
                        std::clog << "Exception caught while executing the \"reproduce\" procedure for template "
                                  << mhstcfeVector[idI].userId() << " and probe subject "
                                  << allUsersProbeSamplesUsersIds[allUsersProbeSamplesI]
                                  << std::endl;
                        std::clog << "Exception message: " << e.what() << std::endl;
                        return EXIT_FAILURE;
                    }
                }
            }
        }
        std::cout << "Test stage done." << std::endl;
    }

    return EXIT_SUCCESS;
}
