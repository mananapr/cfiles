cmake_minimum_required (VERSION 3.10)

project(cfiles
    VERSION 0.0.1
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(FLAGS "-ggdb -Wall -Wextra -Wundef -Wwrite-strings -Wpointer-arith -Wmissing-declarations -Wredundant-decls")
set(FLAGS "${FLAGS} -Wno-unused-parameter -Wno-missing-field-initializers -Wformat=2 -Wcast-align -Wformat-nonliteral -Wformat-security")
set(FLAGS "${FLAGS} -Wsign-compare -Wstrict-aliasing -Wshadow -Winline -Wpacked -Wmissing-format-attribute -Wmissing-noreturn -Winit-self")
set(FLAGS "${FLAGS} -Wmissing-include-dirs -Wunused-but-set-variable -Warray-bounds -Wreturn-type ")
set(FLAGS "${FLAGS} -Wno-switch -Wno-error=unused-parameter")
set(FLAGS "${FLAGS} -Wno-error=missing-field-initializers")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAGS}")

#Runs pkg-config --libs --cflags ncurses
include(FindPkgConfig)
pkg_search_module(NCURSES REQUIRED ncurses)

add_executable(cfiles
    cf.c
)
target_include_directories(cfiles PRIVATE
    ${NCURSES_INCLUDE_DIRS}
)
target_compile_definitions(cfiles PRIVATE
    ${NCURSES_CFLAGS}
)
target_link_libraries(cfiles
    ${NCURSES_LIBRARIES}
)
