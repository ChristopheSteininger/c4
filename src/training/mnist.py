import numpy
import torch
from torch.utils.data import Dataset, DataLoader
from torch import nn
from torchvision import datasets
from torchvision.transforms import ToTensor
import matplotlib.pyplot as plt


BATCH_SIZE = 64

LEARNING_RATE = 0.001

EPOCHS = 10

DEVICE = "cpu"


training_data = datasets.FashionMNIST(
    root="data",
    train=True,
    download=True,
    transform=ToTensor()
)

test_data = datasets.FashionMNIST(
    root="data",
    train=False,
    download=True,
    transform=ToTensor()
)

labels_map = {
    0: "T-Shirt",
    1: "Trouser",
    2: "Pullover",
    3: "Dress",
    4: "Coat",
    5: "Sandal",
    6: "Shirt",
    7: "Sneaker",
    8: "Bag",
    9: "Ankle Boot",
}

fig = plt.figure(figsize=(8, 8))
rows = 3
cols = 3
for i in range(1, cols * rows + 1):
    sample_idx = torch.randint(len(training_data), size=(1,)).item()
    img, label = training_data[sample_idx]
    fig.add_subplot(rows, cols, i)
    plt.title(labels_map[label])
    plt.axis("OFF")
    plt.imshow(img.squeeze(), cmap="gray")
# plt.show()

training_dataloader = DataLoader(training_data, batch_size = BATCH_SIZE)
test_dataloader = DataLoader(test_data, batch_size=BATCH_SIZE)

for x,y in training_dataloader:
    training_features, training_labels = next(iter(training_dataloader))

    print(f"Shape of x: {x.shape}")
    print(f"Shape of y: {y.shape}")

    break

class Net(nn.Module):
    def __init__(self):
        super(Net, self).__init__()

        self.flatten = nn.Flatten()
        self.stack = nn.Sequential(
            nn.Linear(28 * 28 * 1, 512),
            nn.ReLU(),
            nn.Linear(512, 512),
            nn.ReLU(),
            nn.Linear(512, 10),
        )
    
    def forward(self, input):
        flat = self.flatten(input)
        logits = self.stack(flat)

        # softmax = nn.Softmax(dim=1)
        # return softmax(logits)
        return logits

model = Net().to(DEVICE)
print(model.parameters)

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

        if batch % 100 == 0:
            print(f"loss: {loss.item()} batch #{batch}")

def evaluate():
    model.eval()

    total_loss = 0
    num_correct = 0

    with torch.no_grad():
        for x, y in test_dataloader:
            device_x = x.to(DEVICE)
            device_y = y.to(DEVICE)

            prediction = model(device_x)
            
            total_loss += loss_fn(prediction, device_y).item()
            num_correct += (prediction.argmax(1) == device_y) \
                .type(torch.float) \
                .sum() \
                .item()
    
    avg_loss = total_loss / len(test_dataloader)
    avg_correct = num_correct / len(test_dataloader.dataset)

    print(f'Test Error:\n Accuracy: {(100 * avg_correct):>0.1f}%, Avg loss: {avg_loss:>8f}')

for i in range(EPOCHS):
    training_loop()
    evaluate()
