import random
import math
import os

import settings
import position


OUTPUT_DIR = "src/training/data/validated/"

INPUT_FILENAME = "src/training/data/raw_samples.csv"

SAMPLES_PER_FILE = 1_000

MAX_OUTPUT_FILES = 10_000


def get_samples_file_name_by_index(file_index):
    return f"{OUTPUT_DIR}samples_{file_index}.csv"


def get_samples_file_name(sample_index):
    return get_samples_file_name_by_index(sample_index // SAMPLES_PER_FILE)


def get_samples_file_line(sample_index):
    return sample_index % SAMPLES_PER_FILE


def load_data(filename):
    data = []

    with open(filename) as file:
        for line in file:
            cols = line.rstrip().split(",")

            data.append({
                "position": cols[0],
                "scores": list(map(int, cols[1:settings.BOARD_WIDTH + 1])),
                "heuristic_move": int(cols[-1]),
            })

    print(f"Found a total of {len(data):,} samples.")
    return data


def remove_duplicates(data):
    deduped = {}
    num_duplicates = 0

    for d in data:
        if d["position"] in deduped:
            duplicate = deduped[d["position"]]

            for i in range(settings.BOARD_WIDTH):
                assert d["scores"][i] == duplicate["scores"][i]

            num_duplicates += 1

        else:
            deduped[d["position"]] = d

    print(f"Removed {num_duplicates:,} ({num_duplicates * 100.0 / len(data):>0.1f}%) duplicate samples.")
    return list(deduped.values())


def validate_samples(data):
    for d in data:
        features = list(map(int, d["position"]))
        for f in features:
            assert f == 0 or f == 1

        valid_moves = position.get_valid_moves(features)
        for col, score in enumerate(d["scores"]):
            if col in valid_moves:
                assert score != -1234
            else:
                assert score == -1234

        assert 0 <= d["heuristic_move"] < settings.BOARD_WIDTH
        assert d["scores"][d["heuristic_move"]] != -1234


def delete_old_samples():
    if not os.path.exists(OUTPUT_DIR):
        return

    num_deleted_files = 0
    for file_name in os.listdir(OUTPUT_DIR):
        os.remove(OUTPUT_DIR + file_name)
        num_deleted_files += 1

    os.rmdir(OUTPUT_DIR)
    print(f"Deleted {num_deleted_files:,} old sample files.")


def write_files(data):
    os.makedirs(OUTPUT_DIR)
    random.shuffle(data)

    num_files = math.ceil(len(data) / SAMPLES_PER_FILE)
    assert num_files < MAX_OUTPUT_FILES

    sample_index = 0
    for _ in range(num_files):
        with open(get_samples_file_name(sample_index), "w") as file:
            num_rows = min(SAMPLES_PER_FILE, len(data) - sample_index)
            for _ in range(num_rows):
                d = data[sample_index]
                sample_index += 1

                cols = [d["position"], *d["scores"], d["heuristic_move"]]
                line = ",".join(map(str, cols))

                file.write(line + "\n")

    print(f"Wrote {len(data):,} validated samples to {num_files:,} output files.")


def main():
    data = load_data(INPUT_FILENAME)

    dedup = remove_duplicates(data)
    validate_samples(dedup)

    delete_old_samples()
    write_files(dedup)


if __name__ == "__main__":
    main()
