# windows_10.py -- tkinter's ten windows, the twin of windows_10.satl. Each window is made, mapped
# and closed before the next one, and every line it prints is the line satl prints.
#
# One Tk, withdrawn so it is never shown (satl likewise opens its one GTK "desk" the first time a
# window word runs), and each window a Toplevel on it. update_idletasks() is Tk's "show it now" --
# it maps the window and waits for the window manager to say it is mapped -- and winfo_ismapped()
# is then Tk's answer to satl's my_window.ok.
import tkinter as tk


def main():

    root = tk.Tk()
    root.withdraw()

    for i in range(1, 11):
        my_window = tk.Toplevel(root)
        my_window.title(f"window {i}")
        my_window.geometry("800x600")
        root.update_idletasks()
        print("true" if my_window.winfo_ismapped() else "false")
        my_window.destroy()
        print(f'(closed window "window {i}")')

    root.destroy()


if __name__ == "__main__":
    main()
