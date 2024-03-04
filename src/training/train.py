import torch
from torch import nn
from torch.utils.data import DataLoader, random_split
import time

import data
from settings import BOARD_WIDTH, BOARD_AREA


TRAIN_RATIO = 0.9

BATCH_SIZE = 100

LEARNING_RATE = 0.001

EPOCHS = 20

DEVICE = "cpu"


class Net(nn.Module):
    def __init__(self):
        super(Net, self).__init__()

        num_neurons_1 = 64
        num_neurons_2 = 32
        num_neurons_3 = 16

        self._first_layer = nn.Linear(BOARD_AREA, num_neurons_1 // 2)
        self._last_layer = nn.ModuleList([nn.Linear(num_neurons_3, 1) for _ in range(BOARD_WIDTH)])

        self._stack = nn.Sequential(
            nn.ReLU(),
            nn.Linear(num_neurons_1, num_neurons_2),
            nn.ReLU(),
            nn.Linear(num_neurons_2, num_neurons_3),
            nn.ReLU(),
        )

        self.single_output = None

    def forward(self, input):
        own = self._first_layer(input[:, :BOARD_AREA])
        other = self._first_layer(input[:, BOARD_AREA:])
        both = torch.cat((own, other), dim=1)

        middle = self._stack(both)

        if self.single_output is None:
            return torch.cat([layer(middle) for layer in self._last_layer], dim=1)
        else:
            return self._last_layer[self.single_output](middle)


def _train(model, training_dataloader, loss_fn, optimiser):
    model.train()

    for X, y, _ in training_dataloader:
        device_X = X.to(DEVICE)
        device_y = y.to(DEVICE)

        total_loss = 0
        for col in range(BOARD_WIDTH):
            valid_indexes = device_y[:, col] != -1234

            valid_X = device_X[valid_indexes]
            valid_y = device_y[valid_indexes, col].reshape((len(valid_X), 1))

            model.single_output = col
            predictions = model(valid_X)

            total_loss += loss_fn(predictions, valid_y)

        total_loss.backward()
        optimiser.step()
        optimiser.zero_grad()


def _evaluate(model, testing_dataloader, loss_fn):
    model.eval()
    model.single_output = None

    total_loss = 0
    num_correct = 0

    num_correct_model_moves = 0
    num_correct_heuristic_moves = 0

    with torch.no_grad():
        for X, y, heuristic_moves in testing_dataloader:
            device_X = X.to(DEVICE)
            device_y = y.to(DEVICE)

            predictions = model(device_X)
            predictions[device_y == -1234] = -1234

            total_loss += loss_fn(predictions, device_y).item()

            max_scores = device_y.max(dim=1, keepdim=True)[0]
            prediction_scores = device_y.gather(dim=1, index=predictions.argmax(dim=1, keepdim=True))
            heuristic_scores = device_y.gather(dim=1, index=heuristic_moves)

            num_correct += _count_bools(torch.round(predictions) == device_y)
            num_correct_model_moves += _count_bools(prediction_scores == max_scores)
            num_correct_heuristic_moves += _count_bools(heuristic_scores == max_scores)

    avg_loss = total_loss / len(testing_dataloader)
    avg_correct = num_correct / (BOARD_WIDTH * len(testing_dataloader.dataset))

    avg_model_moves = num_correct_model_moves / len(testing_dataloader.dataset)
    avg_heuristic_moves = num_correct_heuristic_moves / len(testing_dataloader.dataset)

    print(f"    Avg correct: {100 * avg_correct:>0.1f}%, Avg loss: {avg_loss:>0.3f}")
    print(f"    Avg correct model moves: {100 * avg_model_moves:>0.1f}%, Avg correct heuristic moves: {100 * avg_heuristic_moves:>0.1f}%")


def _count_bools(bools):
    return bools \
        .type(torch.float) \
        .sum() \
        .item()


def main():
    dataset = data.C4Dataset()
    training_dataset, testing_dataset = random_split(dataset, [TRAIN_RATIO, 1 - TRAIN_RATIO])

    training_dataloader = DataLoader(training_dataset, batch_size=BATCH_SIZE, shuffle=True, num_workers=0)
    testing_dataloader = DataLoader(testing_dataset, batch_size=BATCH_SIZE, shuffle=True, num_workers=0)

    model = Net().to(DEVICE)
    loss_fn = nn.L1Loss()
    optimiser = torch.optim.Adam(model.parameters(), lr=LEARNING_RATE)

    print("Starting training . . .")
    print(f"    Training samples: {len(training_dataloader.dataset):,}")
    print(f"    Testing samples: {len(testing_dataloader.dataset):,}")

    print("\nInitial:")
    _evaluate(model, testing_dataloader, loss_fn)

    for epoch in range(EPOCHS):
        print(f"\nEpoch {epoch}:")

        train_start_time = time.time()
        _train(model, training_dataloader, loss_fn, optimiser)

        eval_start_time = time.time()
        _evaluate(model, testing_dataloader, loss_fn)

        print(f"    Training time: {eval_start_time - train_start_time:>0.2f} s, " \
              + f"Testing time: {time.time() - eval_start_time:>0.2f} s")


if __name__ == "__main__":
    main()
