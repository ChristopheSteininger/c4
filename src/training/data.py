import torch
from torch.utils.data import Dataset
import random


DATA_FILENAMES = [
    # "tst/data/all_8_ply_positions.txt",
    "src/training/data/samples.csv",
]
DATA_PLAYERS = ["x", "o"]
DATA_LABELS = {
    "loss": 0,
    "draw": 1,
    "win": 2,
}

BOARD_WIDTH = 7
BOARD_HEIGHT = 6
BOARD_AREA = BOARD_WIDTH * BOARD_HEIGHT

TRAIN_TEST_RATIO = 0.9


class C4Dataset(Dataset):
    def __init__(self, data):
        self.data = data

    def __len__(self):
        return len(self.data)

    def __getitem__(self, index):
        item = self.data[index]
        # print(item["features"])
        # print(torch.tensor(item["features"]))
        return torch.tensor(item["features"], dtype=torch.float32), torch.tensor(item["label"])


def _print_feature(features):
    for row in range(BOARD_HEIGHT - 1, -1, -1):
        line = ""
        for col in range(BOARD_WIDTH):
            line += " "
            if features[row + col * BOARD_HEIGHT] == 1:
                line += DATA_PLAYERS[0]
            elif features[row + col * BOARD_HEIGHT + BOARD_AREA] == 1:
                line += DATA_PLAYERS[1]
            else:
                line += "."
        
        print(line)
    
    footer = ""
    for i in range(BOARD_WIDTH):
        footer += f" {i}"
    print(footer)


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
    
    if num_moves[1] > num_moves[0]:
        raise RuntimeError(f"Sample has more player 2 moves ({num_moves[1]}) than player 1 moves ({num_moves[0]}).")
    
    if num_moves[0] - num_moves[1] > 1:
        raise RuntimeError(f"Sample has too many player 1 moves ({num_moves[0]}) compared to player 2 moves ({num_moves[1]}).")
    
    return features


def _line_to_label(cols):
    label = cols[-1]
    if label not in DATA_LABELS:
        raise RuntimeError(f"Unknown label: {label}")

    return DATA_LABELS[label]


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
                mirrored[row + col * BOARD_HEIGHT + player * BOARD_AREA] = \
                    features[row + mirror_col * BOARD_HEIGHT + player * BOARD_AREA]

    return mirrored


def _add_mirror_positions(data):
    with_mirror = []

    for d in data:
        with_mirror.append(d)
        with_mirror.append({
            "features": _mirror(d["features"]),
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

            if len(cols) != BOARD_AREA + 1:
                raise RuntimeError(f"Invalid line: '{line}'")

            data.append({
                "features": _line_to_features(cols),
                "label": _line_to_label(cols),
                "line": line,
            })

    print(f"    Loaded {len([d for d in data if d['label'] == 0]):,} losses.")
    print(f"    Loaded {len([d for d in data if d['label'] == 1]):,} draws.")
    print(f"    Loaded {len([d for d in data if d['label'] == 2]):,} wins.")

    return data

def get_datasets():
    # Data is column first, then row, then player. Flattened. Binary.
    data = []
    for filename in DATA_FILENAMES:
        data.extend(_load_data(filename))
    
    # _print_feature(data[123]["features"])
    # _print_feature(_mirror(data[123]["features"]))

    selected_data = _even_labels(data)
    final_data = _add_mirror_positions(selected_data)
    # final_data = selected_data

    # Shuffle to avoid patterns caused by grouping and flipping.
    random.shuffle(final_data)
    train_test_split = int(TRAIN_TEST_RATIO * len(final_data))

    training_dataset = C4Dataset(final_data[:train_test_split])
    testing_dataset = C4Dataset(final_data[train_test_split:])

    # training_dataset = C4Dataset(_even_labels(_load_data(DATA_FILENAMES[1])))
    # testing_dataset = C4Dataset(_even_labels(_load_data(DATA_FILENAMES[0])))

    return training_dataset, testing_dataset
