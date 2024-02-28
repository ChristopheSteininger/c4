import torch
from torch.utils.data import Dataset
import random
import time

import position
from settings import BOARD_WIDTH, BOARD_HEIGHT, BOARD_AREA, NUM_FEATURES


DATA_FILENAME = "src/training/data/samples.csv"

DATA_PLAYERS = ["o", "x"]

TRAIN_TEST_RATIO = 0.9

MAX_SAMPLES = 10_000_000


class C4Dataset(Dataset):
    def __init__(self, data):
        self._data = data

    def __len__(self):
        return len(self._data)

    def __getitem__(self, index):
        item = self._data[index]

        features = torch.tensor(item["features"], dtype=torch.float32)
        labels = torch.tensor([item["score"], item["best_move"]], dtype=torch.float32)

        return features, labels


def _line_to_best_move(cols):
    best_move = int(cols[-2])
    if best_move < 0 or best_move >= BOARD_WIDTH:
        raise RuntimeError(f"Invalid best move: {best_move}")

    return best_move


def _line_to_features(cols):
    features = [0] * (2 * BOARD_AREA)
    num_moves = [0, 0]

    for player, player_symbol in enumerate(DATA_PLAYERS):
        for col in range(BOARD_AREA):
            if cols[col] is player_symbol:
                features[col + player * BOARD_AREA] = 1
                num_moves[player] += 1

    if abs(num_moves[0] - num_moves[1]) > 1:
        raise RuntimeError(f"Sample has incorrect number of player 1 moves ({num_moves[0]}) " \
                            + "compared to player 2 moves ({num_moves[1]}).")

    return features


def _line_to_score(cols):
    score = int(cols[-1])
    if score < -17 or score > 17:
        raise RuntimeError(f"Invalid position score: {score}")

    return score


def _mirror(features):
    mirrored = [0] * (2 * BOARD_AREA)

    for player in range(2):
        for col in range(BOARD_WIDTH):
            mirror_col = BOARD_WIDTH - col - 1

            to_index = position.get_feature_index(0, col, player)
            from_index = position.get_feature_index(0, mirror_col, player)

            mirrored[to_index:to_index + BOARD_HEIGHT] = features[from_index:from_index + BOARD_HEIGHT]

    return mirrored


def _add_mirror_positions(data):
    with_mirror = []
    for d in data:
        with_mirror.append(d)
        with_mirror.append({
            "features": _mirror(d["features"]),
            "best_move": BOARD_WIDTH - d["best_move"] - 1,
            "score": d["score"],
            "line": d["line"],
        })

    print(f"Mirrored positions from {len(data):,} to {len(with_mirror):,} samples.")

    return with_mirror


def _load_data(filename):
    data = []
    print(f"Loading file {filename} . . .")

    with open(filename) as file:
        for line in file.readlines():
            cols = line.strip().split(",")

            if len(cols) != 2 + BOARD_AREA:
                raise RuntimeError(f"Invalid line: '{line}'")

            data.append({
                "features": _line_to_features(cols),
                "best_move": _line_to_best_move(cols),
                "score": _line_to_score(cols),
                "line": line,
            })

    print(f"    Loaded {len([d for d in data if d['score'] < 0]):,} losses.")
    print(f"    Loaded {len([d for d in data if d['score'] == 0]):,} draws.")
    print(f"    Loaded {len([d for d in data if d['score'] > 0]):,} wins.")

    if len(data) > MAX_SAMPLES:
        print(f"    Cutting number of samples from {len(data):,} to {MAX_SAMPLES:,}.")
        random.shuffle(data)
        return data[:MAX_SAMPLES]

    return data


def get_datasets():
    start_time = time.time()

    raw_data = _load_data(DATA_FILENAME)
    final_data = _add_mirror_positions(raw_data)

    # Shuffle to avoid patterns caused by grouping and flipping.
    random.shuffle(final_data)
    train_test_split = int(TRAIN_TEST_RATIO * len(final_data))

    training_dataset = C4Dataset(final_data[:train_test_split])
    testing_dataset = C4Dataset(final_data[train_test_split:])

    print(f"Loaded data in {time.time() - start_time:>0.2f} s")

    return training_dataset, testing_dataset
