#!/usr/bin/python3
MIN_VALUE = 2
MAX_VALUE = 100000

def IsPrime(pNumber):
    for n in range(2,pNumber):
        if (pNumber % n) == 0:
            return False
    return True

print("Looking for primes between",MIN_VALUE,"and",MAX_VALUE)

for n in range(MIN_VALUE,MAX_VALUE):
    if IsPrime(n):
        print(n,end=",")