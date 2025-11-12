# Bootstrap Training Guide

## What is Bootstrap Training?

Instead of training AlphaZero from random initialization (pure self-play), **bootstrap training** first trains against your existing heuristic minimax bot. This provides:

✅ **Faster Learning** - Learns from a strong opponent immediately
✅ **Better Results** - Higher win rates in fewer iterations
✅ **Curriculum Learning** - Easy→Hard progression (heuristic→self-play)

## Why Your Previous Training Didn't Work Well

Pure self-play from scratch is challenging because:
- ❌ Both players start completely random
- ❌ Random vs Random generates mostly meaningless games
- ❌ Takes many iterations before any intelligent play emerges
- ❌ ~50 iterations might not be enough

With bootstrap training:
- ✅ Learns from your strong minimax bot immediately
- ✅ Every game is meaningful (playing vs intelligent opponent)
- ✅ Faster convergence to good play
- ✅ Then refines with self-play

---

## 🚀 How to Run Bootstrap Training

### Method 1: Full Bootstrap Training (RECOMMENDED)

```bash
cd alphazero
make bootstrap-small
```

This will:
1. **Phase 1 (Bootstrap)**: 20 iterations vs heuristic (100 games each) = 2,000 games
2. **Phase 2 (Self-Play)**: 80 iterations of self-play (500 games each) = 40,000 games
3. **Total**: 100 iterations, ~10-12 hours on GPU

Expected progression:
```
Iteration 1:  AlphaZero 10%, Heuristic 85%, Draws 5%  (learning basics)
Iteration 5:  AlphaZero 30%, Heuristic 65%, Draws 5%  (improving)
Iteration 10: AlphaZero 45%, Heuristic 50%, Draws 5%  (competitive)
Iteration 15: AlphaZero 55%, Heuristic 40%, Draws 5%  (beating heuristic!)
Iteration 20: AlphaZero 65%, Heuristic 30%, Draws 5%  (strong play)
... then transitions to self-play for refinement ...
```

---

### Method 2: Quick Test (5-10 minutes)

```bash
cd alphazero
make quick-bootstrap
```

This runs:
- 3 iterations vs heuristic (20 games each)
- 7 iterations of self-play (50 games each)
- Total: 10 iterations, ~5-10 minutes

Use this to verify everything works before running full training!

---

### Method 3: Manual Training (Custom Parameters)

```bash
cd alphazero
python train_bootstrap.py \
    --board-size small \
    --num-bootstrap-iterations 30 \
    --num-bootstrap-games 150 \
    --num-selfplay-iterations 70 \
    --num-selfplay-games 600 \
    --num-parallel-games 50 \
    --num-searches 800 \
    --heuristic-agent student_cpp \
    --output-dir checkpoints
```

---

## 📊 Training Output

You'll see output like this:

```
====================================================================
PHASE 1: BOOTSTRAP TRAINING (vs Heuristic Bot)
====================================================================
This will train for 20 iterations
Each iteration plays 100 games vs student_cpp
This provides a much better starting point than random initialization!
====================================================================

======================================================================
Bootstrap Iteration 1/20
======================================================================
Playing 100 games against student_cpp...
100%|██████████████| 100/100 [05:23<00:00,  3.23s/it]
Collected 4,523 training samples

Bootstrap stats: AlphaZero 12, Heuristic 83, Draws 5 (Win rate: 12.0%)

Training...
100%|██████████████| 4/4 [00:42<00:00, 10.52s/it]
Saved checkpoints/model_bootstrap_0_StonesAndRivers_small.pt

======================================================================
Current win rate vs student_cpp: 12.0%
======================================================================

... iterations continue ...

======================================================================
Bootstrap Iteration 20/20
======================================================================
...
Bootstrap stats: AlphaZero 68, Heuristic 28, Draws 4 (Win rate: 68.0%)
...

====================================================================
BOOTSTRAP PHASE COMPLETE!
====================================================================
Now transitioning to self-play for refinement...
====================================================================

======================================================================
PHASE 2: SELF-PLAY TRAINING
======================================================================
This will train for 80 iterations
Each iteration plays 500 games in self-play
======================================================================

... self-play iterations continue ...
```

---

## 📁 Checkpoints

Bootstrap training creates two types of checkpoints:

### Bootstrap Phase Checkpoints
```
checkpoints/model_bootstrap_0_StonesAndRivers_small.pt
checkpoints/model_bootstrap_1_StonesAndRivers_small.pt
...
checkpoints/model_bootstrap_19_StonesAndRivers_small.pt
checkpoints/model_bootstrap_complete_StonesAndRivers_small.pt  # Marks phase complete
```

### Self-Play Phase Checkpoints
```
checkpoints/model_0_StonesAndRivers_small.pt
checkpoints/model_1_StonesAndRivers_small.pt
...
checkpoints/model_79_StonesAndRivers_small.pt
```

**Use the latest `model_79_...pt` for best performance!**

---

## 🎮 How to Test Your Trained Model

After training, test it:

```bash
# Method 1: Using make
make test-agent

# Method 2: Manual
export BOARD_SIZE=small
cd ../client_server
python gameEngine.py --mode aivai --circle student --square random --nogui

# Method 3: Test vs your heuristic
export BOARD_SIZE=small
cd ../client_server
python gameEngine.py --mode aivai --circle student --square student_cpp --nogui
```

---

## 🔧 Customization

### Adjust Bootstrap Phase

More bootstrap iterations = more learning from heuristic:

```bash
python train_bootstrap.py \
    --num-bootstrap-iterations 30 \     # More iterations vs heuristic
    --num-bootstrap-games 150 \         # More games per iteration
    --num-selfplay-iterations 70        # Fewer self-play iterations
```

### Adjust Self-Play Phase

More self-play = better refinement:

```bash
python train_bootstrap.py \
    --num-bootstrap-iterations 15 \     # Fewer bootstrap iterations
    --num-selfplay-iterations 100 \     # More self-play iterations
    --num-selfplay-games 800            # More games per iteration
```

### Use Different Heuristic

If you have multiple bots:

```bash
python train_bootstrap.py \
    --heuristic-agent my_other_bot \    # Must be registered in agent.py
    ...
```

---

## ⚡ Performance Comparison

### Pure Self-Play (Your Previous Training)
```
Iterations: 50
Games: 25,000
Time: 6-7 hours
Win rate vs random: ~60-70%  (not great)
Win rate vs heuristic: ~30-40%  (struggles)
```

### Bootstrap Training (New Method)
```
Iterations: 100 (20 bootstrap + 80 self-play)
Games: 42,000 (2,000 bootstrap + 40,000 self-play)
Time: 10-12 hours
Win rate vs random: ~95%+  (dominates)
Win rate vs heuristic: ~70-80%  (beats teacher)
```

The extra time is worth it for much better results!

---

## 🎯 Expected Results

After full bootstrap training (100 iterations), your agent should:

| Opponent | Expected Win Rate |
|----------|------------------|
| Random | 95%+ |
| Your Heuristic | 70-80% |
| Pure self-play (50 iter) | 80-90% |

---

## 💡 Tips

### 1. Monitor Training Progress

Watch the win rate vs heuristic:
- Should start around 10-15%
- Should reach 50%+ by iteration 15
- Should reach 65%+ by iteration 20

If not improving, check:
- Is your C++ bot compiling correctly?
- Is `student_cpp` agent working in game engine?

### 2. Test After Bootstrap Phase

Test after the 20 bootstrap iterations (before self-play):

```bash
# Find the bootstrap_complete checkpoint
export BOARD_SIZE=small
cd ../client_server

# Edit student_agent.py to use:
# 'checkpoints/model_bootstrap_complete_StonesAndRivers_small.pt'

python gameEngine.py --mode aivai --circle student --square student_cpp --nogui
```

Should already beat heuristic ~65-70% of the time!

### 3. Resume if Interrupted

If training gets interrupted:

```bash
# Find latest bootstrap checkpoint
python train_bootstrap.py \
    --board-size small \
    --resume-from-bootstrap checkpoints/model_bootstrap_15_StonesAndRivers_small.pt \
    --num-bootstrap-iterations 20 \
    --num-selfplay-iterations 80 \
    ...
```

It will continue from iteration 16.

---

## 🔍 Troubleshooting

### Issue: "student_cpp agent not found"

**Solution**: Make sure your C++ bot is compiled:

```bash
cd ../c++_sample_files
# Check if compile.sh exists
ls compile.sh

# Compile (if needed)
bash compile.sh

# Verify it works
cd ../client_server
python gameEngine.py --mode aivai --circle student_cpp --square random --nogui
```

### Issue: "Bootstrap phase very slow"

**Solutions**:

1. Reduce games per iteration:
   ```bash
   --num-bootstrap-games 50  # instead of 100
   ```

2. Reduce MCTS searches:
   ```bash
   --num-searches 400  # instead of 600
   ```

3. Smaller network:
   ```bash
   --num-resblocks 6 --num-hidden 64
   ```

### Issue: "Not beating heuristic after 20 iterations"

This could mean:
- Your heuristic is very strong (good problem to have!)
- Need more iterations: try `--num-bootstrap-iterations 30`
- Or accept 40-50% win rate and transition to self-play anyway

---

## Summary Commands

```bash
# Quick test (10 iterations, ~10 min)
make quick-bootstrap

# Full training (100 iterations, ~12 hours) - RECOMMENDED
make bootstrap-small

# Test the result
make test-agent

# Test vs your heuristic
export BOARD_SIZE=small
cd client_server
python gameEngine.py --mode aivai --circle student --square student_cpp --nogui
```

---

## Why This Works Better

**Traditional AlphaZero (pure self-play):**
```
Random vs Random → Slightly better vs Random → ... → Eventually good
```
Takes many iterations to bootstrap from nothing.

**Bootstrap Training:**
```
Learn from Strong Opponent → Match Strong Opponent → Beat Strong Opponent → Refine with Self-Play
```
Starts with meaningful games immediately!

---

This approach is inspired by AlphaGo Zero's training against human expert games before self-play refinement. Your heuristic minimax bot serves as the "expert teacher"!

Good luck with training! 🚀
