cmake_minimum_required(VERSION 3.13)
project(appbuild)

set(CMAKE_CXX_STANDARD 14)

include_directories(./)

set( SOURCE_FILES 
    "source/shell.cpp"
    "source/arg_list.cpp"
    "source/build_task.cpp"
    "source/build_task_compile.cpp"
    "source/build_task_resource_files.cpp"
    "source/configuration.cpp"
    "source/dependencies.cpp"
    "source/lz4/lz4.c"
    "source/main.cpp"
    "source/misc.cpp"
    "source/project.cpp"
    "source/source_files.cpp"
    "source/she_bang.cpp"
    "source/new_project.cpp"
    "source/json.cpp"
    "source/command_line_options.cpp"
)

add_executable(appbuild ${SOURCE_FILES} )
target_link_libraries(appbuild stdc++ pthread rt m)


