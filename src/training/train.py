import torch
from torch import nn
from torch.utils.data import DataLoader, random_split
import time

import data
from settings import BOARD_WIDTH, BOARD_AREA


TRAIN_RATIO = 0.9

BATCH_SIZE = 10_000

LEARNING_RATE = 0.003

EPOCHS = 1000000

DEVICE = "cpu"


class Net(nn.Module):
    def __init__(self):
        super(Net, self).__init__()

        num_neurons_1 = 20
        num_neurons_2 = 20
        num_neurons_3 = 16

        self._first_layer = nn.Linear(BOARD_AREA, num_neurons_1 // 2)

        self._stack = nn.Sequential(
            nn.Linear(num_neurons_1, num_neurons_2),
            nn.ReLU(),
            nn.Linear(num_neurons_2, num_neurons_3),
            nn.ReLU(),
            nn.Linear(num_neurons_3, BOARD_WIDTH)
        )

    def forward(self, input):
        own = self._first_layer(input[:, :BOARD_AREA])
        other = self._first_layer(input[:, BOARD_AREA:])
        both = torch.cat((own, other), dim=1)

        return self._stack(both)


def _print_data_stats(dataloader):
    num_correct_heuristic_moves = 0
    num_best_moves = 0
    num_invalid_moves = 0

    for _, y, invalid_moves, is_heuristic_correct in dataloader:
        num_correct_heuristic_moves += is_heuristic_correct.sum().item()
        num_best_moves += y.sum().item()
        num_invalid_moves += invalid_moves.sum().item()

    num_samples = len(dataloader.dataset)
    num_moves = num_samples * BOARD_WIDTH

    print(f"    Number of samples:  {num_samples:,}")
    print(f"    Heuristic accuracy: {100 * num_correct_heuristic_moves / num_samples:>0.1f}%")
    print(f"    Correct moves:      {100 * num_best_moves / num_moves:>0.1f}%")
    print(f"    Invalid moves:      {100 * num_invalid_moves / num_moves:>0.1f}%")


def _train(model, training_dataloader, loss_fn, optimiser):
    model.train()

    for X, y, _, _ in training_dataloader:
        device_X = X.to(DEVICE)
        device_y = y.to(DEVICE)

        predictions = model(device_X)
        loss = loss_fn(predictions, device_y)

        loss.backward()
        optimiser.step()
        optimiser.zero_grad()


def _evaluate(model, testing_dataloader, loss_fn):
    model.eval()

    total_loss = 0
    num_correct_labels = 0
    num_correct_model_moves = 0

    with torch.no_grad():
        for X, y, invalid_moves, _ in testing_dataloader:
            device_X = X.to(DEVICE)
            device_y = y.to(DEVICE)

            predictions = model(device_X)
            predictions[invalid_moves] = 0
            prediction_moves = predictions.argmax(dim=1, keepdim=True)

            num_correct_labels += (predictions.round().clamp(0, 1) == device_y).sum().item()
            total_loss += loss_fn(predictions, device_y).item()
            num_correct_model_moves += device_y.gather(dim=1, index=prediction_moves).sum().item()

    num_batches = len(testing_dataloader)
    num_samples = len(testing_dataloader.dataset)
    num_moves = num_samples * BOARD_WIDTH

    print(f"    Avg loss: {total_loss / num_batches:>0.3f}")
    print(f"    Avg move accuracy: {100 * num_correct_model_moves / num_samples:>0.1f}%, " \
        + f"Avg label accuracy: {100 * num_correct_labels / num_moves:>0.1f}%")


def main():
    dataset = data.C4Dataset()
    training_dataset, testing_dataset = random_split(dataset, [TRAIN_RATIO, 1 - TRAIN_RATIO])

    training_dataloader = DataLoader(training_dataset, batch_size=BATCH_SIZE, shuffle=True, num_workers=8, persistent_workers=True)
    testing_dataloader = DataLoader(testing_dataset, batch_size=BATCH_SIZE, shuffle=True, num_workers=8, persistent_workers=True)

    model = Net().to(DEVICE)
    loss_fn = nn.BCEWithLogitsLoss()
    optimiser = torch.optim.Adam(model.parameters(), lr=LEARNING_RATE)

    print("Training dataset:")
    _print_data_stats(training_dataloader)

    print("\nTesting dataset:")
    _print_data_stats(testing_dataloader)

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
