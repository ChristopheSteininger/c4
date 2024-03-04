import random
import math

import settings
import position


INPUT_FILENAME = "src/training/data/samples.csv"

SAMPLES_PER_FILE = 1_000

MAX_OUTPUT_FILES = 10_000


def get_samples_file_name_by_index(file_index):
    return f"src/training/data/validated/samples_{file_index}.csv"


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


def validate_invalid_moves(data):
    for d in data:
        features = list(map(int, d["position"]))

        valid_moves = position.get_valid_moves(features)
        for col, score in enumerate(d["scores"]):
            if col in valid_moves:
                assert score != -1234
            else:
                assert score == -1234


def write_files(data):
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
    validate_invalid_moves(dedup)

    random.shuffle(dedup)
    write_files(dedup)


if __name__ == "__main__":
    main()
