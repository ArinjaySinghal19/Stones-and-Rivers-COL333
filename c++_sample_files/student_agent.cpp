#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "game_state.h"
#include "minimax.h"
#include "heuristics.h"
#include <iostream>
#include <deque>
#include <string>
#include <random>
#include <algorithm>
#include <chrono>


namespace py = pybind11;

/*
=========================================================
 STUDENT AGENT FOR STONES & RIVERS GAME
---------------------------------------------------------
 The Python game engine passes the BOARD state into C++.
 Each board cell is represented as a dictionary in Python:

    {
        "owner": "circle" | "square",          // which player owns this piece
        "side": "stone" | "river",             // piece type
        "orientation": "horizontal" | "vertical"  // only relevant if side == "river"
    }

 In C++ with pybind11, this becomes:

    std::vector<std::vector<std::map<std::string, std::string>>>

 Meaning:
   - board[y][x] gives the cell at (x, y).
   - board[y][x].empty() → true if the cell is empty (no piece).
   - board[y][x].at("owner") → "circle" or "square".
   - board[y][x].at("side") → "stone" or "river".
   - board[y][x].at("orientation") → "horizontal" or "vertical".

=========================================================
*/

// ---- Student Agent ----
class StudentAgent {
public:
    explicit StudentAgent(std::string side) : side(std::move(side)) {}
    int num_moves = 0;
    Move choose(const std::vector<std::vector<std::map<std::string, std::string>>>& board, 
                int row, int col, const std::vector<int>& score_cols, 
                float current_player_time, float opponent_time) {
        // Start timing
        auto start_time = std::chrono::high_resolution_clock::now();
        
        int rows = board.size();
        int cols = board[0].size();

        // Create game state
        GameState current_state(board, side, rows, cols, score_cols);
        Move selected;
        // Use Minimax with Alpha-Beta Pruning and repetition checking
        const int MINIMAX_DEPTH = 2;
        selected = run_minimax_with_repetition_check(current_state, MINIMAX_DEPTH, side, recent_keys, current_player_time);
        
        
        // Calculate and display elapsed time
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        double elapsed_seconds = duration.count();
        
        return selected;
    }

private:
    std::string side;
    std::deque<std::string> recent_keys; // Rolling history of recent position keys
};

// ---- PyBind11 bindings ----
PYBIND11_MODULE(student_agent_module, m) {
    py::class_<Move>(m, "Move")
        .def_readonly("action", &Move::action)
        .def_readonly("from_pos", &Move::from)
        .def_readonly("to_pos", &Move::to)
        .def_readonly("pushed_to", &Move::pushed_to)
        .def_readonly("orientation", &Move::orientation);

    py::class_<StudentAgent>(m, "StudentAgent")
        .def(py::init<std::string>())
        .def("choose", &StudentAgent::choose);
}