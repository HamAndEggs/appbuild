#include <iostream>
#include "hello.h"

void HelloClass::SayHello()
{
	std::cout << "BUILD_DATE_TIME is " << BUILD_DATE_TIME << std::endl;
	std::cout << "HelloClass says, \"Hello world\"" << std::endl;

	#ifdef RELEASE_BUILD
		std::cout << "Release Build" << std::endl;
	#endif

	#ifdef DEBUG_BUILD
		std::cout << "Debug Build" << std::endl;
	#endif

	std::cout << "The number " << A_NUMBER_DEF << std::endl;
}
