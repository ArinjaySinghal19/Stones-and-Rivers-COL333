"""
Dynamic Bot Loader for Tournament System

This module provides functionality to dynamically load C++ bots from different directories,
ensuring proper module isolation to allow multiple bots to coexist in the same process.
"""

import importlib.util
import sys
from pathlib import Path
from typing import Optional, Dict, Any


class BotLoader:
    """Handles dynamic loading of tournament bots with module isolation"""
    
    def __init__(self, tournament_bots_dir: str = None):
        """
        Initialize the bot loader.
        
        Args:
            tournament_bots_dir: Path to the tournament_bots directory.
                               If None, will use ../tournament_bots relative to this file.
        """
        if tournament_bots_dir is None:
            # Default to tournament_bots directory at project root
            self.tournament_bots_dir = Path(__file__).parent.parent / "tournament_bots"
        else:
            self.tournament_bots_dir = Path(tournament_bots_dir)
        
        self.loaded_modules: Dict[str, Any] = {}  # Cache loaded modules
        
    def list_available_bots(self):
        """List all available bots in the tournament_bots directory"""
        if not self.tournament_bots_dir.exists():
            return []
        
        bots = []
        for bot_dir in self.tournament_bots_dir.iterdir():
            if bot_dir.is_dir() and (bot_dir / "build").exists():
                # Check if .so file exists
                so_files = list((bot_dir / "build").glob("student_agent_module*.so"))
                if so_files:
                    bots.append(bot_dir.name)
        
        return sorted(bots)
    
    def get_bot_path(self, bot_name: str) -> Path:
        """Get the path to a bot's directory"""
        bot_path = self.tournament_bots_dir / bot_name
        if not bot_path.exists():
            raise FileNotFoundError(f"Bot directory not found: {bot_path}")
        return bot_path
    
    def get_bot_module_path(self, bot_name: str) -> Path:
        """Get the path to a bot's compiled .so module"""
        bot_dir = self.get_bot_path(bot_name)
        build_dir = bot_dir / "build"
        
        if not build_dir.exists():
            raise FileNotFoundError(f"Build directory not found for bot '{bot_name}': {build_dir}")
        
        # Find the .so file - now using bot-specific module name (bot1_module.*.so, bot2_module.*.so, etc.)
        module_pattern = f"{bot_name}_module*.so"
        so_files = list(build_dir.glob(module_pattern))
        
        if not so_files:
            raise FileNotFoundError(
                f"No compiled module found for bot '{bot_name}' in {build_dir}\n"
                f"Looking for pattern: {module_pattern}\n"
                f"Please compile the bot first."
            )
        
        if len(so_files) > 1:
            print(f"Warning: Multiple .so files found for bot '{bot_name}', using {so_files[0]}")
        
        return so_files[0]
    
    def load_bot_module(self, bot_name: str, player: str = None):
        """
        Dynamically load a bot's compiled module with isolation.
        
        Args:
            bot_name: Name of the bot (directory name in tournament_bots/)
            player: Optional player identifier for additional namespace isolation
                   (useful when loading the same bot for both players)
        
        Returns:
            The loaded module object
        
        Raises:
            FileNotFoundError: If bot directory or .so file not found
            ImportError: If module loading fails
        """
        # Create unique cache key to prevent conflicts
        if player:
            cache_key = f"tournament_bot_{bot_name}_{player}"
        else:
            cache_key = f"tournament_bot_{bot_name}"
        
        # Check cache
        if cache_key in self.loaded_modules:
            print(f"[BotLoader] Using cached module for {cache_key}")
            return self.loaded_modules[cache_key]
        
        # Get path to .so file
        so_path = self.get_bot_module_path(bot_name)
        
        print(f"[BotLoader] Loading bot '{bot_name}' from {so_path}")
        print(f"[BotLoader] Cache key: {cache_key}")
        
        # Each bot is now compiled with a unique module name (bot1_module, bot2_module, etc.)
        # This solves the shared library caching issue - each .so has a different symbol namespace
        module_name = f"{bot_name}_module"
        
        # Temporarily add bot's build directory to sys.path
        build_dir = str(so_path.parent)
        if build_dir not in sys.path:
            sys.path.insert(0, build_dir)
            added_to_path = True
        else:
            added_to_path = False
        
        try:
            # Import using the bot-specific module name
            print(f"[BotLoader] Importing module: {module_name}")
            module = importlib.import_module(module_name)
            
            # Store it under our cache key
            self.loaded_modules[cache_key] = module
            
            print(f"[BotLoader] Successfully loaded and cached as {cache_key}")
            
            return self.loaded_modules[cache_key]
            
        except Exception as e:
            # Clean up on failure
            if cache_key in self.loaded_modules:
                del self.loaded_modules[cache_key]
            if cache_key in sys.modules:
                del sys.modules[cache_key]
            raise ImportError(f"Failed to load module {cache_key}: {e}")
        finally:
            # Remove build directory from sys.path
            if added_to_path and build_dir in sys.path:
                sys.path.remove(build_dir)
    
    def create_agent(self, bot_name: str, player: str):
        """
        Create an agent instance from a bot module.
        
        Args:
            bot_name: Name of the bot
            player: Player identifier ("circle" or "square")
        
        Returns:
            An instance of the bot's StudentAgent class
        """
        module = self.load_bot_module(bot_name, player)
        
        # Each bot now has a unique class name (AgentBot1, AgentBot2, etc.)
        # Pattern: Agent{BotName} where BotName has first letter capitalized
        agent_class_name = f"Agent{bot_name.capitalize()}"
        
        if not hasattr(module, agent_class_name):
            raise AttributeError(
                f"Module for bot '{bot_name}' does not have '{agent_class_name}' class. "
                f"Available attributes: {dir(module)}"
            )
        
        # Create agent instance
        agent_class = getattr(module, agent_class_name)
        agent = agent_class(player)
        print(f"[BotLoader] Created C++ agent for bot '{bot_name}' playing as '{player}'")
        
        # Return the agent directly (will be wrapped by TournamentBotWrapper in load_tournament_bot)
        return agent


# Singleton instance for easy access
_default_loader = None

def get_default_loader() -> BotLoader:
    """Get the default bot loader singleton"""
    global _default_loader
    if _default_loader is None:
        _default_loader = BotLoader()
    return _default_loader


def load_tournament_bot(bot_name: str, player: str):
    """
    Convenience function to load a tournament bot using the default loader.
    
    Args:
        bot_name: Name of the bot (directory name in tournament_bots/)
        player: Player identifier ("circle" or "square")
    
    Returns:
        An instance of the bot's StudentAgent class
    """
    loader = get_default_loader()
    return loader.create_agent(bot_name, player)


if __name__ == "__main__":
    # Test the bot loader
    print("=== Testing Bot Loader ===\n")
    
    loader = BotLoader()
    
    print("Available bots:")
    bots = loader.list_available_bots()
    for bot in bots:
        print(f"  - {bot}")
    
    if bots:
        print(f"\nTesting load of '{bots[0]}'...")
        try:
            agent = loader.create_agent(bots[0], "circle")
            print(f"✓ Successfully created agent: {agent}")
        except Exception as e:
            print(f"✗ Failed to load bot: {e}")
    else:
        print("\nNo bots found. Please compile some bots first.")
