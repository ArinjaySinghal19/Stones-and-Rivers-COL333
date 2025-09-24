#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "game_state.h"
#include "minimax.h"
#include "mcts.h"
#include "heuristics.h"
#include <iostream>
#include <deque>
#include <string>

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

        Heuristics heuristics;
        heuristics.debug_heuristic(current_state, side);
        std::cout << "hello" << std::endl;
        
        Move selected;
        if (algorithm == "minimax") {
            // Use Minimax with Alpha-Beta Pruning (depth 3)
            const int MINIMAX_DEPTH = 2;
            selected = run_minimax(current_state, MINIMAX_DEPTH);
        } else {
            // Use MCTS (default)
            int base_iterations = 200;
            int max_iterations = base_iterations;
            if (current_player_time < 10.0) max_iterations = base_iterations / 2;
            if (current_player_time < 5.0)  max_iterations = base_iterations / 4;
            if (current_player_time < 2.0)  max_iterations = base_iterations / 10;
            selected = run_mcts(current_state, max_iterations);
        }

        // If the proposed move would repeat a recent state, try to pick a non-repeating alternative
        if (would_repeat_after(current_state, selected)) {
            auto legal = current_state.get_legal_moves();
            bool replaced = false;
            for (const auto& m : legal) {
                if (!would_repeat_after(current_state, m)) {
                    selected = m;
                    replaced = true;
                    break;
                }
            }
            // if all moves repeat, keep the original 'selected'
            (void)replaced;
        }

        // Record the resulting state key into rolling history (max 5)
        record_resulting_key(current_state, selected);
        return selected;
    }

private:
    std::string side;
    std::deque<std::string> recent_keys; // last 5 state keys

    static std::string make_state_key(const std::vector<std::vector<std::map<std::string, std::string>>>& board,
                                      const std::string& player_to_move,
                                      int rows, int cols) {
        std::string key;
        // Rough reserve: player+sep + per-cell ~5 chars + row seps
        key.reserve(player_to_move.size() + 1 + rows * cols * 5 + rows + 1);
        key += player_to_move;
        key.push_back('#');
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < cols; ++x) {
                const auto& cell = board[y][x];
                if (cell.empty()) {
                    key.push_back('.');
                } else {
                    char owner = (cell.at("owner") == "circle") ? 'c' : 's';
                    if (cell.at("side") == "river") {
                        char ori = (cell.at("orientation") == "horizontal") ? 'h' : 'v';
                        key.push_back(owner);
                        key.push_back('r');
                        key.push_back(ori);
                    } else {
                        key.push_back(owner);
                        key.push_back('t');
                        key.push_back('-');
                    }
                }
                key.push_back('|');
            }
            key.push_back('/');
        }
        return key;
    }

    bool would_repeat_after(const GameState& state, const Move& move) const {
        GameState sim = state.copy();
        sim.apply_move(move);
        std::string key = make_state_key(sim.board, sim.current_player, sim.rows, sim.cols);
        for (const auto& k : recent_keys) {
            if (k == key) return true;
        }
        return false;
    }

    void record_resulting_key(const GameState& state, const Move& move) {
        GameState sim = state.copy();
        sim.apply_move(move);
        std::string key = make_state_key(sim.board, sim.current_player, sim.rows, sim.cols);
        recent_keys.push_back(key);
        while (recent_keys.size() > 5) recent_keys.pop_front();
    }
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