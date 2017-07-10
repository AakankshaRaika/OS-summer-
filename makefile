OBJS = main.o
CC = g++
DEBUG = -g
CFLAGS = -Wall -c $(DEBUG)
LFLAGS = -Wall $(DEBUG)

virtualmem: $(OBJS)
        $(CC) $(LFLAGS) $(OBJS) -o virtualmem $(LDFLAGS)

main.o : main.cpp
        $(CC) $(CFLAGS) main.cpp

clean:
        \rm *.o *~ virtualmem

tar:
        tar cfv p1.tar main.cpp
