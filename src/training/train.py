import torch
from torch import nn
from torch.utils.data import DataLoader, random_split
import time

import data
import position
from settings import BOARD_WIDTH, BOARD_AREA


DATA_FILENAME = "src/training/data/samples_validated.csv"

TRAIN_RATIO = 0.9

BATCH_SIZE = 64

LEARNING_RATE = 0.001

EPOCHS = 50

DEVICE = "cpu"


class Net(nn.Module):
    def __init__(self):
        super(Net, self).__init__()

        num_neurons_1 = 32
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

        losses = []
        for col in range(BOARD_WIDTH):
            y_col = device_y[:, col]
            valid_indexes = y_col != -1234

            valid_X = device_X[valid_indexes]
            valid_y = device_y[valid_indexes, col].reshape((len(valid_X), 1))

            model.single_output = col
            predictions = model(valid_X)

            loss = loss_fn(predictions, valid_y)
            losses.append(loss)

        optimiser.zero_grad()
        for loss in losses:
            loss.backward()
        optimiser.step()


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
            num_correct += (torch.round(predictions) == device_y).type(torch.float).sum().item()

            max_scores = device_y.max(dim=1, keepdim=True)[0]
            prediction_scores = device_y.gather(dim=1, index=predictions.argmax(dim=1, keepdim=True))
            heuristic_scores = device_y.gather(dim=1, index=heuristic_moves.reshape((len(heuristic_moves), 1)))

            num_correct_model_moves += (prediction_scores == max_scores).type(torch.float).sum().item()
            num_correct_heuristic_moves += (heuristic_scores == max_scores).type(torch.float).sum().item()

    avg_loss = total_loss / len(testing_dataloader)
    avg_correct = num_correct / (BOARD_WIDTH * len(testing_dataloader.dataset))

    print(f"    Avg correct: {100 * avg_correct:>0.1f}%, Avg loss: {avg_loss:>0.3f}")
    print(f"    Avg correct model moves: {100 * num_correct_model_moves / len(testing_dataloader.dataset):>0.1f}%, " \
        + f"avg correct heuristic moves: {100 * num_correct_heuristic_moves / len(testing_dataloader.dataset):>0.1f}%")


def main():
    dataset = data.C4Dataset(DATA_FILENAME)
    training_dataset, testing_dataset = random_split(dataset, [TRAIN_RATIO, 1 - TRAIN_RATIO])

    training_dataloader = DataLoader(training_dataset, batch_size=BATCH_SIZE, shuffle=True, num_workers=0)
    testing_dataloader = DataLoader(testing_dataset, batch_size=BATCH_SIZE, shuffle=True, num_workers=0)

    model = Net().to(DEVICE)
    loss_fn = nn.MSELoss()
    # optimiser = torch.optim.SGD(model.parameters(), lr=LEARNING_RATE)
    optimiser = torch.optim.Adam(model.parameters())

    print("Starting training . . .")
    print(f"    Training samples: {len(training_dataloader.dataset):,}")
    print(f"    Testing samples: {len(testing_dataloader.dataset):,}")

    for epoch in range(EPOCHS):
        print(f"\nEpoch {epoch}:")
        start_time = time.time()

        _train(model, training_dataloader, loss_fn, optimiser)
        _evaluate(model, testing_dataloader, loss_fn)

        print(f"    Time: {time.time() - start_time:>0.2f} s")


if __name__ == "__main__":
    main()
