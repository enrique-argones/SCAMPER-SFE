/// \file HybridTurboCodeStrengthenedFuzzyExtractor/main.cpp
/// \brief Command-line program to enroll subjects and evaluate the unimodal \ref HybridStrengthenedTurboCodeFuzzyExtractor.
///
/// \section htcsfe_usage Usage
/// \code{.unparsed}
/// HybridTurboCodeStrengthenedFuzzyExtractor -m MODE -G GENERAL_CONFIG -H HSTCFE_FOLDER
///                                           [-e ENROLLMENT_FOLDER] [-p PROBE_FOLDER -r RECOVERED_FOLDER] [-R]
/// \endcode
///
/// | Option | Description |
/// |--------|-------------|
/// | \c -h, \c --help | Shows the help message. |
/// | \c -m, \c --mode | Operation mode: \c enrollment, \c test, or \c both. Mandatory. |
/// | \c -G, \c --general | General HSTCFE configuration file (see \ref HybridStrengthenedTurboCodeFuzzyExtractor::loadGeneralData). Mandatory. |
/// | \c -H, \c --hybridFuzzyExtractorFilesFolder | Folder where the user-specific helper data and keys are written (enrollment) or read (test). Mandatory. |
/// | \c -e, \c --enrollmentSamplesFilesFolder | Folder with one \c .txt samples file per subject. Required in enrollment mode. |
/// | \c -p, \c --probeSamplesFilesFolder | Folder with one \c .txt probe samples file per subject. Required in test mode. |
/// | \c -r, \c --recovered | Output folder for recovered messages and \c authenticationResults.txt. Required in test mode. |
/// | \c -R, \c --resultsOnly | In test mode, only write \c authenticationResults.txt (no per-comparison \c .recovered files). |
///
/// \section htcsfe_enrollment Enrollment mode
/// For every samples file \c ID.txt in the enrollment folder, a copy of the general fuzzy extractor runs
/// \ref HybridStrengthenedTurboCodeFuzzyExtractor::generate (in parallel through the global \ref threadPool) and writes
/// \c ID.userSpecificHSTCFE (helper data) and \c ID.Key (secret) in the HSTCFE folder.
///
/// \section htcsfe_test Test mode
/// Every probe samples file is compared against every enrolled template whose helper data is valid. For each comparison,
/// \ref HybridStrengthenedTurboCodeFuzzyExtractor::reproduceBatch decodes one message per probe sample, and one line per probe sample is
/// appended to \c authenticationResults.txt:
/// \code{.unparsed}
/// PROBE_ID TEMPLATE_ID AUTHENTICATED GENUINE
/// \endcode
/// where the identifiers are the file stems after their first underscore, \c AUTHENTICATED is 1 if the decoded message equals
/// the enrolled key, and \c GENUINE is 1 if the probe and template files have the same stem. Unless \c -R is given, the decoded
/// messages are also written to \c probe_PROBEID_hstcfe_TEMPLATEID.recovered.
///
/// \section htcsfe_example Example
/// \code{.unparsed}
/// HybridTurboCodeStrengthenedFuzzyExtractor -m both -G generalConfigFile.txt -H HSTCFEs/ -e enrollment/ -p probe/ -r recovered/ -R
/// \endcode
/// \author Enrique Argones Rúa
/// \date 2025

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <cstdlib>
#include <argagg/argagg.hpp>
#include <armadillo>
#include <HSTCFE/hybridstrengthenedturbocodefuzzyextractor.h>
#include <TurboCodes/turbocode.h>
#include <TurboCodes/psk.h>
#include <TurboCodes/vectorfiles.h>
#include <ThreadPool/ThreadPool.hpp>
#include <ThreadPool/PendingTasksGuard.hpp>
#include <atomic>

/// Parameters of the enrollment mode.
typedef struct {
    /// Folder with one \c .txt enrollment samples file per subject.
    std::string enrollmentSamplesFilesFolderName;
    /// Bit-loading factor (not used: the value is taken from the general configuration file).
    real_number stdDevFactor;
    /// Bit-loading offset (not used: the value is taken from the general configuration file).
    real_number stdDevOffset;
    /// Minimum fuzzy-commitment reliability (not used: the value is taken from the general configuration file).
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

/// Program entry point. See the file documentation for the command-line options and the output files.
/// @param argc Number of command-line arguments.
/// @param argv Command-line arguments.
/// @return \c EXIT_SUCCESS on success, \c EXIT_FAILURE on error.
int main(int argc, char *argv[])
{
    EnrollmentModeParameters enrollmentParameters;
    TestModeParameters testParameters;
    GeneralModeParameters generalParameters;
    HybridStrengthenedTurboCodeFuzzyExtractor hstcfe;
    bool enrollmentMode(false), testMode(false);
    bool resultsOnly(false);
    real_number_vector_vector uniformDistributedTrainSetSamples;

    argagg::parser argparser {{
        { "help", {"-h", "--help"},
         "shows this help message.", 0},
        {"mode", {"-m","--mode"},
         "Operation mode {\"enrollment\", \"test\", or \"both\"}.", 1},
        {"resultsOnly", {"-R","--resultsOnly"},
         "Results only. If given, only the authentication results are stored in the recovered messages folder.", 0},
        {"enrollmentSamplesFilesFolder", {"-e", "--enrollmentSamplesFilesFolder"},
         "Enrollment samples files folder", 1},
        {"probeSamplesFilesFolder", {"-p", "--probeSamplesFilesFolder"},
         "Probe samples files folder", 1},
        {"hybridFuzzyExtractorFilesFolder", {"-H", "--hybridFuzzyExtractorFilesFolder"},
         "Hybrid Fuzzy Extractor files folder", 1},
        {"HybridStrengthenedTurboCodeFuzzyExtractorGeneralConfigFile", {"-G", "--general"},
         "General Hybrid Strengthened Turbo Code Fuzzy Extractor configuration file", 1},
        {"recoveredMessagesFilesFolderName", {"-r", "--recovered"},
         "Folder where the recovered messages are stored.", 1}
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
    if (!args["HybridStrengthenedTurboCodeFuzzyExtractorGeneralConfigFile"]) {
        argagg::fmt_ostream fmt(std::cerr);
        fmt << "HybridStrengthenedTurboCodeFuzzyExtractorGeneralConfigFile is a mandatory argument." << std::endl;
        fmt << "Usage: " << argv[0] << " [OPTION]\n" << argparser;
        return EXIT_FAILURE;
    } else {
        generalParameters.generalHybridFuzzyExtractorConfigFileName =
            args["HybridStrengthenedTurboCodeFuzzyExtractorGeneralConfigFile"].as<std::string>("");
    }
    if (args["hybridFuzzyExtractorFilesFolder"]) {
        generalParameters.hybridFuzzyExtractorFilesFolderName = args["hybridFuzzyExtractorFilesFolder"].as<std::string>("");
        std::cout << "Hybrid Fuzzy Extractor files folder: "
                  << generalParameters.hybridFuzzyExtractorFilesFolderName << std::endl;
    } else {
        argagg::fmt_ostream fmt(std::cerr);
        fmt << "hybridFuzzyExtractorFilesFolder option not found." << std::endl;
        fmt << "Usage: " << argv[0] << " [OPTION]\n" << argparser;
        return EXIT_FAILURE;
    }
    if (enrollmentMode) {
        if (args["enrollmentSamplesFilesFolder"]) {
            enrollmentParameters.enrollmentSamplesFilesFolderName = args["enrollmentSamplesFilesFolder"].as<std::string>("");
            std::cout << "Enrollment samples folder: "
                      << enrollmentParameters.enrollmentSamplesFilesFolderName << std::endl;
        } else {
            argagg::fmt_ostream fmt(std::cerr);
            fmt << "enrollmentSamplesFile option has to be used in enrollment mode." << std::endl;
            fmt << "Usage: " << argv[0] << " [OPTION]\n" << argparser;
            return EXIT_FAILURE;
        }
    }
    if (testMode) {
        if (args["probeSamplesFilesFolder"]) {
            testParameters.probeSamplesFilesFolderName = args["probeSamplesFilesFolder"].as<std::string>("");
            std::cout << "Probe samples files folder: "
                      << testParameters.probeSamplesFilesFolderName << std::endl;
        } else {
            std::cout << "Probe samples file folder not provided in \"test\" mode." << std::endl;
            argagg::fmt_ostream fmt(std::cerr);
            fmt << "Usage: " << argv[0] << " [OPTION]\n" << argparser;
            return EXIT_SUCCESS;
        }
        if (args["recoveredMessagesFilesFolderName"]) {
            testParameters.recoveredMessagesFilesFolderName = args["recoveredMessagesFilesFolderName"].as<std::string>("");
            std::cout << "Recovered messages files folder: "
                      << testParameters.recoveredMessagesFilesFolderName << std::endl;
        } else {
            std::cout << "Recovered messages files folder not provided in \"test\" mode." << std::endl;
            argagg::fmt_ostream fmt(std::cerr);
            fmt << "Usage: " << argv[0] << " [OPTION]\n" << argparser;
            return EXIT_SUCCESS;
        }
    }
    std::cout << "Loading general Hybrid Fuzzy Extractor Config File: "
              << generalParameters.generalHybridFuzzyExtractorConfigFileName << std::endl;
    hstcfe.loadGeneralData(generalParameters.generalHybridFuzzyExtractorConfigFileName);
    std::cout << "Done." << std::endl;
    if (enrollmentMode) {
        std::vector<real_number_vector_vector> enrollmentSamples;
        std::vector<std::string> enrollmentSamplesFilesNames;
        std::cout << "Reading sample files from enrollment samples folder... ";
        std::cout.flush();
        readSamplesFilesFromPath(enrollmentParameters.enrollmentSamplesFilesFolderName,
                                 std::string(".txt"),
                                 enrollmentSamplesFilesNames,
                                 enrollmentSamples);
        std::cout << "Done!" << std::endl;
        unsigned int subjectsCount = enrollmentSamples.size();
        std::cout << "Creating " << subjectsCount << " HybridStrengthenedTurboCodeFuzzyExtractor objects... ";
        std::cout.flush();
        std::vector<HybridStrengthenedTurboCodeFuzzyExtractor> hstcfeVector(subjectsCount, hstcfe);
        std::cout << "Done!";
        std::cout << std::endl;
        std::vector< std::future<void> > futuresVector(subjectsCount);
        PendingTasksGuard<void> pendingTasksGuard(futuresVector);
        std::cout.flush();
        for (unsigned int subjectI = 0; subjectI < subjectsCount; subjectI++) {
            std::cout << "\rSending generate processes to Thread Pool... " << 100.0*(subjectI + 1.0)/subjectsCount << "% ";
            std::cout.flush();
            hstcfeVector[subjectI].setUserId(enrollmentSamplesFilesNames[subjectI]);
            auto func = std::bind(&HybridStrengthenedTurboCodeFuzzyExtractor::generate,
                                  &hstcfeVector[subjectI],
                                  std::ref(enrollmentSamples[subjectI]));
            futuresVector[subjectI] = threadPool.submit(pendingTasksGuard.cancellable(func));
        }
        std::cout << "Done." << std::endl;
        std::vector<std::string> fullUserSpecificHSTCFEFilesNames(subjectsCount);
        for (unsigned int subjectI = 0; subjectI < subjectsCount; subjectI++) {
            std::cout << "\rGenerate processes progress: " << 100.0*(subjectI + 1.0)/subjectsCount << "% ";
            std::cout.flush();
            try {
                if (!futuresVector[subjectI].valid()) {
                    throw std::future_error(std::future_errc::no_state);
                } else {
                    futuresVector[subjectI].get();
                }
            } catch (const std::exception &e) {
                std::clog << "Exception caught while executing the \"generate\" procedure for subject = " << enrollmentSamplesFilesNames[subjectI] << std::endl;
                std::clog << "\t" << e.what() << std::endl;
                return EXIT_FAILURE;
            }
            hstcfeVector[subjectI].saveUserSpecificData(generalParameters.hybridFuzzyExtractorFilesFolderName + std::string("/") +
                                                        enrollmentSamplesFilesNames[subjectI] + std::string(".userSpecificHSTCFE"));
            hstcfeVector[subjectI].saveKey(generalParameters.hybridFuzzyExtractorFilesFolderName + std::string("/") +
                                           enrollmentSamplesFilesNames[subjectI] + std::string(".Key"));
        }
        std::cout << " Done." << std::endl;
    }
    if (testMode) {
        std::vector<real_number_vector_vector> probeSamples;
        std::vector<std::string> probeSamplesFilesNames;
        std::cout << "Reading sample files from probe samples folder... ";
        std::cout.flush();
        readSamplesFilesFromPath(testParameters.probeSamplesFilesFolderName,
                                 std::string(".txt"),
                                 probeSamplesFilesNames,
                                 probeSamples);
        std::cout << "Done! Number of probe files: " << probeSamplesFilesNames.size() << "." << std::endl;
        std::vector<std::string> userIds;
        std::vector<HybridStrengthenedTurboCodeFuzzyExtractor> hstcfeVector;
        std::cout << "Reading user-specific HSTCFE files from " << generalParameters.hybridFuzzyExtractorFilesFolderName << "... ";
        std::cout.flush();
        try {
            readHybridStrengthenedTurboCodeFuzzyExtractorsFromPath(
                generalParameters.hybridFuzzyExtractorFilesFolderName,
                ".userSpecificHSTCFE",
                hstcfe,
                userIds,
                hstcfeVector);
        } catch (std::exception &e) {
            std::cerr << "Exception caught when reading user-specific HSTCFE files: " << e.what() << std::endl;
            return EXIT_FAILURE;
        }
        std::cout << "Done! Number of HSTCFE files read: " << userIds.size() << "." << std::endl;
        for (uint probeI = 0; probeI < probeSamples.size(); probeI++) {
            std::vector < std::future< binary_polynomial_vector > > futuresVector(hstcfeVector.size());
            PendingTasksGuard<binary_polynomial_vector> pendingTasksGuard(futuresVector);
            for (uint ustcfeI = 0; ustcfeI < userIds.size(); ustcfeI++) {
                std::cout << "\rReproducing probe samples from " << probeSamplesFilesNames[probeI]
                          << " (" << probeI + 1 << "/" << probeSamples.size() << ") ... ";
                std::cout << "Starting reproduce procedures " << 100.0*(1.0 + ustcfeI)/userIds.size() << " % ";
                std::cout.flush();
                if (hstcfeVector[ustcfeI].validForReproduce()) {
                    auto func = std::bind(&HybridStrengthenedTurboCodeFuzzyExtractor::reproduceBatch,
                                          &hstcfeVector[ustcfeI],
                                          std::ref(probeSamples[probeI]));
                    futuresVector[ustcfeI] = threadPool.submit(pendingTasksGuard.cancellable(func));
                }
            }
            for (uint ustcfeI = 0; ustcfeI < userIds.size(); ustcfeI++) {
                if (hstcfeVector[ustcfeI].validForReproduce()) {
                    binary_polynomial_vector recoveredMessages;
                    std::cout << "\rReproducing probe samples from " << probeSamplesFilesNames[probeI]
                              << " (" << probeI + 1 << "/" << probeSamples.size() << ") ... ";
                    std::cout << "Finishing reproduce procedures " << 100.0*(1.0 + ustcfeI)/userIds.size() << " % ";
                    std::cout.flush();
                    try {
                        if (!futuresVector[ustcfeI].valid()) {
                            throw std::future_error(std::future_errc::no_state);
                        } else {
                            // Store recovered messages
                            recoveredMessages = futuresVector[ustcfeI].get();
                            std::string recoveredMessagesFileName;
                            std::ofstream recoveredMessagesFile;
                            if (!resultsOnly) {
                                recoveredMessagesFileName = testParameters.recoveredMessagesFilesFolderName +
                                                          "/probe_" + probeSamplesFilesNames[probeI] +
                                                          "_hstcfe_" + userIds[ustcfeI] +
                                                          ".recovered";
                                recoveredMessagesFile.open(recoveredMessagesFileName);
                                if (!recoveredMessagesFile) {
                                    throw std::iostream::failure(std::string("Error opening recovered messages file ") +
                                                                 recoveredMessagesFileName + std::string(" for writing."));
                                }
                            }
                            std::string authenticationResultsFileName(testParameters.recoveredMessagesFilesFolderName +
                                                                  "/authenticationResults.txt");
                            std::ofstream authenticationResultsFile;
                            if ((probeI == 0) && (ustcfeI == 0)) {
                                authenticationResultsFile.open(authenticationResultsFileName, std::ios_base::out);
                            } else {
                                authenticationResultsFile.open(authenticationResultsFileName, std::ios_base::app);
                            }
                            if (!authenticationResultsFile) {
                                throw std::iostream::failure(std::string("Error opening authentication results file ") +
                                                             authenticationResultsFileName + std::string(" for writing."));
                            }
                            binary_polynomial genuineMessage = hstcfeVector[ustcfeI].turboCodeInputMessage();
                            bool isGenuine(true);
                            isGenuine = (probeSamplesFilesNames[probeI] == userIds[ustcfeI]);
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
                                authenticationResultsFile << probeSamplesFilesNames[probeI].substr(probeSamplesFilesNames[probeI].find_first_of('_') + 1) << " "
                                                          << userIds[ustcfeI].substr(userIds[ustcfeI].find_first_of('_') + 1)
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
                                  << userIds[ustcfeI] << " and probe subject "
                                  << probeSamplesFilesNames[probeI]
                                  << std::endl;
                        return EXIT_FAILURE;
                    }
                }
            }
        }
        std::cout << "Done." << std::endl;
    }
    hstcfe.releaseNormalizers();
    return EXIT_SUCCESS;
}
