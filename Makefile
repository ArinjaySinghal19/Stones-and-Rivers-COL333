# Stones and Rivers C++ Agent Makefile
# Replaces compile_cpp.sh, quick_rebuild.sh, and run_game.sh scripts

.PHONY: all setup build rebuild clean distclean run test help

# Variables
PROJECT_DIR := $(shell pwd)
CPP_DIR := $(PROJECT_DIR)/c++_sample_files
CLIENT_DIR := $(PROJECT_DIR)/client_server
BUILD_DIR := $(CPP_DIR)/build
VENV_DIR := $(PROJECT_DIR)/.venv
PYTHON := $(VENV_DIR)/bin/python
PIP := $(VENV_DIR)/bin/pip

# Default target
all: build

# Help target
help:
	@echo "🎮 Stones and Rivers C++ Agent Makefile 🎮"
	@echo "=============================================="
	@echo ""
	@echo "Available targets:"
	@echo "  all            - Build the complete project (default)"
	@echo "  setup          - Set up Python virtual environment and dependencies"
	@echo "  build          - Complete build: setup + compile + deploy"
	@echo "  rebuild        - Quick rebuild after C++ changes"
	@echo "  run            - Run the game (AI vs AI, no GUI)"
	@echo "  run-gui        - Run the game with GUI (AI vs AI)"
	@echo "  run-gui-human  - Run the game with GUI (Human vs AI)"
	@echo "  test           - Test the compiled module"
	@echo "  clean          - Remove build artifacts"
	@echo "  distclean      - Remove everything (build + venv + client files)"
	@echo "  help           - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make                    # Build everything"
	@echo "  make rebuild            # Quick recompile after changes"
	@echo "  make run                # Run game without GUI (AI vs AI)"
	@echo "  make run-gui            # Run game with GUI (AI vs AI)"
	@echo "  make run-gui-human      # Run game with GUI (Human vs AI)"
	@echo "  make clean              # Clean build artifacts"
	@echo "  make distclean          # Complete cleanup"

# Set up Python virtual environment and dependencies
setup: $(VENV_DIR)/bin/python
	@echo "🐍 Setting up Python environment..."
	@$(PIP) install --quiet --upgrade pip
	@$(PIP) install --quiet pygame numpy scipy pybind11
	@echo "✅ Python environment ready"

$(VENV_DIR)/bin/python:
	@echo "📦 Creating virtual environment..."
	@python3 -m venv $(VENV_DIR)

# Configure and build C++ module
$(BUILD_DIR)/Makefile: setup
	@echo "⚙️ Configuring CMake..."
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && \
		PYBIND_DIR=$$($(PYTHON) -c "import pybind11; print(pybind11.get_cmake_dir())") && \
		cmake .. \
			-Dpybind11_DIR="$$PYBIND_DIR" \
			-DCMAKE_C_COMPILER=gcc \
			-DCMAKE_CXX_COMPILER=g++ \
			-DCMAKE_BUILD_TYPE=Release
	@echo "✅ CMake configuration complete"

# Compile C++ module
$(BUILD_DIR)/student_agent_module.cpython-*.so: $(BUILD_DIR)/Makefile $(CPP_DIR)/student_agent.cpp
	@echo "🔨 Compiling C++ code..."
	@cd $(BUILD_DIR) && make -j$$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
	@echo "✅ C++ compilation successful"

# Deploy to client directory
$(CLIENT_DIR)/build: $(BUILD_DIR)/student_agent_module.cpython-*.so
	@echo "📦 Setting up client directory..."
	@cp -r $(BUILD_DIR) $(CLIENT_DIR)/
	@echo "✅ Client directory setup complete"

# Complete build process
build: $(CLIENT_DIR)/build
	@echo ""
	@echo "🎉 SUCCESS! C++ agent compilation and setup complete!"
	@echo "================================================"
	@echo ""
	@echo "📋 Quick Usage Guide:"
	@echo "   • Test without GUI: make run"
	@echo "   • Test with GUI:    make run-gui"
	@echo "   • Quick rebuild:    make rebuild"
	@echo "   • Clean build:      make clean"
	@echo ""
	@echo "✨ Happy coding!"

# Quick rebuild (only recompile, don't reconfigure)
rebuild:
	@if [ ! -d "$(BUILD_DIR)" ]; then \
		echo "❌ Build directory not found. Run 'make build' first for initial setup."; \
		exit 1; \
	fi
	@echo "🔨 Quick C++ Rebuild"
	@echo "==================="
	@echo "🔧 Rebuilding C++ module..."
	@cd $(BUILD_DIR) && make -j$$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
	@echo "📦 Updating client directory..."
	@cp -r $(BUILD_DIR) $(CLIENT_DIR)/
	@echo "✅ Quick rebuild complete!"

# Test the compiled module
test: $(CLIENT_DIR)/build
	@echo "🧪 Testing the setup..."
	@echo "   Testing C++ module import..."
	@cd $(CLIENT_DIR) && $(PYTHON) -c "import build.student_agent_module as student_agent; print('✅ C++ module imported successfully!')"
	@echo "   Testing agent creation..."
	@cd $(CPP_DIR) && $(PYTHON) -c "from student_agent_cpp import StudentAgent; agent = StudentAgent('circle'); print('✅ C++ agent created successfully!')"
	@echo "✅ All tests passed!"

# Run the game without GUI
run: test
	@echo "🎮 Starting Stones and Rivers Game (no GUI)..."
	@echo "==============================================="
	@echo "Mode: aivai | Circle: student_cpp | Square: student_cpp"
	@cd $(CLIENT_DIR) && PYTHONPATH=$(CPP_DIR):$$PYTHONPATH $(PYTHON) gameEngine.py --mode aivai --circle student_cpp --square student_cpp --nogui

# Run the game with GUI
run-gui: test
	@echo "🎮 Starting Stones and Rivers Game (with GUI)..."
	@echo "==============================================="
	@echo "Mode: aivai | Circle: student_cpp | Square: student_cpp"
	@cd $(CLIENT_DIR) && PYTHONPATH=$(CPP_DIR):$$PYTHONPATH $(PYTHON) gameEngine.py --mode aivai --circle student_cpp --square student_cpp 

# Run the game with GUI (Human vs AI)
run-gui-human: test
	@echo "🎮 Starting Stones and Rivers Game (with GUI - Human vs AI)..."
	@echo "============================================================="
	@echo "Mode: hvai | Circle: human | Square: student_cpp"
	@cd $(CLIENT_DIR) && PYTHONPATH=$(CPP_DIR):$$PYTHONPATH $(PYTHON) gameEngine.py --mode hvai --circle student_cpp --time 10 

run-hvh: test
	@echo "🎮 Starting Stones and Rivers Game (with GUI - Human vs Human)..."
	@echo "============================================================="
	@echo "Mode: hvh | Circle: human | Square: human"
	@cd $(CLIENT_DIR) && PYTHONPATH=$(CPP_DIR):$$PYTHONPATH $(PYTHON) gameEngine.py --mode hvh --time 10

# Clean build artifacts but keep virtual environment
clean:
	@echo "🧹 Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@rm -rf $(CLIENT_DIR)/build
	@echo "✅ Build artifacts cleaned"

# Complete cleanup - remove everything
distclean: clean
	@echo "🧹 Performing complete cleanup..."
	@rm -rf $(VENV_DIR)
	@rm -rf $(CLIENT_DIR)/__pycache__
	@echo "✅ Complete cleanup finished"
	@echo "Run 'make build' to start fresh"

# Check if we're in the right directory
check-project:
	@if [ ! -f "$(PROJECT_DIR)/README.md" ] || [ ! -d "$(CPP_DIR)" ]; then \
		echo "❌ Error: Please run make from the project root directory"; \
		echo "   Expected to find README.md and c++_sample_files/ directory"; \
		exit 1; \
	fi

# Add dependency checking to main targets
setup: check-project
build: check-project
rebuild: check-project