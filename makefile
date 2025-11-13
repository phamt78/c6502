
CC = gcc
CFLAGS = -g -Wall -O0

# Target C files
C_FILES = main.c c6502.c bus.c

# Program Name
PROGRAM = neslogs

all: $(PROGRAM)

$(PROGRAM): $(C_FILES)
	$(CC) $(CFLAGS) -o $(PROGRAM) $(C_FILES)

clean:
	rm -f $(PROGRAM)

.PHONY: all clean
