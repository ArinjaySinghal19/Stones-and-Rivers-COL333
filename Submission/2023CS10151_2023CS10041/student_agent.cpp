#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "game_state.h"
#include "minimax.h"
#include "transposition_table.h"
#include <iostream>
#include <deque>
#include <string>
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

Move flip_topmost_piece(const GameState& state, const std::string& side) {
    // Find the topmost piece of the given side and flip it
    if(side=="circle"){
    for (int y = 0; y < state.rows; ++y) {
        for (int x = 0; x < state.cols; ++x) {
            const auto& cell = state.board[y][x];
            if (!cell.empty() && cell.at("owner") == side) {
                if (cell.at("side") == "stone") {
                    // Flip stone to river (default horizontal)
                    return {"flip", {x, y}, {x, y}, {}, "horizontal"};
                } else if (cell.at("side") == "river") {
                    // Flip river orientation
                    return {"rotate", {x,y}, {x,y}, {}, ""};
                }
            }
        }
    }
}
    if(side=="square"){
    for (int y = state.rows-1; y >= 0; --y) {
        for (int x = state.cols-1; x >= 0; --x) {
            const auto& cell = state.board[y][x];
            if (!cell.empty() && cell.at("owner") == side) {
                if (cell.at("side") == "stone") {
                    // Flip stone to river (default horizontal)
                    return {"flip", {x, y}, {x, y}, {}, "horizontal"};
                } else if (cell.at("side") == "river") {
                    // Flip river orientation
                    return {"rotate", {x,y}, {x,y}, {}, ""};
                }
            }
        }
    }
}
    // If no piece found, return a dummy move
    return {"move", {0,0}, {0,0}, {}, ""};
}

// ---- Student Agent ----
class StudentAgent {
public:
    explicit StudentAgent(std::string side) : side(std::move(side)) {
        // Initialize transposition table (64 MB default)
        // Persistent across moves for the entire game
        tt = new TranspositionTable(64);
    }
    
    ~StudentAgent() {
        delete tt;
    }
    
    bool end_time = false;
    int end_piece_row = -1;
    int end_piece_col = -1;
    bool flipped_to_river = false;
    Move choose(const std::vector<std::vector<std::map<std::string, std::string>>>& board, 
                int rows, int cols, const std::vector<int>& score_cols, 
                float current_player_time, float opponent_time) {
        // Start timing
        auto start_time = std::chrono::high_resolution_clock::now();

        // Create game state
        GameState current_state(board, side, rows, cols, score_cols);
        Move selected;

        if (current_player_time < 1) end_time=true;
        if(end_time && flipped_to_river){
            selected = {"rotate", {end_piece_col, end_piece_row}, {end_piece_col, end_piece_row}, {}, ""};
            return selected;
        }

        if(end_time && !flipped_to_river){
            selected = flip_topmost_piece(current_state, side);
            if(selected.action == "flip"){
                flipped_to_river = true;
                end_piece_row = selected.from[1];
                end_piece_col = selected.from[0];
                return selected;
            }
        }



        auto minimax_start = std::chrono::high_resolution_clock::now();
        // Use Minimax with Alpha-Beta Pruning, stalemate avoidance, and Transposition Table
        // Adjust depth based on remaining time
        double cut_off_time = 40.0;
        if(rows==15) cut_off_time = 50.0;
        if(rows==17) cut_off_time = 60.0;
        int MINIMAX_DEPTH = 3;
        if (current_player_time >= cut_off_time) MINIMAX_DEPTH = 4;
        if (current_player_time <= 4.0) MINIMAX_DEPTH = 1;
        selected = run_minimax_with_repetition_check(current_state, MINIMAX_DEPTH, side, recent_board_hashes, tt);
        move_count++;
        auto end_time_point = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> minimax_duration = end_time_point - minimax_start;
        std::cout << "C++ agent returned move in " << minimax_duration.count() << " seconds." << std::endl;
        return selected;
    }

private:
    std::string side;
    std::deque<uint64_t> recent_board_hashes; // Rolling history of recent board states (for stalemate detection)
    TranspositionTable* tt;  // Transposition table for caching positions
    int move_count = 0;
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