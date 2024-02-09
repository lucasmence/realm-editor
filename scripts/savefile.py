import tkinter as tkinter
from tkinter import filedialog
import sys
import os


root = tkinter.Tk()
root.withdraw()

files = [('JSON Documment', '*.json')] 

path = sys.argv[1]
name = sys.argv[2]

filepath = filedialog.asksaveasfile(initialdir = path, initialfile = name, filetypes = files, defaultextension = files) 

text_file = open("../temp/savefile.data", "w")
text_file.write(filepath.name)
text_file.close()

os.remove(filepath)

sys.stdout.write(filepath.name)
sys.stdout.flush()
sys.exit(0)

