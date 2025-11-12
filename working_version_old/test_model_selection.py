#!/usr/bin/env python3
"""Test which model would be selected by the agent."""

import os

model_dir = "alphazero/checkpoints"
board_size = "small"

# List all model files for this board size
model_files = [
    f for f in os.listdir(model_dir)
    if f.startswith("model_") and f.endswith(f"_{board_size}.pt")
]

print(f"Found {len(model_files)} model files:")
for f in sorted(model_files):
    print(f"  - {f}")

# Apply selection logic
def get_model_number(filename):
    """Extract iteration number from model filename."""
    parts = filename.split('_')
    # Handle bootstrap_complete specially - treat as end of bootstrap phase
    if 'bootstrap_complete' in filename:
        return 999999  # High number but not the highest
    # Skip partial bootstrap files
    if 'bootstrap' in filename:
        return -1  # Put at beginning
    try:
        return int(parts[1]) + 1000000  # Self-play models are later than bootstrap
    except (ValueError, IndexError):
        return -1

model_files.sort(key=get_model_number)
latest_model = model_files[-1]

print(f"\n✅ Selected model: {latest_model}")
print(f"   Model number: {get_model_number(latest_model)}")

# Show the ranking
print("\nModel ranking (lowest to highest priority):")
for i, f in enumerate(model_files):
    num = get_model_number(f)
    marker = " ← SELECTED" if f == latest_model else ""
    print(f"  {i+1}. {f:50s} (priority: {num:8d}){marker}")
