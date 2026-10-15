cmake_minimum_required(VERSION 3.16)
project(BattlegroundModePolicyTests LANGUAGES CXX)

# The AzerothCore module is built by the parent core's module discovery.
# This standalone project only runs dependency-free policy tests.
enable_testing()
add_executable(battleground_mode_policy_test tests/policy_test.cpp)
target_include_directories(battleground_mode_policy_test PRIVATE src)
target_compile_features(battleground_mode_policy_test PRIVATE cxx_std_17)
target_compile_options(battleground_mode_policy_test PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Werror>)
add_test(NAME battleground_mode_policy COMMAND battleground_mode_policy_test)
