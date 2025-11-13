# Tournament System for Stones & Rivers

## Overview

This tournament system allows you to run automated matches between different C++ bot configurations for hyperparameter tuning. Each bot runs in a **separate process** via the client-server architecture, ensuring true isolation of shared libraries (.so files).

## Quick Start

### Run a Tournament

```bash
/Users/arinjaysinghal/miniconda3/bin/python3 tournament/run_full_tournament.py bot1 bot2 --timeout 400 --board-size small
```

**Parameters:**
- `bot1 bot2 ...`: Names of bots to compete (must exist in `tournament_bots/` directory)
- `--timeout`: Timeout per match in seconds (default: 400s, should be > 300s to allow full game)
- `--board-size`: Board size - `small`, `medium`, or `large` (default: small)

### Results Location

After the tournament completes:

1. **Match Logs**: `tournament/logs/match_bot1_vs_bot2_TIMESTAMP.txt`
   - Contains full server output and bot outputs
   - Use these to determine winners and analyze gameplay

2. **Tournament Summary**: `tournament/results/summary_TIMESTAMP.txt`
   - Lists all matches with log file locations
   - No winner parsing - analyze logs manually

3. **Tournament JSON**: `tournament/results/tournament_TIMESTAMP.json`
   - Machine-readable results with log file paths

## Creating New Bots

1. **Copy bot template:**
   ```bash
   cp -r tournament_bots/bot1 tournament_bots/bot3
   ```

2. **Modify configuration** in `tournament_bots/bot3/student_agent.cpp`:
   ```cpp
   const int MINIMAX_DEPTH = 3;  // Change depth
   std::cout << "[BOT3 - DEPTH=3] Making move..." << std::endl;
   ```

3. **Update CMakeLists.txt:**
   ```cmake
   project(bot3_module)
   pybind11_add_module(bot3_module ...)
   ```

4. **Update PyBind11 module name:**
   ```cpp
   PYBIND11_MODULE(bot3_module, m) {
       py::class_<StudentAgent>(m, "AgentBot3")
           .def(py::init<std::string>())
           .def("choose", &StudentAgent::choose);
   }
   ```

5. **Update bot_loader.py** (add bot3 to naming pattern if needed)

6. **Compile:**
   ```bash
   cd tournament_bots/bot3
   mkdir build && cd build
   cmake .. && make
   ```

## Architecture

### Why Client-Server?

PyBind11 has a global type registry - once a C++ type is bound to Python, it cannot be bound again in the same process. The client-server architecture solves this:

- **Server**: Runs the game engine (one process)
- **Bot1 Client**: Separate process loading `bot1_module.so`
- **Bot2 Client**: Separate process loading `bot2_module.so`

Each bot's shared library loads independently without conflicts!

### Tournament Flow

1. `run_full_tournament.py` generates match pairings (round-robin with color swap)
2. For each match:
   - Starts web server on unique port
   - Launches bot1 as `circle` player (subprocess)
   - Launches bot2 as `square` player (subprocess)
   - Waits for game completion (max 300s)
   - Saves all logs to txt file
3. Creates summary with log file index

## Analyzing Results

Each match log contains:
- Server output with game state changes
- Bot1 stdout/stderr (includes DEPTH markers and move thinking)
- Bot2 stdout/stderr

Look for:
- `INFO:__main__:Game finished! Winner: circle` (or `square` or `draw`)
- `[BOT1 - DEPTH=1]` markers to verify correct bot loaded
- Move counts and thinking times

## Troubleshooting

### Match Timeouts
- Default match timeout is 300s in `run_tournament_match.py`
- Tournament script timeout should be > 330s (match + cleanup)
- Increase with `--timeout` parameter

### Port Conflicts
- Each match uses sequential ports starting from 9100
- Script kills processes on ports before each match
- If issues persist: `killall python3` before running

### Bot Not Loading
- Check `tournament_bots/botN/build/*.so` exists
- Verify module name matches in CMakeLists.txt and student_agent.cpp
- Check bot_loader.py expects correct naming pattern

### No Logs Generated
- Check `tournament/logs/` directory exists (auto-created)
- Verify match completed (check JSON results for `completed: true`)
- Look for errors in tournament output

## Example: 3-Bot Tournament

```bash
# Create bot3 with depth 4
cp -r tournament_bots/bot2 tournament_bots/bot3
# Edit bot3 to use DEPTH=4, module name bot3_module, class AgentBot3
cd tournament_bots/bot3/build && cmake .. && make

# Run tournament
/Users/arinjaysinghal/miniconda3/bin/python3 tournament/run_full_tournament.py bot1 bot2 bot3 --timeout 400 --board-size small
```

This will run 6 matches (each pair plays twice with color swap).

## Time Budget

For hyperparameter tuning with 1 minute per player:
- Set `--timeout 400` (allows 5min game + overhead)
- Each match can run up to 5 minutes
- Full tournament: 2 bots = 2 matches ≈ 10 minutes
- Full tournament: 3 bots = 6 matches ≈ 30 minutes
- Full tournament: 5 bots = 20 matches ≈ 100 minutes

Results saved in `tournament/logs/` and `tournament/results/`.
