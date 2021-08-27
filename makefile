appbuild:	./source/shell.cpp.o						\
			./source/arg_list.cpp.o						\
			./source/build_task.cpp.o					\
			./source/build_task_compile.cpp.o			\
			./source/build_task_resource_files.cpp.o	\
			./source/configuration.cpp.o				\
			./source/dependencies.cpp.o					\
			./source/lz4/lz4.c.o						\
			./source/main.cpp.o							\
			./source/misc.cpp.o							\
			./source/project.cpp.o						\
			./source/source_files.cpp.o					\
			./source/she_bang.cpp.o						\
			./source/new_project.cpp.o					\
			./source/json.cpp.o							\
			./source/command_line_options.cpp.o

	gcc 	./source/shell.cpp.o						\
			./source/arg_list.cpp.o						\
			./source/build_task.cpp.o					\
			./source/build_task_compile.cpp.o			\
			./source/build_task_resource_files.cpp.o	\
			./source/configuration.cpp.o				\
			./source/dependencies.cpp.o					\
			./source/lz4/lz4.c.o						\
			./source/main.cpp.o							\
			./source/misc.cpp.o							\
			./source/project.cpp.o						\
			./source/source_files.cpp.o					\
			./source/she_bang.cpp.o						\
			./source/new_project.cpp.o					\
			./source/json.cpp.o							\
			./source/command_line_options.cpp.o			\
			-lstdc++ -lpthread -lrt -lm -o appbuild

%.o: %
	gcc -c -o $@ $< $(CFLAGS) -I/usr/include -I./ -DALLOW_INCLUDE_OF_SCHEMA -DNDEBUG -O3 -Wall -std=c++14

clean:
	rm -f ./source/*.cpp.o
	rm -f ./source/lz4/lz4.c.o
	rm -f ./appbuild
