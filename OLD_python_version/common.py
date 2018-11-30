import os

from bitstring import ConstBitStream
from decompress import decompress
from tools import *

from PyQt5.QtGui import QImage, qRgba, qRed, qGreen, qBlue, qAlpha

import conf

################################################################################
# I have no idea why.
def adjust_w(w):
  adj_w = w + 1

  if adj_w % 4:
    adj_w = int(round(adj_w / 4.0) * 4)
  
  return adj_w

################################################################################

def get_rgba(data, w, h, crop = True):
  image = adjust_scanline(data, w)
  image = QImage(image, w, h, QImage.Format_ARGB32)
  if crop:
    image = image.copy(0, 0, w - 2, h - 2)
  return image

################################################################################

def get_grayscale(data, w, h, crop = True):
  table = []
  for i in range(256):
    table.append((255 << 24) | (i << 16) | (i << 8) | i)
  image = QImage(data, w, h, QImage.Format_Indexed8)
  image.setColorTable(table)
  image = image.convertToFormat(QImage.Format_ARGB32)
  
  if crop:
    image = image.copy(0, 0, w - 2, h - 2)
  return image

################################################################################

def get_indexed(data, w, h, colors = 256, crop = True, forceColorTable0ToBlackTransparent = False):
  if conf.debug_extra and not os.path.isdir(conf.chunk_output_directory):
    print("You have chosen extra debug output, but the 'chunk_output_directory' in the 'conf.py' file is not set to a valid directory. Please set it and run the program again.")
    exit(-1)

  all_colors = set()

  palette = data[:colors * 4]
  table   = []
  for i in range(colors):
    r = palette[(i * 4) + 0]
    g = palette[(i * 4) + 1]
    b = palette[(i * 4) + 2]
    a = palette[(i * 4) + 3]
    color = qRgba(r, g, b, a)
    table.append(color)

    # if color == 0xFF000000:
    #   print("black index is", i)

    # if color in all_colors:
    #   print("WARNING {}: color already exists at value {}!".format(i, hex(color)))
    #   print("original index was {}".format(table.index(color)))

    all_colors.add(color)

  img_start  = colors * 4

  mask_start = img_start + (w * h)
  image = data[img_start:mask_start]
  indexed_image_raw_data = data[img_start:mask_start]
  image_size = len(image)
  image = QImage(image, w, h, QImage.Format_Indexed8)

  if conf.debug_extra:
    save_filename = r"C:\tempextract\mask\indexed_{}.png".format(conf.debug_count)
    print('saving: ', save_filename)
    image.save(save_filename)
  # conf.debug_count += 1

  # if forceColorTable0ToBlackTransparent:
  #   table[0] = 0
  #   table[1] = 0
  #   table[2] = 0
  #   table[3] = 0

  image.setColorTable(table)
  image = image.convertToFormat(QImage.Format_ARGB32)
  
  mask = data[mask_start:]

  if conf.debug_extra and mask:
    print('width, height: {},{}, size: {}', w, h, len(mask))
    mask_image = QImage(mask, w, h, QImage.Format_Grayscale8)
    save_filename = r"C:\tempextract\mask\mask_{}.png".format(conf.debug_count)
    print('saving: ', save_filename)
    mask_image.save(save_filename)
    conf.debug_count += 1

  # if conf.debug:
  #   print("data is size: ", len(data))
  #   print("palette is size: ", len(palette))
  #   print("image start: ", img_start)
  #   print("mask_start: ", mask_start)
  #   print("image_size: ", image_size)
  #   print("mask_size: ", len(mask))

  if len(palette) + image_size + len(mask) != len(data):
    print("Warning: not all data in chunk was used!")
  for i in range(len(mask)):
    x = i % w
    y = i / w
    pixel = image.pixel(x, y)

    # if indexed_image_raw_data[i] == 0:
    #   # print("WARNING: non 100% alpha value! Value is: {}".format(a))
    #   # print("color is: ", hex(color))
    #   pixel = qRgba(0xFF, 0, 0, 0xFF)  # qAlpha(pixel))
    # else:
    pixel = qRgba(qRed(pixel), qGreen(pixel), qBlue(pixel), mask[i]) #qAlpha(pixel))

    image.setPixel(x, y, pixel)


  
  if crop:
    image = image.copy(0, 0, w - 2, h - 2)
  return image, len(mask) > 0

################################################################################

def process_chunk(data, offset, bothChunkSize=None, forceColorTable0ToBlackTransparent=False):
  if offset > data.len / 8:
    return None, 0, 0
  
  temp_pos = data.bytepos
  data.bytepos = offset
  
  chunk_type = data.read("uintle:16")
  unk0  = data.read("uintle:16")
  
  unk1a = data.read("uintle:16")
  unk1b = data.read("uintle:16")
  x     = data.read("uintle:16")
  y     = data.read("uintle:16")
  w     = data.read("uintle:16")
  h     = data.read("uintle:16")
  
  size  = data.read("uintle:32")
  unk3a = data.read("uintle:16")
  unk3b = data.read("uintle:16")
  
  unk4a = data.read("uintle:16")
  unk4b = data.read("uintle:16")
  
  unk5a = data.read("uintle:16")
  unk5b = data.read("uintle:16")

  if conf.debug:
    print("Offset:    ", offset)
    print("Type, Unk0:", chunk_type, unk0)
    print("Unk1:      ", unk1a, unk1b)
    print("X, Y:      ", x, y)
    print("W, H:      ", w, h)
    print("Size:      ", size)
    print("Unk3:      ", unk3a, unk3b)
    print("Unk4:      ", unk4a, unk4b)
    print("Unk5:      ", unk5a, unk5b)
    print()
  
  # If size is zero, then we're uncompressed.
  if size == 0:
    # This is so fucking hacky, but I don't care.
    if chunk_type == 3 or chunk_type == 2:
      if ((chunk_type == 0 or chunk_type == 2) and (unk0 > 0 or not (unk5a == 0x00 and unk5b == 0x00))) or ((chunk_type == 1 or chunk_type == 3) and (unk0 > 0 and not (unk5a == 0x00 and unk5b == 0x00))):
        while True:
          data.read(12 * 8)
          flag = data.read("uintle:32")
          if flag == 0x00:
            break
      
      # 1024 for the palette, plus one for each pixel.
      size = 1024 + (w * h)
      chunk = bytearray(data.read(size * 8).bytes)
      image, masked = get_indexed(chunk, w, h)
      data.bytepos = temp_pos
      return image, x, y, masked
    
    data.bytepos = temp_pos
    return None, 0, 0
  
  # This is so fucking hacky, but I don't care.
  # if (chunk_type == 0 and unk0 > 0) or (chunk_type == 2):
  if ((chunk_type == 0 or chunk_type == 2) and (unk0 > 0 or not (unk5a == 0x00 and unk5b == 0x00))) or ((chunk_type == 1 or chunk_type == 3) and (unk0 > 0 and not (unk5a == 0x00 and unk5b == 0x00))):
    data.bytepos += 16
    while True:
      cur_pos = data.bytepos
      bytesToRead = size * 8
      print("trying to use {} bytes from pos {}".format(bytesToRead, cur_pos))
      comp = data.read(bytesToRead)
      try:
        dec = decompress(comp)
        if conf.debug:
          print("Decoded Chunk Size: {}".format(len(dec)))
      except:
        # data.bytepos = cur_pos + 16
        data.bytepos = cur_pos
        while True:
          data.read(12 * 8)
          flag = data.read("uintle:32")
          if flag == 0x00:
            break
        continue
      break
    # dump_bs_to_file(comp)
    # dump_to_file(dec, "temp2.dat")
      
  else:
    comp = data.read(size * 8)
    dec = decompress(comp)
  
  adj_w = adjust_w(w)
  
  # Indexed image.
  if chunk_type == 3 or chunk_type == 2:
    print("using indexed mode")
    chunk, masked = get_indexed(dec, adj_w, h, forceColorTable0ToBlackTransparent=forceColorTable0ToBlackTransparent)
  else:
    print("using RGBA Mode")
    chunk = get_rgba(dec, adj_w, h)
    masked = True

  # chunk_type = data.read("uintle:16")
  # unk0 = data.read("uintle:16")
  #
  # unk1a = data.read("uintle:16")
  # unk1b = data.read("uintle:16")
  # x = data.read("uintle:16")
  # y = data.read("uintle:16")
  # w = data.read("uintle:16")
  # h = data.read("uintle:16")
  #
  # size = data.read("uintle:32")
  # unk3a = data.read("uintle:16")
  # unk3b = data.read("uintle:16")
  #
  # unk4a = data.read("uintle:16")
  # unk4b = data.read("uintle:16")
  #
  # unk5a = data.read("uintle:16")
  # unk5b = data.read("uintle:16")
  #
  # if conf.debug:
  #   print("----extra chunk-----")
  #   print("Offset: N/A, assume comes after previous chunk")
  #   print("Type, Unk0:", chunk_type, unk0)
  #   print("Unk1:      ", unk1a, unk1b)
  #   print("X, Y:      ", x, y)
  #   print("W, H:      ", w, h)
  #   print("Size:      ", size)
  #   print("Unk3:      ", unk3a, unk3b)
  #   print("Unk4:      ", unk4a, unk4b)
  #   print("Unk5:      ", unk5a, unk5b)
  #   print()


  print("chunk ends at: ", data.bytepos)

  # amountToRead = (bothChunkSize - size)
  #
  # print("processing extra chunk of size: ", amountToRead)
  #
  #
  #
  # comp = data.read(amountToRead * 8)
  # dec = decompress(comp)
  
  data.bytepos = temp_pos
  return chunk, x, y, masked

################################################################################

def adjust_scanline(data, w):
  scanline = 4*(((w)+3)&0xfffc)
  
  for i in range(scanline, len(data)):
    data[i] = (data[i] + data[i - scanline]) % 256
  
  return data

################################################################################