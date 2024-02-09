import tkinter as tkinter
from tkinter import filedialog
import sys

root = tkinter.Tk()
root.withdraw()

files = [('JSON Documment', '*.json')] 

path = sys.argv[1]
name = sys.argv[2]

filepath = filedialog.asksaveasfile(initialdir = path, initialfile = name, filetypes = files, defaultextension = files) 

sys.stdout.write(filepath.name)
sys.stdout.flush()
sys.exit(0)

