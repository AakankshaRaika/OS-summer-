# build an executable named soc from soc.c

all : main.cpp soc.cpp soc.h
	g++ -Wall -c main.cpp
clean: 
	$(RM) soc
