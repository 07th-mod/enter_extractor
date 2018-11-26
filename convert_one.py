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

#
# input = r'C:\temp\buptest\kei1.bup'
# input = r'C:\temp\buptest\ri1.bup'
# input = r'c:\temp\buptest\ha1.bup'
# input = r'c:\temp\buptest\hnit1a.bup'
# input = r'c:\temp\buptest\ta1.bup'
# input = r'c:\temp\buptest\miyo1.bup'
# input = r'c:\temp\buptest\miyuki.bup'


# input = r'c:\temp\buptest\renasen1.bup'

# input = r'C:\temp\buptest\sui_aks2.bup'
# conf.SWITCH = False

# output = r'C:\temp\buptest\out'

# convert_pic(sys.argv[1], sys.argv[2])
# convert_bup(input, output)