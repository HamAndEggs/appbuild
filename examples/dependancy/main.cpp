#include <iostream>

#include "hello.h"

int main(int argc, char *argv[])
{
	std::cout << "BUILD_DATE_TIME is " << BUILD_DATE_TIME << std::endl;
    
    HelloClass Hello;
    Hello.SayHello();
    return 0;
}
 