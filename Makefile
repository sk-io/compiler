CC = gcc
CFLAGS = -Wall -Wextra -Werror
OUTPUT = compiler
FILES = main.c lex.c parse.c emit.c util.c

all:
	$(CC) -o $(OUTPUT) $(CFLAGS) $(FILES)
