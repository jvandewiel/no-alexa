# extract and convert raw image data to useable files

import os
import sys
import numpy

sourceFile = 'dump_008.bin'
targetFile = '001.img'

# params
pageSize = 4096
oobSize = 256
pagesPerBlock = 64

# derived
blockSize = pageSize * pagesPerBlock
rawBlockSize = (pageSize + oobSize) * pagesPerBlock
rawPageSize = pageSize + oobSize
fileSize = os.path.getsize(sourceFile)

totalPages = int(fileSize / rawPageSize)    
totalBlocks = int(totalPages / pagesPerBlock)    

# files
in_file = open(sourceFile, 'rb')
#out_file = open(targetFile, 'ab')

mask = bytearray([0x8e, 0x21, 0x49, 0x9d, 0x9f, 0x3a, 0xa0, 0xf])


def init():
    try:
        in_file = open(sourceFile, 'rb')        
            
    except:
        print('Can\'t open a file:', sourceFile)
        return False
    
    print('PageSize: 0x%x' % pageSize)
    print('OOBSize: 0x%x' % oobSize )
    print('PagePerBlock: 0x%x' % pagesPerBlock)
    print('BlockSize: 0x%x' % blockSize)
    print('RawPageSize: 0x%x' % rawPageSize)
    print('FileSize: 0x%x' % fileSize)
    print('PageCount: 0x%x' % totalPages)        
    print('')

def get_block_offset(self, block):
    return block * rawBlockSize

def get_page_offset(pageno):
    return pageno * rawPageSize

def read_page(pageno, remove_oob = False):
    offset = get_page_offset(pageno)    
    if offset > fileSize:
        #print('Offset out of range:', offset)
        return b''

    in_file.seek(offset)
    print('offset: ', offset)
    if remove_oob:
        print('reading without oob')
        buf = in_file.read(pageSize)    
    else:
        print('reading with oob')
        buf = in_file.read(rawPageSize)
    # print first 8
    print_bytes(buf)
    return buf

def read_block(blockno, remove_oob = False):
    offset = get_page_offset(blockno * 64)    
    if offset > fileSize:
        return b''

    in_file.seek(offset)

    #print('Reading')
    blockBuf = in_file.read(blockSize)
    if remove_oob:
        pageBuf = bytearray(pageSize * pagesPerBlock)
        for page in range(0, pagesPerBlock):
            data = blockBuf[0,pageSize]
            pos = page * pageSize
            pageBuf[pos:pos + len(data)] = data
        return pageBuf

    return blockBuf 

def read_oob(pageno):
    in_file.seek(pageno*rawPageSize + pageSize)
    return in_file.read(oobSize)

init()
# fileSize = os.path.getsize(sourceFile)

# partition class
class partInfo: 
    def __init__(self, id, name, startPage, endPage): 
        self.id = id
        self.name = name
        self.startPage = startPage
        self.endPage = endPage

# partition table - offsets without oob
partitions = []
#partitions.append(partInfo(0, 'brhgptpl_0', 0, 512))
#partitions.append(partInfo(1, 'reserve0', 800, 2336))
#partitions.append(partInfo(2, 'lk_a', 4784, 7856))
#partitions.append(partInfo(3, 'lk_b', 12800, 15872))
#partitions.append(partInfo(4, 'brhgptpl_1', 33056, 33568))
#partitions.append(partInfo(5, 'reserve1', 33856, 37440))
#partitions.append(partInfo(6, 'idme_nand', 43440, 47536))
#partitions.append(partInfo(7, 'brhgptpl_2', 66112, 66624))
#partitions.append(partInfo(8, 'reserve2', 67728, 71312))
#partitions.append(partInfo(9, 'misc', 76544, 80640))
#partitions.append(partInfo(10, 'brhgptpl_3', 99216, 99728))
#partitions.append(partInfo(11, 'reserve3', 100784, 104368))
#partitions.append(partInfo(12, 'tee1', 109600, 119840))
partitions.append(partInfo(13, 'boot_a', 4864, 8768))
#partitions.append(partInfo(14, 'tee2', 277312, 287552))
#partitions.append(partInfo(15, 'boot_b', 524864, 556096))
#partitions.append(partInfo(16, 'persist', 641680, 658064))

# print first 8 bytes in hex
def print_bytes(buf):
    print(hex(buf[0]), hex(buf[1]), hex(buf[2]), hex(buf[3]), hex(buf[4]), hex(buf[5]), hex(buf[6]), hex(buf[7]))

def save_partitions():
    for part in partitions:        
        # loop over pages, extract and save
        partFilename = part.name + '.img'
        print('extracting ', partFilename)
        partFile = open('./partitions/' + partFilename, 'ab')
        print('starting at page: ', part.startPage)
        print('ending at page: ', part.endPage)

        for pageNum in range(part.startPage, part.endPage):
            buf = read_page(pageNum, True)
            decoded = decode(buf)
            print_bytes(decoded)
            partFile.write(decoded)
            
        partFile.close()

def invert(src):
    inv = bytearray(len(src))
    for i in range(0, len(src)):
        inv[i] = src[i] ^ 0b11111111  # invert
    return inv

def delta(source, mask):
    size = len(source)
    steps = int(size/len(mask))
    d = bytearray(size)   

    for i in range(0, steps):
        for j in range(0, len(mask)):
            #v = a[i] - b[i]
            d[i*j] = source[i*j] - mask[j]
        
    return d

def decode(buf):
    inverted = invert(buf)
    conv = delta(inverted, mask)
    return conv

def extractOob():
    oobFile = open('oob.bin','ab')
    for page in range(0, totalPages):
        buf = read_page(page, False)
        #print(len(buf))
        # oob is last 256 bytes
        oob = buf[pageSize:rawPageSize]
        #print(oob)   
        oobFile.write(oob)
    oobFile.close()

def get_bytes(buf):
    res = ''
    for i in range(0,len(buf)):
        res = res + hex(buf[i]) + ' '
    return res

def get_hex_bytes(buf):
    res = ''
    for i in range(0,len(buf)):
        r = hex(buf[i]).replace('0x','')
        # pad if 1 char
        if len(r) == 1:
            r = '0' + r
        res = res + r + ' '
    return res


#save_partitions()

#extractOob()

# extract initial boot stuff, from 0x00 to 0x3520 (first page)
def extract_firstpage():
    print('extracting first page')
    f = open('first_page.hex','ab')
    buf = in_file.read(3520)
    # print
    print(get_hex_bytes(buf))

    f.write(buf)
    f.close()

extract_firstpage()

in_file.close()
#out_file.close()
