OBJS = main.o
CC = g++
DEBUG = -g
CFLAGS = -Wall -c $(DEBUG)
LFLAGS = -Wall $(DEBUG)

p2 : $(OBJS)
        $(CC) $(LFLAGS) $(OBJS) -o p2 $(LDFLAGS)

main.o : main.cpp
        $(CC) $(CFLAGS) main.cpp

clean:
        \rm *.o *~ p2

tar:
        tar cfv p1.tar main.cpp
