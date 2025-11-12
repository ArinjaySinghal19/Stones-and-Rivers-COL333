#!/usr/bin/env python3
"""
Apply trained weights to heuristics.h

This script updates the Weights struct in heuristics.h with weights
from a training results file.

Usage:
    python apply_weights.py <weights_file>
    
Examples:
    python apply_weights.py c++_sample_files/weight_training_results/best_weights.json
    python apply_weights.py my_weights.json
"""

import json
import sys
import re
from pathlib import Path


def read_weights_from_json(filepath):
    """Read weights from JSON file"""
    with open(filepath, 'r') as f:
        data = json.load(f)
    
    if 'weights' not in data:
        raise ValueError("JSON file must contain 'weights' field")
    
    return data['weights'], data.get('fitness', 'unknown')


def update_heuristics_h(cpp_dir, weights_dict):
    """Update the Weights struct in heuristics.h"""
    heuristics_h = Path(cpp_dir) / "heuristics.h"
    
    if not heuristics_h.exists():
        raise FileNotFoundError(f"File not found: {heuristics_h}")
    
    # Read the file
    with open(heuristics_h, 'r') as f:
        content = f.read()
    
    # Build the replacement string
    new_weights_lines = []
    for name, value in weights_dict.items():
        new_weights_lines.append(f"        double {name} = {value:.6f};")
    
    new_weights_section = "\n".join(new_weights_lines)
    
    # Replace the weights in the struct
    pattern = r'(struct Weights \{)(.*?)(^\s*\};)'
    
    def replacer(match):
        return f"{match.group(1)}\n{new_weights_section}\n    {match.group(3)}"
    
    new_content = re.sub(pattern, replacer, content, flags=re.DOTALL | re.MULTILINE)
    
    # Backup original
    backup_file = heuristics_h.with_suffix('.h.backup')
    with open(backup_file, 'w') as f:
        f.write(content)
    print(f"✓ Backup created: {backup_file}")
    
    # Write updated content
    with open(heuristics_h, 'w') as f:
        f.write(new_content)
    print(f"✓ Updated: {heuristics_h}")
    
    return True


def main():
    if len(sys.argv) < 2:
        print("Usage: python apply_weights.py <weights_file>")
        print("\nExample:")
        print("  python apply_weights.py c++_sample_files/weight_training_results/best_weights.json")
        sys.exit(1)
    
    weights_file = Path(sys.argv[1])
    
    if not weights_file.exists():
        print(f"Error: File not found: {weights_file}")
        sys.exit(1)
    
    print("=" * 70)
    print("Applying Trained Weights to heuristics.h")
    print("=" * 70)
    print()
    
    # Read weights
    print(f"Reading weights from: {weights_file}")
    weights, fitness = read_weights_from_json(weights_file)
    print(f"Fitness: {fitness}")
    print()
    
    # Show weights
    print("Weights to apply:")
    for name, value in weights.items():
        print(f"  {name:40s}: {value:10.6f}")
    print()
    
    # Confirm
    response = input("Apply these weights to c++_sample_files/heuristics.h? [y/N]: ")
    if response.lower() != 'y':
        print("Cancelled.")
        sys.exit(0)
    
    # Apply
    try:
        update_heuristics_h("c++_sample_files", weights)
        print()
        print("=" * 70)
        print("Success!")
        print("=" * 70)
        print()
        print("Next steps:")
        print("1. Compile the C++ code: cd c++_sample_files && make")
        print("2. Test the updated bot")
        print()
        print("To revert: cp c++_sample_files/heuristics.h.backup c++_sample_files/heuristics.h")
        
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)


if __name__ == '__main__':
    main()
