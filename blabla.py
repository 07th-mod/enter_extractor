import sys
from sys import argv
import struct
import os

def hexdump(src, length=16, sep='.'):
    FILTER = ''.join([(len(repr(chr(x))) == 3) and chr(x) or sep for x in range(256)])
    lines = []
    for c in range(0, len(src), length):
        chars = src[c:c+length]
        hex = ' '.join(["%02x" % ord(x) for x in chars])
        if len(hex) > 24:
            hex = "%s %s" % (hex[:24], hex[24:])
        printable = ''.join(["%s" % ((ord(x) <= 127 and FILTER[ord(x)]) or sep) for x in chars])
        lines.append("%08x:  %-*s  |%s|\n" % (c, length*3, hex, printable))
    print(( ''.join(lines)))

def u8(b, off=0):
    return b[off]

def u16(b, off=0):
    return struct.unpack("<H", b[off:off+2])[0]

def u32(b, off=0):
    return struct.unpack("<I", b[off:off+4])[0]

def c_str(b, off=0):
    part = b[off:]
    term = part.find(b"\x00")
    if term == -1:
        print("full dump:")
        hexdump(b)
        print("part dump (offset 0x{:x})".format(off))
        hexdump(part)
        raise Exception("failed to find C-string here")
    return part[:term].decode('utf-8')


folder_map = dict()


class File():

    def __init__(self, data, parent_data):
        name_off = u32(data[0:3] + b"\x00")
        self.name = c_str(parent_data, name_off)
        self.flags = u8(data, 3)
        self.offset = u32(data, 4)
        self.size = u32(data, 8)

    def __str__(self):
        return "name: {} flags: {} offset: {} size: {} offset: {} size: {}".format(self.name, self.flags, self.offset, self.size, hex(self.offset), hex(self.size))

class Info():

    def __init__(self, data):
        self.num_files = u32(data, 0)

        self.files = []
        for x in range(self.num_files):
            start = 4 + x * 0xC
            file_info = data[start:start+0xC]
            self.files.append(File(file_info, data))

        for file in self.files:
            if file.name == ".":
                folder_map[file.offset] = self
            elif file.name == "..":
                self.parent = folder_map[file.offset]

        print("Info object created with {} files, consisting of:".format(self.num_files))

        for x in self.files:
            print(str(x))

fin = None


def extract_folder(folder, path, is_rom1):
    address_shift_amount = None
    if is_rom1:
        address_shift_amount = 11 #equal to multiplying by 0x800, the old method
    else:
        address_shift_amount = 9  #rom2 stores addresses with a different shift amount

    os.mkdir(path)
    for file in folder.files:
        if file.name in [".", ".."]:
            continue
        print("{}/{}{}".format(path, file.name, "/" if file.flags & 0x80 else ""))
        if file.flags & 0x80:
            extract_folder(folder_map[file.offset], os.path.join(path, file.name), is_rom1)
        else:
            fin.seek(file.offset << address_shift_amount)
            fout = open(os.path.join(path, file.name), "wb")
            fout.write(fin.read(file.size))
            fout.close()

is_rom1 = True

def main():
    global fin
    fin = None

    if len(sys.argv) < 3:
        print("First argument must be rom file path. Second argument must be a folder where output files will be placed.\n For example, 'blabla.py files\abc.rom output_folder\abc'")
        exit(-1)

    inputFile = sys.argv[1]
    ouput_folder = sys.argv[2]

    fin = open(inputFile, "rb")
    first4bytes = fin.read(4)
    print(first4bytes)

    if first4bytes == b'ROM ':
        print("Extracting Rom Format 1")
        is_rom1 = True
    elif first4bytes == b'ROM2':
        print("Extracting Rom Format 2")
        is_rom1 = False
    else:
        raise Exception("Unknown 4 magic bytes at start of file")

    unk0 = (u16(fin.read(2)), u16(fin.read(2)))
    print('Unknown', unk0)

    header_size = u32(fin.read(4))
    print('headerSize', header_size)
    unk1 = None
    if is_rom1:
        unk1 = fin.read(4)
    else:
        unk1 = fin.read(20) #not sure if this is the correct offset!

    print('Unknown', unk1)

    folders = []

    while fin.tell() < header_size:
        start = fin.tell()
        seek_location = start + 0xC
        fin.seek(seek_location)
        print("Seeked to " + hex(seek_location))

        info_size = u32(fin.read(4))
        print("info size: {} ({} hex)".format(info_size, hex(info_size)))
        fin.seek(start)

        data = fin.read(info_size)

        # hexdump(data)
        info = Info(data)
        print('created info object')
        folders.append(info)
        
        end = start + info_size
        # align to 0x10
        end = (end + 0xF) & ~0xF
        fin.seek(end)

    print("EXTRACTION DISABLED - UNCOMMENT TO ENABLE")
    extract_folder(folders[0], path=ouput_folder, is_rom1=is_rom1)


if __name__ == "__main__":
    main()