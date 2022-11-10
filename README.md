# Sudoku-Solver-AC3
Implementation of Mackworth's AC-3 local consistency
algorithm to improve performance of a naive Sudoku solver.

Requires a compiler that implements the C++20 standard.
If you are using a compiler besides GCC 12, you will need
to update the first like of the makefile.

The python script is intended to be a user-friendly
interface for entering a Sudoku puzzle.  It requires
Python's `curses` library, which generally is not installed
on Windows.  Additionally, if the `Tkinter` library is
installed, it can be used to prompt the user for a file
using a familiar file dialog interface.

The Sudoku solver can also be invoked without the Python
interface.  Simply pass the input filename as the first
command line argument.

All input files should be in the following format:
```
  6     1
 7  6  5 
8  1 32  
  5 4 8  
 4 7 2 9 
  8 1 7  
  12 5  3
 6  7  8 
2     4  
```
Note that
- the file has exactly 9 rows
- each row has exactly 9 characters
    (plus a newline at the end)
- there are no deliminators to designate where each of the $3\times 3$ sub-grids starts/ends
- all characters besides the digits $1, 2, \dots, 9$
    are treated as unknown characters
    - any of the spaces in the example above could be
    replaced with a dashe, period, question mark, X,
    zero, or any other character that isn't in
    $1, 2, \dots, 9$.