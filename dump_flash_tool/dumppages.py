# pylint: disable=invalid-name
# pylint: disable=line-too-long
import sys
import flashimage
from os.path import exists

use_ansi = False

# output filename


# initial
#start_page = 0
#end_page = 131070 #16000 # last 131070
# 0=0x800 1=0x400 to see if same data, 2=read per byte len=0x01 + slow => WORKS
target_dir = './fixed_pages/' 

# idme 1536-2048
#start_page = 1536
#end_page = 2048 
#target_dir = './idme/'

# open flash
flash_image_io = flashimage.IO() 

if not flash_image_io.is_initialized():
    print('Device not ready, aborting...')
    sys.exit(0)

# loop over pages
#print('extract from pages(0x%x - 0x%x)' % (start_page, end_page))    

fix_pages = [94590]

#for pageno in range (start_page, end_page):
for pageno in fix_pages:
    # make name for file
    target_file = target_dir + str(pageno) + '.bin'
    # check file exists
    if not exists(target_file):
        # if not, read_page with all checks, balances and oob
        data = flash_image_io.read_page(pageno)
        # ecc?

        # check size
        if data:
            # if ok, save to file
            with open(target_file, 'wb') as wfd:
                wfd.write(data)
                wfd.close()

print('done')