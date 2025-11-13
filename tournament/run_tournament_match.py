#!/usr/bin/env python3
"""
Tournament Match Runner using Client-Server Architecture

This script runs a single match between two bots using the web server architecture,
which provides true process isolation (each bot runs in its own process).
"""

import subprocess
import time
import signal
import sys
import os
import re
from pathlib import Path

# Add parent directory to path for imports
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

class TournamentMatch:
    def __init__(self, bot1_name, bot2_name, board_size='small', port=8080):
        """
        Initialize tournament match.
        
        Args:
            bot1_name: Name of first bot (will play as circle)
            bot2_name: Name of second bot (will play as square)
            board_size: Board size ('small', 'medium', 'large')
            port: Port for web server
        """
        self.bot1_name = bot1_name
        self.bot2_name = bot2_name
        self.board_size = board_size
        self.port = port
        
        self.server_process = None
        self.bot1_process = None
        self.bot2_process = None
        
        self.project_root = Path(__file__).parent.parent
        self.client_server_dir = self.project_root / "client_server"
        
    def start_server(self):
        """Start the web server"""
        print(f"🌐 Starting server on port {self.port}...")
        
        # Use the same python executable that's running this script
        python_exe = sys.executable
        
        # Start server directly with Python (web_server.py takes port as positional arg)
        cmd = [
            python_exe,
            'web_server.py',
            str(self.port)
        ]
        
        self.server_process = subprocess.Popen(
            cmd,
            cwd=str(self.client_server_dir),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
        # Wait for server to start
        print("   Waiting for server to initialize...")
        time.sleep(3)
        
        if self.server_process.poll() is not None:
            stdout, stderr = self.server_process.communicate()
            print(f"❌ Server failed to start!")
            print(f"STDOUT: {stdout}")
            print(f"STDERR: {stderr}")
            return False
        
        print(f"✅ Server started on port {self.port}")
        return True
    
    def start_bot(self, bot_name, player_side):
        """
        Start a bot client.
        
        Args:
            bot_name: Name of the bot (e.g., 'bot1', 'bot2')
            player_side: 'circle' or 'square'
        """
        print(f"🤖 Starting {bot_name} as {player_side}...")
        
        # Use the same python executable that's running this script
        python_exe = sys.executable
        
        cmd = [
            python_exe,
            'bot_client.py',
            player_side,
            str(self.port),
            '--strategy', bot_name  # Will trigger tournament bot loading
        ]
        
        process = subprocess.Popen(
            cmd,
            cwd=str(self.client_server_dir),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
        # Give bot time to connect
        time.sleep(1)
        
        if process.poll() is not None:
            stdout, stderr = process.communicate()
            print(f"❌ Bot {bot_name} failed to start!")
            print(f"STDOUT: {stdout}")
            print(f"STDERR: {stderr}")
            return None
        
        print(f"✅ {bot_name} connected as {player_side}")
        return process
    
    def wait_for_game_completion(self, timeout=300):
        """
        Wait for the game to complete by monitoring bot processes.
        
        Args:
            timeout: Maximum time to wait (seconds)
        
        Returns:
            dict with game results or None if timeout/error
        """
        print(f"⏳ Waiting for game to complete (timeout: {timeout}s)...")
        
        start_time = time.time()
        last_check = start_time
        
        # Also capture server output
        server_output = []
        
        while time.time() - start_time < timeout:
            # Check if both bot processes have finished
            bot1_done = self.bot1_process and self.bot1_process.poll() is not None
            bot2_done = self.bot2_process and self.bot2_process.poll() is not None
            
            # Collect server output continuously
            if self.server_process:
                try:
                    # Non-blocking read from server
                    import select
                    if hasattr(select, 'select'):
                        ready, _, _ = select.select([self.server_process.stdout], [], [], 0.1)
                        if ready:
                            line = self.server_process.stdout.readline()
                            if line:
                                server_output.append(line)
                except:
                    pass
            
            if bot1_done and bot2_done:
                print("✅ Game completed!")
                
                # Collect remaining output from both bots
                bot1_stdout, bot1_stderr = self.bot1_process.communicate()
                bot2_stdout, bot2_stderr = self.bot2_process.communicate()
                
                # Collect remaining server output
                try:
                    server_stdout, server_stderr = self.server_process.communicate(timeout=2)
                    server_output.append(server_stdout if server_stdout else "")
                    server_full_output = "".join(server_output)
                except:
                    server_full_output = "".join(server_output)
                    server_stderr = ""
                
                # Parse results and save logs
                result = self.parse_game_result(bot1_stdout, bot2_stdout, bot1_stderr, bot2_stderr, 
                                                server_full_output, server_stderr)
                return result
            
            # Print progress every 5 seconds
            current_time = time.time()
            if current_time - last_check >= 5:
                elapsed = int(current_time - start_time)
                print(f"   ... still running ({elapsed}s elapsed)")
                last_check = current_time
            
            time.sleep(0.5)
        
        print(f"⏰ Game timeout after {timeout}s")
        return {
            'timeout': True,
            'bot1': self.bot1_name,
            'bot2': self.bot2_name,
            'winner': 'timeout'
        }
    
    def parse_game_result(self, bot1_stdout, bot2_stdout, bot1_stderr, bot2_stderr, server_stdout="", server_stderr=""):
        """Parse game result from bot output and save full logs"""
        # Create logs directory
        logs_dir = self.project_root / "tournament" / "logs"
        logs_dir.mkdir(exist_ok=True)
        
        # Generate unique log filename with timestamp
        timestamp = time.strftime("%Y%m%d_%H%M%S")
        log_file = logs_dir / f"match_{self.bot1_name}_vs_{self.bot2_name}_{timestamp}.txt"
        
        # Save complete logs
        with open(log_file, 'w') as f:
            f.write("="*80 + "\n")
            f.write(f"MATCH: {self.bot1_name} (circle) vs {self.bot2_name} (square)\n")
            f.write(f"Port: {self.port}, Board: {self.board_size}\n")
            f.write(f"Timestamp: {timestamp}\n")
            f.write("="*80 + "\n\n")
            
            f.write("="*80 + "\n")
            f.write("SERVER OUTPUT\n")
            f.write("="*80 + "\n")
            f.write(server_stdout)
            if server_stderr:
                f.write("\n\n--- SERVER STDERR ---\n")
                f.write(server_stderr)
            f.write("\n\n")
            
            f.write("="*80 + "\n")
            f.write(f"BOT1 ({self.bot1_name} - CIRCLE) STDOUT\n")
            f.write("="*80 + "\n")
            f.write(bot1_stdout)
            f.write("\n\n")
            
            f.write("="*80 + "\n")
            f.write(f"BOT1 ({self.bot1_name} - CIRCLE) STDERR\n")
            f.write("="*80 + "\n")
            f.write(bot1_stderr)
            f.write("\n\n")
            
            f.write("="*80 + "\n")
            f.write(f"BOT2 ({self.bot2_name} - SQUARE) STDOUT\n")
            f.write("="*80 + "\n")
            f.write(bot2_stdout)
            f.write("\n\n")
            
            f.write("="*80 + "\n")
            f.write(f"BOT2 ({self.bot2_name} - SQUARE) STDERR\n")
            f.write("="*80 + "\n")
            f.write(bot2_stderr)
            f.write("\n\n")
        
        print(f"📝 Full logs saved to: {log_file}")
        
        result = {
            'bot1': self.bot1_name,
            'bot2': self.bot2_name,
            'bot1_side': 'circle',
            'bot2_side': 'square',
            'log_file': str(log_file),
            'timestamp': timestamp
        }
        
        return result
    
    def cleanup(self):
        """Cleanup all processes"""
        print("\n🧹 Cleaning up processes...")
        
        for name, process in [
            ('bot1', self.bot1_process),
            ('bot2', self.bot2_process),
            ('server', self.server_process)
        ]:
            if process and process.poll() is None:
                print(f"   Terminating {name}...")
                process.terminate()
                try:
                    process.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    print(f"   Force killing {name}...")
                    process.kill()
                    process.wait()
    
    def run(self):
        """Run the complete match"""
        try:
            print("\n" + "="*70)
            print(f"🏆 TOURNAMENT MATCH: {self.bot1_name} vs {self.bot2_name}")
            print(f"   Board: {self.board_size}, Port: {self.port}")
            print("="*70 + "\n")
            
            # Start server
            if not self.start_server():
                return None
            
            # Start bots
            self.bot1_process = self.start_bot(self.bot1_name, 'circle')
            if not self.bot1_process:
                return None
            
            self.bot2_process = self.start_bot(self.bot2_name, 'square')
            if not self.bot2_process:
                return None
            
            print(f"\n🎮 Game started! Watch at http://localhost:{self.port}")
            
            # Wait for completion
            result = self.wait_for_game_completion()
            
            if result:
                print("\n" + "="*70)
                print("📊 MATCH RESULT")
                print("="*70)
                print(f"   {self.bot1_name} (circle) vs {self.bot2_name} (square)")
                print(f"   Winner: {result.get('winner_bot', 'unknown').upper()}")
                print("="*70 + "\n")
                
                # Look for DEPTH markers to validate bot loading
                if '[BOT1 - DEPTH=1]' in result['bot1_stderr']:
                    print(f"✅ {self.bot1_name} correctly loaded with DEPTH=1")
                if '[BOT2 - DEPTH=2]' in result['bot2_stderr']:
                    print(f"✅ {self.bot2_name} correctly loaded with DEPTH=2")
            
            return result
            
        except KeyboardInterrupt:
            print("\n⚠️  Match interrupted by user")
            return None
        
        finally:
            self.cleanup()


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='Run a tournament match between two bots')
    parser.add_argument('bot1', help='First bot name (plays as circle)')
    parser.add_argument('bot2', help='Second bot name (plays as square)')
    parser.add_argument('--board-size', choices=['small', 'medium', 'large'], 
                       default='small', help='Board size')
    parser.add_argument('--port', type=int, default=8080, help='Server port')
    
    args = parser.parse_args()
    
    match = TournamentMatch(args.bot1, args.bot2, args.board_size, args.port)
    result = match.run()
    
    if result:
        sys.exit(0)
    else:
        sys.exit(1)


if __name__ == '__main__':
    main()
