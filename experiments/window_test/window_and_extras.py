# window_and_extras.py -- the twin of window_and_extras.satl, which race_windows.cpp times it
# against: satl_18_vars as py_18_vars, a capsule (here a method) per variable, made ten times. Every
# line it prints is the line satl prints, which is how to check the two did the same work:
#     diff <(satl window_and_extras.satl 2>/dev/null) <(pypy window_and_extras.py)
#
# The window is tkinter's: ONE Tk for every object, made by the first object and withdrawn so it is
# never shown -- satl's one GTK "desk" opens the same way, the first time a window word runs, and
# every window after shares it. Each object's window is a Toplevel on it, shown where the object is
# made, as satl's is. update_idletasks() is Tk's "show it now" -- it maps the window and waits for
# the window manager to say it is mapped -- and winfo_ismapped() is then Tk's answer to satl's
# my_window.ok.
import json
import os
import re
import subprocess
import sys
import threading
import tkinter as tk
from fractions import Fraction


class py_18_vars:
    root = None                                    # the one Tk every object's window sits on

    def __init__(self):
        if py_18_vars.root is None:
            py_18_vars.root = tk.Tk()
            py_18_vars.root.withdraw()
        self.my_binary_number = 0b10101010         # Python's binary is an int written in base 2
        self.my_color = 0xABCDEF
        self.my_float = 0.0
        self.my_fraction = Fraction(1, 2)
        self.my_hex_code = 0x000000                # and its hex an int written in base 16
        self.my_infinite_number = float("inf")     # a float: Python's int has no infinity
        self.my_number = 111
        self.my_percent = 99.00                    # in percent: 99.00 is 99%
        self.my_program = ["echo", "hello, world!"]
        self.my_str = "void"
        self.my_thread = None
        self.my_window = tk.Toplevel(py_18_vars.root)
        self.my_window.title("hello_title")
        self.my_window.geometry("800x600")
        py_18_vars.root.update_idletasks()
        self.my_bool = False
        self.my_bash = "echo hello from bash"
        self.my_list = [1, 2, 3]
        self.my_map = {}
        self.my_multiple = "void"                  # a str or an int, as satellite.container.multiple

        # THE FILE, as satl's constructor does it: a text file is a list of lines, read when it is
        # opened and written when it is closed. There already: emptied, then opened (read). Not
        # there: made new -- "x" refuses to write over a file, as satellite.file.new does.
        if os.path.exists("something.txt"):
            open("something.txt", "w").close()                     # satellite.file.clear
            with open("something.txt") as read:                    # satellite.file.open
                self.my_file = read.read().splitlines()
        else:
            open("something.txt", "x").close()                     # satellite.file.new
            self.my_file = []

    def binary_number(self):
        self.show(self.my_binary_number + 0b10101010)                   # 340

    def use_color(self):
        self.show(self.foreground("hello, world!", self.my_color))      # hello, world!
        self.show(f"x{self.my_color:06X}")                              # xABCDEF

    def write_to_file(self):
        self.my_file.append("hello, world!")
        with open("something.txt", "w") as written:                     # .close()
            written.write("\n".join(self.my_file) + "\n")
        with open("something.txt") as read:                             # .open()
            self.my_file = read.read().splitlines()
        self.show(self.my_file[0])                                      # hello, world!

    def add_to_float(self):
        to_add = 9.9
        self.my_float = self.my_float + to_add
        self.show(f"my_float: {self.my_float}")                         # my_float: 9.9

    def add_to_fraction(self):
        to_add = Fraction(1, 2)
        # satl cannot add two fractions yet, so neither does this: they are compared
        self.show(self.my_fraction == to_add)                           # true
        self.show(f"my_fraction: {self.my_fraction}")                   # my_fraction: 1/2

    def change_hex_code(self):
        self.my_hex_code = 0x0000FFCC
        self.show(f"hex code: x{self.my_hex_code:08X}")                 # hex code: x0000FFCC

    def add_infinity(self):
        self.show(f"infinite_number: {self.infinity(self.my_infinite_number)}")  # (infinity)
        to_add = float("inf")
        # satl has no sums on an infinity yet (Python has), so this does what satl does: makes it negative
        self.my_infinite_number = -to_add
        self.show(f"infinite_number: {self.infinity(self.my_infinite_number)}")  # (-infinity)

    def add_number(self):
        self.my_number = self.my_number + 100000000000
        self.show(f"my_number: {self.my_number}")                       # my_number: 100000000111

    def add_percent(self):
        to_add = 99.1
        self.my_percent = self.my_percent + to_add
        self.show(f"my_percent: {self.my_percent:g}%")                  # my_percent: 198.1%

    def run_program(self):
        self.show(self.run(self.my_program))                            # hello, world!, then 0

    def display_str(self):
        self.show(f"my_str: {self.my_str}")                             # my_str: void

    def start_thread(self):
        self.my_thread = threading.Thread(target=self.binary_number)
        self.my_thread.start()
        self.my_thread.join()                                           # 340

    def open_window(self):
        self.show(self.my_window.winfo_ismapped() == 1)                 # true
        self.my_window.destroy()
        self.show('(closed window "hello_title")')                      # (closed window "hello_title")

    def flip_bool(self):
        self.my_bool = self.my_number > 1000
        self.show(f"my_bool: {'true' if self.my_bool else 'false'}")    # my_bool: true

    def run_bash(self):
        self.show(self.run(["bash", "-c", "--", self.my_bash]))         # hello from bash, then 0

    def add_to_list(self):
        self.my_list.append(4)
        self.show("{" + ", ".join(str(n) for n in self.my_list) + "}")  # {1, 2, 3, 4}

    def add_to_map(self):
        self.my_map["hello"] = 1
        self.show(json.dumps(self.my_map))                              # {"hello": 1}

    def change_multiple(self):
        self.my_multiple = 5
        self.show(self.my_multiple + 1)                                 # 6

    # WORDS satl HAS BUILT IN, which Python has to write out.

    def show(self, value):
        """satellite.console.display: true and false in lower case, and a colour's codes left out of
        anything that is not a terminal -- a pipe or a file -- as satl leaves them out."""
        if isinstance(value, bool):
            value = "true" if value else "false"
        text = str(value)
        if not sys.stdout.isatty():
            text = re.sub(r"\x1b\[[0-9;]*m", "", text)
        print(text)

    def foreground(self, text, rgb):
        """A string's .foreground(colour): the terminal's own 24-bit codes, in the string itself."""
        return f"\x1b[38;2;{rgb >> 16};{(rgb >> 8) & 0xFF};{rgb & 0xFF}m{text}\x1b[39m"

    def infinity(self, x):
        """satl shows an infinity as (infinity) and (-infinity); Python as inf and -inf."""
        return "(infinity)" if x > 0 else "(-infinity)"

    def run(self, words):
        """A satellite.variable.program's start() and join(): satl gives the program one pipe for
        its output and its errors, shows what comes through it, and answers the exit code."""
        finished = subprocess.run(words, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        print(finished.stdout, end="")
        return finished.returncode


def main():

    print(len(sys.argv))                                                # 1 -- the program counts

    counter = 0

    while counter < 10:
        local_object = py_18_vars()
        local_object.binary_number()
        local_object.use_color()
        local_object.write_to_file()
        local_object.add_to_float()
        local_object.add_to_fraction()
        local_object.change_hex_code()
        local_object.add_infinity()
        local_object.add_number()
        local_object.add_percent()
        local_object.run_program()
        local_object.display_str()
        local_object.start_thread()
        local_object.open_window()
        local_object.flip_bool()
        local_object.run_bash()
        local_object.add_to_list()
        local_object.add_to_map()
        local_object.change_multiple()
        counter = counter + 1

    py_18_vars.root.destroy()


if __name__ == "__main__":
    main()
