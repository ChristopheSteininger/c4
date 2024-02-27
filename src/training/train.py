import torch
from torch import nn
from torch.utils.data import DataLoader
import time

import data
import position
from settings import NUM_FEATURES


BATCH_SIZE = 64

LEARNING_RATE = 0.005

EPOCHS = 50

DEVICE = "cpu"

SOFTMAX = nn.Softmax(dim=1)


class Net(nn.Module):
    def __init__(self):
        super(Net, self).__init__()

        self.stack = nn.Sequential(
            nn.Linear(NUM_FEATURES, 16),
            nn.ReLU(),
            nn.Linear(16, 3),
        )

    def forward(self, input):
        return self.stack(input)


def _train(model, training_dataloader, optimiser, loss_fn):
    model.train()

    for x, y in training_dataloader:
        device_x = x.to(DEVICE)
        device_y = y.to(DEVICE)

        predictions = model(device_x)
        loss = loss_fn(predictions, device_y[:, 0])

        optimiser.zero_grad()
        loss.backward()
        optimiser.step()


def _simulate(model, features, best_move):
    valid_moves = position.get_valid_moves(features)
    move_features = features.repeat(len(valid_moves), 1)

    if best_move not in valid_moves:
        data.print_feature(features)
        raise RuntimeError(f"The best move in the position ({best_move}) is not valid.")

    for i, col in enumerate(valid_moves):
        position.move(move_features[i], col)

    move_features = move_features.to(DEVICE)
    move_scores = model(move_features)
    best_move_guess_index = SOFTMAX(move_scores)[:, 2].argmax()

    return 1 if valid_moves.index(best_move) == best_move_guess_index else 0


def _evaluate(epoch, model, testing_dataloader, loss_fn):
    model.eval()

    total_loss = 0
    num_correct = 0
    num_correct_moves = 0
    check_correct_moves = epoch % 10 == 0 or epoch == EPOCHS - 1

    with torch.no_grad():
        for x, y in testing_dataloader:
            device_x = x.to(DEVICE)
            device_y = y.to(DEVICE)

            predictions = model(device_x)

            total_loss += loss_fn(predictions, device_y[:, 0]).item()
            num_correct += (predictions.argmax(dim=1) == device_y[:, 0]) \
                .type(torch.float) \
                .sum() \
                .item()

            if check_correct_moves:
                for i in range(device_x.size(dim=0)):
                    num_correct_moves += _simulate(model, device_x[i], device_y[i, 1])

    avg_loss = total_loss / len(testing_dataloader)
    avg_correct = num_correct / len(testing_dataloader.dataset)
    avg_correct_mvoes = num_correct_moves / len(testing_dataloader.dataset)

    print(f"    Accuracy: {(100 * avg_correct):>0.1f}%, Avg loss: {avg_loss:>8f}")
    if check_correct_moves:
        print(f"    Avg correct moves: {100 * avg_correct_mvoes:>0.1f}%")


def main():
    training_dataset, testing_dataset = data.get_datasets()

    training_dataloader = DataLoader(training_dataset, batch_size=BATCH_SIZE, shuffle=True)
    testing_dataloader = DataLoader(testing_dataset, batch_size=BATCH_SIZE, shuffle=True)

    model = Net().to(DEVICE)
    loss_fn = nn.CrossEntropyLoss()
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
