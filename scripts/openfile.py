import tkinter as tkinter
from tkinter import filedialog
import sys

root = tkinter.Tk()
root.withdraw()

files = [('JSON Documment', '*.json')] 
path = sys.argv[1]

filepath = filedialog.askopenfilename(initialdir = path, filetypes = files)

text_file = open("../temp/filename.data", "w")
text_file.write(filepath)
text_file.close()

sys.stdout.write(filepath)
sys.stdout.flush()
sys.exit(0)