#include "transposition_table.h"
#include "game_state.h"
#include <iostream>
#include <chrono>

// Simple test to verify incremental hashing is working
int main() {
    // Create a simple test board
    std::vector<std::vector<std::map<std::string, std::string>>> test_board(16,
        std::vector<std::map<std::string, std::string>>(12));

    // Add a few pieces
    test_board[0][0] = {{"owner", "circle"}, {"side", "stone"}};
    test_board[1][1] = {{"owner", "square"}, {"side", "river"}, {"orientation", "horizontal"}};

    GameState state(test_board, "circle", 16, 12, {4, 5, 6, 7});
    TranspositionTable tt(1);  // 1MB table

    // Test 1: Verify incremental hash initialization
    std::cout << "=== Test 1: Hash Initialization ===\n";
    state.initialize_hash(&tt);
    uint64_t initial_hash = state.get_hash();
    uint64_t computed_hash = tt.compute_hash(state);

    std::cout << "Incremental hash: " << initial_hash << "\n";
    std::cout << "Computed hash:    " << computed_hash << "\n";

    if (initial_hash == computed_hash) {
        std::cout << "✅ Hash initialization correct!\n\n";
    } else {
        std::cout << "❌ Hash initialization FAILED!\n\n";
        return 1;
    }

    // Test 2: Verify incremental hash after make_move
    std::cout << "=== Test 2: Incremental Hash After Move ===\n";
    Move test_move = {"move", {0, 0}, {2, 2}, {}, ""};
    GameState::UndoInfo undo = state.make_move(test_move);

    uint64_t incremental_after_move = state.get_hash();
    uint64_t computed_after_move = tt.compute_hash(state);

    std::cout << "Incremental hash after move: " << incremental_after_move << "\n";
    std::cout << "Computed hash after move:    " << computed_after_move << "\n";

    if (incremental_after_move == computed_after_move) {
        std::cout << "✅ Incremental update correct!\n\n";
    } else {
        std::cout << "❌ Incremental update FAILED!\n\n";
        return 1;
    }

    // Test 3: Verify hash restoration after undo_move
    std::cout << "=== Test 3: Hash Restoration After Undo ===\n";
    state.undo_move(test_move, undo);

    uint64_t restored_hash = state.get_hash();
    uint64_t computed_after_undo = tt.compute_hash(state);

    std::cout << "Restored hash:  " << restored_hash << "\n";
    std::cout << "Initial hash:   " << initial_hash << "\n";
    std::cout << "Computed hash:  " << computed_after_undo << "\n";

    if (restored_hash == initial_hash && restored_hash == computed_after_undo) {
        std::cout << "✅ Hash restoration correct!\n\n";
    } else {
        std::cout << "❌ Hash restoration FAILED!\n\n";
        return 1;
    }

    // Test 4: Performance comparison
    std::cout << "=== Test 4: Performance Comparison ===\n";
    const int NUM_ITERATIONS = 100000;

    // Time incremental hash (just reading the value)
    auto start = std::chrono::high_resolution_clock::now();
    volatile uint64_t dummy = 0;
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        dummy += state.get_hash();
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto incremental_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    // Time computed hash (recomputing from scratch)
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        dummy += tt.compute_hash(state);
    }
    end = std::chrono::high_resolution_clock::now();
    auto computed_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "Incremental hash (read): " << incremental_time << " μs for " << NUM_ITERATIONS << " iterations\n";
    std::cout << "Computed hash (recompute): " << computed_time << " μs for " << NUM_ITERATIONS << " iterations\n";
    std::cout << "Speedup: " << (double)computed_time / incremental_time << "x faster\n\n";

    std::cout << "=== All Tests Passed! ===\n";
    return 0;
}
