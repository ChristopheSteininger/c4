import torch
from torch import nn
from torch.utils.data import DataLoader, random_split
import time

import data
import position
from settings import BOARD_WIDTH, NUM_FEATURES


DATA_FILENAME = "src/training/data/samples_shuf.csv"

TRAIN_RATIO = 0.9

BATCH_SIZE = 64

LEARNING_RATE = 0.005

EPOCHS = 50

DEVICE = "cpu"

MAX_SIMULATE_SAMPLES = 10_000


class Net(nn.Module):
    def __init__(self):
        super(Net, self).__init__()

        num_a = 16
        num_b = 16

        self.first = nn.Linear(NUM_FEATURES // 2, num_a // 2)
        self.stack = nn.Sequential(
            nn.ReLU(),
            nn.Linear(num_a, num_b),
            nn.ReLU(),
            nn.Linear(num_b, BOARD_WIDTH),
        )

    def forward(self, input):
        own = self.first(input[:, :NUM_FEATURES // 2])
        other = self.first(input[:, NUM_FEATURES // 2:])
        both = torch.cat((own, other), dim=1)

        return self.stack(both)


def _train(model, training_dataloader, optimiser, loss_fn):
    model.train()

    for X, y, _ in training_dataloader:
        device_X = X.to(DEVICE)
        device_y = y.to(DEVICE)

        predictions = model(device_X)
        loss = loss_fn(predictions, device_y)

        optimiser.zero_grad()
        loss.backward()
        optimiser.step()


def _simulate(prediction, features, scores, heuristic_move):
    valid_moves = position.get_valid_moves(features)

    max_score = max(scores[move] for move in valid_moves).item()
    prediction_move = prediction.argmax()

    model_correct = 1 if scores[prediction_move] == max_score else 0
    heuristic_correct = 1 if scores[heuristic_move] == max_score else 0

    return model_correct, heuristic_correct


def _evaluate(epoch, model, testing_dataloader, loss_fn):
    model.eval()

    total_loss = 0
    num_correct = 0

    num_correct_model_moves = 0
    num_correct_heuristic_moves = 0
    num_moves_checked = 0
    check_correct_moves = epoch % 1 == 0 or epoch == EPOCHS - 1

    with torch.no_grad():
        for X, y, heuristic_moves in testing_dataloader:
            device_X = X.to(DEVICE)
            device_y = y.to(DEVICE)

            predictions = model(device_X)

            total_loss += loss_fn(predictions, device_y).item()
            num_correct += (torch.round(predictions) == device_y) \
                .type(torch.float) \
                .sum() \
                .item()

            if check_correct_moves:
                for i in range(min(device_X.size(dim=0), MAX_SIMULATE_SAMPLES - num_moves_checked)):
                    model_correct, heuristic_correct = _simulate(predictions[i], device_X[i], device_y[i], heuristic_moves[i])

                    num_correct_model_moves += model_correct
                    num_correct_heuristic_moves += heuristic_correct
                    num_moves_checked += 1

    avg_loss = total_loss / len(testing_dataloader)
    avg_correct = num_correct / (BOARD_WIDTH * len(testing_dataloader.dataset))

    print(f"    Avg correct: {100 * avg_correct:>0.1f}%, Avg loss: {avg_loss:>8f}")
    if check_correct_moves:
        avg_correct_model_moves = num_correct_model_moves / num_moves_checked
        avg_correct_heuristic_moves = num_correct_heuristic_moves / num_moves_checked
        print(f"    Avg correct model moves: {100 * avg_correct_model_moves:>0.1f}%, " \
              + f"avg correct heuristic moves: {100 * avg_correct_heuristic_moves:>0.1f}%")


def main():
    dataset = data.C4Dataset(DATA_FILENAME)
    training_dataset, testing_dataset = random_split(dataset, [TRAIN_RATIO, 1 - TRAIN_RATIO])

    training_dataloader = DataLoader(training_dataset, batch_size=BATCH_SIZE, shuffle=True, num_workers=0)
    testing_dataloader = DataLoader(testing_dataset, batch_size=BATCH_SIZE, shuffle=True, num_workers=0)

    model = Net().to(DEVICE)
    loss_fn = nn.MSELoss()
    optimiser = torch.optim.SGD(model.parameters(), lr=LEARNING_RATE)

    print("Starting training . . .")
    print(f"    Training samples = {len(training_dataloader.dataset):,}")
    print(f"    Testing samples = {len(testing_dataloader.dataset):,}")

    for epoch in range(EPOCHS):
        print(f"\nEpoch {epoch}:")
        start_time = time.time()

        _train(model, training_dataloader, optimiser, loss_fn)
        _evaluate(epoch, model, testing_dataloader, loss_fn)

        print(f"    Time: {time.time() - start_time:>0.2f} s")


if __name__ == "__main__":
    main()
