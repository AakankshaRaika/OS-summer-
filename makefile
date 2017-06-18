# build an executable named soc from soc.c

all : main

main.o : main.cpp
	g++ -Wall -c main.cpp
clean: 
	$(RM) soc
