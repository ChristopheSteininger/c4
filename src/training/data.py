import torch
from torch.utils.data import Dataset
import random


DATA_FILENAME = "tst/data/all_8_ply_positions.txt"
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

"""
b,b,b,b,b,b,
b,b,b,b,b,b,
x,o,b,b,b,b,
x,o,x,o,x,o,
b,b,b,b,b,b,
b,b,b,b,b,b,
b,b,b,b,b,b,
win


0,0,0,0,0,0,
0,0,0,0,0,0,
1,0,0,0,0,0,
1,0,1,0,1,0,
0,0,0,0,0,0,
0,0,0,0,0,0,
0,0,0,0,0,0,

0,0,0,0,0,0,
0,0,0,0,0,0,
0,1,0,0,0,0,
0,1,0,1,0,1,
0,0,0,0,0,0,
0,0,0,0,0,0,
0,0,0,0,0,0,
"""


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


def _even_labels(data):
    grouped_data = _group_by_label(data)
    num_samples = min([len(g) for g in grouped_data])

    selected_data = []
    for group in grouped_data:
        random.shuffle(group)

        selected_data.extend(group[:num_samples])

    random.shuffle(selected_data)
    print(f"Selected {len(grouped_data)} x {num_samples:,} ({len(selected_data):,}) even samples.")

    return selected_data


def _line_to_features(cols):
    features = [0] * (2 * BOARD_AREA)

    for player, player_symbol in enumerate(DATA_PLAYERS):
        for col in range(BOARD_AREA):
            if cols[col] is player_symbol:
                features[col + player * BOARD_AREA] = 1
    
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


def _load_data(filename):
    data = []

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

    print(f"Loaded {len([d for d in data if d['label'] == 0]):,} losses.")
    print(f"Loaded {len([d for d in data if d['label'] == 1]):,} draws.")
    print(f"Loaded {len([d for d in data if d['label'] == 2]):,} wins.")

    return data

def get_datasets():
    # Data is column first, then row, then player. Flattened. Binary.
    data = _load_data(DATA_FILENAME)
    selected_data = _even_labels(data)

    random.shuffle(selected_data)
    train_test_split = int(TRAIN_TEST_RATIO * len(selected_data))

    training_dataset = C4Dataset(selected_data[:train_test_split])
    testing_dataset = C4Dataset(selected_data[train_test_split:])

    return training_dataset, testing_dataset
