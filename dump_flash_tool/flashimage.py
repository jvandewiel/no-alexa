# pylint: disable=invalid-name
# pylint: disable=line-too-long
from optparse import OptionParser
import pprint
import os
import struct
import sys
import time
import flashdevice

class IO:
    def __init__(self):
        self.UseAnsi = False
        self.UseSequentialMode = False
        self.DumpProgress = True
        self.DumpProgressInterval = 1

        print('using flashdevice')
        self.FlashDevice = flashdevice.IO()
            

    def is_initialized(self):
        return self.FlashDevice.is_initialized()

    def set_use_ansi(self, use_ansi):
        self.UseAnsi = use_ansi
        self.FlashDevice.set_use_ansi(use_ansi)

    def read_page(self, pageno):
        #print('image io - read_page', pageno)

        max_loops = 8 # number of possible re-reads of a given page

        # we read the page 3x, and only export if matching;
        # this we can do max_loops
        good_read = False

        for iter in range(0, max_loops):
            # page
            #print(iter)
            # sequential read
            data = self.FlashDevice.read_page(pageno)
            ref1_data = self.FlashDevice.read_page(pageno)                        
            ref2_data = data #//ref2_data = self.FlashDevice.read_page(pageno)                        
           
            if not data or not ref1_data or not ref2_data:
                print('no data')
                break

            if(data == ref1_data and ref1_data == ref2_data):
                print('good read for page ', pageno, ' data identical at iteration ', iter)
                good_read = True
                return data

        if not good_read: # guess we always end up here when loop fails and no data is returned
            print('unable to get good read for page ', pageno)



    

            
            