# This file was based in part on the instructions found at 
# https://github.com/vincentlaucsb/csv-parser?tab=readme-ov-file#cmake-instructions .
cmake_minimum_required(VERSION 3.5)
project(tttb)
set(CMAKE_CXX_STANDARD 20) # Setting this to a version earlier than 20 likely won't work


# Getting CMake to use/build static rather than shared libraries
# (as part of my process for debugging 'can't find shared library errors'
# when the program is run on other computers):
# The following code came from the cpp-terminal library's CMakeLists.txt file. (Note that I changed the value for this option in that file for Linux from ON to OFF.)
option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
# The above line is based on:
# https://stackoverflow.com/a/42842447/13097194




# Adding locations of cpp-terminal and csv-parser libraries:
# NOTE: You will need to replace, for any operating system for which you're compiling 
# this code, my folder paths with your own folder paths. I recognize that hard-coding
# these paths isn't ideal, but as a relative novice to CMake, this approach worked
# well for me and hopefully won't be too hard for you to update on your end.

# You'll also need to download (and likely compile) these libraries before incorporating them into your project.

# The if() and endif() calls were based on mlvljr's response at
# https://stackoverflow.com/a/32259072/13097194,
# the community wiki answer at
# https://stackoverflow.com/a/32259072 ,
# and the CMake documentation at https://gitlab.kitware.com/cmake/community/-/wikis/doc/tutorials/How-To-Write-Platform-Checks .

if(UNIX AND NOT APPLE)
add_subdirectory(/home/kjb3/D1V1/kjb3docs/Programming/CPP/third_party_libraries/csv-parser /home/kjb3/D1V1/Documents/!Dell64docs/Programming/CPP/third_party_libraries/csv-parser_subdiroutput)
add_subdirectory(/home/kjb3/D1V1/kjb3docs/Programming/CPP/third_party_libraries/cpp-terminal /home/kjb3/D1V1/Documents/!Dell64docs/Programming/CPP/third_party_libraries/cpp-terminal_subdiroutput)
# Note RE subdirectory output: https://stackoverflow.com/a/35260629/13097194
endif()

if(WIN32)
add_subdirectory(C:/Users/kburc/WnD1/3rd_party_libraries/csv-parser C:/Users/kburc/WnD1/3rd_party_libraries/csv-parser_subdiroutput)
add_subdirectory(C:/Users/kburc/WnD1/3rd_party_libraries/cpp-terminal C:/Users/kburc/WnD1/3rd_party_libraries/cpp-terminal_subdiroutput)
endif()

IF(APPLE)
add_subdirectory(/Users/alexandraburchfiel/Downloads/csv-parser-master /Users/alexandraburchfiel/Downloads/csv-parser-master_subdiroutput)
add_subdirectory(/Users/alexandraburchfiel/Downloads/cpp-terminal /Users/alexandraburchfiel/Downloads/cpp-terminal_subdiroutput)
endif()


add_executable(tttb tttb.cpp)
target_link_libraries(tttb csv cpp-terminal)

