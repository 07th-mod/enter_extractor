import os

from bitstring import ConstBitStream

import conf
from common import process_chunk

from PyQt5.QtGui import QImage, qRgba, qAlpha, qRed, qGreen, qBlue

#checks which regions were actually used by the decoder
class RegionChecker:
  def __init__(self, size):
    self.bitmap = [False] * size

  #mark a region. The 'end' index is included
  def add(self, first_index, last_index):
    for i in range(first_index, last_index + 1):
      self.bitmap[i] = True

    print("added region: [{},{}]".format(first_index, last_index))

  def get_regions(self):
    in_unused_region = False
    start_marker = None
    for i, value in enumerate(self.bitmap):
      if not in_unused_region and value == False:
        in_unused_region = True
        start_marker = i
      elif in_unused_region and value == True:
        in_unused_region = False
        print('size: {} start: {} end: {}'.format(i - start_marker, start_marker, i))

    if in_unused_region:
      print('size: {} start: {} end: {}'.format(i - start_marker, start_marker, i))


# A color we can draw over no matter what, distinct from 0x00000000.
# I haven't checked to see if this is actually used by any images, but hot pink seems unlikely.
TRANSPARENT_COLOR = 0x00FF0080
BLACK = 0xFF000000
RED = 0xFFFF0000

################################################################################

def blit(src, dst, x, y, masked):
  w = src.width()
  h = src.height()
  
  out = dst.copy()
  for i in range(w):
    for j in range(h):
      dst_pixel = dst.pixel(x + i, y + j)
      src_pixel = src.pixel(i, j)
      
      # If src has transparency data, we use it, otherwise, we borrow from dst.
      # This logic doesn't quite work for bustup/l/si4?
      if masked or dst_pixel == TRANSPARENT_COLOR:
        out_pixel = src_pixel
      else:
        out_pixel = qRgba(qRed(src_pixel), qGreen(src_pixel), qBlue(src_pixel), qAlpha(dst_pixel))
      
      out.setPixel(x + i, y + j, out_pixel)
  
  return out


#OK, so what they've done this time around is the following:
# - For the base image, the image has BLACK wherever the facial expression should go. This is important
# - For the facial expressions and the mouth images, the images are BLACK wherever the below layer color should be taken
# The reason the base image is BLACK where the facial expression should go is that, if the facial expression or mouth image
# was meant to be BLACK, it would be drawn as transparent (the wrong color). But because the base expression is already
# black at that pixel, the end result will still be BLACK.
# You can examine this by enabling debug mode
def blit_switch(src, dst, x, y, masked, black_is_transparent):
  w = src.width()
  h = src.height()

  out = dst.copy()
  for i in range(w):
    for j in range(h):
      dst_pixel = dst.pixel(x + i, y + j)
      src_pixel = src.pixel(i, j)

      if dst_pixel == TRANSPARENT_COLOR:
        out_pixel = src_pixel
      elif black_is_transparent and src_pixel == BLACK:
        out_pixel = dst_pixel
      else:
        out_pixel = src_pixel
      # elif masked:
      #   out_pixel = src_pixel
      # else:
      #   out_pixel = qRgba(qRed(src_pixel), qGreen(src_pixel), qBlue(src_pixel), qAlpha(dst_pixel))

      out.setPixel(x + i, y + j, out_pixel)

  return out

################################################################################

def convert_bup(filename, out_dir):
  filesize = None
  with open(filename, 'rb') as f:
    test = f.read()
    filesize = len(test)
    print("File is of size: ", filesize)

  rc = RegionChecker(filesize)

  data = ConstBitStream(filename = filename)

  out_template = os.path.join(out_dir, os.path.splitext(os.path.basename(filename))[0])

  bytePosStart = data.bytepos

  magic   = data.read("bytes:4")

  if conf.SWITCH:
    unk_header_1 = data.read("uintle:32")

  size    = data.read("uintle:32")
  ew      = data.read("uintle:16")
  eh      = data.read("uintle:16")
  width   = data.read("uintle:16")
  height  = data.read("uintle:16")
  
  tbl1    = data.read("uintle:32")

  base_chunks = data.read("uintle:32") #num expressions in switch version
  exp_chunks  = data.read("uintle:32") #unnknown on switch version

  if conf.SWITCH:
    unk_header_2 = data.read("uintle:32") #unknown in switch version

  rc.add(bytePosStart, data.bytepos)

  
  if conf.debug:
    print('---------- BUP HEADER -----------')
    print("magic:     ", magic)
    if conf.SWITCH:
      print("unk1:      ", unk_header_1)
    print("size:      ", size)
    print("EW, EH:    ", ew, eh)
    print("Width:     ", width)
    print("Height:    ", height)
    print("tbl1:      ", tbl1)
    print("basechunks:", base_chunks)
    print("exp_chunks:", exp_chunks)
    print("Section ends at {}{}".format(hex(base_chunks), hex(exp_chunks)))
    if conf.SWITCH:
      print("Unk2:      ", unk_header_2)
    print()

  if conf.SWITCH:
    skip_amount = 32 * 3
  else:
    skip_amount = 32

  # Dunno what this is for.
  print("Skipping {} bits in the tbl1 header".format(tbl1 * skip_amount))
  bytePosStart = data.bytepos
  for i in range(tbl1):
    print(data.read(skip_amount))
  rc.add(bytePosStart, data.bytepos)

  # for i in range(20):
  #   print(data.read("uintle:32"))

  base = QImage(width, height, QImage.Format_ARGB32)

  # if conf.SWITCH:
  #   base.fill(0)
  # else:
  base.fill(TRANSPARENT_COLOR)

  for i in range(base_chunks):
    try:
        print('--------- Decoding Base Chunk [{}]----------'.format(i))
        offset = data.read("uintle:32")
        print("Testing offset", offset)

        if conf.SWITCH:
          bothChunkSize = data.read("uintle:32")
          print("bothChunkSize", bothChunkSize)

        rc.add(offset, offset + bothChunkSize - 1)

        temp_pos = data.bytepos
        chunk, x, y, masked = process_chunk(data, offset, bothChunkSize)
        if conf.debug_extra:
          chunk.save("%s_%d_Base_Chunk_%s.png" % (out_template, i, str(masked)))


        if conf.SWITCH:
          base = blit_switch(chunk, base, x, y, masked, False)
        else:
          base = blit(chunk, base, x, y, masked)

    except Exception as e:
      print("FAILED:")
      data.bytepos = temp_pos
      print(e)

  if conf.debug:
    base.save("%s_BASE.png" % (out_template))

  #seems to be a padding of 3 * 4 = 12 bytes before the start of the expression section
  bytePosStart = data.bytepos
  if conf.SWITCH:
    skip_amount = 32 * 3
    # Dunno what this is for.
    print("Skipping {} bytes before the expression header".format(skip_amount/8))
    print(data.read(skip_amount))

  rc.add(bytePosStart, data.bytepos)

  # rc.get_regions()
  # return

  for i in range(exp_chunks):
    print('--------- Decoding Expression Chunk [{}]----------'.format(i))
    # if conf.SWITCH:
    #   unka = data.read("uintle:32")
    #   unkb = data.read("uintle:32")
    #   unkc = data.read("uintle:32")
    bytePosStart = data.bytepos
    if conf.SWITCH:
      name_bytes       = data.read("bytes:20")
    else:
      name_bytes       = data.read("bytes:16")

    name = name_bytes.strip(b'\0').decode("CP932")

    face_off   = data.read("uintle:32")

    if conf.SWITCH:
      face_size  = data.read("uintle:32")

    if conf.SWITCH:
      #there appear to be 24 zero bytes here always...not sure if i'm correct on this
      zero_bytes = data.read("bytes:24")
      for b in zero_bytes:
        if b != 0:
          print("WARNING: Non-zero byte found in probably 24 zero padding bytes in character expression header")
      print("Zero padding bytes: ", zero_bytes)
    else:
      unk1       = data.read("uintle:32")
      unk2       = data.read("uintle:32")
      unk3       = data.read("uintle:32")


    mouth1_off = data.read("uintle:32")
    if conf.SWITCH:
      mouth1_size = data.read("uintle:32")

    mouth2_off = data.read("uintle:32")
    if conf.SWITCH:
      mouth2_size = data.read("uintle:32")

    mouth3_off = data.read("uintle:32")
    if conf.SWITCH:
      mouth3_size = data.read("uintle:32")

    rc.add(bytePosStart, data.bytepos)

    print('---------- Expression HEADER -----------')
    print("name:     [{}]".format(name))
    print("face_off:      ", face_off)
    print("face_size: ", face_size)

    if conf.SWITCH:
      print("mouth1_size: ", mouth1_size)
      print("mouth2_size: ", mouth2_size)
      print("mouth3_size: ", mouth3_size)
    else:
      print("unk1: ", unk1)
      print("unk2: ", unk2)
      print("unk3: ", unk3)

    print("mouth1_off:", mouth1_off)
    print("mouth2_off:", mouth2_off)
    print("mouth3_off:", mouth3_off)
    print()
    
    if not face_off:
      base.save("%s_%s.png" % (out_template, name))
      # base.save("%s.png" % (out_template))
      # return
      continue

    rc.add(face_off, face_off + face_size - 1)
    rc.add(mouth1_off, mouth1_off+ mouth1_size- 1)
    rc.add(mouth2_off, mouth2_off+ mouth2_size- 1)
    rc.add(mouth3_off, mouth3_off+ mouth3_size- 1)
    face, x, y, masked = process_chunk(data, face_off, forceColorTable0ToBlackTransparent=True)
    if conf.debug:
      face.save("%s_%s_FACE_%s.png" % (out_template, name, str(masked)))

    if conf.SWITCH:
      exp_base = blit_switch(face, base, x, y, masked, True)
    else:
      exp_base = blit(face, base, x, y, masked)

    # exp_base.save("test/%s.png" % name)
    # exp_base.save("%s_%s.png" % (out_template, name))
    # face.save("%s_%sf.png" % (out_template, name))
    # break
    
    for j, mouth_off in enumerate([mouth1_off, mouth2_off, mouth3_off]):
      if not mouth_off:
        continue

      mouth, x, y, masked = process_chunk(data, mouth_off, forceColorTable0ToBlackTransparent=True)
      
      if not mouth:
        continue
      if conf.SWITCH:
        exp = blit_switch(mouth, exp_base, x, y, masked, True)
      else:
        exp = blit(mouth, exp_base, x, y, masked)
      
      exp.save("%s_%s_%d.png" % (out_template, name, j))

      if conf.debug:
        mouth.save("%s_%s_%d_MOUTH_%s.png" % (out_template, name, j, str(masked)))

  rc.get_regions()

################################################################################
