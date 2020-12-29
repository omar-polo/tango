CC =		cc
CFLAGS =	-Wall -g

.PHONY: all clean install

all: tango

tango: tango.o
	${CC} -o tango tango.o ${LDFLAGS}

clean:
	rm -f *.o tango
