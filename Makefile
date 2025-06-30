CC = gcc
CFLAGS = -g -Wall -Wextra 
LDFLAGS = -lncurses -lm -pthread
SRC = src/main.c src/parser.c src/calculate.c src/ui.c
HEADER = include/parser.h include/calculate.h include/ui.h
OBJ = $(SRC:.c=.o) 
TARGET = pulse
DEBUG_LOG = vgcore*

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.c $(HEADER)
	$(CC) $(CFLAGS) -c $< -o $@

run : $(TARGET)
	./$(TARGET)

gdb : $(TARGET)
	gdb $(TARGET)

val : $(TARGET)
	valgrind --leak-check=yes -s ./$(TARGET)

clean : 
	@echo "Removing build files"
	rm -f $(TARGET) $(OBJ) $(DEBUG_LOG)
	
clean_txt :
	@echo "Removing text files"
	rm -f *.txt

copy:
	@echo "Copying source files to readable text documents"
	cp Makefile makefile.txt
	cp src/main.c main.txt
	cp src/parser.c parser.txt
	cp src/calculate.c calculate.txt
	cp src/ui.c ui.txt
	cp include/parser.h parser-h.txt
	cp include/calculate.h calculate-h.txt
	cp include/ui.h ui.txt
