#ifndef MCTS_H
#define MCTS_H

#include "game_state.h"
#include <memory>
#include <vector>

// ---- MCTS Node ----
class MCTSNode {
public:
    GameState state;
    MCTSNode* parent;
    std::vector<std::unique_ptr<MCTSNode>> children;
    std::vector<Move> untried_moves;
    int visits;
    double wins;
    Move move_from_parent;
    
    MCTSNode(const GameState& s, MCTSNode* p = nullptr, const Move& m = Move{})
        : state(s), parent(p), visits(0), wins(0.0), move_from_parent(m) {
        untried_moves = state.get_legal_moves();
    }
    
    MCTSNode* select_child();
    MCTSNode* expand();
    double simulate();
    void backpropagate(double result);
    bool is_fully_expanded() const { return untried_moves.empty(); }
    double ucb1_value(double exploration_param = 1.41) const;
    Move get_best_move() const;
};

// ---- MCTS Algorithm ----
Move run_mcts(const GameState& initial_state, int max_iterations);

#endif // MCTS_H