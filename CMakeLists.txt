# This file was based in part on the instructions found at 
# https://github.com/vincentlaucsb/csv-parser?tab=readme-ov-file#cmake-instructions .
cmake_minimum_required(VERSION 3.10)
project(tttb)
set(CMAKE_CXX_STANDARD 23)
# add_subdirectory(/home/kjb3/Downloads/DataFrame /home/kjb3/Downloads/DataFrame_subdiroutput)
add_subdirectory(/home/kjb3/D1V1/Documents/!Dell64docs/Programming/CPP/3rd_party_libraries/csv-parser /home/kjb3/D1V1/Documents/!Dell64docs/Programming/CPP/3rd_party_libraries/csv-parser_subdiroutput)
# Note RE subdirectory output: https://stackoverflow.com/a/35260629/13097194
add_executable(tttb tttb.cpp)
target_link_libraries(tttb csv)
#target_link_libraries(tttb csv DataFrame::DataFrame)
