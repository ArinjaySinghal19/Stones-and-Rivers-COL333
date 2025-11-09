# How to Run - AlphaZero for Stones and Rivers

Complete guide for training and using the AlphaZero implementation for Stones and Rivers.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Installation](#installation)
3. [Training Workflow](#training-workflow)
4. [Using the Trained Agent](#using-the-trained-agent)
5. [Transfer Learning](#transfer-learning)
6. [Testing and Evaluation](#testing-and-evaluation)
7. [Troubleshooting](#troubleshooting)

---

## Prerequisites

### System Requirements

- **Python**: 3.8 or higher
- **RAM**: 8GB minimum, 16GB recommended
- **GPU**: NVIDIA GPU with CUDA support (optional but highly recommended)
- **Disk Space**: ~5GB for checkpoints

### Check Python Version

```bash
python --version  # Should be 3.8+
```

### Check GPU Availability (Optional)

```bash
python -c "import torch; print(f'CUDA available: {torch.cuda.is_available()}')"
```

---

## Installation

### Step 1: Navigate to AlphaZero Directory

```bash
cd Stones-and-Rivers-COL333/alphazero
```

### Step 2: Install Dependencies

**Option A: Using Make (Recommended)**

```bash
make install
```

**Option B: Manual Installation**

```bash
pip install torch>=1.12.0 numpy>=1.23.0 tqdm>=4.64.0
```

**Option C: Using the existing requirements**

```bash
cd ..
pip install -r client_server/requirements.txt  # For game engine
cd alphazero
pip install -r requirements.txt                # For AlphaZero
```

### Step 3: Create Checkpoint Directory

```bash
mkdir -p checkpoints
```

---

## Training Workflow

### Quick Start (Small Board)

This is the **recommended starting point**:

```bash
cd alphazero
make train-small
```

This will:
- Train on the small board (13×12)
- Run 50 iterations
- Generate 500 self-play games per iteration
- Save checkpoints to `checkpoints/`
- Take approximately **6-7 hours on GPU** (or 20+ hours on CPU)

### Training Progress

You'll see output like:

```
Training on small board (13x12)
Training from scratch

Iteration 1/50
Parallel self-play...
100%|██████████| 10/10 [05:23<00:00, 32.35s/it]
Collected 4523 training samples
Training...
100%|██████████| 4/4 [00:42<00:00, 10.52s/it]
Saved checkpoints/model_0_StonesAndRivers_small.pt

Iteration 2/50
...
```

### Custom Training Parameters

For faster testing (reduces quality):

```bash
python train.py \
    --board-size small \
    --num-iterations 10 \
    --num-selfplay-iterations 100 \
    --num-parallel-games 20 \
    --num-searches 200 \
    --output-dir checkpoints
```

For high-quality training (slower):

```bash
python train.py \
    --board-size small \
    --num-iterations 100 \
    --num-selfplay-iterations 1000 \
    --num-parallel-games 100 \
    --num-searches 800 \
    --num-resblocks 12 \
    --num-hidden 256 \
    --output-dir checkpoints
```

### Resuming Training

If training is interrupted:

```bash
# Manual resume
python train.py \
    --board-size small \
    --resume-from checkpoints/model_25_StonesAndRivers_small.pt \
    --num-iterations 50

# Or use Make (finds latest automatically)
make resume-small
```

---

## Using the Trained Agent

### Option 1: Automatic Integration (Recommended)

The `student_agent.py` automatically detects and uses trained models:

```bash
# Set the board size
export BOARD_SIZE=small

# Run game with student agent
cd ../client_server
python gameEngine.py --mode aivai --circle student --square random --nogui
```

### Option 2: GUI Mode

```bash
export BOARD_SIZE=small
python gameEngine.py --mode hvai --square student --board-size small
```

You (human) play as circle, AlphaZero plays as square.

### Option 3: Watch AI vs AI

```bash
export BOARD_SIZE=small
python gameEngine.py --mode aivai --circle student --square student --nogui
```

### Option 4: Manual Agent Usage

Create a test script:

```python
import sys
sys.path.append('../alphazero')

from alphazero_agent import AlphaZeroAgent
from gameEngine import default_start_board, score_cols_for

# Load agent
agent = AlphaZeroAgent(
    player='circle',
    model_path='../alphazero/checkpoints/model_49_StonesAndRivers_small.pt',
    board_size='small',
    num_searches=600
)

# Get move
board = default_start_board(13, 12)
move = agent.choose(board, 13, 12, score_cols_for(12), 120.0, 120.0)
print(f"AlphaZero chose: {move}")
```

---

## Transfer Learning

Transfer learning lets you train larger boards faster by starting from a smaller board's knowledge.

### Complete Pipeline

```bash
cd alphazero

# 1. Train small board from scratch (~6 hours on GPU)
make train-small

# 2. Transfer to medium board (~5 hours on GPU)
make transfer-medium

# 3. Transfer to large board (~9 hours on GPU)
make transfer-large
```

### Manual Transfer Learning

```bash
# Find latest small board checkpoint
ls -t checkpoints/model_*_small.pt | head -n1

# Transfer to medium
python train.py \
    --board-size medium \
    --transfer-from checkpoints/model_49_StonesAndRivers_small.pt \
    --num-iterations 30 \
    --num-selfplay-iterations 400

# Transfer to large
python train.py \
    --board-size large \
    --transfer-from checkpoints/model_29_StonesAndRivers_medium.pt \
    --num-iterations 30 \
    --num-selfplay-iterations 300
```

### Fine-tuning with Frozen Backbone

For even faster transfer (but potentially lower quality):

```bash
python train.py \
    --board-size medium \
    --transfer-from checkpoints/model_49_StonesAndRivers_small.pt \
    --freeze-backbone \
    --num-iterations 20
```

This freezes the convolutional layers and only trains the heads.

---

## Testing and Evaluation

### Test Against Random Agent

```bash
make test-agent
```

Or manually:

```bash
cd client_server
export BOARD_SIZE=small
python gameEngine.py --mode aivai --circle student --square random --nogui
```

### Test Against Heuristic Bot

If you have a heuristic bot in the codebase:

```bash
cd client_server
export BOARD_SIZE=small
python gameEngine.py --mode aivai --circle student --square <your_heuristic> --nogui
```

### Multiple Games for Statistics

Create a test script `test_multiple.sh`:

```bash
#!/bin/bash

WINS=0
LOSSES=0
DRAWS=0

for i in {1..10}; do
    echo "Game $i/10"
    export BOARD_SIZE=small

    # Run game and capture result
    OUTPUT=$(python gameEngine.py --mode aivai --circle student --square random --nogui 2>&1)

    if echo "$OUTPUT" | grep -q "circle wins"; then
        WINS=$((WINS + 1))
    elif echo "$OUTPUT" | grep -q "square wins"; then
        LOSSES=$((LOSSES + 1))
    else
        DRAWS=$((DRAWS + 1))
    fi
done

echo "Results: Wins=$WINS, Losses=$LOSSES, Draws=$DRAWS"
```

Run it:

```bash
chmod +x test_multiple.sh
./test_multiple.sh
```

---

## Troubleshooting

### Issue 1: "Import alphazero_agent could not be resolved"

**Solution**: This is just an IDE warning. The import works at runtime because the path is added dynamically.

You can ignore it, or add to your IDE's Python path:
- VSCode: Add `"alphazero"` to `python.analysis.extraPaths` in `.vscode/settings.json`

### Issue 2: "CUDA out of memory"

**Solution**: Reduce batch size and parallel games:

```bash
python train.py \
    --batch-size 64 \
    --num-parallel-games 20 \
    --num-searches 400
```

### Issue 3: "No trained model found"

**Solution**: Train a model first:

```bash
cd alphazero
make train-small
```

Or use the quick training for testing:

```bash
make quick-train-small
```

### Issue 4: Training is very slow

**Solutions**:

1. **Use GPU**:
   ```bash
   python train.py --device cuda --board-size small
   ```

2. **Reduce MCTS searches**:
   ```bash
   python train.py --num-searches 400  # Default is 600
   ```

3. **Smaller network**:
   ```bash
   python train.py --num-resblocks 6 --num-hidden 64
   ```

4. **Fewer parallel games**:
   ```bash
   python train.py --num-parallel-games 20  # Default is 50
   ```

### Issue 5: "RuntimeError: mat1 and mat2 shapes cannot be multiplied"

**Solution**: This happens when loading a model trained on different board size.

Make sure:
```bash
export BOARD_SIZE=small  # Match the model you're using
```

### Issue 6: Agent plays random moves

**Possible causes**:

1. **Model not fully trained**: Train for more iterations
2. **Using wrong board size**: Check `BOARD_SIZE` environment variable
3. **Model file corrupted**: Re-train or use a different checkpoint

---

## Advanced Usage

### Monitor Training with TensorBoard (Optional)

Add to `train.py`:

```python
from torch.utils.tensorboard import SummaryWriter
writer = SummaryWriter('runs/experiment_1')
# Log metrics during training
```

### Adjust MCTS at Inference

For faster inference (lower quality):

```python
agent = AlphaZeroAgent(
    player='circle',
    model_path='checkpoints/model_49_StonesAndRivers_small.pt',
    board_size='small',
    num_searches=200  # Reduced from 600
)
```

For better play (slower):

```python
agent = AlphaZeroAgent(
    player='circle',
    model_path='checkpoints/model_49_StonesAndRivers_small.pt',
    board_size='small',
    num_searches=1200  # Increased from 600
)
```

---

## Summary Cheat Sheet

```bash
# INSTALLATION
cd alphazero && make install

# TRAINING
make train-small          # Train small board
make transfer-medium      # Transfer to medium
make transfer-large       # Transfer to large

# TESTING
make test-agent          # Test vs random

# PLAYING
export BOARD_SIZE=small
cd ../client_server
python gameEngine.py --mode aivai --circle student --square random --nogui

# CLEANUP
make clean               # Remove checkpoints
```

---

## Expected Timeline

| Task | Time (GPU) | Time (CPU) |
|------|-----------|------------|
| Install | 2 min | 2 min |
| Train small (50 iter) | 6-7 hours | 20-30 hours |
| Transfer medium (30 iter) | 5 hours | 15-20 hours |
| Transfer large (30 iter) | 9 hours | 25-35 hours |
| Quick test (10 iter) | 45 min | 3-4 hours |

---

## Questions?

Check the README.md in the alphazero directory for more detailed documentation on:
- Architecture details
- Hyperparameter tuning
- Neural network structure
- Action encoding
- Advanced features

Good luck with your training!
