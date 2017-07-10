OBJS = main.o
CC = g++
DEBUG = -g
CFLAGS = -Wall -c $(DEBUG)
LFLAGS = -Wall $(DEBUG)
LDFLAGS=-pthread

p1 : $(OBJS)
        $(CC) $(LFLAGS) $(OBJS) -o p1 $(LDFLAGS)

main.o : main.cpp
        $(CC) $(CFLAGS) main.cpp

clean:
        \rm *.o *~ p1

tar:
        tar cfv p1.tar main.cpp
