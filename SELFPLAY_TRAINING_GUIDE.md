# Real Weight Training with Self-Play - Quick Guide

## What Changed

The weight training system now:

✅ **Actually plays games** (not mock fitness!)  
✅ **Uses self-play**: Perturbed weights vs Original weights  
✅ **Updates C++ and recompiles** for each evaluation  
✅ **Shows GUI** for verification (you can watch!)  
✅ **Uses CLI** for fast training (no graphics)

## Quick Start

### Step 1: Verify Games Are Actually Running (GUI Mode)

```bash
python verify_real_games.py
```

This will:
1. Open a game window (GUI) so you can **watch the game**
2. Play perturbed weights vs original weights
3. Verify timing is realistic (not milliseconds)
4. Test CLI mode for speed

**What you'll see:**
- A game window opens
- Pieces move on the board
- Game completes in 10-30+ seconds
- Winner is determined

**Expected:** You should see an actual game being played!

### Step 2: Train with GUI (Slow, But You Can Watch)

```bash
python train_weights_selfplay.py --iterations 3 --games-per-eval 2 --gui
```

Parameters:
- `--gui`: Shows game window for each game (SLOW)
- `--iterations 3`: Only 3 iterations (for testing)
- `--games-per-eval 2`: Only 2 games per evaluation

**Use this to verify** that games are actually being played!

### Step 3: Train with CLI (Fast, No GUI)

```bash
python train_weights_selfplay.py --iterations 20 --games-per-eval 4
```

Parameters:
- No `--gui`: Uses CLI mode (faster, no window)
- `--iterations 20`: 20 hill climbing iterations
- `--games-per-eval 4`: 4 games per evaluation (2 each side)

**Use this for actual training** once verified!

## How It Works

### Self-Play Strategy

Each iteration:

1. **Generate perturbed weights** (5-10% random change)
2. **Set perturbed weights** → Update heuristics.h → Compile
3. **Play game 1**: Perturbed (Circle) vs Current (Square)
4. **Play game 2**: Current (Circle) vs Perturbed (Square)
5. **Count wins**: Did perturbed win >= 50% of games?
6. **Accept or reject**:
   - If perturbed won >= 50% → **Accept** (new current)
   - If perturbed won < 50% → **Reject** (keep current)

### Why Self-Play?

- **Direct comparison**: Perturbed vs Current weights
- **Fair evaluation**: Each plays both sides (Circle/Square)
- **Meaningful**: Winning = actual improvement
- **No external opponent needed**

## Verification Checks

The system verifies games are real by checking:

✓ **Compilation happens** (make output)  
✓ **Games take time** (not milliseconds)  
✓ **Winner is determined** (from game output)  
✓ **Scores are extracted** (from output)

## Expected Timing

| Mode | Games/Hour | Recommended For |
|------|------------|-----------------|
| **GUI** | ~60-120 | Verification only |
| **CLI** | ~300-600 | Actual training |

For 20 iterations with 4 games each:
- **GUI mode**: ~40-80 minutes
- **CLI mode**: ~10-20 minutes

## Files Created

### Training Scripts
- `train_weights_selfplay.py` - Main training with self-play
- `verify_real_games.py` - Verification with GUI

### Results (saved to `c++_sample_files/weight_training_results/`)
- `best_weights_selfplay.json` - Best weights + stats
- `best_weights_selfplay.cpp` - C++ format (ready to use)
- `training_history_selfplay.json` - Full training log

## Example Output

### GUI Verification
```
VERIFICATION TEST - GUI MODE
Press Enter to start the GUI game...

Playing: Perturbed weights vs Original weights
You should see a game window open...

Game 1/1
Setting up perturbed weights...
Compiling C++ agent... ✓ Done
Playing game (gui mode)... CIRCLE wins! (took 23.4s)

RESULTS
Perturbed wins: 1
Current wins:   0

Did you see the game window and watch pieces moving?
Enter 'yes' if games are actually running: yes

✓ Great! Games are verified to be running.
```

### Training Example
```
HILL CLIMBING WITH REAL SELF-PLAY
Max iterations:      20
Games per eval:      4
Mode:                CLI (fast)

ITERATION 1/20
Current fitness: 50.0% win rate

Generated perturbed weights (5-10% change)

Self-Play Evaluation: Perturbed vs Original (4 games)

Game 1/4
  Perturbed (CIRCLE) vs Current (SQUARE)
  Setting up perturbed weights...
  Compiling C++ agent... ✓ Done
  Playing game (cli mode)... CIRCLE wins! (took 12.3s)

Game 2/4
  Current (CIRCLE) vs Perturbed (SQUARE)
  Setting up current weights...
  Compiling C++ agent... ✓ Done
  Playing game (cli mode)... SQUARE wins! (took 15.7s)

[... games 3 and 4 ...]

RESULTS
Perturbed wins: 3
Current wins:   1
Perturbed win rate: 75.0%

✓ IMPROVEMENT: 75.0% win rate >= 50%
  Accepting perturbed weights as new current
  ★ NEW BEST: 75.0% win rate
```

## Troubleshooting

### "No game window appears" (GUI mode)
- Check pygame is installed: `pip install pygame`
- Try running gameEngine.py directly first

### "Games finish in < 1 second"
- ⚠️ Games are NOT actually running!
- Check compilation succeeds
- Verify C++ agent is being used

### "Compilation failed"
- Check Makefile exists in c++_sample_files/
- Try `cd c++_sample_files && make` manually
- Verify C++ code compiles

### "All games are draws"
- Check both sides are using updated weights
- Verify heuristics.h is being updated
- Try increasing time limit per move

## Comparison: Old vs New

### Old System (train_weights.py)
- ✗ Mock fitness (math formula)
- ✗ No actual games
- ✗ 370 "games" in < 1 second
- ✗ Not testing real performance

### New System (train_weights_selfplay.py)
- ✓ Real game simulation
- ✓ Self-play evaluation
- ✓ ~1-2 minutes per game
- ✓ Actually optimizes bot strength

## Advanced Usage

### Custom Time Limits

Edit `verify_real_games.py` or `train_weights_selfplay.py`:

```python
winner, stats = self.play_game(
    player1="student_cpp",
    player2="student_cpp",
    time_limit=10.0,  # Change this (seconds per move)
    verbose=verbose
)
```

### More Games Per Evaluation

```bash
python train_weights_selfplay.py --games-per-eval 8
```

More games = more reliable, but slower.

### Save Intermediate Results

The system automatically saves after each iteration to:
- `weight_training_results/training_history_selfplay.json`

You can resume or analyze partial results.

## Summary

**Before:** Mock fitness, fake games, no real optimization  
**After:** Real games, self-play, actual bot improvement

**Key insight:** Perturbed weights must **beat** current weights to be accepted.

**Verification:** Use `--gui` flag to watch games and confirm they're real.

**Training:** Use CLI mode (no `--gui`) for fast, real training.

---

**Next:** Run `python verify_real_games.py` to see it in action!
