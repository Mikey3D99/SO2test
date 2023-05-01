cmake_minimum_required(VERSION 3.22)
project(SO2 C)
set(CURSES_LIBRARY "/usr/lib/x86_64-linux-gnu/libncurses.so.6.3")
set(CURSES_INCLUDE_PATH "/usr/include")

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

add_executable(SO2 main.c client/player/player.c client/player/player.h client/constants/constants.c client/constants/constants.h client/server/server.c client/server/server.h)
find_package(Curses REQUIRED)  # Find the ncurses package
include_directories(${CURSES_INCLUDE_DIR})  # Add the ncurses include directories
set_target_properties(SO2 PROPERTIES OUTPUT_NAME "SO2")

target_link_libraries(SO2 ${CURSES_LIBRARIES} tinfo)  # Link ncurses to your project
if (UNIX AND NOT APPLE)
    add_custom_target(run_in_terminal
            COMMAND x-terminal-emulator -e "$<TARGET_FILE:SO2>"
            DEPENDS SO2
            USES_TERMINAL
            )
elseif (APPLE)
    add_custom_target(run_in_terminal
            COMMAND open -a Terminal.app "$<TARGET_FILE:SO2>"
            DEPENDS SO2
            USES_TERMINAL
            )
elseif (WIN32)
    add_custom_target(run_in_terminal
            COMMAND start cmd.exe /K "$<TARGET_FILE:SO2>"
            DEPENDS SO2
            USES_TERMINAL
            )
endif ()
