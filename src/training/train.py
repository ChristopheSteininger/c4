import torch
from torch import nn
from torch.utils.data import DataLoader

import data

BATCH_SIZE = 64

LEARNING_RATE = 0.001

EPOCHS = 50

DEVICE = "cpu"

class Net(nn.Module):
    def __init__(self):
        super(Net, self).__init__()

        self.stack = nn.Sequential(
            nn.Linear(2 * 7 * 6, 16),
            nn.ReLU(),
            # nn.Linear(32, 32),
            # nn.ReLU(),
            nn.Linear(16, 3),
        )
    
    def forward(self, input):
        logits = self.stack(input)

        # softmax = nn.Softmax(dim=1)
        # return softmax(logits)
        return logits

training_dataset, testing_dataset = data.get_datasets()

training_dataloader = DataLoader(training_dataset, batch_size=BATCH_SIZE, shuffle=True)
testing_dataloader = DataLoader(testing_dataset, batch_size=BATCH_SIZE, shuffle=True)

print(f"Training samples = {len(training_dataloader.dataset)}")
print(f"Testing samples = {len(testing_dataloader.dataset)}")

model = Net().to(DEVICE)
loss_fn = nn.CrossEntropyLoss()
optimiser = torch.optim.SGD(model.parameters(), lr=LEARNING_RATE)

def training_loop():
    model.train()

    for batch, (x, y) in enumerate(training_dataloader):
        device_x = x.to(DEVICE)
        device_y = y.to(DEVICE)

        prediction = model(device_x)
        loss = loss_fn(prediction, device_y)

        optimiser.zero_grad()
        loss.backward()
        optimiser.step()

        # if batch % 100 == 0:
        #     print(f"loss: {loss.item()} batch #{batch}")

def evaluate():
    model.eval()

    total_loss = 0
    num_correct = 0

    with torch.no_grad():
        for x, y in testing_dataloader:
            device_x = x.to(DEVICE)
            device_y = y.to(DEVICE)

            prediction = model(device_x)
            
            total_loss += loss_fn(prediction, device_y).item()
            num_correct += (prediction.argmax(1) == device_y) \
                .type(torch.float) \
                .sum() \
                .item()
    
    avg_loss = total_loss / len(testing_dataloader)
    avg_correct = num_correct / len(testing_dataloader.dataset)

    print(f'Test Error:\n Accuracy: {(100 * avg_correct):>0.1f}%, Avg loss: {avg_loss:>8f}')

for i in range(EPOCHS):
    print(f"Epoch {i}")
    training_loop()
    evaluate()
