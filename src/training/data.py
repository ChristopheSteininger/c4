import torch
from torch.utils.data import Dataset
import linecache

from settings import BOARD_WIDTH, BOARD_HEIGHT, BOARD_AREA, NUM_FEATURES


MAX_SAMPLES = 100_000


class C4Dataset(Dataset):
    def __init__(self, filename):
        self._filename = filename
        self._num_samples = 2 * self._num_file_lines(filename)

    def __len__(self):
        return min(MAX_SAMPLES, self._num_samples)

    def __getitem__(self, index):
        line = linecache.getline(self._filename, (index // 2) + 1)
        data = line.split(",")

        features = torch.tensor(list(map(int, data[0])), dtype=torch.float32)
        scores = torch.tensor(list(map(int, data[1:BOARD_WIDTH + 1])), dtype=torch.float32)
        scores[scores == -1234] = 0
        heuristic_move = int(data[-1])

        if index % 2 == 1:
            features = self._mirror(features)
            scores = scores.flip(dims=(0,))
            heuristic_move = BOARD_WIDTH - heuristic_move - 1

        return features, scores, heuristic_move

    def _num_file_lines(self, filename):
        line_count = 0
        with open(filename) as file:
            for _ in file.readlines():
                line_count += 1

        return line_count

    def _mirror(self, features):
        return features \
            .reshape((2, BOARD_WIDTH, BOARD_HEIGHT)) \
            .flip(dims=(1,)) \
            .reshape((2 * BOARD_AREA))
