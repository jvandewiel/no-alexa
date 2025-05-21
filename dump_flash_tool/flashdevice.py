# python script to connect to and read from flash device
# specific for the Macronix 32 gigabit flash in de echo dot 3

# pylint: disable=invalid-name
# pylint: disable=line-too-long
from array import array as Array
import time
import struct
import sys
import traceback
from pyftdi import ftdi

# Flash interface
ADR_CE = 0x10 # Chip Enable
ADR_WP = 0x20 # Write Protect
ADR_CL = 0x40 # Command Latch Enable
ADR_AL = 0x80 # Addres Latch Enable

# Nand commands for MX30LF4G28AD*
NAND_CMD_READ = 0x00
#NAND_CMD_READ1 = 0x01
NAND_CMD_RNDREAD = 0x05
#NAND_CMD_PAGEPROG = 0x10
#NAND_CMD_READ_OOB = 0x50
#NAND_CMD_ERASE1 = 0x60
NAND_CMD_STATUS = 0x70
#NAND_CMD_STATUS_MULTI = 0x71
#NAND_CMD_SEQIN = 0x80
#NAND_CMD_RNDIN = 0x85
NAND_CMD_READID = 0x90
#NAND_CMD_ERASE2 = 0xd0
#NAND_CMD_PARAM = 0xec
#NAND_CMD_RESET = 0xff
#NAND_CMD_LOCK = 0x2a
#NAND_CMD_UNLOCK1 = 0x23
#NAND_CMD_UNLOCK2 = 0x24
NAND_CMD_READSTART = 0x30
NAND_CMD_RNDREADSTART = 0xE0
#NAND_CMD_CACHEDPROG = 0x15
NAND_CMD_ONFI = 0xEC
NAND_CMD_READUNIQUE_ID = 0xED
NAND_CMD_GETFEATURE = 0xEE
NAND_CMD_RESET = 0xFF

NAND_CI_CHIPNR_MSK = 0x03
NAND_CI_CELLTYPE_MSK = 0x0C
NAND_CI_CELLTYPE_SHIFT = 2

NAND_STATUS_FAIL = (1<<0) # HIGH - FAIL,  LOW - PASS
NAND_STATUS_IDLE = (1<<5) # HIGH - IDLE,  LOW - ACTIVE
NAND_STATUS_READY = (1<<6) # HIGH - READY, LOW - BUSY
NAND_STATUS_NOT_PROTECTED = (1<<7) # HIGH - NOT,   LOW - PROTECTED

# Status registers, 2-4 are not used, p36
SR_CHIP_STATUS = 0 
SR_CACHEPROG_RESULT = 1
SR_READYBUSY_PER = 5 
SR_READYBUSY = 6
SR_WRITEPROTECT = 7

# as per the manual p.48
ID_CODES = [0xc2, 0xdc, 0x90, 0xa2, 0x57, 0x03]

class IO:
    def __init__(self):
        self.Debug = False
        self.PageSize = 0
        self.OOBSize = 0
        self.PageCount = 0
        self.BlockCount = 0
        self.PagePerBlock = 0
        self.BitsPerCell = 0
        self.WriteProtect = True
        self.CheckBadBlock = True
        self.RemoveOOB = False
        self.UseSequentialMode = False
        self.UseAnsi = False
        self.Identified = False
        self.ONFIData = 0
        self.WaitingTime = 0
        self.SlowMode = True
        # prints around all read/write statements
        self.DataDebug = False

        try:
            print("Opening FTDI device")
            self.ftdi = ftdi.Ftdi()
        except:
            print("Error openging FTDI device")
            self.ftdi = None

        if self.ftdi is not None:
            try:
                self.ftdi.open(0x0403, 0x6010, interface = 1)
            except:
                traceback.print_exc(file = sys.stdout)

            if self.ftdi.is_connected:
                self.ftdi.set_bitmode(0, self.ftdi.BitMode.MCU)
                

                # Clock FTDI chip at 12MHz instead of 60MHz
                if self.SlowMode:
                    print('ftdi clock @12mhz')
                    self.ftdi.write_data(Array('B', [ftdi.Ftdi.ENABLE_CLK_DIV5]))
                else :
                    print('ftdi clock @60mhz')
                    self.ftdi.write_data(Array('B', [ftdi.Ftdi.DISABLE_CLK_DIV5]))

                self.ftdi.set_latency_timer(1) # self.ftdi.LATENCY_MIN
                self.ftdi.purge_buffers()
                self.ftdi.write_data(Array('B', [ftdi.Ftdi.SET_BITS_HIGH, 0x0, 0x1]))

        self.__wait_ready()
        self.__get_id()
        

    # wait for flash device to be ready
    def __wait_ready(self):
        if self.DataDebug:
            print('waiting...')
        if self.ftdi is None or not self.ftdi.is_connected:
            return

        while 1:
            # get status for SR_READYBUSY
            #if self.__check_status(SR_READYBUSY):
            #    return

            #self.__read_status()
            self.ftdi.write_data(Array('B', [ftdi.Ftdi.GET_BITS_HIGH]))
            data = self.ftdi.read_data_bytes(1)
            if self.DataDebug:
                print('read', data)
            if not data or len(data) <= 0:
                raise Exception('FTDI device Not ready. Try restarting it.')

            # & = sets each bit to 1 if both bits are 1
            srbit = 6
            if self.DataDebug:
                print('bitwise and', data[0] & srbit, data[0] & srbit == srbit)
            # we are checking status bit 2 -> should have 6
            #if  data[0] & 2 == 0x2:
            if data[0] & srbit == srbit:
                if self.DataDebug:
                    print('status is ready')
                    print('Read data', hex(data[0]))
                return

            if self.Debug:
                print('Not Ready', data)

        return

    # read
    def __read(self, cl, al, count):
        if self.DataDebug:
            print('__read', cl, al, count)

        cmds = []
        cmd_type = 0

        # adress or command latch?
        if cl == 1:
            cmd_type |= ADR_CL
        if al == 1:
            cmd_type |= ADR_AL

        cmds += [ftdi.Ftdi.READ_EXTENDED, cmd_type, 0]

        for _ in range(1, count, 1):
            cmds += [ftdi.Ftdi.READ_SHORT, 0]

        cmds.append(ftdi.Ftdi.SEND_IMMEDIATE)
                
        if self.ftdi is None or not self.ftdi.is_connected:
            print('not connected')
            return

        self.ftdi.write_data(Array('B', cmds))
        if self.DataDebug:
            print('wrote', len(cmds), self.bytes_to_string(cmds))

        # we read 2x amount of bytes and need to seperate data
        if self.SlowMode:
            data = self.ftdi.read_data_bytes(count*2)
            data = data[0:-1:2]            
        else:
            data = self.ftdi.read_data_bytes(count)   

        #data = self.ftdi.read_data_bytes(count)        
        if self.DataDebug:
            print('read', len(data), self.bytes_to_string(data))
        return bytes(data)

    def __write(self, cl, al, data):
        cmds = []
        cmd_type = 0
        if cl == 1:
            cmd_type |= ADR_CL
        if al == 1:
            cmd_type |= ADR_AL
        if not self.WriteProtect:
            cmd_type |= ADR_WP

        cmds += [ftdi.Ftdi.WRITE_EXTENDED, cmd_type, 0, ord(data[0])]
        for i in range(1, len(data), 1):
            #if i == 256:
            #    cmds += [Ftdi.WRITE_SHORT, 0, ord(data[i])]
            #time.sleep(0.001)
            cmds += [ftdi.Ftdi.WRITE_SHORT, 0, ord(data[i])]

        if self.ftdi is None or not self.ftdi.is_connected:
            return        
        self.ftdi.write_data(Array('B', cmds))
        
    def __send_cmd(self, cmd):
        #self.__wait_ready()
        self.__write(1, 0, chr(cmd))       

    def __send_address(self, addr, count):
        #self.__wait_ready()
        data = ''
    
        for _ in range(0, count, 1):
            data += chr(addr & 0xff)
            addr = addr>>8
        self.__write(0, 1, data)
        

    def __get_status(self):        
        self.__send_cmd(NAND_CMD_STATUS)
        status = self.__read_data(1)[0]
        return status

    def __read_data(self, count):
        #self.__wait_ready()     
        return self.__read(0, 0, count)

    def __write_data(self, data):
        self.__wait_ready()
        return self.__write(0, 0, data)

    def __read_onfi(self):
        self.__wait_ready()
        # Check and read ONFI 
        self.__send_cmd(NAND_CMD_READID)
        self.__send_address(0x20, 1)

        onfitmp = self.__read_data(4)
        print('onfitmp:', onfitmp)
        onfi = (onfitmp[0:4] == b'ONFI')

        if onfi:
            print('got onfi data')
            self.__send_cmd(NAND_CMD_ONFI)
            self.__send_address(0, 1)            
            onfi_data = self.__read_data(0x100)
            print('onfi data, read ', len(onfi_data), 'bytes')
                        
            # check onfi data header            
            onfi_header = onfi_data[0:4] == b'ONFI'
            print('got onfi header', onfi_data[0:4], onfi_header)

            if onfi_header:
                
                self.ONFIData = onfi_data                
                # assign 
                self.PageSize = 4096 # onfi_data[80:83]
                self.ChipSizeMB = 4096 #onfi_data[101]
                self.EraseSize = 278528 # ??onfi_data[101]
                self.Options = 1 #onfi_data[101]                
                self.AddrCycles = 5 # onfi_data[100]
                print('set addr cycles to', onfi_data[100], self.AddrCycles)
                self.dump_onfi()
                

    def __get_id(self):
        self.Name = ''
        self.ID = 0
        self.PageSize = 0
        self.ChipSizeMB = 0
        self.EraseSize = 0
        self.Options = 0
        self.AddrCycles = 0

        self.__send_cmd(NAND_CMD_READID)       
        self.__send_address(0, 1)
        flash_identifiers = self.__read_data(8)       
        
        if not flash_identifiers:
            print('unable to get flash id')
            return False

        print('flash identifiers', self.bytes_to_string(flash_identifiers))

        # compare with expected ID codes
        for i in range(0,len(ID_CODES)):
            if flash_identifiers[i] != ID_CODES[i]:
                print('mismatch ID codes')
                self.Identified = False
                return False
        
        self.Identified = True
        #if DEVICE_DESCRIPTION[1] == flash_identifiers[0]:
        #    (self.Name, self.ID, self.PageSize, self.ChipSizeMB, self.EraseSize, self.Options, self.AddrCycles) = DEVICE_DESCRIPTION
        #    self.Identified = True
               
        self.__read_onfi()
        self.__read_unique_id()
        self.__read_feature_config()

        # assign
        
        if flash_identifiers[0] == 0xc2:
            self.Manufacturer = 'Macronix'
        else:
            self.Manufacturer = 'Unknown'

        idstr = ''
        for idbyte in flash_identifiers:
            idstr += "%X" % idbyte
        if idstr[0:4] == idstr[-4:]:
            idstr = idstr[:-4]
            if idstr[0:2] == idstr[-2:]:
                idstr = idstr[:-2]
        self.IDString = idstr
        self.IDLength = int(len(idstr) / 2)
        
        print('IDLength:', self.IDLength)
        print('PageSize:', self.PageSize)

        self.BitsPerCell = 1
        self.OOBSize = 256
        self.EraseSize = 64 * 4096
        self.PageSize = 4096        

        self.PageCount = 131072

        self.RawPageSize = self.PageSize + self.OOBSize
        self.BlockSize = self.EraseSize
        self.BlockCount = 2048

        self.PagePerBlock = 64
        self.RawBlockSize = 270848

        return True

    # check and read unique id
    def __read_unique_id(self):
        
        self.__send_cmd(NAND_CMD_READUNIQUE_ID)
        self.__send_address(0x00, 1)

        # how does this work? with the xor of what against what?
        
        for iter in range(0, 1):
            unique_id = self.__read_data(32)
            print('complete unique_id:', self.bytes_to_string(unique_id))

            # as per page 350, xor 1st 16 bytes with 2nd 16 bytes
            id_first = unique_id[0:16]
            id_second = unique_id[17:32]

            print('first 16 bytes: ', self.bytes_to_string(id_first))
            print('last 16 bytes : ', self.bytes_to_string(id_second))

            # xor
            result = bytearray(16)
            for i in range(0,15):           
                result[i] = id_first[i] ^ id_second[i]

            print('unique id xor:  ', self.bytes_to_string(result))
            print()
        

    # read set features, page 52
    def __read_feature_config(self):
        
        feature_addresses = [0x01, 0x80, 0x89, 0x90, 0xa0, 0xb0]
        feature_description = ['Timing mode', 'Programmable I/O Drive Strength', 'Special Read for Data Recovery Operation', 'Array Operation Mode', 'Block Protection Operation', 'Configuration']
        feature_value = bytearray(len(feature_addresses))

        for i in range(0, len(feature_addresses)):
            self.__send_cmd(NAND_CMD_GETFEATURE)
            self.__send_address(feature_addresses[i], 1)
            feature_value = self.__read_data(4) # 4 params
            # print as binary
            str = ''
            for j in range(0,len(feature_value)):                
                str = str + bin(feature_value[j]) + ' '
            print(feature_description[i],' for address [', hex(feature_addresses[i]), ']:', str)
        
        print()
        

    def is_initialized(self):
        return self.Identified

    def set_use_ansi(self, use_ansi):
        self.UseAnsi = use_ansi

    def is_slow_mode(self):
        return self.Slow

    def get_bits_per_cell(self, cellinfo):
        bits = cellinfo & NAND_CI_CELLTYPE_MSK
        bits >>= NAND_CI_CELLTYPE_SHIFT
        return bits+1

    def dump_info(self):
        print()
        print('Full ID:       ', self.IDString)
        print('ID Length:     ', self.IDLength)
        print('Name:          ', self.Name)
        print('ID:             0x%x' % self.ID)
        print('Page size:      0x{0:x}({0:d})'.format(self.PageSize))
        print('OOB size:       0x{0:x} ({0:d})'.format(self.OOBSize))
        print('Page count:     0x%x' % self.PageCount)
        print('Size:           0x%x' % self.ChipSizeMB)
        print('Erase size:     0x%x' % self.EraseSize)
        print('Block count:   ', self.BlockCount)
        print('Options:       ', self.Options)
        print('Address cycle: ', self.AddrCycles)
        print('Bits per Cell: ', self.BitsPerCell)
        print('Manufacturer:  ', self.Manufacturer)
        print()

    def dump_onfi(self):
        onfi_data = self.ONFIData
        # from macronix manual
        print('\nONFI parameters')
        print('Revision Information and Features Block')
        print('Parameter Page Signature                      ', self.bytes_to_string(onfi_data[0:4]))
        print('Revision Number                               ', self.bytes_to_string(onfi_data[4:6]))
        print('Features supported                            ', self.bytes_to_string(onfi_data[6:8]))
        print('Optional Commands Supported                   ', self.bytes_to_string(onfi_data[8:10]))
        print('Reserved                                      ', self.bytes_to_string(onfi_data[10:32]))
        print('\nManufacturer Information Block')
        print('Device manufacturer (12 ascii chars)          ', self.bytes_to_ascii(onfi_data[31:43]))
        print('Device model (20 ascii chars)                 ', self.bytes_to_ascii(onfi_data[44:64]))
        print('JEDEC Manufacturer ID                         ', hex(onfi_data[64]))
        print('Date Code                                     ', self.bytes_to_string(onfi_data[65:67]))
        print('Reserved                                      ', self.bytes_to_string(onfi_data[67:80]))
        print('\nMemory Organization Block')
        print('Number of Data Bytes per Page                 ', self.bytes_to_string(onfi_data[80:84]))
        print('Number of Spare Bytes per Page                ', self.bytes_to_string(onfi_data[84:86]))
        print('Number of Data Bytes per Partial Page         ', self.bytes_to_string(onfi_data[86:90]))
        print('Number of Spare Bytes per Partial Page        ', self.bytes_to_string(onfi_data[90:92]))
        print('Number of Pages per Block                     ', self.bytes_to_string(onfi_data[92:96]))
        print('Number of Blocks per Logical Unit             ', self.bytes_to_string(onfi_data[96:100]))
        print('Number of Logical Units (LUNs)                ', hex(onfi_data[100]))
        print('Number of Address Cycles                      ', hex(onfi_data[100]))
        print('Number of Bits per Cell                       ', hex(onfi_data[102]))
        print('Bad Blocks Maximum per LUN                    ', self.bytes_to_string(onfi_data[103:105]))
        print('Block endurance                               ', self.bytes_to_string(onfi_data[105:107]))
        print('Guarantee Valid Blocks at Beginning of Target ', hex(onfi_data[107]))
        print('Block endurance for guaranteed valid blocks   ', self.bytes_to_string(onfi_data[108:110]))
        print('Number of Programs per Page                   ', hex(onfi_data[110]))
        print('Partial Programming Attributes                ', hex(onfi_data[111]))
        print('Number of Bits ECC Correctability             ', hex(onfi_data[112]))
        print('Number of Interleaved Address Bits            ', hex(onfi_data[113]))
        print('Interleaved Operation Attributes              ', hex(onfi_data[114]))
        print('Reserved                                      ', self.bytes_to_string(onfi_data[115:128]))
        print('\nElectrical Parameters Block')
        print('I/O Pin Capacitance                           ', hex(onfi_data[128]))
        print('Timing Mode Support                           ', self.bytes_to_string(onfi_data[129:131]))
        print('Program Cache Timing Mode Support             ', self.bytes_to_string(onfi_data[131:133]))
        print('tPROG Maximum Page Program Time (uS)          ', self.bytes_to_string(onfi_data[133:135]))
        print('tBERS(tERASE) Maximum Block Erase Time (uS)   ', self.bytes_to_string(onfi_data[135:137]))
        print('tR Maximum Page Read Time (uS)                ', self.bytes_to_string(onfi_data[137:139]))
        print('tCCS Minimum Change Column Setup Time (ns)    ', self.bytes_to_string(onfi_data[139:141]))
        print('Reserved                                      ', self.bytes_to_string(onfi_data[141:164]))
        print('\nVendor Blocks')
        print('Vendor Specific Revision Number               ', self.bytes_to_string(onfi_data[164:166]))
        print('Reserved                                      ', hex(onfi_data[166]))
        print('Reliability enhancement function              ', hex(onfi_data[167]))
        print('Reserved                                      ', hex(onfi_data[168]))
        print('Number of special read for data recovery (N)  ', hex(onfi_data[169]))
        print('Vendor Specific                               ', self.bytes_to_string(onfi_data[170:254]))
        print('Integrity CRC                                 ', self.bytes_to_string(onfi_data[254:256]))

    def read_page(self, pageno):
        bytes_to_read = bytearray()
        #print('reading page ', pageno)
        # send a reset for every page read
        # send page read command
        self.__send_cmd(NAND_CMD_READ)
        # send flash address
        self.__send_address(pageno<<16, self.AddrCycles)        
        # send page read confirmed command 0x30
        self.__send_cmd(NAND_CMD_READSTART)        
        # c code is read per byte
        # we read in blocks 0f 1024
        

        #print('alt read')        
        length = self.PageSize + self.OOBSize
        chunksize = 0x01 #0x800
        while length > 0:            
            read_len = chunksize # pages of chunksize
            if length < chunksize:
                read_len = length                    
            bytes_to_read += self.__read_data(read_len)
            length -= chunksize
    
        #bytes_to_read = self.__read_data(4096) # self.PageSize+self.OOBSize        
        #print('read bytes ', len(bytes_to_read))
        return bytes_to_read

    # does NOT work
    # read page and then random columns in the page
    def read_random_page(self, pageno):
        bytes_to_read = bytearray()
        self.__wait_ready()

        # send random read command
        self.__send_cmd(NAND_CMD_RNDREAD)
        # only the column addres!
        self.__send_address(pageno<<16, 2)   
        
        # send random read confirmed command 
        self.__send_cmd(NAND_CMD_RNDREADSTART)
        self.__wait_ready()      
        # read data in a page 
        bytes_to_read = self.__read_data(self.PageSize+self.OOBSize)
        return bytes_to_read

    # flush buffers, send reset to nand?
    def flush(self):
        print('flushing')
        self.ftdi.purge_buffers()

    def read_seq(self, pageno, remove_oob = False, raw_mode = False):
        print('read seq', pageno)
        page = []
        self.__send_cmd(NAND_CMD_READ0)
        self.__wait_ready()
        self.__send_address(pageno<<8, self.AddrCycles)
        self.__wait_ready()

        bad_block = False

        for i in range(0, self.PagePerBlock, 1):
            page_data = self.__read_data(self.RawPageSize)

            if i in (0, 1):
                if page_data[self.PageSize + 5] != 0xff:
                    bad_block = True

            if remove_oob:
                page += page_data[0:self.PageSize]
            else:
                page += page_data

            self.__wait_ready()

        if self.ftdi is None or not self.ftdi.is_connected:
            return ''

        self.ftdi.write_data(Array('B', [ftdi.Ftdi.SET_BITS_HIGH, 0x1, 0x1]))
        self.ftdi.write_data(Array('B', [ftdi.Ftdi.SET_BITS_HIGH, 0x0, 0x1]))

        data = ''

        if bad_block and not raw_mode:
            print('\nSkipping bad block at %d' % (pageno / self.PagePerBlock))
        else:
            for ch in page:
                data += chr(ch)

        return data

    # return bytes array as string in hex format
    def bytes_to_string(self, buf):
        res = ''
        for i in range(0,len(buf)):
            res = res + hex(buf[i]) + ' '
        return res

    # return byutes array as ascii string
    def bytes_to_ascii(self, buf):
        str = '['
        for i in range(0,len(buf)):
            str = str + chr(buf[i]) 
        str += ']'
        return str

    # try to use the data recovery feature, as per page 57 
    def recover_page(self, pageno, remove_oob = False):
        print('recover page ', pageno)
        self.__wait_ready()
        bytes_to_read = bytearray()

        if self.Options & LP_OPTIONS:
            self.__wait_ready()
            self.__send_cmd(NAND_CMD_READ0)
            self.__wait_ready()
            self.__send_address(pageno<<16, self.AddrCycles)
            self.__wait_ready()
            self.__send_cmd(NAND_CMD_READSTART)
            self.__wait_ready()
            if self.PageSize > 0x1000:        
                #print('alt read')        
                length = self.PageSize + self.OOBSize
                while length > 0:
                    self.__wait_ready()
                    read_len = 0x1000
                    if length < 0x1000:
                        read_len = length                    
                    bytes_to_read += self.__read_data(read_len)
                    self.__wait_ready()
                    length -= 0x1000
            else:    
                # this is hat we use
                self.__wait_ready()            
                bytes_to_read = self.__read_data(self.PageSize+self.OOBSize)
                self.__wait_ready()

            print('read bytes ', len(bytes_to_read))
            

        print('done reading')    
        return bytes_to_read


    def __reset(self):
        # send reset command
        self.__send_cmd(NAND_CMD_RESET)
        # wait reset finish 
        self.__wait_ready()

    def __read_status(self):        
        # send status read command
        self.__send_cmd(NAND_CMD_STATUS)
        # read status value 
        status = self.__read_data(1)
        # convert to bits as per SR description in manual?
        print('__read_status', self.bytes_to_string(status))
        return status
    
    # check status register bit 7 ~ bit 0
    def __check_status(self, status_reg):
        print('check status reg',status_reg )
        status = self.__read_status()

        #if  data[0] & 2 == 0x2:
        if status & status_reg == status_reg:
            return True
        else:
            return False

    # wait flash device for a specfic time, in us
    def __wait_time(self, time_value):
        self.WaitingTime = time_value
        # wait for time_value
        # wait()


