#!/usr/bin/env python3
"""End-to-end test of the SCAMPER SFE programs on synthetic data.

Generates two synthetic biometric modalities, enrolls and tests every subject with the unimodal program
(HybridTurboCodeStrengthenedFuzzyExtractor), then enrolls and tests chimeric subjects with the multimodal program
(MultimodalHybridStrengthenedTurboCodeFuzzyExtractor), and checks the false rejection and false acceptance rates.

Usage: end_to_end.py WORK_DIR
Environment: SFE_UNIMODAL, SFE_MULTIMODAL (program paths) and SFE_TURBOCODES_CONFIG (folder with the Turbo Code
configuration files). ctest sets them (see the top-level CMakeLists.txt).
"""

import os
import random
import shutil
import subprocess
import sys

SUBJECTS = 12            # subjects per modality
TRAIN_SUBJECTS = 40      # subjects of the training set (empirical CDF of each feature)
ENROLLMENT_SAMPLES = 8
PROBE_SAMPLES = 3
MODALITIES = {"faceA": 24, "veinB": 16}   # modality name -> number of features
MAX_FRR = 0.30           # loose bounds: the data are synthetic and the keys are random
MAX_FAR = 0.02


def write_samples(path, samples):
    with open(path, "w") as f:
        f.write(f"{len(samples)} {len(samples[0])}\n")
        for s in samples:
            f.write(" ".join(f"{x:.6f}" for x in s) + "\n")


def subject_samples(rng, mean, noise, count):
    return [[m + rng.gauss(0.0, n) for m, n in zip(mean, noise)] for _ in range(count)]


def make_modality(rng, root, name, dim):
    folders = {k: os.path.join(root, name, k) for k in ("train", "enrollment", "probe", "HSTCFEs", "recovered")}
    for d in folders.values():
        os.makedirs(d, exist_ok=True)
    noise = [rng.uniform(0.05, 0.45) for _ in range(dim)]     # reliable and unreliable features
    for i in range(TRAIN_SUBJECTS):
        mean = [rng.gauss(0.0, 1.0) for _ in range(dim)]
        write_samples(os.path.join(folders["train"], f"train_{i:03d}.txt"), subject_samples(rng, mean, noise, 4))
    for i in range(SUBJECTS):
        mean = [rng.gauss(0.0, 1.0) for _ in range(dim)]
        stem = f"{name}_{i:03d}"   # exactly one underscore (required by the multimodal program)
        write_samples(os.path.join(folders["enrollment"], stem + ".txt"),
                      subject_samples(rng, mean, noise, ENROLLMENT_SAMPLES))
        write_samples(os.path.join(folders["probe"], stem + ".txt"), subject_samples(rng, mean, noise, PROBE_SAMPLES))
    return folders


def rates(results_file):
    genuine = impostor = false_rejections = false_acceptances = 0
    with open(results_file) as f:
        for line in f:
            fields = line.split()
            if len(fields) != 4:
                continue
            authenticated, is_genuine = fields[2] == "1", fields[3] == "1"
            if is_genuine:
                genuine += 1
                false_rejections += not authenticated
            else:
                impostor += 1
                false_acceptances += authenticated
    assert genuine > 0 and impostor > 0, f"no comparisons in {results_file}"
    return false_rejections / genuine, false_acceptances / impostor, genuine, impostor


def run(command):
    print("+", " ".join(command), flush=True)
    subprocess.run(command, check=True, stdout=subprocess.DEVNULL)


def main():
    work = os.path.abspath(sys.argv[1])
    unimodal, multimodal = os.environ["SFE_UNIMODAL"], os.environ["SFE_MULTIMODAL"]
    turbo_code = os.path.join(os.environ["SFE_TURBOCODES_CONFIG"], "best-found-turbo-code_rate_1_3.txt")
    shutil.rmtree(work, ignore_errors=True)
    rng = random.Random(20261105)
    failures = []

    general_files, modality_folders = [], []
    for name, dim in MODALITIES.items():
        folders = make_modality(rng, work, name, dim)
        general = os.path.join(work, name, "generalConfigFile.txt")
        with open(general, "w") as f:
            f.write(f"TurboCodeConfigFile {turbo_code}\n"
                    "TurboCodeMaxDecodingIterations 16\n"
                    f"TrainSetSamplesFilesFolder {folders['train']}\n"
                    "CodeOffsetStdDevFactor 1.1\n"
                    "CodeOffsetStdDevOffset 0.2\n"
                    "FuzzyCommitmentMinReliability 2.0\n")
        run([unimodal, "-m", "both", "-G", general, "-H", folders["HSTCFEs"], "-e", folders["enrollment"],
             "-p", folders["probe"], "-r", folders["recovered"]])
        frr, far, g, i = rates(os.path.join(folders["recovered"], "authenticationResults.txt"))
        print(f"unimodal {name}: FRR {frr:.3f} ({g} genuine), FAR {far:.4f} ({i} impostor)")
        if frr > MAX_FRR or far > MAX_FAR:
            failures.append(f"unimodal {name}")
        general_files.append(general)
        modality_folders.append(folders)

    multimodal_folder = os.path.join(work, "multimodal", "MHSTCFEs")
    multimodal_recovered = os.path.join(work, "multimodal", "recovered")
    os.makedirs(multimodal_folder)
    os.makedirs(multimodal_recovered)
    config = os.path.join(work, "multimodal", "config.txt")
    with open(config, "w") as f:
        f.write("codeOffsetStdDevFactor 1.1\ncodeOffsetStdDevOffset 0.2\nfuzzyCommitmentMinReliability 2.5\n")
        f.write(f"modalitiesCount {len(MODALITIES)}\n")
        for label, key in (("enrollmentSamplesFilesFolders", "enrollment"), ("probeSamplesFilesFolders", "probe"),
                           ("hybridStrengthenedTurboCodeFuzzyExtractorFilesFolders", "HSTCFEs")):
            f.write(label + "\n" + "".join(f"  {m[key]}/\n" for m in modality_folders))
        f.write("hybridStrengthenedTurboCodeFuzzyExtractorGeneralConfigFiles\n" +
                "".join(f"  {g}\n" for g in general_files))
        f.write(f"multimodalHybridStrengthenedTurboCodeFuzzyExtractorFilesFolder {multimodal_folder}\n")
        f.write(f"recoveredMessagesFilesFolder {multimodal_recovered}\n")
    run([multimodal, "-c", config, "-m", "both", "-R"])
    frr, far, g, i = rates(os.path.join(multimodal_recovered, "authenticationResults.txt"))
    print(f"multimodal: FRR {frr:.3f} ({g} genuine), FAR {far:.4f} ({i} impostor)")
    if frr > MAX_FRR or far > MAX_FAR:
        failures.append("multimodal")

    if failures:
        print("FAILED:", ", ".join(failures))
        return 1
    print("end-to-end test passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
