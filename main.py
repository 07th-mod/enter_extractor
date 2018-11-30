#external dependencies:
# - Bitstring (Run pip install bitstring)
# - PyQt5 (Run pip install pyqt5 . Ignore any IDE warnings - this is because of pyqt5's wierd way of installing)
import os
import sys

from pic import convert_pic
from bup import convert_bup

def change_file_extension(path, new_extension):
    directories, filename_with_ext = os.path.split(path)
    filename_no_ext, extension = os.path.splitext(filename_with_ext)
    return os.path.join(directories, filename_no_ext + new_extension)

print("WARNING - The python version of the decoder has been depreciated!!!! You should use the C++ version instead!")
print("Press enter key to continue anyway. If you want to run this script multiple times, you'll need to delete the below readline.")
sys.stdin.readline()

if len(sys.argv) < 3:
    print("Wrong number of arguments - first argument is path to scan for .pic files, second argument is output folder. Folder structure will be kept.")
    exit(-1)

if len(sys.argv) > 3:
    print('more than 2 arguments provided - Using Debug mode')
    debug = True
else:
    debug = False

scan_dir = sys.argv[1]
output_dir = sys.argv[2]

for root, subFolders, files in os.walk(scan_dir):
    for file in files:
        fullPath = os.path.join(root, file)
        relPath = os.path.relpath(fullPath, scan_dir)
        filename, extension = os.path.splitext(file)

        #only process .pic files for now
        if extension != '.pic':
            continue

        input_pic_path = fullPath

        output_png_path  = os.path.join(output_dir, change_file_extension(relPath, '.png'))

        #make directories in output file path so convert_pic doesn't fail
        directories, filename_only = os.path.split(output_png_path)
        os.makedirs(directories, exist_ok=True)

        convert_pic(input_pic_path, output_png_path)
        print("Successfully saved [{}] to [{}]".format(input_pic_path, output_png_path))


