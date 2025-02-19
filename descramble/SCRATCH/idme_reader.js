// simple script to extract information from the idme_nand dump
var fs = require('fs');
const { version } = require('os');
const { dirname } = require('path');
const { getEnabledCategories } = require('trace_events');

class IdmeReader {

    offset = 0x100000;
    magic = 'beefdeed';
    blockSize = 4096*64;
    data;

    constructor(filename) {
        try {
            var size = fs.statSync(filename).size;
            if (size > 0) {
                var buf = Buffer.alloc(size);
                var binfile = fs.openSync(filename, 'r');
                var bytesRead = fs.readSync(binfile, buf, 0, this.blockSize, this.offset);
                this.log(`read ${bytesRead} bytes from ${filename}`);
                console.log(buf);
                // check header
                var chkBuf = Buffer.from(this.magic);
                var header = buf.subarray(0, chkBuf.length);

                if(this.matchingBuf(header, chkBuf)) {
                    this.log('valid header')
                    this.data = buf;
                } else {
                    this.log('invalid header')
                }
                
            } else {
                this.log('invalid size')
            }
        } catch (err) {
            console.log(err);
            return undefined
        }
    }

    /**
     * Compare 2 buffers and return true if matching completely byte-for-byte
     * @param {*} buf1 
     * @param {*} buf2 
     * @returns 
     */
    matchingBuf(buf1, buf2) {
        var result = true;
        if (buf1.length !== buf2.length) {
            this.log(`different buffer size, can't match`);
            return false
        }
        // check each
        for (var i = 0; i < buf1.length; i++) {
            if (buf1[i] !== buf2[i]) { return false }
        }
        return result;
    }

    /**
     * Parse the data in the file
     * 
     * structure
     * 16 bytes: header + version(?)
     */
    parse() {
        idme = {
            header: this.bytes(0, 8),
            version: this.bytes(8, 12),
            unknown: this.bytes(12, 16),
            entries: this.entries()
        }        
        this.log(idme)
    }

    /**
     * return bytes in data at given offset and length
     */
    bytes(start, end) {
        return this.data.subarray(start,end)
    }

    /**
     * get entries
     * each 24 bytes 
     */
    entries() {
        var entry = true;
        var offset = 16;
        var len = 44; // variable
        var i = 1;

        // iterate over data
        while(entry) {

            entry = this.bytes(offset + (i * len), offset + ((i + 1) * len));
            console.log(entry.toString());
            if(entry[0] === 0x00) {
                entry = false
            } else {
                i += 1
            }
        }
    }

    log(msg) {
        console.log(`IDME: ${msg}`)
    }
}

var idme = new IdmeReader('./exported/partitions/idme_nand.bin');
idme.parse();