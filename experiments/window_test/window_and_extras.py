# window_and_extras.py -- satl_18_vars as plain Python, for race_windows.cpp: the same 18
# variables, one method for each of the satl file's capsules, the object made ten times.
#
# Each object makes its own Tk -- a whole Tcl interpreter and display connection apiece, where satl
# starts its GTK once. Nothing here calls update_idletasks() or mainloop(), so Tk never puts these
# windows on the screen (checked 2026-10-03: 0 of 10 mapped), while satl's ten are drawn.
import subprocess
import sys
import threading
import tkinter as tk
from fractions import Fraction


class py_18_vars:
    def __init__(self):
        self.my_binary_number = 0b10101010
        self.my_color = 0xABCDEF
        self.my_file = open("something.txt", "w")
        self.my_float = 0.0
        self.my_fraction = Fraction(1, 2)
        self.my_hex_code = 0x000000
        self.my_infinite_number = float("inf")
        self.my_number = 111
        self.my_percent = 99.00
        self.my_program = ["echo", "hello, world!"]
        self.my_str = "void"
        self.my_thread = None
        self.my_window = tk.Tk()
        self.my_bool = False
        self.my_bash = "echo hello from bash"
        self.my_list = [1, 2, 3]
        self.my_map = {}
        self.my_multiple = "void"

    def binary_number(self):
        print(self.my_binary_number + 0b10101010)

    def use_color(self):
        print(f"\033[38;2;{self.my_color >> 16};{(self.my_color >> 8) & 0xFF};{self.my_color & 0xFF}mhello, world!\033[0m")
        print(hex(self.my_color))

    def write_to_file(self):
        self.my_file.write("hello, world!\n")
        self.my_file.close()
        self.my_file = open("something.txt")
        print(self.my_file.readline(), end="")

    def add_to_float(self):
        to_add = 9.9
        self.my_float = self.my_float + to_add
        print("my_float:", self.my_float)

    def add_to_fraction(self):
        to_add = Fraction(1, 2)
        print(self.my_fraction == to_add)           # compared, as the satl file does
        print("my_fraction:", self.my_fraction)

    def change_hex_code(self):
        self.my_hex_code = 0x0000FFCC
        print("hex code:", hex(self.my_hex_code))

    def add_infinity(self):
        print("infinite_number:", self.my_infinite_number)
        to_add = float("inf")
        self.my_infinite_number = -to_add           # made negative, as the satl file does
        print("infinite_number:", self.my_infinite_number)

    def add_number(self):
        self.my_number = self.my_number + 100000000000
        print("my_number:", self.my_number)

    def add_percent(self):
        to_add = 99.1
        self.my_percent = self.my_percent + to_add
        print(f"my_percent: {self.my_percent}%")

    def run_program(self):
        print(subprocess.run(self.my_program).returncode)

    def display_str(self):
        print("my_str:", self.my_str)

    def start_thread(self):
        self.my_thread = threading.Thread(target=self.binary_number)
        self.my_thread.start()
        self.my_thread.join()

    def open_window(self):
        print(self.my_window.winfo_exists())
        self.my_window.destroy()
        print(self.my_window)

    def flip_bool(self):
        self.my_bool = self.my_number > 1000
        print("my_bool:", self.my_bool)

    def run_bash(self):
        print(subprocess.run(["bash", "-c", self.my_bash]).returncode)

    def add_to_list(self):
        self.my_list.append(4)
        print(self.my_list)

    def add_to_map(self):
        self.my_map["hello"] = 1
        print(self.my_map)

    def change_multiple(self):
        self.my_multiple = 5
        print(self.my_multiple + 1)


def main():

    print(len(sys.argv))

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


if __name__ == "__main__":
    main()
