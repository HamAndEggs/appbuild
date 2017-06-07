#!/bin/bash
# This script builds and installs the appbuild app. It does a full rebuild each time.
# Anything in the bin folder will be deleted.

echo "Preparing build folder"

# Remove the bin folder then recreate it, this ensures no crap is hanging about that could effect the outcome of the build.
rm -drf ./bin
mkdir -p ./bin

# Compile the cpp code.
echo "Compiling"

echo "project.cpp"
gcc -I /usr/include -std=c++11 -DNDEBUG -O3 -c source/project.cpp -o bin/project.o

echo "dependencies.cpp"
gcc -I /usr/include -std=c++11 -DNDEBUG -O3 -c source/dependencies.cpp -o bin/dependencies.o

echo "configuration.cpp"
gcc -I /usr/include -std=c++11 -DNDEBUG -O3 -c source/configuration.cpp -o bin/configuration.o

echo "source_files.cpp"
gcc -I /usr/include -std=c++11 -DNDEBUG -O3 -c source/source_files.cpp -o bin/source_files.o

echo "json.cpp"
gcc -I /usr/include -std=c++11 -DNDEBUG -O3 -c source/json.cpp -o bin/json.o

echo "json_writer.cpp"
gcc -I /usr/include -std=c++11 -DNDEBUG -O3 -c source/json_writer.cpp -o bin/json_writer.o

echo "build_task.cpp"
gcc -I /usr/include -std=c++11 -DNDEBUG -O3 -c source/build_task.cpp -o bin/build_task.o

echo "misc.cpp"
gcc -I /usr/include -std=c++11 -DNDEBUG -O3 -c source/misc.cpp -o bin/misc.o

echo "main.cpp"
gcc -I /usr/include -std=c++11 -DNDEBUG -O3 -c source/main.cpp -o bin/main.o

echo "arg_list.cpp"
gcc -I /usr/include -std=c++11 -DNDEBUG -O3 -c source/arg_list.cpp -o bin/arg_list.o

echo "Linking"
gcc ./bin/build_task.o ./bin/dependencies.o ./bin/arg_list.o ./bin/misc.o ./bin/main.o ./bin/project.o ./bin/source_files.o ./bin/json.o ./bin/json_writer.o ./bin/configuration.o -lstdc++ -lpthread -lrt -o ./bin/appbuild

if [ -f ./bin/appbuild ]; then
	echo "Do you wish to install this program?"
	echo "May ask for password as it needs to use sudo to copy the file."
	read -p "(y/n)?" answer
	if [ $answer == "y" ] || [ $answer == "Y" ] ;then
		echo "copying to /usr/local/bin"
		sudo cp ./bin/appbuild /usr/local/bin
	else
		echo "Application not installed"
	fi
else
	echo "Output executable is not found, compile must have failed. Contact author if you can not fix the issue yourself. Be nice and he may fix it for you."
fi

echo "Finished"
