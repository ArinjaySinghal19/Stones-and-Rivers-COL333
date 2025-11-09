"""
Training script for AlphaZero on Stones and Rivers
Supports training on different board sizes with optional transfer learning
"""

import torch
import argparse
import os
import sys

from game_adapter import StonesAndRivers
from model import StonesRiversNet, create_model_for_board_size
from alphazero import AlphaZeroParallel


def main():
    parser = argparse.ArgumentParser(description='Train AlphaZero for Stones and Rivers')

    # Board configuration
    parser.add_argument('--board-size', type=str, default='small',
                       choices=['small', 'medium', 'large'],
                       help='Board size to train on')

    # Model architecture
    parser.add_argument('--num-resblocks', type=int, default=9,
                       help='Number of residual blocks')
    parser.add_argument('--num-hidden', type=int, default=128,
                       help='Number of hidden channels')

    # Training hyperparameters
    parser.add_argument('--num-iterations', type=int, default=50,
                       help='Number of training iterations')
    parser.add_argument('--num-selfplay-iterations', type=int, default=500,
                       help='Number of self-play games per iteration')
    parser.add_argument('--num-parallel-games', type=int, default=50,
                       help='Number of parallel self-play games')
    parser.add_argument('--num-epochs', type=int, default=4,
                       help='Number of training epochs per iteration')
    parser.add_argument('--batch-size', type=int, default=128,
                       help='Training batch size')

    # MCTS parameters
    parser.add_argument('--num-searches', type=int, default=600,
                       help='Number of MCTS searches per move')
    parser.add_argument('--C', type=float, default=2,
                       help='UCB exploration constant')
    parser.add_argument('--temperature', type=float, default=1.25,
                       help='Temperature for action sampling')
    parser.add_argument('--dirichlet-epsilon', type=float, default=0.25,
                       help='Dirichlet noise mixing parameter')
    parser.add_argument('--dirichlet-alpha', type=float, default=0.3,
                       help='Dirichlet noise alpha parameter')

    # Optimizer parameters
    parser.add_argument('--lr', type=float, default=0.001,
                       help='Learning rate')
    parser.add_argument('--weight-decay', type=float, default=0.0001,
                       help='Weight decay')

    # Transfer learning
    parser.add_argument('--transfer-from', type=str, default=None,
                       help='Path to pretrained model for transfer learning')
    parser.add_argument('--freeze-backbone', action='store_true',
                       help='Freeze backbone when using transfer learning')

    # Checkpointing
    parser.add_argument('--resume-from', type=str, default=None,
                       help='Resume training from checkpoint')
    parser.add_argument('--output-dir', type=str, default='checkpoints',
                       help='Directory to save checkpoints')

    # Device
    parser.add_argument('--device', type=str, default='auto',
                       choices=['auto', 'cpu', 'cuda'],
                       help='Device to use for training')

    args = parser.parse_args()

    # Setup device
    if args.device == 'auto':
        device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    else:
        device = torch.device(args.device)

    print(f"Using device: {device}")

    # Create output directory
    os.makedirs(args.output_dir, exist_ok=True)

    # Initialize game
    game = StonesAndRivers(args.board_size)
    print(f"Training on {args.board_size} board ({game.row_count}x{game.column_count})")

    # Create or load model
    if args.resume_from:
        print(f"Resuming from checkpoint: {args.resume_from}")
        checkpoint = torch.load(args.resume_from, map_location=device)

        model = StonesRiversNet(game, args.num_resblocks, args.num_hidden, device)
        model.load_state_dict(checkpoint['model_state_dict'])

        optimizer = torch.optim.Adam(model.parameters(), lr=args.lr,
                                    weight_decay=args.weight_decay)
        optimizer.load_state_dict(checkpoint['optimizer_state_dict'])

        start_iteration = checkpoint.get('iteration', 0) + 1
        print(f"Resuming from iteration {start_iteration}")

    elif args.transfer_from:
        print(f"Transfer learning from: {args.transfer_from}")
        model = create_model_for_board_size(
            args.board_size,
            args.num_resblocks,
            args.num_hidden,
            device,
            transfer_from=args.transfer_from,
            freeze_backbone=args.freeze_backbone
        )

        # Create optimizer (with frozen params excluded if applicable)
        trainable_params = filter(lambda p: p.requires_grad, model.parameters())
        optimizer = torch.optim.Adam(trainable_params, lr=args.lr,
                                    weight_decay=args.weight_decay)
        start_iteration = 0

    else:
        print("Training from scratch")
        model = StonesRiversNet(game, args.num_resblocks, args.num_hidden, device)
        optimizer = torch.optim.Adam(model.parameters(), lr=args.lr,
                                    weight_decay=args.weight_decay)
        start_iteration = 0

    # Training arguments
    training_args = {
        'C': args.C,
        'num_searches': args.num_searches,
        'num_iterations': args.num_iterations - start_iteration,
        'num_selfPlay_iterations': args.num_selfplay_iterations,
        'num_parallel_games': args.num_parallel_games,
        'num_epochs': args.num_epochs,
        'batch_size': args.batch_size,
        'temperature': args.temperature,
        'dirichlet_epsilon': args.dirichlet_epsilon,
        'dirichlet_alpha': args.dirichlet_alpha
    }

    print("\nTraining configuration:")
    for key, value in training_args.items():
        print(f"  {key}: {value}")

    # Create AlphaZero trainer
    alphaZero = AlphaZeroParallel(model, optimizer, game, training_args)

    # Override save location
    original_learn = alphaZero.learn

    def learn_with_custom_save():
        for iteration in range(training_args['num_iterations']):
            # Call original learn iteration
            memory = []

            alphaZero.model.eval()
            print(f"\nIteration {iteration + start_iteration + 1}/{args.num_iterations}")
            print("Parallel self-play...")

            from tqdm import trange
            for _ in trange(training_args['num_selfPlay_iterations'] //
                          training_args['num_parallel_games']):
                memory += alphaZero.selfPlay()

            print(f"Collected {len(memory)} training samples")

            alphaZero.model.train()
            print("Training...")

            for epoch in trange(training_args['num_epochs']):
                alphaZero.train(memory)

            # Save with custom path
            checkpoint = {
                'iteration': iteration + start_iteration,
                'model_state_dict': alphaZero.model.state_dict(),
                'optimizer_state_dict': alphaZero.optimizer.state_dict(),
                'board_rows': alphaZero.game.row_count,
                'board_cols': alphaZero.game.column_count,
                'args': training_args
            }
            save_path = os.path.join(args.output_dir,
                                    f"model_{iteration + start_iteration}_{alphaZero.game}.pt")
            torch.save(checkpoint, save_path)
            print(f"Saved {save_path}")

    learn_with_custom_save()

    print("\nTraining complete!")


if __name__ == '__main__':
    main()
