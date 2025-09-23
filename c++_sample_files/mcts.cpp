#include "mcts.h"
#include <random>
#include <cmath>
#include <algorithm>
#include <limits>

// ---- MCTS Implementation ----
double MCTSNode::ucb1_value(double exploration_param) const {
    if (visits == 0) return std::numeric_limits<double>::infinity();
    
    double avg_reward = wins / visits;
    double exploration = exploration_param * sqrt(log(parent->visits) / visits);
    return avg_reward + exploration;
}

MCTSNode* MCTSNode::select_child() {
    MCTSNode* best_child = nullptr;
    double best_value = -std::numeric_limits<double>::infinity();
    
    for (auto& child : children) {
        double value = child->ucb1_value();
        if (value > best_value) {
            best_value = value;
            best_child = child.get();
        }
    }
    
    return best_child;
}

MCTSNode* MCTSNode::expand() {
    if (untried_moves.empty()) return nullptr;
    
    // Select a random untried move
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, untried_moves.size() - 1);
    int idx = dist(gen);
    
    Move move = untried_moves[idx];
    untried_moves.erase(untried_moves.begin() + idx);
    
    // Create new state by applying the move
    GameState new_state = state.copy();
    new_state.apply_move(move);
    
    // Create new child node
    auto child = std::make_unique<MCTSNode>(new_state, this, move);
    MCTSNode* child_ptr = child.get();
    children.push_back(std::move(child));
    
    return child_ptr;
}

double MCTSNode::simulate() {
    GameState sim_state = state.copy();
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Limit simulation depth to avoid infinite games
    int max_depth = 50;
    int depth = 0;
    
    std::string original_player = state.current_player;
    
    while (!sim_state.is_terminal() && depth < max_depth) {
        std::vector<Move> moves = sim_state.get_legal_moves();
        if (moves.empty()) break;
        
        // Select random move
        std::uniform_int_distribution<> dist(0, moves.size() - 1);
        Move move = moves[dist(gen)];
        
        sim_state.apply_move(move);
        depth++;
    }
    
    // Evaluate the final state from the perspective of the original player
    return sim_state.evaluate(original_player);
}

void MCTSNode::backpropagate(double result) {
    visits++;
    wins += result;
    
    if (parent != nullptr) {
        // Invert result for parent (opponent's perspective)
        parent->backpropagate(1.0 - result);
    }
}

Move MCTSNode::get_best_move() const {
    if (children.empty()) {
        // Return a random legal move if no children
        std::vector<Move> legal_moves = state.get_legal_moves();
        if (!legal_moves.empty()) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dist(0, legal_moves.size() - 1);
            return legal_moves[dist(gen)];
        }
        return {"move", {0,0}, {0,0}, {}, ""};
    }
    
    // Select child with highest visit count (most promising)
    MCTSNode* best_child = nullptr;
    int max_visits = -1;
    
    for (const auto& child : children) {
        if (child->visits > max_visits) {
            max_visits = child->visits;
            best_child = child.get();
        }
    }
    
    return best_child ? best_child->move_from_parent : Move{"move", {0,0}, {0,0}, {}, ""};
}

// ---- MCTS Algorithm ----
Move run_mcts(const GameState& initial_state, int max_iterations) {
    auto root = std::make_unique<MCTSNode>(initial_state);
    
    for (int iteration = 0; iteration < max_iterations; iteration++) {
        // MCTS iteration
        MCTSNode* node = root.get();
        
        // 1. Selection - traverse down the tree using UCB1
        while (node->is_fully_expanded() && !node->children.empty()) {
            node = node->select_child();
        }
        
        // 2. Expansion - add a new child node
        if (!node->state.is_terminal() && !node->is_fully_expanded()) {
            node = node->expand();
        }
        
        // 3. Simulation - play out from this node
        double result = 0.5; // default draw
        if (node != nullptr) {
            result = node->simulate();
        }
        
        // 4. Backpropagation - update statistics
        if (node != nullptr) {
            node->backpropagate(result);
        }
    }
    
    return root->get_best_move();
}