
# build an executable named soc from soc.c

main.o : main.cpp soc.cpp soc.h
        g++ -Wall -c main.cpp
soc : soc.o main.o
        g++ -Wall soc.o main.o -o soc
clean:
        $(RM) soc
