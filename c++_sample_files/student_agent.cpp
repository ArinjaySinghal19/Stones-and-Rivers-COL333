#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "game_state.h"
#include "minimax.h"
#include "mcts.h"

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

    Move choose(const std::vector<std::vector<std::map<std::string, std::string>>>& board, 
                int row, int col, const std::vector<int>& score_cols, 
                float current_player_time, float opponent_time) {
        const std::string algorithm = "mcts"; // "mcts" or "minimax"
        int rows = board.size();
        int cols = board[0].size();

        // Create game state
        GameState current_state(board, side, rows, cols, score_cols);
        
        if (algorithm == "minimax") {
            // Use Minimax with Alpha-Beta Pruning (depth 4)
            const int MINIMAX_DEPTH = 4;
            return run_minimax(current_state, MINIMAX_DEPTH);
        } else {
            // Use MCTS (default)
            // Calculate number of iterations based on game state
            // Base iterations: 100, but can be adjusted based on remaining time
            int base_iterations = 100;
            
            // Reduce iterations if we have very little time left
            int max_iterations = base_iterations;
            if (current_player_time < 10.0) {
                max_iterations = base_iterations / 2;  // 50 iterations
            }
            if (current_player_time < 5.0) {
                max_iterations = base_iterations / 4;  // 25 iterations
            }
            if (current_player_time < 2.0) {
                max_iterations = base_iterations / 10; // 10 iterations
            }
            
            // Run MCTS to find the best move
            return run_mcts(current_state, max_iterations);
        }
    }

private:
    std::string side;
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