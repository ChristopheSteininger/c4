import torch
from torch.utils.data import Dataset
import linecache

import settings
import validate
import os


MAX_SAMPLES = 1_000_000


class C4Dataset(Dataset):
    def __init__(self):
        self._num_samples = 2 * self._count_num_samples()

    def __len__(self):
        return min(MAX_SAMPLES, self._num_samples)

    def __getitem__(self, index):
        sample_index = index // 2
        file_name = validate.get_samples_file_name(sample_index)
        file_line = validate.get_samples_file_line(sample_index)

        line = linecache.getline(file_name, file_line + 1)
        data = line.split(",")

        feature_ints = list(map(int, data[0]))
        score_ints = list(map(int, data[1:settings.BOARD_WIDTH + 1]))
        heuristic_int = int(data[-1])

        features = torch.tensor(feature_ints, dtype=torch.float32)
        scores = torch.tensor(score_ints, dtype=torch.float32)

        max_score = scores.max()
        labels = (scores == max_score).type(torch.float32)
        invalid_moves = scores == -1234
        is_heuristic_correct = (scores[heuristic_int] == max_score).type(torch.int64).reshape((1,))

        if index % 2 == 1:
            features = self._mirror(features)
            labels = labels.flip(dims=(0,))
            invalid_moves = invalid_moves.flip(dims=(0,))

        return features, labels, invalid_moves, is_heuristic_correct

    def _count_num_samples(self):
        file_index = 0
        num_samples = 0

        while True:
            filename = validate.get_samples_file_name_by_index(file_index)
            if not os.path.exists(filename):
                break

            num_samples += self._num_file_lines(filename)
            file_index += 1

        return num_samples

    def _num_file_lines(self, filename):
        line_count = 0
        with open(filename) as file:
            for _ in file.readlines():
                line_count += 1

        return line_count

    def _mirror(self, features):
        return features \
            .reshape((2, settings.BOARD_WIDTH, settings.BOARD_HEIGHT)) \
            .flip(dims=(1,)) \
            .reshape((2 * settings.BOARD_AREA))
