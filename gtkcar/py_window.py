import tkinter as tk

class tk_window():
    def __init__(self):
        self.the_window = tk.Tk()
        self.the_window.title("Tkinter Scrolling Test")
        self.the_window.geometry("400x250")

    def add_text(self):
        self.the_window.mainloop()
        self.the_window.destroy()

def main():
    my_window = tk_window()

    my_window.add_text()

if __name__ == "__main__":
    main()