#!/usr/bin/env python3
"""
Full Tournament Runner - Round Robin with Color Swapping

Runs a complete tournament where each bot plays against every other bot
twice: once as circle and once as square.
"""

import subprocess
import time
import json
import sys
import os
from pathlib import Path
from datetime import datetime

class FullTournament:
    def __init__(self, bots, board_size='small', base_port=9100, match_timeout=30):
        """
        Initialize tournament.
        
        Args:
            bots: List of bot names (e.g., ['bot1', 'bot2'])
            board_size: 'small', 'medium', or 'large'
            base_port: Starting port number (will increment for each match)
            match_timeout: Timeout per match in seconds
        """
        self.bots = bots
        self.board_size = board_size
        self.base_port = base_port
        self.match_timeout = match_timeout
        
        self.project_root = Path(__file__).parent.parent
        self.results_dir = self.project_root / "tournament" / "results"
        self.results_dir.mkdir(exist_ok=True)
        
        # Generate timestamp for this tournament
        self.timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        self.results_file = self.results_dir / f"tournament_{self.timestamp}.json"
        
        self.matches = []
        self.results = {
            'timestamp': self.timestamp,
            'board_size': board_size,
            'match_timeout': match_timeout,
            'bots': bots,
            'matches': [],
            'standings': {}
        }
    
    def generate_matches(self):
        """Generate all match pairings (round-robin with color swap)"""
        matches = []
        
        # Each bot plays every other bot twice (once as each color)
        for i, bot1 in enumerate(self.bots):
            for bot2 in self.bots[i+1:]:
                # Match 1: bot1 as circle, bot2 as square
                matches.append({
                    'match_id': len(matches) + 1,
                    'circle': bot1,
                    'square': bot2
                })
                
                # Match 2: bot2 as circle, bot1 as square (color swap)
                matches.append({
                    'match_id': len(matches) + 1,
                    'circle': bot2,
                    'square': bot1
                })
        
        self.matches = matches
        return matches
    
    def run_match(self, match_id, circle_bot, square_bot, port):
        """Run a single match"""
        print("\n" + "="*70)
        print(f"🎮 MATCH {match_id}/{len(self.matches)}")
        print(f"   Circle: {circle_bot} vs Square: {square_bot}")
        print(f"   Port: {port}, Timeout: {self.match_timeout}s")
        print("="*70)
        
        python_exe = sys.executable
        
        # Run the match with timeout (timeout is total time for match script to complete)
        # The match script itself handles the game and waits for completion
        cmd = [
            python_exe,
            str(self.project_root / 'tournament' / 'run_tournament_match.py'),
            circle_bot,
            square_bot,
            '--board-size', self.board_size,
            '--port', str(port)
        ]
        
        try:
            # Give extra time beyond match timeout for cleanup
            result = subprocess.run(
                cmd,
                timeout=self.match_timeout + 30,
                capture_output=True,
                text=True,
                cwd=str(self.project_root)
            )
            
            # Parse output to extract log file location
            output = result.stdout + result.stderr
            
            match_result = {
                'match_id': match_id,
                'circle': circle_bot,
                'square': square_bot,
                'completed': result.returncode == 0,
                'timeout': False,
                'log_file': None
            }
            
            # Extract log file path from output
            import re
            log_match = re.search(r'Full logs saved to:\s*(.+\.txt)', output)
            if log_match:
                match_result['log_file'] = log_match.group(1).strip()
            
            print(f"\n✅ Match {match_id} completed!")
            if match_result['log_file']:
                print(f"   Logs saved to: {match_result['log_file']}")
            
            return match_result
            
        except subprocess.TimeoutExpired:
            print(f"\n⏰ Match {match_id} TIMEOUT after {self.match_timeout}s")
            return {
                'match_id': match_id,
                'circle': circle_bot,
                'square': square_bot,
                'winner': 'timeout',
                'completed': False,
                'timeout': True
            }
        
        except Exception as e:
            print(f"\n❌ Match {match_id} ERROR: {e}")
            return {
                'match_id': match_id,
                'circle': circle_bot,
                'square': square_bot,
                'winner': 'error',
                'error': str(e),
                'completed': False
            }
    
    def calculate_standings(self):
        """Calculate tournament standings from match results"""
        # Since we're not parsing winners, just return empty standings
        standings = {bot: {'matches_completed': 0} for bot in self.bots}
        
        for match in self.results['matches']:
            if match.get('completed'):
                standings[match['circle']]['matches_completed'] = standings[match['circle']].get('matches_completed', 0) + 1
                standings[match['square']]['matches_completed'] = standings[match['square']].get('matches_completed', 0) + 1
        
        self.results['standings'] = standings
        return standings
    
    def save_results(self):
        """Save tournament results to JSON and text summary"""
        # Save JSON
        with open(self.results_file, 'w') as f:
            json.dump(self.results, f, indent=2)
        
        # Create text summary with match logs index
        summary_file = self.results_dir / f"summary_{self.timestamp}.txt"
        with open(summary_file, 'w') as f:
            f.write("="*80 + "\n")
            f.write("TOURNAMENT SUMMARY\n")
            f.write("="*80 + "\n")
            f.write(f"Timestamp: {self.timestamp}\n")
            f.write(f"Board Size: {self.board_size}\n")
            f.write(f"Match Timeout: {self.match_timeout}s\n")
            f.write(f"Bots: {', '.join(self.bots)}\n")
            f.write(f"Total Matches: {len(self.matches)}\n")
            f.write("="*80 + "\n\n")
            
            f.write("MATCH LOGS:\n")
            f.write("-"*80 + "\n")
            for match in self.results['matches']:
                f.write(f"\nMatch {match['match_id']}: {match['circle']} (circle) vs {match['square']} (square)\n")
                f.write(f"  Completed: {match.get('completed', False)}\n")
                if match.get('log_file'):
                    f.write(f"  Log file: {match['log_file']}\n")
                else:
                    f.write(f"  Log file: NOT AVAILABLE\n")
                f.write("\n")
            
            f.write("="*80 + "\n")
            f.write("\nNOTE: Analyze individual match log files to determine winners.\n")
            f.write("Each log contains server output and bot outputs.\n")
            f.write("="*80 + "\n")
        
        print(f"\n💾 Results saved to: {self.results_file}")
        print(f"📄 Summary saved to: {summary_file}")
    
    def print_standings(self):
        """Print tournament summary"""
        print("\n" + "="*70)
        print("🏆 TOURNAMENT SUMMARY")
        print("="*70)
        print(f"Total matches: {len(self.results['matches'])}")
        
        completed = sum(1 for m in self.results['matches'] if m.get('completed'))
        print(f"Completed matches: {completed}")
        
        print("\nMatch logs saved in: tournament/logs/")
        print("Analyze individual log files to determine winners.")
        print("="*70)
    
    def run(self):
        """Run the complete tournament"""
        # Generate matches first
        self.generate_matches()
        
        print("\n" + "="*70)
        print("🏆 TOURNAMENT START")
        print("="*70)
        print(f"   Bots: {', '.join(self.bots)}")
        print(f"   Board: {self.board_size}")
        print(f"   Matches: {len(self.matches)} (round-robin with color swap)")
        print(f"   Timeout per match: {self.match_timeout}s")
        print(f"   Results will be saved to: {self.results_file}")
        print("="*70)
        
        # Run all matches
        for i, match in enumerate(self.matches):
            port = self.base_port + i
            
            # Clean up any stray SERVER processes (not this script!)
            # Kill only processes on the specific port
            os.system(f'lsof -ti:{port} | xargs kill -9 2>/dev/null')
            time.sleep(1)
            
            result = self.run_match(
                match['match_id'],
                match['circle'],
                match['square'],
                port
            )
            
            self.results['matches'].append(result)
            
            # Save intermediate results after each match
            self.save_results()
            
            # Small delay between matches
            if i < len(self.matches) - 1:
                print("\n⏸️  Pausing 2s before next match...")
                time.sleep(2)
        
        # Calculate final standings
        self.calculate_standings()
        self.save_results()
        
        # Print results
        self.print_standings()
        
        print(f"\n✅ Tournament completed!")
        print(f"📊 Full results: {self.results_file}")


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='Run a full tournament')
    parser.add_argument('bots', nargs='+', help='Bot names (e.g., bot1 bot2)')
    parser.add_argument('--board-size', choices=['small', 'medium', 'large'],
                       default='small', help='Board size')
    parser.add_argument('--timeout', type=int, default=30,
                       help='Timeout per match in seconds')
    parser.add_argument('--base-port', type=int, default=9100,
                       help='Starting port number')
    
    args = parser.parse_args()
    
    tournament = FullTournament(
        bots=args.bots,
        board_size=args.board_size,
        base_port=args.base_port,
        match_timeout=args.timeout
    )
    
    tournament.run()


if __name__ == '__main__':
    main()
