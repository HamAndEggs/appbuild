#include <iostream>

This will not build, that is correct.
I need this to test for memory leeks when a build fails.

int main(int argc, char *argv[])
{
    std::cout << "Hello world, an app compiled with appbuild.\n";
    return 0;
}