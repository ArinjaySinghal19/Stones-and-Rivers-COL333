"""
Test script to verify AlphaZero installation and setup
"""

import sys
import os

def test_imports():
    """Test if all required modules can be imported."""
    print("Testing imports...")

    try:
        import torch
        print(f"  ✓ PyTorch {torch.__version__}")
    except ImportError as e:
        print(f"  ✗ PyTorch not found: {e}")
        return False

    try:
        import numpy as np
        print(f"  ✓ NumPy {np.__version__}")
    except ImportError as e:
        print(f"  ✗ NumPy not found: {e}")
        return False

    try:
        from tqdm import tqdm
        print(f"  ✓ tqdm")
    except ImportError as e:
        print(f"  ✗ tqdm not found: {e}")
        return False

    return True


def test_modules():
    """Test if all AlphaZero modules can be imported."""
    print("\nTesting AlphaZero modules...")

    try:
        from game_adapter import StonesAndRivers
        print("  ✓ game_adapter")
    except ImportError as e:
        print(f"  ✗ game_adapter: {e}")
        return False

    try:
        from model import StonesRiversNet
        print("  ✓ model")
    except ImportError as e:
        print(f"  ✗ model: {e}")
        return False

    try:
        from mcts import MCTS, MCTSParallel
        print("  ✓ mcts")
    except ImportError as e:
        print(f"  ✗ mcts: {e}")
        return False

    try:
        from alphazero import AlphaZero, AlphaZeroParallel
        print("  ✓ alphazero")
    except ImportError as e:
        print(f"  ✗ alphazero: {e}")
        return False

    try:
        from alphazero_agent import AlphaZeroAgent
        print("  ✓ alphazero_agent")
    except ImportError as e:
        print(f"  ✗ alphazero_agent: {e}")
        return False

    return True


def test_game_adapter():
    """Test game adapter functionality."""
    print("\nTesting game adapter...")

    try:
        from game_adapter import StonesAndRivers

        # Test small board
        game = StonesAndRivers('small')
        state = game.get_initial_state()
        print(f"  ✓ Small board: {game.row_count}×{game.column_count}")
        print(f"    State shape: {state.shape}")
        print(f"    Action size: {game.action_size}")

        # Test encoding
        encoded = game.get_encoded_state(state)
        print(f"    Encoded shape: {encoded.shape}")

        # Test valid moves
        valid_moves = game.get_valid_moves(state)
        print(f"    Valid moves: {valid_moves.sum()} actions")

        return True

    except Exception as e:
        print(f"  ✗ Error: {e}")
        import traceback
        traceback.print_exc()
        return False


def test_model():
    """Test neural network model."""
    print("\nTesting model...")

    try:
        import torch
        from game_adapter import StonesAndRivers
        from model import StonesRiversNet

        game = StonesAndRivers('small')
        device = torch.device('cpu')

        model = StonesRiversNet(game, num_resBlocks=4, num_hidden=64, device=device)
        print(f"  ✓ Model created")
        print(f"    Parameters: {sum(p.numel() for p in model.parameters()):,}")

        # Test forward pass
        state = game.get_initial_state()
        encoded = game.get_encoded_state(state)
        tensor_state = torch.tensor(encoded, device=device).unsqueeze(0)

        policy, value = model(tensor_state)
        print(f"    Policy shape: {policy.shape}")
        print(f"    Value shape: {value.shape}")

        return True

    except Exception as e:
        print(f"  ✗ Error: {e}")
        import traceback
        traceback.print_exc()
        return False


def test_cuda():
    """Test CUDA availability."""
    print("\nTesting CUDA...")

    try:
        import torch

        if torch.cuda.is_available():
            print(f"  ✓ CUDA available")
            print(f"    Device: {torch.cuda.get_device_name(0)}")
            print(f"    Memory: {torch.cuda.get_device_properties(0).total_memory / 1e9:.1f} GB")
        else:
            print(f"  ℹ CUDA not available (will use CPU)")

        return True

    except Exception as e:
        print(f"  ✗ Error: {e}")
        return False


def main():
    """Run all tests."""
    print("=" * 60)
    print("AlphaZero Installation Test")
    print("=" * 60)

    tests = [
        ("Imports", test_imports),
        ("Modules", test_modules),
        ("Game Adapter", test_game_adapter),
        ("Model", test_model),
        ("CUDA", test_cuda),
    ]

    results = []
    for name, test_func in tests:
        try:
            result = test_func()
            results.append((name, result))
        except Exception as e:
            print(f"\n  ✗ Unexpected error in {name}: {e}")
            results.append((name, False))

    # Summary
    print("\n" + "=" * 60)
    print("Summary")
    print("=" * 60)

    all_passed = True
    for name, result in results:
        status = "✓ PASS" if result else "✗ FAIL"
        print(f"  {status}: {name}")
        if not result:
            all_passed = False

    print("=" * 60)

    if all_passed:
        print("\n✓ All tests passed! AlphaZero is ready to use.")
        print("\nNext steps:")
        print("  1. Train a model: make train-small")
        print("  2. Test the agent: make test-agent")
        return 0
    else:
        print("\n✗ Some tests failed. Please check the errors above.")
        print("\nTroubleshooting:")
        print("  1. Install dependencies: make install")
        print("  2. Check Python version: python --version (need 3.8+)")
        print("  3. Verify game engine: cd ../client_server && python gameEngine.py --help")
        return 1


if __name__ == '__main__':
    sys.exit(main())
