# API for terminal interface
import curses # pip install windows-curses
try:
    # for loading from file
    import os
    import tkinter.filedialog
    LOAD_FROM_FILE = True
except ModuleNotFoundError:
    LOAD_FROM_FILE = False

class Terminal_Interface:
    # xterm-256 colour constants
    BLUE = curses.COLOR_BLUE
    RED = curses.COLOR_RED
    GREEN = curses.COLOR_GREEN
    # may be changed to 7 if terminal doesn't support xterm-256 colours
    # (can't check until a curses window is created)
    GREY = 242

    def __init__(self, height=15, width=50):
        self.height = height
        self.width = width
        # 2d array of unknown values (where ' ' denotes an unknown value)
        self.matrix = [[' ' for _ in range(9)] for r in range(9)]
        # wrapper method takes care of most setup and cleanup
        curses.wrapper(self._main)

    def display_matrix_values(self, x_offset=0, y_offset=0):
        r = 1 + y_offset # row in terminal
        for row in self.matrix:
            c = 1 + x_offset # column in terminal
            for val in row:
                if c != 1 + y_offset and c % 4 == 0:
                    c += 1
                self.window.addch(r + y_offset, c + x_offset, str(val), )
                c += 1
            r += 1
            if (r + y_offset) % 4 == 0:
                r += 1


    def display_updated_matrix_values(self, m_old, m_new, x_offset=0, y_offset=0):
        """
        After solving the problem, fill in remaining squares in a different colour
        """
        NEW_ATTR = curses.color_pair(Terminal_Interface.BLUE)
        r = 1 + y_offset # row in terminal
        for row_old, row_new in zip(m_old, m_new):
            c = 1 + y_offset # row in terminal
            for val_old, val_new in zip(row_old, row_new):
                if c != 1 + y_offset and c % 4 == 0:
                    c += 1
                if val_old != val_new:
                    self.window.addch(r+y_offset, c+x_offset, str(val_new), NEW_ATTR)
                c += 1
            r += 1
            if (r + y_offset) % 4 == 0:
                r += 1


    def draw_sudoku_gridlines(self):
        # gridlines
        for i in range(0, 13, 4):
            for j in range(1, 12):
                self.window.addch(i, j, curses.ACS_HLINE,
                    curses.color_pair(Terminal_Interface.GREY))
                self.window.addch(j, i, curses.ACS_VLINE,
                    curses.color_pair(Terminal_Interface.GREY))
        
        # nicely rounded corners
        self.window.addch(0, 0, curses.ACS_ULCORNER,
            curses.color_pair(Terminal_Interface.GREY))
        self.window.addch(0, 12, curses.ACS_URCORNER,
            curses.color_pair(Terminal_Interface.GREY))
        self.window.addch(12, 0, curses.ACS_LLCORNER,
            curses.color_pair(Terminal_Interface.GREY))
        self.window.addch(12, 12, curses.ACS_LRCORNER,
            curses.color_pair(Terminal_Interface.GREY))
        
        # where lines intersect
        for i in range(4, 12, 4):
            for j in range(4, 12, 4):
                self.window.addch(i, j, curses.ACS_PLUS,
                    curses.color_pair(Terminal_Interface.GREY))
            self.window.addch(i, 0, curses.ACS_LTEE,
                curses.color_pair(Terminal_Interface.GREY))
            self.window.addch(i, 12, curses.ACS_RTEE,
                curses.color_pair(Terminal_Interface.GREY))
            self.window.addch(12, i, curses.ACS_BTEE,
                curses.color_pair(Terminal_Interface.GREY))
            self.window.addch(0, i, curses.ACS_TTEE,
                curses.color_pair(Terminal_Interface.GREY))


    def draw_ui_instructions(self, x_offset, y_offset=1):
        TITLE = "Sudoku Solver (with AC-3)"
        ACTIONS = (
            "  (C)lear Puzzle",
            "  (L)oad from File" if LOAD_FROM_FILE else "",
            "  (S)olve",
            "  (Q)uit",
            "",
            "Use arrow keys or mouse",
            "clicks and the number",
            "keys to fill in the",
            "Sudoku puzzle.",
        )

        self.window.addstr(y_offset, x_offset, TITLE, curses.A_BOLD)

        for row, action in enumerate(ACTIONS, y_offset + 2):
            self.window.addstr(row, x_offset, action)
    

    @staticmethod
    def load_sudoku_from_file(path):
        with open(path, 'r') as file_in:
            matrix = []
            for line in file_in:
                line = line.rstrip('\n')
                row = []
                for char in line:
                    if '1' <= char <= '9':
                        row.append(char)
                    else:
                        # replace 0s and non-digits with spaces
                        # (to denote unknown positions)
                        row.append(' ')
                for _ in range(len(row), 9):
                    # if row has fewer than 9 values, remaining are unknown
                    row.append(' ')
                # if tow has >9 characters, keep only the first 9
                matrix.append(row[:9])
            for _ in range(len(matrix), 9):
                # if file has less than 9 rows, leave the other rows blank
                matrix.append([' '] * 9)
        with open(path, 'r') as file_in:
            msg = file_in.read().splitlines()[-1].strip()
        # if file has >9 lines, keep only the first 9
        return matrix[:9], msg


    def _main(self, stdscr):
        self.window = stdscr
        curses.resizeterm(16, 60)
        self.window.clear()
        curses.use_default_colors()
        for i in range(1, 256):
            # -1 denotes a transparent background
            curses.init_pair(i, i, -1)
        if curses.COLORS < 256:
            # if terminal doesn't support enough colours
            # change the GREY constant
            global GREY
            GREY = 7
        
        # draws the 9x9 grid and 3x3 sub-grids
        self.draw_sudoku_gridlines()

        # write the instructions to the terminal
        # (to the right of the puzzle)
        self.draw_ui_instructions(15)

        # configure curses to listen for mouse clicks
        curses.mouseinterval(10) # 10 ms
        curses.mousemask(curses.BUTTON1_CLICKED) # left click
        
        # move cursor to the first unknown position
        stdscr.move(1, 1)
        # update the terminal
        stdscr.refresh()


        while True:
            # wait for user to enter a character
            key = stdscr.getch()
            if key | 0x20 == ord('q'):
                # (Q)uit
                break
            elif key == curses.KEY_LEFT:
                y, x = stdscr.getyx()
                x -= 1 # move cursor left
                if x % 4 == 0: # cursor in gridline
                    if x == 0:
                        x = 11 # wrap around to right side
                    else:
                        x -= 1 # move left of vertical line
                self.window.move(y, x)
            elif key == curses.KEY_RIGHT:
                y, x = self.window.getyx()
                x += 1 # move cursor right
                if x % 4 == 0: # cursor in gridline
                    if x == 12:
                        x = 1 # wrap around to left side
                    else:
                        x += 1 # move right of vertical line
                self.window.move(y, x)
            elif key == curses.KEY_UP:
                y, x = self.window.getyx()
                y -= 1 # move cursor down
                if y % 4 == 0: # cursor in gridline
                    if y == 0:
                        y = 11 # wrap around to bottom
                    else:
                        y -= 1 # move below horizontal line
                self.window.move(y, x) # set new position
            elif key == curses.KEY_DOWN:
                y, x = self.window.getyx() # get current cursor position
                y += 1
                if y % 4 == 0: # cursor in gridline
                    if y == 12:
                        y = 1 # wrap around to top
                    else:
                        y += 1 # move below the vertical line
                self.window.move(y, x) # set new position
            elif ord('1') <= key <= ord('9') or key == ord(' '):
                y, x = self.window.getyx() # get current cursor position
                self.matrix[ 3*y//4 ][ 3*x//4 ] = chr(key)
                self.window.addch(y, x, chr(key))
                x += 1
                if x % 4 == 0:
                    if x == 12:
                        if y != 11:
                            x = 1
                            y += 1
                            if y % 4 == 0:
                                y += 1
                        else:
                            x = 11
                    else:
                        x += 1
                    self.window.move(y, x) # set new position
            elif key in (curses.KEY_BACKSPACE, curses.KEY_DC, 0xb, 0x7F):
                y, x = self.window.getyx() # get current cursor position
                self.window.addch(y, x, ' ')
                if key != curses.KEY_DC:
                    x -= 1
                    if x % 4 == 0:
                        if x == 0 and y != 1:
                            x = 11
                            y -= 1
                            if y % 4 == 0:
                                y -= 1
                        elif x == 0:
                            x = 1
                        elif x % 4 == 0:
                            x -= 1
                self.window.move(y, x)
            elif key | 0x20 == ord('c'):
                # (C)lear
                y, x = self.window.getyx()
                self.matrix = [[' ' for _ in range(9)] for _ in range(9)]
                self.display_matrix_values()
                self.window.move(y, x)
            elif key | 0x20 == ord('l') and LOAD_FROM_FILE:
                # (L)oad
                abs_path = tkinter.filedialog.askopenfilename(
                    title="Select a Sudoku Puzzle",
                    filetypes=(("Text", "*.txt"), ("Input", "*.in")),
                    initialdir='.',
                )
                if abs_path: # make sure user entered a path
                    self.matrix, _ = self.load_sudoku_from_file(abs_path)
                    self.display_matrix_values()
                    # convert absolute path to a relative path
                    rel_path = os.path.relpath(abs_path)
                    self.window.addnstr(13, 0, f"Loaded Sudoku from {rel_path}", self.width)
                    self.window.move(11, 11)
            elif key == curses.KEY_MOUSE:
                try:
                    _, x, y, _, _ = curses.getmouse()
                    if 0 < x < 12 and 0 < y < 12 and x % 4 != 0 and y % 4 != 0:
                        self.window.move(y, x)
                except curses.error as e:
                    # ignore any weird mouse movements that cause errors
                    # happens when scrolling the cursor off the screen
                    pass
            elif key | 0x20 == ord('s'):
                with open("puzzle_in.txt", 'w') as f:
                    for row in self.matrix:
                        print("".join(row), file=f)
                curses.curs_set(0)
                self.window.addnstr(13, 0, "Solving...", self.width)
                os.system("make run > /dev/null")
                matrix_new, msg = self.load_sudoku_from_file("puzzle_out.txt")
                self.window.addstr(13, 0, f"{msg:{self.width}}")
                self.display_updated_matrix_values(self.matrix, matrix_new)
                self.window.move(12, self.width - 1)
            elif key == curses.KEY_RESIZE:
                pass # ignore terminal resize
            else:
                # unknown key pressed
                curses.beep()
                curses.flash()
            
            # update the terminal with the changes (if any)
            stdscr.refresh()

if __name__ == "__main__":
    Terminal_Interface()