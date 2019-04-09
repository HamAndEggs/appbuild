#!/usr/local/bin/appbuild -#
#include <iostream>

static const int MIN_VALUE = 2;
static const int MAX_VALUE = 100000;

static bool IsPrime(int pNumber)
{
    for(int n = 2 ; n < pNumber ; n++ )
    {
        if( (pNumber % n) == 0 )
            return false;
    }

    return true;
}

int main(int argc, char *argv[])
{
    std::cout << "Looking for primes between " << MIN_VALUE << " and " << MAX_VALUE << std::endl;

    bool doneFirst = false;
    for( int n = MIN_VALUE ; n < MAX_VALUE ; n++ )
    {
        if( IsPrime(n) )
        {
            if( doneFirst )
                std::cout << ", ";

            doneFirst = true;
            std::cout << n;
        }
    }
    std::cout << std::endl;

    return 0;
}
