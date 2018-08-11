import os
import sys

import conf
from pic import convert_pic
from bup import convert_bup

input = sys.argv[1]
output = sys.argv[2]

_, input_extension = os.path.split(input)

if input_extension == '.pic':
  convert_pic(input, output)
else:
  convert_bup(input, output)

