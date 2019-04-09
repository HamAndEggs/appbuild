#!/bin/bash
# This script builds and installs the appbuild app. It does a full rebuild each time.
# Anything in the bin folder will be deleted.

echo "Preparing build folder"

# Remove the bin folder then recreate it, this ensures no crap is hanging about that could effect the outcome of the build.
rm -drf ./bin
mkdir -p ./bin

COMPILE_FLAGS_C="-DNDEBUG -O3 -Wall -c"
COMPILE_FLAGS_CPP11="-std=c++11 $COMPILE_FLAGS_C"

#echo $COMPILE_FLAGS_C
#echo $COMPILE_FLAGS_CPP11

# Compile the cpp code.
echo "Compiling"

echo "build_task.cpp"
gcc -I /usr/include $COMPILE_FLAGS_CPP11 source/build_task.cpp -o bin/build_task.o &

echo "build_task_compile.cpp"
gcc -I /usr/include $COMPILE_FLAGS_CPP11 source/build_task_compile.cpp -o bin/build_task_compile.o &

echo "build_task_resource_files.cpp"
gcc -I /usr/include $COMPILE_FLAGS_CPP11 source/build_task_resource_files.cpp -o bin/build_task_resource_files.o &

# doing a few at a time.
wait

echo "lz4.c"
gcc -I /usr/include $COMPILE_FLAGS_C -c source/lz4/lz4.c -o bin/lz4.o &

echo "project.cpp"
gcc -I /usr/include $COMPILE_FLAGS_CPP11 source/project.cpp -o bin/project.o &

echo "dependencies.cpp"
gcc -I /usr/include $COMPILE_FLAGS_CPP11 source/dependencies.cpp -o bin/dependencies.o &

echo "configuration.cpp"
gcc -I /usr/include $COMPILE_FLAGS_CPP11 source/configuration.cpp -o bin/configuration.o &

# doing a few at a time.
wait

echo "source_files.cpp"
gcc -I /usr/include $COMPILE_FLAGS_CPP11 source/source_files.cpp -o bin/source_files.o &

echo "resource_files.cpp"
gcc -I /usr/include $COMPILE_FLAGS_CPP11 source/resource_files.cpp -o bin/resource_files.o &

echo "misc.cpp"
gcc -I /usr/include $COMPILE_FLAGS_CPP11 source/misc.cpp -o bin/misc.o &

echo "main.cpp"
gcc -I /usr/include $COMPILE_FLAGS_CPP11 source/main.cpp -o bin/main.o &

# doing a few at a time.
wait

echo "json.cpp"
gcc -I /usr/include $COMPILE_FLAGS_CPP11 source/json.cpp -o bin/json.o &

echo "json_writer.cpp"
gcc -I /usr/include $COMPILE_FLAGS_CPP11 source/json_writer.cpp -o bin/json_writer.o &

echo "arg_list.cpp"
gcc -I /usr/include $COMPILE_FLAGS_CPP11 source/arg_list.cpp -o bin/arg_list.o &

echo "she_bang.cpp"
gcc -I /usr/include $COMPILE_FLAGS_CPP11 source/she_bang.cpp -o bin/she_bang.o &

# Wait for last jobs to be done.
wait

echo "Linking"
gcc ./bin/lz4.o ./bin/build_task.o ./bin/build_task_compile.o ./bin/build_task_resource_files.o ./bin/dependencies.o ./bin/arg_list.o ./bin/misc.o ./bin/main.o ./bin/project.o ./bin/source_files.o ./bin/resource_files.o ./bin/json.o ./bin/json_writer.o ./bin/configuration.o ./bin/she_bang.o -lstdc++ -lpthread -lrt -o ./bin/appbuild

if [ -f ./bin/appbuild ]; then
	echo "Do you wish to install this program?"
	echo "May ask for password as it needs to use sudo to copy the file."
	read -p "(y/n)?" answer
	if [ $answer == "y" ] || [ $answer == "Y" ] ;then
        # Have to remove old install as it's now been moved out of the users local folder...
        rm -f /usr/local/bin/appbuild
		echo "copying to /usr/bin"
		sudo cp ./bin/appbuild /usr/bin
	else
		echo "Application not installed"
	fi
else
	echo "Output executable is not found, compile must have failed. Contact author if you can not fix the issue yourself. Be nice and he may fix it for you."
fi

echo "Finished"
