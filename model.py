import torch
import torch.nn as nn
import torch.nn.functional as F
import numpy as np


class FourPCnn(nn.Module):
    def __init__(self, history_len):
        super().__init__()
        self.device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
        self.history_len = history_len
        self.conv1 = nn.Conv2d(24 * history_len + 15, 256, 5, stride=1, padding=2)  # out size: 256 x 14 x 14
        self.conv2 = nn.Conv2d(256, 256, 3, stride=1, padding=1)  # out size: 256 x 14 x 14
        self.conv3 = nn.Conv2d(256, 256, 3, stride=1, padding=1)  # out size: 256 x 14 x 14
        self.conv4 = nn.Conv2d(256, 256, 3, stride=1, padding=1)  # out size: 256 x 14 x 14
        self.conv5 = nn.Conv2d(256, 256, 3, stride=1)  # out size: 256 x 12 x 12
        self.conv6 = nn.Conv2d(256, 256, 3, stride=1)  # out size: 256 x 10 x 10
        self.conv7 = nn.Conv2d(256, 256, 3, stride=1)  # out size: 256 x 8 x 8
        self.bn1 = nn.BatchNorm2d(256)
        self.bn2 = nn.BatchNorm2d(256)
        self.bn3 = nn.BatchNorm2d(256)
        self.bn4 = nn.BatchNorm2d(256)
        self.bn5 = nn.BatchNorm2d(256)
        self.bn6 = nn.BatchNorm2d(256)
        self.bn7 = nn.BatchNorm2d(256)

        self.conv1_v = nn.Conv2d(256, 256, 3, stride=1, padding=1)  # out size: 256 x 8 x 8
        self.conv2_v = nn.Conv2d(256, 256, 3, stride=1, padding=1)  # out size: 256 x 8 x 8
        self.bn1_v = nn.BatchNorm2d(256)
        self.bn2_v = nn.BatchNorm2d(256)
        self.fc_v = nn.Linear(8 * 8 * 256, 1)

        self.conv1_prob = nn.Conv2d(256, 256, 3, stride=1, padding=1)  # out size: 256 x 8 x 8
        self.conv2_prob = nn.Conv2d(256, 256, 3, stride=1, padding=1)  # out size: 256 x 8 x 8
        self.bn1_prob = nn.BatchNorm2d(256)
        self.bn2_prob = nn.BatchNorm2d(256)
        self.fc_prob = nn.Linear(8 * 8 * 256, 21366)

    def forward(self, x):
        if type(x) != torch.Tensor:
            x = torch.tensor(x, dtype=torch.float32)
        x = x.to(self.device)
        x = x.view(-1, 24 * self.history_len + 15, 14, 14)
        x = F.relu(self.bn1(self.conv1(x)))
        x = F.relu(self.bn2(self.conv2(x)))
        x = F.relu(self.bn3(self.conv3(x)))
        x = F.relu(self.bn4(self.conv4(x)))
        x = F.relu(self.bn5(self.conv5(x)))
        x = F.relu(self.bn6(self.conv6(x)))
        x = F.relu(self.bn7(self.conv7(x)))

        x_v = F.relu(self.bn1_v(self.conv1_v(x)))
        x_v = F.relu(self.bn2_v(self.conv2_v(x_v)))
        x_v = x_v.view(-1, 8 * 8 * 256)
        v = self.fc_v(x_v)

        x_prob = F.relu(self.bn1_prob(self.conv1_prob(x)))
        x_prob = F.relu(self.bn2_prob(self.conv2_prob(x_prob)))
        x_prob = x_prob.view(-1, 8 * 8 * 256)
        prob = self.fc_prob(x_prob)
        return F.log_softmax(prob, dim=1), torch.tanh(v)

    def predict(self, x):
        self.eval()
        with torch.no_grad():
            probs, vs = self(x)
        if probs.is_cuda and vs.is_cuda:
            probs = probs.cpu()
            vs = vs.cpu()
        return np.exp(probs.numpy()), vs.numpy()

