#!/bin/bash
# This script builds and installs the appbuild app. It does a full rebuild each time.
# Anything in the bin folder before this script is run will be deleted.

echo "Preparing build folder"

# Remove the bin folder then recreate it, this ensures no crap is hanging about that could effect the outcome of the build.
rm -drf ./bin
mkdir -p ./bin

RELEASE_COMPILE_FLAGS_C="-DNDEBUG -O3 -Wall -c"
DEBUG_COMPILE_FLAGS_C="-DDEBUG_BUILD -O0 -Wall -c"

COMPILE_FLAGS_C=$RELEASE_COMPILE_FLAGS_C
COMPILE_FLAGS_CPP11="-std=c++11 $COMPILE_FLAGS_C"
COMPILE_INCLUDES="-I/usr/include -I./"
LINKER_FLAGS="-lstdc++ -lpthread -lrt"
EXEC_OUTPUT_FILE="./bin/appbuild"
MAX_THREADS=$((lscpu | grep "^CPU(s):") | awk '{print $2}')

echo "Compiling with $MAX_THREADS threads"
echo
# The files that make the app.
SOURCE_FILES=(
		"./source/shell.cpp"
		"./source/arg_list.cpp"
		"./source/build_task.cpp"
		"./source/build_task_compile.cpp"
		"./source/build_task_resource_files.cpp"
		"./source/configuration.cpp"
		"./source/dependencies.cpp"
		"./source/lz4/lz4.c"
		"./source/main.cpp"
		"./source/misc.cpp"
		"./source/project.cpp"
		"./source/source_files.cpp"
		"./source/she_bang.cpp"
        "./source/new_project.cpp")

OBJECT_FILES=""

#********************************************************************************************************************
# The meat and potatoes of the build...
#********************************************************************************************************************

# Do the compile
let COUNT=0
for t in ${SOURCE_FILES[@]}; do
    SOURCE_FILE="$t"
    BASE_NAME=$(basename $t)
    
    OBJECT_FILE="./bin/$BASE_NAME.o"
    OBJECT_FILES+="$OBJECT_FILE "

    if [[ $BASE_NAME == *.cpp ]]; then
        COMPILE_FLAGS=$COMPILE_FLAGS_CPP11
    elif [[ $BASE_NAME == *.c ]]; then
        COMPILE_FLAGS=$COMPILE_FLAGS_C
    fi

    echo "$BASE_NAME"
    gcc $COMPILE_INCLUDES $COMPILE_FLAGS $SOURCE_FILE -o $OBJECT_FILE &

    #Four threads at a time please.
    let COUNT+=1
    if [ $COUNT == "$MAX_THREADS" ]; then
        let COUNT=0
        wait
    fi
done

# Wait for stragglers to finish
wait

# Link it
echo "Linking"
gcc $OBJECT_FILES $LINKER_FLAGS -o $EXEC_OUTPUT_FILE

# Copy it if they want it.
if [ -f ./bin/appbuild ]; then
    if [ "$1" == "-y" ]; then
        answer="y"
    else
        echo "May ask for password as it needs to use sudo to copy the file."
        echo "Do you wish to install this program?"
        read -p "(y/n)?" answer
    fi

    if [ $answer == "y" ] || [ $answer == "Y" ] ;then
        # Have to remove old install as it's now been moved out of the users local folder...
        sudo rm -f /usr/local/bin/appbuild
        echo "copying to /usr/bin"
        sudo cp ./bin/appbuild /usr/bin
    else
        echo "Application not installed"
    fi
else
    echo "Output executable is not found, compile must have failed. Contact author if you can not fix the issue yourself. Be nice and he may fix it for you."
fi

echo "Finished"
