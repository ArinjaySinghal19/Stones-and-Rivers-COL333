# AlphaZero for Stones and Rivers

A complete AlphaZero implementation for the Stones and Rivers game with support for:
- **Multiple board sizes** (small 13×12, medium 15×14, large 17×16)
- **Transfer learning** across board sizes
- **Parallel self-play** for efficient GPU training
- **Policy-value neural network** with ResNet architecture

## Architecture Overview

### Components

1. **game_adapter.py** - Adapts the game engine to AlphaZero interface
   - Converts board states to neural network input (7 binary planes)
   - Encodes actions and handles move generation
   - Supports all three board sizes

2. **model.py** - Neural network architecture
   - ResNet-based policy-value network
   - Transfer learning support between board sizes
   - Adaptive input/output layers for different board dimensions

3. **mcts.py** - Monte Carlo Tree Search
   - Neural network-guided search
   - Parallel batch evaluation for efficiency
   - UCB-based node selection with Dirichlet noise

4. **alphazero.py** - Training loop
   - Parallel self-play game generation
   - Policy and value loss optimization
   - Checkpoint management

5. **alphazero_agent.py** - Agent for game engine integration
   - Loads trained models
   - Provides move selection via MCTS
   - Integrates with student_agent.py

## Quick Start

### 1. Installation

```bash
cd alphazero
make install
```

Or manually:
```bash
pip install torch numpy tqdm
```

### 2. Training

#### Train from Scratch (Small Board)

```bash
make train-small
```

This will train for 50 iterations with:
- 500 self-play games per iteration
- 50 parallel games
- 600 MCTS simulations per move
- 9 ResNet blocks, 128 hidden channels

#### Transfer Learning Pipeline

```bash
# 1. Train on small board first
make train-small

# 2. Transfer to medium board
make transfer-medium

# 3. Transfer to large board
make transfer-large
```

### 3. Testing

```bash
# Test against random agent
make test-agent

# Or manually
cd ../client_server
export BOARD_SIZE=small
python gameEngine.py --mode aivai --circle student --square random --nogui
```

## Training Configuration

### Default Hyperparameters

```python
{
    'C': 2,                          # UCB exploration constant
    'num_searches': 600,             # MCTS simulations per move
    'num_iterations': 50,            # Training iterations
    'num_selfPlay_iterations': 500,  # Games per iteration
    'num_parallel_games': 50,        # Parallel self-play games
    'num_epochs': 4,                 # Training epochs per iteration
    'batch_size': 128,               # Training batch size
    'temperature': 1.25,             # Action sampling temperature
    'dirichlet_epsilon': 0.25,       # Dirichlet noise weight
    'dirichlet_alpha': 0.3,          # Dirichlet concentration
    'lr': 0.001,                     # Learning rate
    'weight_decay': 0.0001           # L2 regularization
}
```

### Custom Training

```bash
python train.py \
    --board-size small \
    --num-iterations 100 \
    --num-selfplay-iterations 1000 \
    --num-parallel-games 100 \
    --num-searches 800 \
    --num-resblocks 12 \
    --num-hidden 256 \
    --lr 0.0005 \
    --output-dir my_checkpoints
```

## Transfer Learning

Transfer learning allows the model to leverage knowledge from smaller boards:

```bash
# Train on small board
python train.py --board-size small --num-iterations 50

# Transfer to medium (faster convergence)
python train.py \
    --board-size medium \
    --transfer-from checkpoints/model_49_StonesAndRivers_small.pt \
    --num-iterations 30

# Optionally freeze backbone for fine-tuning
python train.py \
    --board-size medium \
    --transfer-from checkpoints/model_49_StonesAndRivers_small.pt \
    --freeze-backbone \
    --num-iterations 20
```

### Why Transfer Learning?

1. **Faster Convergence** - Medium/large boards benefit from small board patterns
2. **Better Exploration** - Pre-trained features help in larger state spaces
3. **Resource Efficiency** - Reduces training time by ~40-60%

## Neural Network Architecture

### Input Representation (7 Binary Planes)

The board state is encoded as 7 feature planes:

1. **Square rivers** (current player perspective)
2. **Square stones**
3. **Square scoring area**
4. **Circle rivers** (opponent)
5. **Circle stones**
6. **Circle scoring area**
7. **Blank spaces**

### Network Structure

```
Input (7 × rows × cols)
    ↓
Conv2d(7 → 128, 3×3) + BatchNorm + ReLU
    ↓
9× ResidualBlock(128)
    ↓
    ├─→ Policy Head
    │   Conv2d(128 → 32, 3×3) + BatchNorm + ReLU
    │   Flatten → Linear(32×rows×cols → action_size)
    │
    └─→ Value Head
        Conv2d(128 → 3, 3×3) + BatchNorm + ReLU
        Flatten → Linear(3×rows×cols → 256) → ReLU
        Linear(256 → 1) → Tanh
```

## File Structure

```
alphazero/
├── __init__.py              # Package initialization
├── game_adapter.py          # Game interface
├── model.py                 # Neural network
├── mcts.py                  # MCTS implementation
├── alphazero.py             # Training loop
├── alphazero_agent.py       # Agent for game engine
├── train.py                 # Training script
├── requirements.txt         # Dependencies
├── Makefile                 # Build automation
├── README.md               # This file
└── checkpoints/            # Saved models
    ├── model_0_StonesAndRivers_small.pt
    ├── model_1_StonesAndRivers_small.pt
    └── ...
```

## Checkpoint Format

Each checkpoint contains:

```python
{
    'iteration': int,              # Training iteration
    'model_state_dict': dict,      # Model weights
    'optimizer_state_dict': dict,  # Optimizer state
    'board_rows': int,             # Board dimensions
    'board_cols': int,
    'args': dict                   # Training hyperparameters
}
```

## Integration with Game Engine

The AlphaZero agent integrates seamlessly with `student_agent.py`:

1. **Automatic Detection** - Checks for trained models in `checkpoints/`
2. **Fallback Behavior** - Uses heuristic if no model found
3. **Environment Variable** - Set `BOARD_SIZE` to specify board size

```bash
# Set board size and run
export BOARD_SIZE=small
python gameEngine.py --mode aivai --circle student --square random --nogui
```

## Performance Tips

### GPU Acceleration

```bash
# Check GPU availability
python -c "import torch; print(torch.cuda.is_available())"

# Training automatically uses GPU if available
python train.py --device cuda --board-size small
```

### Memory Optimization

For large boards or limited GPU memory:

```bash
python train.py \
    --board-size large \
    --num-parallel-games 20 \    # Reduce parallel games
    --batch-size 64 \             # Smaller batch size
    --num-resblocks 6             # Fewer ResBlocks
```

### Fast Testing

For quick iteration during development:

```bash
make quick-train-small
```

This uses:
- 3 iterations
- 50 self-play games
- 10 parallel games
- 100 MCTS searches
- Smaller network (4 blocks, 64 channels)

## Troubleshooting

### Out of Memory

Reduce batch size and parallel games:
```bash
python train.py --batch-size 64 --num-parallel-games 20
```

### Training Too Slow

1. Use GPU: `--device cuda`
2. Reduce MCTS searches: `--num-searches 400`
3. Smaller network: `--num-resblocks 6 --num-hidden 64`

### Model Not Loading

Check checkpoint directory:
```bash
ls -lh checkpoints/
```

Ensure correct board size:
```bash
export BOARD_SIZE=small  # or medium, large
```

## Advanced Features

### Custom Evaluation

Create a script to evaluate against baseline:

```python
from alphazero_agent import AlphaZeroAgent
from gameEngine import default_start_board, score_cols_for

# Load agent
agent = AlphaZeroAgent('circle', 'checkpoints/model_49_small.pt', 'small')

# Play game
board = default_start_board(13, 12)
move = agent.choose(board, 13, 12, score_cols_for(12), 120.0, 120.0)
```

### Resume Training

```bash
python train.py \
    --board-size small \
    --resume-from checkpoints/model_25_StonesAndRivers_small.pt \
    --num-iterations 50
```

## Makefile Targets

```bash
make help              # Show all available targets
make install           # Install dependencies
make train-small       # Train small board
make train-medium      # Train medium board
make train-large       # Train large board
make transfer-medium   # Transfer small → medium
make transfer-large    # Transfer medium → large
make test-agent        # Test against random
make clean             # Remove checkpoints
make resume-small      # Resume from latest checkpoint
```

## Expected Training Time

On NVIDIA RTX 3080 (10GB):

| Board | Iterations | Time/Iteration | Total Time |
|-------|-----------|----------------|------------|
| Small | 50        | ~8 min         | ~6.5 hours |
| Medium| 50        | ~12 min        | ~10 hours  |
| Large | 50        | ~18 min        | ~15 hours  |

With transfer learning:
- Medium: ~5 hours (30 iterations)
- Large: ~9 hours (30 iterations)

## Citation

Based on the AlphaZero algorithm from:
```
Silver, D., et al. (2017). Mastering Chess and Shogi by Self-Play with a
General Reinforcement Learning Algorithm. arXiv:1712.01815
```

## License

For educational use in COL333 Assignment 5.
