# This file was based in part on the instructions found at 
# https://github.com/vincentlaucsb/csv-parser?tab=readme-ov-file#cmake-instructions .
# Note: some of the module-related code was based on the documentation at:
# https://www.kitware.com/import-cmake-the-experiment-is-over/
# Also note that GNinja must be specified (e.g. by running cmake -GNinja .. rather than cmake ..)
# CXX=g++-14 CC=gcc-14 cmake -GNinja ..
# Optionfor g++-15 and gcc-15 (which I installed manually):
# CXX=/opt/gcc-15/bin/g++ CC=/opt/gcc-15/bin/gcc cmake -GNinja ..

# You could also try: CXX=clang++ CC=clang cmake -GNinja ..
# I ended up using 
cmake_minimum_required(VERSION 3.5)
project(tttb)
set(CMAKE_CXX_STANDARD 23)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_SCAN_FOR_MODULES ON)
#set(CMAKE_C_COMPILER /opt/gcc-15/bin/gcc)
#set(CMAKE_CXX_COMPILER /opt/gcc-15/bin/g++)

# The above line is based on:
# https://stackoverflow.com/a/42842447/13097194
# I downloaded and compiled both of the following libraries before incorporating them into my project.
add_subdirectory(/home/kjb3/D1V1/Documents/!Dell64docs/Programming/CPP/3rd_party_libraries/csv-parser /home/kjb3/D1V1/Documents/!Dell64docs/Programming/CPP/3rd_party_libraries/csv-parser_subdiroutput)
add_subdirectory(/home/kjb3/Downloads/cpp-terminal /home/kjb3/Downloads/cpp-terminal_subdiroutput)
# Note RE subdirectory output: https://stackoverflow.com/a/35260629/13097194
# add_library(tttbmodules)
# target_sources (tttbmodules
# PUBLIC 
# FILE_SET CXX_MODULES FILES
# typing_test_module.cpp)
add_executable(tttb tttb.cpp)
# I realized that I needed to link these libraries (defined within add_subdirectory) to both
# tttbmodules and tttb. Otherwise, CMake would fail to link those libraries to tttbmodules.
#target_link_libraries(tttbmodules csv cpp-terminal)
# target_link_libraries(tttb csv cpp-terminal tttbmodules)
target_link_libraries(tttb csv cpp-terminal)
#target_link_libraries(tttb csv DataFrame::DataFrame)
# target_compile_features(tttb PRIVATE cxx_std_23)
# Based on https://stackoverflow.com/a/46809634/13097194
