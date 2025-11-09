# 🎮 AlphaZero Training Guide for Stones and Rivers

## 📊 Training Configurations

### Quick Test Modes (For Development)

#### `quick-bootstrap-gui` (Current - For Debugging)
```bash
cd alphazero
make quick-bootstrap-gui
```
- **Purpose**: Visual debugging, see games being played
- **Bootstrap Phase**: 3 iterations × 5 games = 15 games vs student_cpp
- **Self-Play Phase**: 2 iterations × 10 games = 20 games
- **Total Training Samples**: ~35-50
- **Time**: ~2-3 minutes
- **Model Quality**: Very weak (testing only)
- **Output**: `model_1_StonesAndRivers_small.pt`

#### `quick-bootstrap` (Faster Testing)
```bash
cd alphazero
make quick-bootstrap
```
- **Bootstrap Phase**: 3 iterations × 20 games = 60 games vs student_cpp
- **Self-Play Phase**: 7 iterations × 50 games = 350 games
- **Total Training Samples**: ~400+
- **Time**: ~10-15 minutes (CPU), ~5 minutes (GPU)
- **Model Quality**: Weak (still testing)
- **Output**: `model_6_StonesAndRivers_small.pt`

---

### Production Training

#### `bootstrap-small` (RECOMMENDED ⭐)
```bash
cd alphazero
make bootstrap-small
```
- **Bootstrap Phase**: 20 iterations × 100 games = 2,000 games vs student_cpp
- **Self-Play Phase**: 80 iterations × 500 games = 40,000 games
- **Total Training Samples**: ~42,000+
- **Time**: 10-12 hours (GPU), 3-5 days (CPU)
- **Model Quality**: Strong (production ready)
- **Output**: `model_79_StonesAndRivers_small.pt`
- **Architecture**: 9 ResBlocks, 128 hidden channels

**Training Parameters**:
```python
--num-bootstrap-iterations 20
--num-bootstrap-games 100
--num-selfplay-iterations 80
--num-selfplay-games 500
--num-parallel-games 50
--num-searches 600
--num-resblocks 9
--num-hidden 128
```

---

## 🔄 How Training Works

### Phase 1: Bootstrap Training (vs Heuristic)

```
For each bootstrap iteration (0 to 19):
  1. Load current best model (or random if iteration 0)
  2. Play 100 games: AlphaZero vs student_cpp heuristic
     - AlphaZero plays as circle in 50 games
     - AlphaZero plays as square in 50 games
  3. Collect training data:
     - State: board position
     - Policy: MCTS search probabilities (what moves to consider)
     - Value: game outcome (1.0 for win, -1.0 for loss, 0.0 for draw)
  4. Train neural network:
     - 4 epochs over collected data
     - Batch size: 128
     - Optimizer: Adam
  5. Save checkpoint: model_bootstrap_{iteration}_StonesAndRivers_small.pt
```

**Why bootstrap?**
- Heuristic provides reasonable initial moves
- Model learns basic game mechanics faster
- Avoids random noise in early training
- Transitions to self-play once basics are learned

### Phase 2: Self-Play Training

```
For each self-play iteration (0 to 79):
  1. Load current best model
  2. Play 500 games: Model vs itself (parallel execution)
     - 50 games run in parallel for speed
     - Each game uses 600 MCTS simulations per move
  3. Collect diverse training data:
     - Model explores different strategies
     - Learns from its own mistakes
     - Discovers advanced tactics
  4. Train neural network:
     - 4 epochs over collected data
     - Policy loss + Value loss
  5. Save checkpoint: model_{iteration}_StonesAndRivers_small.pt
```

**Why self-play?**
- Model improves beyond heuristic level
- Discovers novel strategies
- Continuously refines its play
- AlphaGo/AlphaZero's key innovation

---

## 📁 Model Files Explained

### During Training
```
alphazero/checkpoints/
├── model_bootstrap_0_StonesAndRivers_small.pt   ← Bootstrap iteration 0
├── model_bootstrap_1_StonesAndRivers_small.pt   ← Bootstrap iteration 1
├── ...
├── model_bootstrap_19_StonesAndRivers_small.pt  ← Last bootstrap
├── model_0_StonesAndRivers_small.pt             ← Self-play iteration 0
├── model_1_StonesAndRivers_small.pt             ← Self-play iteration 1
├── ...
└── model_79_StonesAndRivers_small.pt            ← FINAL MODEL ✅
```

### After Training Completes
The system creates a "best model" marker:
```
model_bootstrap_complete_StonesAndRivers_small.pt  ← Symlink to best model
```

---

## 🎯 Which Model Gets Used?

When you run `./run_alphazero.sh` or use your agent in a game:

```python
# Location: alphazero/alphazero_agent.py
def create_alphazero_student_agent(player, board_size='small'):
    """Automatically finds the best model"""
    
    # Priority 1: Latest self-play model (strongest)
    if exists('model_79_{board_size}.pt'):  # or model_42, model_31, etc.
        return load('model_79_{board_size}.pt')  # Highest iteration number
    
    # Priority 2: Bootstrap complete (if no self-play models)
    elif exists('model_bootstrap_complete_{board_size}.pt'):
        return load('model_bootstrap_complete_{board_size}.pt')
    
    # Priority 3: Error if nothing found
    else:
        raise FileNotFoundError("No trained model found!")
```

**Selection Logic:**
1. **Self-play models first** - These are always stronger than bootstrap
   - Uses highest iteration number (79 > 42 > 5 > 0)
   - Ignores partial bootstrap checkpoints
2. **Bootstrap complete fallback** - Only if no self-play models exist
3. **Automatic** - You never need to manually specify which model

**Examples:**
- Have `model_79`, `model_42`, `bootstrap_complete` → Uses `model_79` ✅
- Have `model_5`, `bootstrap_complete` → Uses `model_5` ✅  
- Have only `bootstrap_complete` → Uses `bootstrap_complete` ✅
- Have only `model_bootstrap_3` → ERROR (partial checkpoints not used)

**Result**: 
- ✅ Always uses the **most recent/highest iteration** self-play model
- ✅ Falls back to bootstrap_complete only if no self-play training has happened
- ✅ No manual configuration needed!

---

## 🚀 Recommended Training Workflow

### For Development/Testing
```bash
# 1. Install dependencies
cd alphazero
make install

# 2. Quick test (2 min)
make quick-bootstrap-gui

# 3. Test the agent
cd ..
./run_alphazero.sh
# Choose option 1 (vs Random)
```

### For Production/Competition
```bash
# 1. Install dependencies
cd alphazero
make install

# 2. Full bootstrap training (10-12 hours on GPU)
make bootstrap-small

# 3. Test the trained agent
cd ..
./run_alphazero.sh
# Choose option 1 (vs Random) to verify it works
# Choose option 3 (Human vs AI) to play against it
```

### For Transfer Learning
```bash
# 1. Train on small board first
make bootstrap-small

# 2. Transfer to medium board
make bootstrap-medium

# 3. Transfer to large board
make transfer-large
```

---

## 📈 Training Progress Monitoring

During training, you'll see:

```
======================================================================
Bootstrap Iteration 15/20
======================================================================
Playing 100 games against student_cpp...
100%|████████████████████████| 100/100 [05:23<00:00,  3.24s/it]

Bootstrap stats: AlphaZero 42, Heuristic 48, Draws 10 (Win rate: 42.0%)
Collected 2847 training samples
Training...
100%|████████████████████████| 4/4 [01:15<00:00, 18.9s/epoch]
Loss: 1.234
Saved checkpoints/model_bootstrap_15_StonesAndRivers_small.pt

Current win rate vs student_cpp: 42.0%
```

**Good signs**:
- Win rate increasing over iterations
- Loss decreasing
- More training samples collected

---

## ⚙️ Training Parameters Explained

### Architecture
- **`--num-resblocks 9`**: Deeper network = better pattern recognition
  - Quick: 4 (faster, weaker)
  - Production: 9 (slower, stronger)
  
- **`--num-hidden 128`**: Wider network = more features learned
  - Quick: 64 (less memory, weaker)
  - Production: 128 (more memory, stronger)

### MCTS (Monte Carlo Tree Search)
- **`--num-searches 600`**: Simulations per move
  - More = stronger play, slower training
  - Quick: 100-200
  - Production: 600-800

### Training Hyperparameters
- **`--num-epochs 4`**: How many times to train on same data
- **`--batch-size 128`**: Training batch size
- **`--temperature 1.25`**: Exploration during self-play (higher = more exploration)
- **`--C 2`**: MCTS exploration constant

---

## 🐛 Troubleshooting

### "No trained model found"
```bash
# Check if models exist
ls alphazero/checkpoints/*.pt

# If empty, run training
cd alphazero
make quick-bootstrap
```

### Training is slow
```bash
# Use CPU if no GPU (add --device cpu)
# Or reduce:
--num-searches 200        # Instead of 600
--num-parallel-games 10   # Instead of 50
--num-hidden 64           # Instead of 128
```

### Out of memory
```bash
# Reduce batch size or model size:
--batch-size 64           # Instead of 128
--num-resblocks 4         # Instead of 9
--num-hidden 64           # Instead of 128
```

---

## 📊 Expected Performance

### After quick-bootstrap (10 min)
- Beats random player: ~60-70%
- vs student_cpp: ~20-30%
- Skill level: Beginner

### After bootstrap-small (10-12 hours)
- Beats random player: ~95%+
- vs student_cpp: ~40-60%
- Skill level: Intermediate to Advanced

### After 200+ iterations (2-3 days)
- Beats random player: ~99%+
- vs student_cpp: ~70-80%+
- Skill level: Expert
- May discover novel strategies

---

## 🎓 Key Concepts

### Why Bootstrap Works Better
1. **Guided Initialization**: Learn from decent heuristic first
2. **Faster Convergence**: Avoid random exploration waste
3. **Better Starting Point**: Self-play builds on solid foundation

### Why Self-Play is Powerful
1. **Unlimited Data**: Generate infinite training games
2. **Self-Improvement**: Model learns from itself
3. **Novel Strategies**: May surpass human knowledge (like AlphaGo)

### MCTS Role
- Improves neural network predictions through search
- Balances exploration vs exploitation
- Provides better training targets than raw network output

---

## 📖 Further Reading

- Original AlphaZero paper: https://arxiv.org/abs/1712.01815
- AlphaGo Zero: https://www.nature.com/articles/nature24270
- MCTS explanation: https://en.wikipedia.org/wiki/Monte_Carlo_tree_search
