# Quick Start Guide - Running Your Trained AlphaZero Agent

You've completed training! Here's how to run your agent.

## ✅ You've Already Done: Training

```bash
cd alphazero
make train-small  # ✓ COMPLETED
```

Your trained models are in: `alphazero/checkpoints/`

---

## 🚀 How to Run (3 Easy Ways)

### **Method 1: Interactive Script (Recommended)**

From the project root directory:

```bash
./run_alphazero.sh
```

This will show you a menu:
```
How do you want to run?
  1) AlphaZero (student) vs Random [GUI]
  2) AlphaZero (student) vs Random [No GUI - Faster]
  3) Human vs AlphaZero [GUI]
  4) AlphaZero vs AlphaZero [No GUI]
  5) Custom command
```

Just choose a number and press Enter!

---

### **Method 2: Direct Commands**

#### Option A: Watch AlphaZero vs Random (No GUI - Terminal Only)

```bash
export BOARD_SIZE=small
cd client_server
python gameEngine.py --mode aivai --circle student --square random --board-size small --nogui
```

**Output will look like:**
```
🎮 RIVER AND STONES GAME 🎮
==================================================
   0  1  2  3  4  5  6  7  8  9 10 11
...
Circle wins!
```

#### Option B: Watch AlphaZero vs Random (With GUI)

```bash
export BOARD_SIZE=small
cd client_server
python gameEngine.py --mode aivai --circle student --square random --board-size small
```

#### Option C: Play Against AlphaZero (GUI)

```bash
export BOARD_SIZE=small
cd client_server
python gameEngine.py --mode hvai --square student --board-size small
```

You (human) play as Circle (red pieces), AlphaZero plays as Square (blue pieces).

---

### **Method 3: Python Script**

Create `test_agent.py`:

```python
import sys
import os

# Add paths
sys.path.insert(0, 'alphazero')
sys.path.insert(0, 'client_server')

from alphazero_agent import AlphaZeroAgent
from gameEngine import default_start_board, score_cols_for

# Load agent
agent = AlphaZeroAgent(
    player='circle',
    model_path='alphazero/checkpoints/model_49_StonesAndRivers_small.pt',
    board_size='small',
    num_searches=600  # More searches = better play
)

# Test it
board = default_start_board(13, 12)
move = agent.choose(board, 13, 12, score_cols_for(12), 120.0, 120.0)
print(f"AlphaZero chose: {move}")
```

Run it:
```bash
python test_agent.py
```

---

## 🎯 Quick Reference Commands

### Run a Quick Test Game

```bash
# From project root
export BOARD_SIZE=small
cd client_server
python gameEngine.py --mode aivai --circle student --square random --nogui
```

### Run Multiple Games for Statistics

```bash
# Run 10 games
for i in {1..10}; do
    echo "Game $i"
    python gameEngine.py --mode aivai --circle student --square random --nogui | grep -E "wins|draw"
done
```

### Change Board Size

```bash
# For medium board (if you trained it)
export BOARD_SIZE=medium
cd client_server
python gameEngine.py --mode aivai --circle student --square random --board-size medium --nogui

# For large board (if you trained it)
export BOARD_SIZE=large
cd client_server
python gameEngine.py --mode aivai --circle student --square random --board-size large --nogui
```

---

## 🔧 Troubleshooting

### Issue: "No trained model found"

**Check if model exists:**
```bash
ls -lh alphazero/checkpoints/
```

**Should see files like:**
```
model_0_StonesAndRivers_small.pt
model_1_StonesAndRivers_small.pt
...
model_49_StonesAndRivers_small.pt
```

**If missing, retrain:**
```bash
cd alphazero
make train-small
```

---

### Issue: "AlphaZero not available, using fallback"

This means the agent couldn't load. Check:

1. **Board size matches:**
   ```bash
   export BOARD_SIZE=small  # Must match your trained model
   ```

2. **Python can find modules:**
   ```bash
   cd client_server
   python -c "import sys; sys.path.insert(0, '../alphazero'); from alphazero_agent import AlphaZeroAgent; print('OK')"
   ```

3. **Model file is valid:**
   ```bash
   python -c "import torch; print(torch.load('../alphazero/checkpoints/model_49_StonesAndRivers_small.pt'))"
   ```

---

### Issue: Agent plays slowly

**Reduce MCTS searches for faster play:**

Edit `alphazero/alphazero_agent.py` line ~150:

```python
# Change from:
agent = AlphaZeroAgent(player, model_path, board_size, num_searches=400)

# To:
agent = AlphaZeroAgent(player, model_path, board_size, num_searches=200)
```

Or create your own agent:
```python
from alphazero_agent import AlphaZeroAgent

agent = AlphaZeroAgent(
    player='circle',
    model_path='alphazero/checkpoints/model_49_StonesAndRivers_small.pt',
    board_size='small',
    num_searches=100  # Faster but less accurate
)
```

---

## 📊 Expected Performance

After 50 training iterations on small board:

| Opponent | Win Rate | Notes |
|----------|----------|-------|
| Random | ~90-95% | Should dominate |
| Heuristic | ~60-70% | Depends on heuristic quality |
| Self-play | ~50% | By definition |

If your agent isn't performing well:
- Train for more iterations: `make resume-small`
- Check that you're using the latest checkpoint
- Increase MCTS searches at inference time

---

## 🎮 Game Controls (GUI Mode)

When playing in GUI mode:

1. **Select a piece** - Click on your piece (highlighted with yellow border)
2. **Choose action:**
   - Press `M` for Move
   - Press `P` for Push
   - Press `F` for Flip (stone ↔ river)
   - Press `R` for Rotate (river orientation)
3. **Click destination** - Click where you want to move/push
4. **ESC** - Cancel selection
5. **S** - Save game

---

## 💡 Pro Tips

### Watch AlphaZero Think

When running in terminal mode, you'll see:
```
AlphaZero agent loaded from model_49_StonesAndRivers_small.pt
Playing as circle on small board
Using device: cuda
```

This confirms it's loaded correctly.

### Run Without GUI for Faster Games

The `--nogui` flag runs games much faster since it doesn't render graphics:
```bash
python gameEngine.py --mode aivai --circle student --square random --nogui
```

### Test Different Checkpoints

Try different training iterations:
```bash
# Early training (iteration 10)
agent = AlphaZeroAgent('circle', 'alphazero/checkpoints/model_10_StonesAndRivers_small.pt', 'small')

# Late training (iteration 49)
agent = AlphaZeroAgent('circle', 'alphazero/checkpoints/model_49_StonesAndRivers_small.pt', 'small')
```

---

## 📝 Summary

**Simplest way to run:**

```bash
# From project root
./run_alphazero.sh
# Then choose option 2 (AlphaZero vs Random, No GUI)
```

**Or manually:**

```bash
export BOARD_SIZE=small
cd client_server
python gameEngine.py --mode aivai --circle student --square random --board-size small --nogui
```

**That's it!** Your AlphaZero agent will play against the random agent and you'll see the game in the terminal.

---

## 🚀 Next Steps

1. **Run more games** to see performance
2. **Train on medium/large** boards: `make transfer-medium`
3. **Adjust hyperparameters** in `alphazero/train.py`
4. **Compete** against other students' agents!

Enjoy your trained AlphaZero agent! 🎉
