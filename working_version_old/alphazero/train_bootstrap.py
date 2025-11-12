"""
Bootstrap Training Script
Train AlphaZero starting from games against heuristic bot, then transition to self-play
"""

import torch
import argparse
import os

from game_adapter import StonesAndRivers
from model import StonesRiversNet
from alphazero_bootstrap import BootstrapTraining
from alphazero import AlphaZeroParallel


def main():
    parser = argparse.ArgumentParser(description='Bootstrap Training for AlphaZero')

    # Board configuration
    parser.add_argument('--board-size', type=str, default='small',
                       choices=['small', 'medium', 'large'])

    # Model architecture
    parser.add_argument('--num-resblocks', type=int, default=9)
    parser.add_argument('--num-hidden', type=int, default=128)

    # Bootstrap phase
    parser.add_argument('--num-bootstrap-iterations', type=int, default=20,
                       help='Number of bootstrap iterations vs heuristic')
    parser.add_argument('--num-bootstrap-games', type=int, default=100,
                       help='Games vs heuristic per iteration')

    # Self-play phase
    parser.add_argument('--num-selfplay-iterations', type=int, default=80,
                       help='Number of self-play iterations after bootstrap')
    parser.add_argument('--num-selfplay-games', type=int, default=500,
                       help='Self-play games per iteration')
    parser.add_argument('--num-parallel-games', type=int, default=50)

    # Training
    parser.add_argument('--num-epochs', type=int, default=4)
    parser.add_argument('--batch-size', type=int, default=128)

    # MCTS
    parser.add_argument('--num-searches', type=int, default=600)
    parser.add_argument('--C', type=float, default=2)
    parser.add_argument('--temperature', type=float, default=1.25)
    parser.add_argument('--dirichlet-epsilon', type=float, default=0.25)
    parser.add_argument('--dirichlet-alpha', type=float, default=0.3)

    # Optimizer
    parser.add_argument('--lr', type=float, default=0.001)
    parser.add_argument('--weight-decay', type=float, default=0.0001)

    # Opponent
    parser.add_argument('--heuristic-agent', type=str, default='student_cpp',
                       help='Heuristic agent to bootstrap from')

    # GUI
    parser.add_argument('--gui', action='store_true',
                       help='Show GUI during bootstrap training (slower)')

    # Output
    parser.add_argument('--output-dir', type=str, default='checkpoints')

    # Device
    parser.add_argument('--device', type=str, default='auto',
                       choices=['auto', 'cpu', 'cuda'])

    # Resume
    parser.add_argument('--resume-from-bootstrap', type=str, default=None,
                       help='Resume from bootstrap checkpoint')

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

    # Create model
    if args.resume_from_bootstrap:
        print(f"Resuming from bootstrap checkpoint: {args.resume_from_bootstrap}")
        checkpoint = torch.load(args.resume_from_bootstrap, map_location=device)

        model = StonesRiversNet(game, args.num_resblocks, args.num_hidden, device)
        model.load_state_dict(checkpoint['model_state_dict'])

        optimizer = torch.optim.Adam(model.parameters(), lr=args.lr,
                                    weight_decay=args.weight_decay)
        optimizer.load_state_dict(checkpoint['optimizer_state_dict'])

        start_bootstrap_iter = checkpoint.get('iteration', 0) + 1
        bootstrap_complete = checkpoint.get('bootstrap_complete', False)
    else:
        print("Training from scratch with bootstrap")
        model = StonesRiversNet(game, args.num_resblocks, args.num_hidden, device)
        optimizer = torch.optim.Adam(model.parameters(), lr=args.lr,
                                    weight_decay=args.weight_decay)
        start_bootstrap_iter = 0
        bootstrap_complete = False

    # Training arguments
    training_args = {
        'C': args.C,
        'num_searches': args.num_searches,
        'num_epochs': args.num_epochs,
        'batch_size': args.batch_size,
        'temperature': args.temperature,
        'dirichlet_epsilon': args.dirichlet_epsilon,
        'dirichlet_alpha': args.dirichlet_alpha
    }

    # ============================================
    # PHASE 1: BOOTSTRAP TRAINING VS HEURISTIC
    # ============================================

    if not bootstrap_complete and start_bootstrap_iter < args.num_bootstrap_iterations:
        print("\n" + "=" * 70)
        print("PHASE 1: BOOTSTRAP TRAINING (vs Heuristic Bot)")
        print("=" * 70)
        print(f"This will train for {args.num_bootstrap_iterations} iterations")
        print(f"Each iteration plays {args.num_bootstrap_games} games vs {args.heuristic_agent}")
        print("This provides a much better starting point than random initialization!")
        print("=" * 70 + "\n")

        bootstrap_trainer = BootstrapTraining(
            model, optimizer, game, training_args,
            heuristic_agent_name=args.heuristic_agent,
            use_gui=args.gui
        )

        # Run bootstrap iterations
        for iteration in range(start_bootstrap_iter, args.num_bootstrap_iterations):
            print(f"\n{'='*70}")
            print(f"Bootstrap Iteration {iteration + 1}/{args.num_bootstrap_iterations}")
            print(f"{'='*70}")

            # Play against heuristic
            memory, stats = bootstrap_trainer.bootstrap_phase(args.num_bootstrap_games)

            print(f"Collected {len(memory)} training samples")

            # Train
            model.train()
            print("Training...")

            from tqdm import trange
            for epoch in trange(args.num_epochs):
                bootstrap_trainer.train(memory)

            # Save
            checkpoint = {
                'iteration': iteration,
                'bootstrap': True,
                'bootstrap_complete': False,
                'model_state_dict': model.state_dict(),
                'optimizer_state_dict': optimizer.state_dict(),
                'board_rows': game.row_count,
                'board_cols': game.column_count,
                'args': training_args,
                'stats': stats
            }
            save_path = os.path.join(args.output_dir,
                                    f"model_bootstrap_{iteration}_{game}.pt")
            torch.save(checkpoint, save_path)
            print(f"Saved {save_path}")

            # Show win rate trend
            win_rate = stats['alphazero_wins'] / args.num_bootstrap_games * 100
            print(f"\n{'='*70}")
            print(f"Current win rate vs {args.heuristic_agent}: {win_rate:.1f}%")
            print(f"{'='*70}\n")

        # Mark bootstrap as complete
        checkpoint = torch.load(
            os.path.join(args.output_dir,
                        f"model_bootstrap_{args.num_bootstrap_iterations-1}_{game}.pt")
        )
        checkpoint['bootstrap_complete'] = True
        torch.save(checkpoint,
                  os.path.join(args.output_dir,
                              f"model_bootstrap_complete_{game}.pt"))

        print("\n" + "=" * 70)
        print("BOOTSTRAP PHASE COMPLETE!")
        print("=" * 70)
        print("Now transitioning to self-play for refinement...")
        print("=" * 70 + "\n")

    # ============================================
    # PHASE 2: SELF-PLAY TRAINING
    # ============================================

    print("\n" + "=" * 70)
    print("PHASE 2: SELF-PLAY TRAINING")
    print("=" * 70)
    print(f"This will train for {args.num_selfplay_iterations} iterations")
    print(f"Each iteration plays {args.num_selfplay_games} games in self-play")
    print("=" * 70 + "\n")

    # Update args for self-play
    selfplay_args = training_args.copy()
    selfplay_args['num_iterations'] = args.num_selfplay_iterations
    selfplay_args['num_selfPlay_iterations'] = args.num_selfplay_games
    selfplay_args['num_parallel_games'] = args.num_parallel_games

    # Create self-play trainer
    selfplay_trainer = AlphaZeroParallel(model, optimizer, game, selfplay_args)

    # Run self-play with custom save paths
    for iteration in range(args.num_selfplay_iterations):
        memory = []

        selfplay_trainer.model.eval()
        print(f"\n{'='*70}")
        print(f"Self-Play Iteration {iteration + 1}/{args.num_selfplay_iterations}")
        print(f"{'='*70}")
        print("Parallel self-play...")

        from tqdm import trange
        for _ in trange(selfplay_args['num_selfPlay_iterations'] //
                      selfplay_args['num_parallel_games']):
            memory += selfplay_trainer.selfPlay()

        print(f"Collected {len(memory)} training samples")

        selfplay_trainer.model.train()
        print("Training...")

        for epoch in trange(selfplay_args['num_epochs']):
            selfplay_trainer.train(memory)

        # Save
        checkpoint = {
            'iteration': iteration,
            'bootstrap': False,
            'selfplay': True,
            'model_state_dict': selfplay_trainer.model.state_dict(),
            'optimizer_state_dict': selfplay_trainer.optimizer.state_dict(),
            'board_rows': selfplay_trainer.game.row_count,
            'board_cols': selfplay_trainer.game.column_count,
            'args': selfplay_args
        }
        save_path = os.path.join(args.output_dir,
                                f"model_{iteration}_{selfplay_trainer.game}.pt")
        torch.save(checkpoint, save_path)
        print(f"Saved {save_path}")

    print("\n" + "=" * 70)
    print("TRAINING COMPLETE!")
    print("=" * 70)
    print(f"Final model: {save_path}")
    print("=" * 70)


if __name__ == '__main__':
    main()
