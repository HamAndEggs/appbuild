#include <iostream>

#include "hello.h"

int main(int argc, char *argv[])
{
	std::cout << "APP_BUILD_DATE_TIME is " << APP_BUILD_DATE_TIME << std::endl;
    
    HelloClassSharedObject Hello;
    Hello.SayHello();
    return 0;
}
 