import random

from settings import BOARD_WIDTH
import position


INPUT_FILENAME = "src/training/data/samples.csv"
OUTPUT_FILENAME = "src/training/data/samples_validated.csv"


def load_data(filename):
    data = []

    with open(filename) as file:
        for line in file:
            cols = line.rstrip().split(",")

            data.append({
                "position": cols[0],
                "scores": list(map(int, cols[1:BOARD_WIDTH + 1])),
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

            for i in range(BOARD_WIDTH):
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


def write_file(data):
    with open(OUTPUT_FILENAME, "w") as file:
        for d in data:
            cols = [d["position"], *d["scores"], d["heuristic_move"]]
            line = ",".join(map(str, cols))

            file.write(line + "\n")


def main():
    data = load_data(INPUT_FILENAME)
    
    dedup = remove_duplicates(data)
    validate_invalid_moves(dedup)

    random.shuffle(dedup)
    write_file(dedup)


if __name__ == "__main__":
    main()
