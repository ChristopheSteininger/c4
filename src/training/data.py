import torch
from torch.utils.data import Dataset
import random

import position
from settings import BOARD_WIDTH, BOARD_HEIGHT, BOARD_AREA, NUM_FEATURES


DATA_FILENAME = "src/training/data/samples.csv"

DATA_PLAYERS = ["o", "x"]

DATAL_LABELS = {
    "loss": 0,
    "draw": 1,
    "win": 2,
}

TRAIN_TEST_RATIO = 0.9


class C4Dataset(Dataset):
    def __init__(self, data):
        self._data = data

    def __len__(self):
        return len(self._data)

    def __getitem__(self, index):
        item = self._data[index]

        features = torch.tensor(item["features"], dtype=torch.float32)
        labels = torch.tensor([item["label"], item["best_move"]])

        return features, labels


def _line_to_best_move(cols):
    best_move = int(cols[-2])
    if best_move < 0 or best_move >= BOARD_WIDTH:
        raise RuntimeError(f"Invalid best move: {best_move}")

    return best_move


def _even_labels(data):
    grouped_data = _group_by_label(data)
    num_samples = min([len(g) for g in grouped_data])

    selected_data = []
    for group in grouped_data:
        random.shuffle(group)

        selected_data.extend(group[:num_samples])

    print(f"Selected {len(grouped_data)} x {num_samples:,} ({len(selected_data):,}) even samples.")

    return selected_data


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


def _line_to_label(cols):
    score = int(cols[-1])
    if score < -17 or score > 17:
        raise RuntimeError(f"Invalid position score: {score}")

    if score < 0:
        return DATAL_LABELS["loss"]
    elif score == 0:
        return DATAL_LABELS["draw"]
    else:
        return DATAL_LABELS["win"]


def _group_by_label(data):
    grouped = [[] for _ in range(3)]
    for d in data:
        grouped[d["label"]].append(d)

    return grouped


def _mirror(features):
    mirrored = [0] * (2 * BOARD_AREA)

    for player in range(2):
        for col in range(BOARD_WIDTH):
            for row in range(BOARD_HEIGHT):
                mirror_col = BOARD_WIDTH - col - 1
                if position.get_feature(features, row, mirror_col, player):
                    position.set_feature(mirrored, row, col, player)

    return mirrored


def _add_mirror_positions(data):
    with_mirror = []
    for d in data:
        with_mirror.append(d)
        with_mirror.append({
            "features": _mirror(d["features"]),
            "best_move": BOARD_WIDTH - d["best_move"] - 1,
            "label": d["label"],
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
                "label": _line_to_label(cols),
                "line": line,
            })

    print(f"    Loaded {len([d for d in data if d['label'] == 0]):,} losses.")
    print(f"    Loaded {len([d for d in data if d['label'] == 1]):,} draws.")
    print(f"    Loaded {len([d for d in data if d['label'] == 2]):,} wins.")

    return data


def get_datasets():
    raw_data = _load_data(DATA_FILENAME)
    selected_data = _even_labels(raw_data)
    final_data = _add_mirror_positions(selected_data)

    # Shuffle to avoid patterns caused by grouping and flipping.
    random.shuffle(final_data)
    train_test_split = int(TRAIN_TEST_RATIO * len(final_data))

    training_dataset = C4Dataset(final_data[:train_test_split])
    testing_dataset = C4Dataset(final_data[train_test_split:])

    return training_dataset, testing_dataset
