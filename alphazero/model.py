"""
Neural Network Architecture for Stones and Rivers AlphaZero
Supports transfer learning across different board sizes
"""

import torch
import torch.nn as nn
import torch.nn.functional as F


class ResBlock(nn.Module):
    """Residual block for the neural network."""

    def __init__(self, num_hidden):
        super().__init__()
        self.conv1 = nn.Conv2d(num_hidden, num_hidden, kernel_size=3, padding=1)
        self.bn1 = nn.BatchNorm2d(num_hidden)
        self.conv2 = nn.Conv2d(num_hidden, num_hidden, kernel_size=3, padding=1)
        self.bn2 = nn.BatchNorm2d(num_hidden)

    def forward(self, x):
        residual = x
        x = F.relu(self.bn1(self.conv1(x)))
        x = self.bn2(self.conv2(x))
        x += residual
        x = F.relu(x)
        return x


class StonesRiversNet(nn.Module):
    """
    ResNet-based policy-value network for Stones and Rivers.
    Supports multiple board sizes with transfer learning capability.
    """

    def __init__(self, game, num_resBlocks=9, num_hidden=128, device='cpu'):
        super().__init__()

        self.device = device
        self.board_rows = game.row_count
        self.board_cols = game.column_count
        self.action_size = game.action_size

        # Start block: 7 input channels -> num_hidden
        self.startBlock = nn.Sequential(
            nn.Conv2d(7, num_hidden, kernel_size=3, padding=1),
            nn.BatchNorm2d(num_hidden),
            nn.ReLU()
        )

        # Backbone: residual blocks
        self.backBone = nn.ModuleList(
            [ResBlock(num_hidden) for _ in range(num_resBlocks)]
        )

        # Policy head
        self.policyHead = nn.Sequential(
            nn.Conv2d(num_hidden, 32, kernel_size=3, padding=1),
            nn.BatchNorm2d(32),
            nn.ReLU(),
            nn.Flatten(),
            nn.Linear(32 * self.board_rows * self.board_cols, self.action_size)
        )

        # Value head
        self.valueHead = nn.Sequential(
            nn.Conv2d(num_hidden, 3, kernel_size=3, padding=1),
            nn.BatchNorm2d(3),
            nn.ReLU(),
            nn.Flatten(),
            nn.Linear(3 * self.board_rows * self.board_cols, 256),
            nn.ReLU(),
            nn.Linear(256, 1),
            nn.Tanh()
        )

        self.to(device)

    def forward(self, x):
        x = self.startBlock(x)
        for resBlock in self.backBone:
            x = resBlock(x)

        policy = self.policyHead(x)
        value = self.valueHead(x)

        return policy, value


def transfer_weights(source_model, target_model, freeze_backbone=False):
    """
    Transfer weights from a smaller board model to a larger board model.

    Args:
        source_model: Trained model (e.g., small board)
        target_model: New model (e.g., medium/large board)
        freeze_backbone: If True, freeze the transferred convolutional layers

    Returns:
        target_model with transferred weights
    """
    # Transfer start block
    target_model.startBlock.load_state_dict(source_model.startBlock.state_dict())

    # Transfer backbone (ResBlocks)
    for i, (src_block, tgt_block) in enumerate(zip(source_model.backBone, target_model.backBone)):
        tgt_block.load_state_dict(src_block.state_dict())

    # Transfer convolutional parts of policy and value heads
    # Note: Linear layers will differ due to different board sizes

    # Policy head conv layers
    target_model.policyHead[0].load_state_dict(source_model.policyHead[0].state_dict())
    target_model.policyHead[1].load_state_dict(source_model.policyHead[1].state_dict())

    # Value head conv layers
    target_model.valueHead[0].load_state_dict(source_model.valueHead[0].state_dict())
    target_model.valueHead[1].load_state_dict(source_model.valueHead[1].state_dict())

    # Optionally freeze the backbone
    if freeze_backbone:
        for param in target_model.startBlock.parameters():
            param.requires_grad = False
        for block in target_model.backBone:
            for param in block.parameters():
                param.requires_grad = False

    print(f"Transferred weights from {source_model.board_rows}x{source_model.board_cols} "
          f"to {target_model.board_rows}x{target_model.board_cols}")
    if freeze_backbone:
        print("Backbone layers frozen for fine-tuning")

    return target_model


def create_model_for_board_size(board_size, num_resBlocks=9, num_hidden=128,
                                 device='cpu', transfer_from=None,
                                 freeze_backbone=False):
    """
    Factory function to create model with optional transfer learning.

    Args:
        board_size: 'small', 'medium', or 'large'
        num_resBlocks: Number of residual blocks
        num_hidden: Number of hidden channels
        device: Device to place model on
        transfer_from: Path to pretrained model weights (optional)
        freeze_backbone: Whether to freeze backbone when transferring

    Returns:
        Neural network model
    """
    from game_adapter import StonesAndRivers

    game = StonesAndRivers(board_size)
    model = StonesRiversNet(game, num_resBlocks, num_hidden, device)

    if transfer_from is not None:
        # Load source model
        source_checkpoint = torch.load(transfer_from, map_location=device)

        # Determine source board size from checkpoint
        source_rows = source_checkpoint.get('board_rows', 13)
        source_cols = source_checkpoint.get('board_cols', 12)

        if source_rows == 13:
            source_size = 'small'
        elif source_rows == 15:
            source_size = 'medium'
        else:
            source_size = 'large'

        source_game = StonesAndRivers(source_size)
        source_model = StonesRiversNet(source_game, num_resBlocks, num_hidden, device)
        source_model.load_state_dict(source_checkpoint['model_state_dict'])

        # Transfer weights
        model = transfer_weights(source_model, model, freeze_backbone)

    return model
