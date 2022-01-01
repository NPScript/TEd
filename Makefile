CC=gcc -Wall
SRC=$(wildcard *.c)
OBJ=${SRC:.c=.o}
LDFLAGS=

all: ted

options:
	@echo "CC      = ${CC}"
	@echo "SRC     = ${SRC}"
	@echo "OBJ     = ${OBJ}"
	@echo "LDFLAGS = ${LDFLAGS}"

.c.o:
	${CC} -c $<

ted: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm ${OBJ} ted
