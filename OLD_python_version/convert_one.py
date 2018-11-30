import os
import sys

import conf
from pic import convert_pic
from bup import convert_bup


print("WARNING - The python version of the decoder has been depreciated!!!! You should use the C++ version instead!")
print("Press enter key to continue anyway. If you want to run this script multiple times, you'll need to delete the below readline.")
sys.stdin.readline()

input = sys.argv[1]
output = sys.argv[2]

_, input_extension = os.path.splitext(input)

if input_extension == '.pic':
  convert_pic(input, output)
else:
  convert_bup(input, output)

#
# input = r'C:\tempextract\extract\bustup\l\kei1.bup'
# input = r'C:\tempextract\extract\bustup\l\ri1.bup'
# input = r'C:\tempextract\extract\bustup\l\ha1.bup'
# input = r'C:\tempextract\extract\bustup\l\hnit1a.bup'
# input = r'C:\tempextract\extract\bustup\l\ta1.bup'
# input = r'C:\tempextract\extract\bustup\l\miyo1.bup'
# input = r'C:\tempextract\extract\bustup\l\miyuki.bup'


# input = r'C:\tempextract\extract\bustup\l\sa1a.bup'

# input = r'C:\tempextract\extract\bustup\l\renasen1.bup'

# input = r'C:\tempextract\extract\bustup\l\sui_aks2.bup'
# conf.SWITCH = False

# output = r'c:\tempextract\out112018'

# convert_pic(sys.argv[1], sys.argv[2])
# print("converting {} to {}".format( input, output))
# convert_bup(input, output)