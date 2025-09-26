#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <vector>
#include <map>
#include <string>
#include <set>

// ---- Move struct ----
struct Move {
    std::string action;
    std::vector<int> from;
    std::vector<int> to;
    std::vector<int> pushed_to;
    std::string orientation;
};

// ---- Valid Targets struct ----
struct ValidTargets {
    std::set<std::pair<int,int>> moves;
    std::vector<std::pair<std::pair<int,int>, std::pair<int,int>>> pushes; // ((target_x,target_y), (pushed_to_x,pushed_to_y))
};

// ---- Game State representation ----
struct GameState {
    std::vector<std::vector<std::map<std::string, std::string>>> board;
    std::string current_player;
    int rows, cols;
    std::vector<int> score_cols;
    
    GameState(const std::vector<std::vector<std::map<std::string, std::string>>>& b, 
              const std::string& player, int r, int c, const std::vector<int>& sc)
        : board(b), current_player(player), rows(r), cols(c), score_cols(sc) {}
    
    GameState copy() const;
    void apply_move(const Move& move);
    std::vector<Move> get_legal_moves() const;
    bool is_terminal() const;
};

// ---- Helper functions ----
int top_score_row();
int bottom_score_row(int rows);
std::string get_opponent(const std::string& player);
bool in_bounds(int x, int y, int rows, int cols);
bool is_opponent_score_cell(int x, int y, const std::string& player, int rows, int cols, const std::vector<int>& score_cols);
bool is_my_score_cell(int x, int y, const std::string& player, int rows, int cols, const std::vector<int>& score_cols);
bool check_win_state(const std::vector<std::vector<std::map<std::string, std::string>>>& board, 
                     int rows, int cols, const std::vector<int>& score_cols);

std::vector<std::pair<int,int>> get_river_flow_destinations(
    const std::vector<std::vector<std::map<std::string, std::string>>>& board,
    int rx, int ry, int sx, int sy, const std::string& player,
    int rows, int cols, const std::vector<int>& score_cols,
    bool river_push = false);

ValidTargets compute_valid_targets(
    const std::vector<std::vector<std::map<std::string, std::string>>>& board,
    int sx, int sy, const std::string& player,
    int rows, int cols, const std::vector<int>& score_cols);

#endif // GAME_STATE_H