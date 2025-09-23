cmake_minimum_required(VERSION 3.12)
project(student_agent_module)

set(CMAKE_CXX_STANDARD 17)

find_package(pybind11 REQUIRED)

# Add all source files
pybind11_add_module(student_agent_module 
    student_agent.cpp
    game_state.cpp
    minimax.cpp
    mcts.cpp
    heuristics.cpp
)
