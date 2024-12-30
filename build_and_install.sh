#!/bin/bash
# This script builds and installs the appbuild app. It does a full rebuild each time.
# Anything in the bin folder before this script is run will be deleted.
# Don't expect much here, this is the boot strap.

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
        "./source/new_project.cpp"
        "./source/json.cpp"
        "./source/command_line_options.cpp")

INSTALL_LOCATION="/usr/bin/"

# ************************
# Some colours
# ************************
RED='\033[0;31m'
BOLDRED='\033[1;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
BOLDBLUE='\033[1;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

#********************************************************************************************************************
# Some functions
#********************************************************************************************************************
function Message()
{
	if [ -z "$2" ]; then
        echo -e "$GREEN$1$NC"
    else
        echo -e "$1$2$NC"
    fi
}

GetAbsFolder()
{
    mkdir -p $1
    cd $1
    echo $PWD
    cd $OLDPWD
}

function ShowHelp()
{
	Message $YELLOW "usage: build_and_install.sh [options]"
    echo ""
	echo "  -d Builds for debugging, if not given will build for release."
	echo "  -u Runs unit tests, builds itself and the examples using valgrind"
	echo "  --help This help"
	echo ""

}

function PrepareBuildFolder()
{
    Message "Preparing build folder"

    # Remove the bin folder then recreate it, this ensures no crap is hanging about that could effect the outcome of the build.
    rm -drf ./bin
    mkdir -p ./bin
}

VALGRIND_FAIL_CODE=99
VALGRIND_DID_FAIL="FALSE"
function CheckValgridReturnCode()
{
    RETURN_CODE=$?
    if [ $RETURN_CODE == $VALGRIND_FAIL_CODE ]; then
        Message $RED "Failed valgrind check"
        VALGRIND_DID_FAIL="TRUE"
    else
        Message $GREEN "Passed valgrind check"
    fi
}

#********************************************************************************************************************
# The meat and potatoes of the build...
#********************************************************************************************************************
DO_UNIT_TESTS="FALSE"

APP_BUILD_DATE=$(date +%d/%m/%Y)
APP_BUILD_TIME=$(date +%H:%M)
APP_BUILD_GIT_BRANCH=$(git branch --show-current)
APP_BUILD_VERSION=$(cat ./version)

APP_BUILD_GENERATED_DEFINES="-DAPP_BUILD_DATE=\"$APP_BUILD_DATE\" -DAPP_BUILD_TIME=\"$APP_BUILD_TIME\" -DAPP_BUILD_GIT_BRANCH=\"$APP_BUILD_GIT_BRANCH\" -DAPP_BUILD_VERSION=\"$APP_BUILD_VERSION\""

RELEASE_COMPILE_FLAGS="-DALLOW_INCLUDE_OF_SCHEMA -DNDEBUG -O3 -Wall"
DEBUG_COMPILE_FLAGS="-DALLOW_INCLUDE_OF_SCHEMA -DDEBUG_BUILD -O0  -g2 -Wall"

# Check for build type, if not given assume release.
COMPILE_FLAGS_C=$RELEASE_COMPILE_FLAGS


while [ "$1" != "" ];
do
    if [ "$1" == "-d" ]; then
        echo "Building for debug"
        COMPILE_FLAGS_C=$DEBUG_COMPILE_FLAGS
    elif [ "$1" == "-u" ]; then
        echo "Building for unit tests, will not be installed"
        COMPILE_FLAGS_C=$DEBUG_COMPILE_FLAGS
        DO_UNIT_TESTS="TRUE"
        Message $RED "Unit test build only, nothing will be installed. There will be a lot of output."
    elif [ "$1" == "--help" ]; then
        ShowHelp
        exit 0
    fi

    # Goto next param
    shift
done

PrepareBuildFolder

COMPILE_FLAGS_CPP="-std=c++14 $COMPILE_FLAGS_C"
COMPILE_INCLUDES="-I/usr/include -I./"
LINKER_FLAGS="-lstdc++ -lpthread -lrt -lm"
EXEC_OUTPUT_PATH=$(GetAbsFolder "./bin")
EXEC_OUTPUT_FILE="$EXEC_OUTPUT_PATH/appbuild"
MAX_THREADS=$((lscpu | grep "^CPU(s):") | awk '{print $2}')
OBJECT_FILES=""

Message "Compiling with $MAX_THREADS threads, output file $EXEC_OUTPUT_FILE"
Message $YELLOW "  C FLAGS ARE $COMPILE_FLAGS_C"
echo


# Do the compile
let COUNT=0
for t in ${SOURCE_FILES[@]}; do
    SOURCE_FILE="$t"
    BASE_NAME=$(basename $t)
    
    OBJECT_FILE="./bin/$BASE_NAME.o"
    OBJECT_FILES+="$OBJECT_FILE "

    if [[ $BASE_NAME == *.cpp ]]; then
        COMPILE_FLAGS=$COMPILE_FLAGS_CPP
    elif [[ $BASE_NAME == *.c ]]; then
        COMPILE_FLAGS=$COMPILE_FLAGS_C
    fi

    echo "$BASE_NAME"
    gcc $COMPILE_INCLUDES $COMPILE_FLAGS $APP_BUILD_GENERATED_DEFINES -c $SOURCE_FILE -o $OBJECT_FILE &

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
if [ -f $EXEC_OUTPUT_FILE ]; then
    # It build ok, that if just showed it did.
    Message "Build ok."
    if [ "$DO_UNIT_TESTS" == "TRUE" ]; then
        VALGRIND_COMMAND="valgrind --tool=memcheck --leak-check=full --error-exitcode=$VALGRIND_FAIL_CODE"

        echo
        Message $BOLDBLUE "Doing unit tests, you will require valgrind to have been installed as well as libncurses5-dev and libjpeg-dev for a few of the examples."
        Message $BOLDBLUE "    eg: sudo apt install valgrind libjpeg-dev libncurses5-dev"
        echo
        # First rebuilt it self using valgrind.
        Message $BOLDBLUE "Self build test"
        $VALGRIND_COMMAND $EXEC_OUTPUT_FILE
        CheckValgridReturnCode
        echo

        # Do a help display test
        Message $BOLDBLUE "Display help test"
        $VALGRIND_COMMAND $EXEC_OUTPUT_FILE --help
        CheckValgridReturnCode
        echo

        # Now build the examples
        cd ./examples
#****************************************************
            Message $BOLDBLUE "Build hello world"
            cd ./hello_world
            $VALGRIND_COMMAND $EXEC_OUTPUT_FILE -x -V -r
            CheckValgridReturnCode
            $EXEC_OUTPUT_FILE -x
            cd ..
            echo
#****************************************************
            Message $BOLDBLUE "Build dependency test"
            cd ./dependency
            $VALGRIND_COMMAND $EXEC_OUTPUT_FILE -V -r
            CheckValgridReturnCode
            $EXEC_OUTPUT_FILE -x
            cd ..
            echo
#****************************************************
            Message $BOLDBLUE "Build shared object dependency test"
            cd ./dependency-so
            $VALGRIND_COMMAND $EXEC_OUTPUT_FILE -V -r
            CheckValgridReturnCode
            $EXEC_OUTPUT_FILE -x
            cd ..
            echo
#****************************************************
            Message $BOLDBLUE "Build resource test"
            cd ./resource
            # We do not run this one as it needs a frame buffer object.
            $VALGRIND_COMMAND $EXEC_OUTPUT_FILE -V -r
            CheckValgridReturnCode
            cd ..
            echo
#****************************************************
            Message $BOLDBLUE "Build failure test"
            cd ./unit-test-1-build-fail
            $VALGRIND_COMMAND $EXEC_OUTPUT_FILE -V -r
            CheckValgridReturnCode
            cd ..
            echo
#****************************************************
            Message $BOLDBLUE "shebang test, can not run valgrind as the current process is replaced"
            cd ./shebang
            $EXEC_OUTPUT_FILE -# ./hello_world.c
            $EXEC_OUTPUT_FILE -# ./hello_world.cpp
            cd ..
            echo
#****************************************************
            Message $BOLDBLUE "Build embedded project test"
            cd ./embedded-project
            $VALGRIND_COMMAND $EXEC_OUTPUT_FILE -V -e appbuild embedded-project.example
            CheckValgridReturnCode
            $EXEC_OUTPUT_FILE -x -e appbuild embedded-project.example
            cd ..
            echo
#****************************************************

        cd ..
        Message $BLUE "Unit tests finished"
        if [ $VALGRIND_DID_FAIL == "TRUE" ]; then
            Message $BOLDRED "Unit tests failed! Please check output"
            exit 1
        else
            Message $GREEN "All tests passed"
        fi
    else
        if [ "$1" == "-y" ]; then
            answer="y"
        else
            Message $YELLOW "May ask for password as it needs to use sudo to copy the file."
            Message "Do you wish to install this program?"
            read -p "(y/n)?" answer
        fi

        if [ $answer == "y" ] || [ $answer == "Y" ] ;then
            # Have to remove old install as it's now been moved out of the users local folder...
            sudo rm -f $INSTALL_LOCATION/appbuild
            Message "copying to $INSTALL_LOCATION"
            sudo cp ./bin/appbuild $INSTALL_LOCATION
            Message $BLUE "Finished"
        else
            Message $YELLOW "Application not installed"
        fi
    fi
else
    Message $RED "Output executable is not found, compile must have failed. Contact author if you can not fix the issue yourself. Be nice and he may fix it for you."
fi

