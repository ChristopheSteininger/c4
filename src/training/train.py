import torch
from torch import nn
from torch.utils.data import DataLoader, random_split
import time

import data
import position
from settings import NUM_FEATURES


TRAIN_RATIO = 0.9

BATCH_SIZE = 64

LEARNING_RATE = 0.005

EPOCHS = 50

DEVICE = "cpu"

SOFTMAX = nn.Softmax(dim=1)

MAX_SIMULATE_SAMPLES = 10_000


class Net(nn.Module):
    def __init__(self):
        super(Net, self).__init__()

        num_a = 42 * 3
        num_b = 16

        self.first = nn.Linear(NUM_FEATURES // 2, num_a // 2)
        self.stack = nn.Sequential(
            nn.ReLU(),
            nn.Linear(num_a, num_b),
            nn.ReLU(),
            nn.Linear(num_b, 1),
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
        device_y = y.to(DEVICE).reshape((len(y), 1))

        predictions = model(device_X)
        loss = loss_fn(predictions, device_y)

        optimiser.zero_grad()
        loss.backward()
        optimiser.step()


def _simulate(model, features, best_move):
    valid_moves = position.get_valid_moves(features)
    move_features = features.repeat(len(valid_moves), 1)

    if best_move not in valid_moves:
        position.print_feature(features)
        raise RuntimeError(f"The best move in the position ({best_move}) is not valid.")

    for i, col in enumerate(valid_moves):
        position.move(move_features[i], col)

    move_features = move_features.to(DEVICE)
    move_scores = model(move_features)
    best_move_guess_index = move_scores.argmax()

    return 1 if valid_moves.index(best_move) == best_move_guess_index else 0


def _evaluate(epoch, model, testing_dataloader, loss_fn):
    model.eval()

    total_loss = 0
    num_correct = 0

    num_correct_moves = 0
    num_moves_checked = 0
    check_correct_moves = epoch % 10 == 0 or epoch == EPOCHS - 1

    with torch.no_grad():
        for X, y, best_moves in testing_dataloader:
            device_X = X.to(DEVICE)
            device_y = y.to(DEVICE).reshape((len(y), 1))

            predictions = model(device_X)

            total_loss += loss_fn(predictions, device_y).item()
            num_correct += (torch.round(predictions) == device_y) \
                .type(torch.float) \
                .sum() \
                .item()

            if check_correct_moves:
                for i in range(min(device_X.size(dim=0), MAX_SIMULATE_SAMPLES - num_moves_checked)):
                    num_correct_moves += _simulate(model, device_X[i], best_moves[i])
                    num_moves_checked += 1

    avg_loss = total_loss / len(testing_dataloader)
    avg_correct = num_correct / len(testing_dataloader.dataset)

    print(f"    Avg correct: {100 * avg_correct:>0.1f}, Avg loss: {avg_loss:>8f}")
    if check_correct_moves:
        avg_correct_moves = num_correct_moves / num_moves_checked
        print(f"    Avg correct moves: {100 * avg_correct_moves:>0.1f}%")


def main():
    dataset = data.get_dataset()
    training_dataset, testing_dataset = random_split(dataset, [TRAIN_RATIO, 1 - TRAIN_RATIO])

    training_dataloader = DataLoader(training_dataset, batch_size=BATCH_SIZE, shuffle=True)
    testing_dataloader = DataLoader(testing_dataset, batch_size=BATCH_SIZE, shuffle=True)

    model = Net().to(DEVICE)
    loss_fn = nn.MSELoss()
    optimiser = torch.optim.SGD(model.parameters(), lr=LEARNING_RATE)

    print("\nStarting training . . .")
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
