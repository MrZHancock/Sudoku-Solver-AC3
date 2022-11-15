CC = g++-12
CFLAGS = -std=c++20 -O3 -Werror -Wall -pedantic -g
TARGET = Sudoku
OBJFILES = Sudoku.o
all: $(TARGET)
sudoku: sudoku.cpp
	$(CC) $(CFLAGS) -o Sudoku sudoku.cpp

run: sudoku
	./Sudoku
clean:
	rm -rf $(OBJFILES) $(TARGET) *~ 
	rm -rf *.exe
	rm -rf *.out
