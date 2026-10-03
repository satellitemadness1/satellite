# windows_10.py -- tkinter's ten windows, the twin of windows_10.satl. Each window is made, mapped
# and closed before the next one, and every line it prints is the line satl prints.
#
# One Tk, withdrawn so it is never shown (satl likewise opens its one GTK "desk" the first time a
# window word runs), and each window a Toplevel on it. winfo_exists() asks what satl's my_window.ok
# asks: is it closed yet.
#
# NOT THE SAME WORK, measured 2026-10-03 on a headless mutter: update_idletasks() is Tk's "show it
# now", and it WAITS until the window manager says the window is mapped -- 7 to 8 ms a window, over
# half of CPython's whole run. satl's windows here are closed before their first frame is drawn, so
# they never show at all, and nothing waits for them.
import tkinter as tk


def main():

    root = tk.Tk()
    root.withdraw()

    for i in range(1, 11):
        my_window = tk.Toplevel(root)
        my_window.title(f"window {i}")
        my_window.geometry("800x600")
        root.update_idletasks()
        print("true" if my_window.winfo_exists() else "false")
        title = my_window.title()
        my_window.destroy()
        print(f'({"closed window" if not my_window.winfo_exists() else "window"} "{title}")')

    root.destroy()


if __name__ == "__main__":
    main()
