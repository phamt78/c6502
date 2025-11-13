CC = gcc
CFLAGS = -o

# Target C files
C_FILES = main.c c6502.c bus.c

# Program Name
PROGRAM = neslogs

all: $(PROGRAM) $(LOGS)
	
$(PROGRAM): $(C_FILES)
	$(CC) $(C_FILES) $(CFLAGS) $(PROGRAM) 

clean:
	rm $(PROGRAM) 
