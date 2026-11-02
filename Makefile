CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -fsanitize=address,undefined

all: csvstat

csvstat: main.c table.c parser.c output.c table.h parser.h output.h
	$(CC) $(CFLAGS) main.c table.c parser.c output.c -o csvstat -lm

clean:
	rm -f csvstat
