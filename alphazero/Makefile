# Makefile for AlphaZero Training on Stones and Rivers

# Use virtual environment Python if available, otherwise fall back to system python
PYTHON := $(shell if [ -f ../.venv/bin/python ]; then echo "../.venv/bin/python"; else echo "python"; fi)

.PHONY: help install bootstrap-small bootstrap-medium quick-bootstrap train-small train-medium train-large transfer-medium transfer-large test-agent clean

# Default target
help:
	@echo "AlphaZero Training for Stones and Rivers"
	@echo ""
	@echo "Available targets:"
	@echo "  install           - Install required dependencies"
	@echo "  bootstrap-small   - Bootstrap train vs heuristic (RECOMMENDED!)"
	@echo "  quick-bootstrap   - Quick test bootstrap (3+7 iterations)"
	@echo "  quick-bootstrap-gui - Quick bootstrap with GUI visualization"
	@echo "  train-small       - Train on small board (pure self-play)"
	@echo "  train-medium      - Train on medium board"
	@echo "  train-large       - Train on large board"
	@echo "  transfer-medium   - Transfer learning small->medium"
	@echo "  transfer-large    - Transfer learning medium->large"
	@echo "  test-agent        - Test the trained agent"
	@echo "  clean             - Remove checkpoints and cache"
	@echo ""
	@echo "RECOMMENDED workflow (bootstrap training):"
	@echo "  1. make install"
	@echo "  2. make bootstrap-small    (trains vs your heuristic bot first)"
	@echo "  3. make test-agent"
	@echo ""
	@echo "Quick test:"
	@echo "  1. make install"
	@echo "  2. make quick-bootstrap    (fast 10-iteration test)"
	@echo "  3. make test-agent"

# Install dependencies
install:
	$(PYTHON) -m pip install -r requirements.txt

# Bootstrap training (RECOMMENDED - trains vs heuristic first)
bootstrap-small:
	@echo "Starting bootstrap training (vs heuristic bot)..."
	@echo "Phase 1: 40 iterations vs heuristic (100 games each)"
	@echo "Phase 2: 60 iterations of self-play (500 games each)"
	@echo "Total: 100 iterations, ~10-12 hours on GPU"
	@echo ""
	$(PYTHON) train_bootstrap.py \
		--board-size small \
		--num-bootstrap-iterations 40 \
		--num-bootstrap-games 100 \
		--num-selfplay-iterations 60 \
		--num-selfplay-games 500 \
		--num-parallel-games 50 \
		--num-searches 600 \
		--num-resblocks 9 \
		--num-hidden 128 \
		--heuristic-agent student_cpp \
		--output-dir checkpoints

bootstrap-medium:
	@echo "Finding latest small board checkpoint..."
	$(eval LATEST_SMALL := $(shell ls -t checkpoints/model_*_small.pt 2>/dev/null | head -n1))
	@if [ -z "$(LATEST_SMALL)" ]; then \
		echo "Error: No small board checkpoint found."; \
		exit 1; \
	fi
	@echo "Transfer learning from $(LATEST_SMALL)"
	$(PYTHON) train_bootstrap.py \
		--board-size medium \
		--num-bootstrap-iterations 15 \
		--num-bootstrap-games 80 \
		--num-selfplay-iterations 60 \
		--num-selfplay-games 400 \
		--num-parallel-games 40 \
		--transfer-from $(LATEST_SMALL) \
		--output-dir checkpoints

# Quick bootstrap (for testing - 3 bootstrap + 7 selfplay)
quick-bootstrap:
	$(PYTHON) train_bootstrap.py \
		--board-size small \
		--num-bootstrap-iterations 3 \
		--num-bootstrap-games 20 \
		--num-selfplay-iterations 7 \
		--num-selfplay-games 50 \
		--num-parallel-games 10 \
		--num-searches 200 \
		--num-resblocks 4 \
		--num-hidden 64 \
		--output-dir checkpoints

# Quick bootstrap with GUI (slower but you can see the games!)
quick-bootstrap-gui:
	$(PYTHON) train_bootstrap.py \
		--board-size small \
		--num-bootstrap-iterations 3 \
		--num-bootstrap-games 5 \
		--num-selfplay-iterations 2 \
		--num-selfplay-games 10 \
		--num-parallel-games 5 \
		--num-searches 100 \
		--num-resblocks 4 \
		--num-hidden 64 \
		--gui \
		--output-dir checkpoints

# Training targets
train-small:
	$(PYTHON) train.py \
		--board-size small \
		--num-iterations 50 \
		--num-selfplay-iterations 500 \
		--num-parallel-games 50 \
		--num-searches 600 \
		--num-resblocks 9 \
		--num-hidden 128 \
		--output-dir checkpoints

train-medium:
	$(PYTHON) train.py \
		--board-size medium \
		--num-iterations 50 \
		--num-selfplay-iterations 400 \
		--num-parallel-games 40 \
		--num-searches 600 \
		--num-resblocks 9 \
		--num-hidden 128 \
		--output-dir checkpoints

train-large:
	$(PYTHON) train.py \
		--board-size large \
		--num-iterations 50 \
		--num-selfplay-iterations 300 \
		--num-parallel-games 30 \
		--num-searches 600 \
		--num-resblocks 9 \
		--num-hidden 128 \
		--output-dir checkpoints

# Transfer learning targets
transfer-medium:
	@echo "Finding latest small board checkpoint..."
	$(eval LATEST_SMALL := $(shell ls -t checkpoints/model_*_small.pt 2>/dev/null | head -n1))
	@if [ -z "$(LATEST_SMALL)" ]; then \
		echo "Error: No small board checkpoint found. Run 'make train-small' first."; \
		exit 1; \
	fi
	@echo "Using checkpoint: $(LATEST_SMALL)"
	$(PYTHON) train.py \
		--board-size medium \
		--transfer-from $(LATEST_SMALL) \
		--num-iterations 30 \
		--num-selfplay-iterations 300 \
		--num-parallel-games 40 \
		--num-searches 600 \
		--output-dir checkpoints

transfer-large:
	@echo "Finding latest medium board checkpoint..."
	$(eval LATEST_MEDIUM := $(shell ls -t checkpoints/model_*_medium.pt 2>/dev/null | head -n1))
	@if [ -z "$(LATEST_MEDIUM)" ]; then \
		echo "Error: No medium board checkpoint found. Run 'make transfer-medium' first."; \
		exit 1; \
	fi
	@echo "Using checkpoint: $(LATEST_MEDIUM)"
	$(PYTHON) train.py \
		--board-size large \
		--transfer-from $(LATEST_MEDIUM) \
		--num-iterations 30 \
		--num-selfplay-iterations 200 \
		--num-parallel-games 30 \
		--num-searches 600 \
		--output-dir checkpoints

# Quick training (for testing)
quick-train-small:
	$(PYTHON) train.py \
		--board-size small \
		--num-iterations 3 \
		--num-selfplay-iterations 50 \
		--num-parallel-games 10 \
		--num-searches 100 \
		--num-resblocks 4 \
		--num-hidden 64 \
		--output-dir checkpoints

# Test agent against random
test-agent:
	cd .. && $(PYTHON) client_server/gameEngine.py \
		--mode aivai \
		--circle student \
		--square random \
		--board-size small \
		--nogui

# Clean up
clean:
	rm -rf checkpoints/*.pt
	rm -rf __pycache__
	find . -type d -name "__pycache__" -exec rm -rf {} +
	find . -type f -name "*.pyc" -delete

# Resume training
resume-small:
	$(eval LATEST := $(shell ls -t checkpoints/model_*_small.pt 2>/dev/null | head -n1))
	@if [ -z "$(LATEST)" ]; then \
		echo "No checkpoint found, starting from scratch"; \
		make train-small; \
	else \
		echo "Resuming from $(LATEST)"; \
		$(PYTHON) train.py \
			--board-size small \
			--resume-from $(LATEST) \
			--num-iterations 50 \
			--output-dir checkpoints; \
	fi
