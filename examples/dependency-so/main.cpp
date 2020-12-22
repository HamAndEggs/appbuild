#include <iostream>
#include <unistd.h>
#include <string.h>

#include "hello.h"

int main(int argc, char *argv[])
{    
	std::cout << "APP_BUILD_DATE_TIME is " << APP_BUILD_DATE_TIME << '\n';

    HelloClassSharedObject Hello;
    Hello.SayHello();
    return 0;
}
 