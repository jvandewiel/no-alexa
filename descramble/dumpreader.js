/**
 * Open binary file and reader handler for flash dump data
 * buffer https://nodejs.org/en/knowledge/advanced/buffers/how-to-use-buffers/
 * 
 */

var fs = require('fs');
const { BlockList } = require('net');

const pageSize = 4352; // oob + page are combined
const chunks = 4; // chunks of data in the file
const modSize = 64; // 64 seems enough as 0+63, 5+68 are identical

/**
 * Wrapper around the binary data dump file
 */
class FlashDumpReader {

    // parameters for the data file
    filename = '';
    binfile = null; // binary data
    pagesPerBlock = 64;
    blockSize = pageSize * this.pagesPerBlock;
    pageCount = 0;
    blockCount = 0;
    pages = [];
    // descrambled buffer
    descrambledFile = null;

    /**
     * partition table
     * 1 sector translates to n pages
     */
    partitionTable = [
        { id: 0, name: 'brhgptpl_0', part_id: 0, start_sect: 0x0, nr_sects: 0x40 },
        { id: 1, name: 'reserve0', part_id: 1, start_sect: 0x40, nr_sects: 0xC0 },
        { id: 2, name: 'lk_a', part_id: 2, start_sect: 0x100, nr_sects: 0x180 },
        { id: 3, name: 'lk_b', part_id: 3, start_sect: 0x280, nr_sects: 0x180 },
        { id: 4, name: 'brhgptpl_1', part_id: 4, start_sect: 0x400, nr_sects: 0x40 },
        { id: 5, name: 'reserve1', part_id: 5, start_sect: 0x440, nr_sects: 0x1C0 },
        { id: 6, name: 'idme_nand', part_id: 6, start_sect: 0x600, nr_sects: 0x200 },
        { id: 7, name: 'brhgptpl_2', part_id: 7, start_sect: 0x800, nr_sects: 0x40 },
        { id: 8, name: 'reserve2', part_id: 8, start_sect: 0x840, nr_sects: 0x1C0 },
        { id: 9, name: 'misc', part_id: 9, start_sect: 0xA00, nr_sects: 0x200 },
        { id: 10, name: 'brhgptpl_3', part_id: 10, start_sect: 0xC00, nr_sects: 0x40 },
        { id: 11, name: 'reserve3', part_id: 11, start_sect: 0xC40, nr_sects: 0x1C0 },
        { id: 12, name: 'tee1', part_id: 12, start_sect: 0xE00, nr_sects: 0x500 },
        { id: 13, name: 'boot_a', part_id: 13, start_sect: 0x1300, nr_sects: 0xF40 },
        { id: 14, name: 'tee2', part_id: 14, start_sect: 0x2240, nr_sects: 0x500 },
        { id: 15, name: 'boot_b', part_id: 15, start_sect: 0x2740, nr_sects: 0xF40 },
        { id: 16, name: 'persist', part_id: 16, start_sect: 0x3680, nr_sects: 0x800 },
        { id: 17, name: 'userdata', part_id: 17, start_sect: 0x3E80, nr_sects: 0x1BF80 }
    ]

    /**
     * constructor
     */
    constructor() {
        //
    }

    // load binary file
    loadBinFile(filename) {
        try {
            var size = fs.statSync(filename).size;
            if (size > 0) {
                var buf = Buffer.alloc(size);
                var binfile = fs.openSync(filename, 'r');
                var bytesRead = fs.readSync(binfile, buf, 0, size, 0);
                this.log(`read ${bytesRead} bytes from ${filename}`);
                return buf;
            } else {
                return undefined
            }
        } catch (err) {
            console.log(err);
            return undefined
        }
    }

    /**
     * merge all pages into single file 
     * load data, copy to buffer, export as targetfile
     * @param {*} sourceDir 
     * @param {*} targetFile 
     * @param {*} pageCount 
     */
    mergePages(sourceDir, targetFile, pageCount) {
        this.log(`merging ${pageCount} pages from ${sourceDir} to ${targetFile}`);
        var finalBuf = Buffer.alloc(pageSize * pageCount);
        for (var i = 0; i < pageCount; i++) {
            var filename = `${sourceDir}/${i}.bin`;
            var pageBuf = this.loadBinFile(filename);
            pageBuf.copy(finalBuf, i * pageSize, 0, pageBuf.length);
        }
        this.exportBuf(finalBuf, targetFile);
        this.log('finished merging');
    }

    /**
     * Close the file
     */
    closeFile() {
        this.log(`closing binary file ${this.filename}`)
        fs.closeSync(this.binfile);
    }

    /**
     * open the file and make it ready for reading
     */
    openFile(filename) {
        if (this.binfile === null) {
            this.log(`opening binary file ${filename}`)
            this.binfile = fs.openSync(filename, 'r');
        }

        this.filename = filename;
        var stats = fs.statSync(filename);
        this.pageCount = stats.size / pageSize;
        this.blockCount = this.pageCount / this.pagesPerBlock;
        this.log(`-------------------------------------------------------------\n`);
        this.log(`## Summary\n`);
        this.log(`Page Size:       ${pageSize}`);
        this.log(`Pages per Block: ${this.pagesPerBlock}`);
        this.log(`Block Size:      ${this.blockSize}`);
        this.log(`Page count:      ${this.pageCount}`);
        this.log(`Block count:     ${this.blockCount}`);
        this.log(`Actual filesize: ${stats.size / (1024 * 1024 * 1024)} GB (${stats.size} bytes)`);
        this.log(`-------------------------------------------------------------\n`);
    }

    /**
     * Read a given page by number
     * determine offset, read pageSize bytes and return page object
     * @param {*} page 
     */
    page(pageNum) {
        var offset = pageNum * pageSize
        //this.log(`reading page ${pageNum} (offset: ${offset}, ${offset.toString(16)})`);

        var buf = new Buffer.alloc(pageSize);
        var bytesRead = 0;
        while (bytesRead = fs.readSync(this.binfile, buf, 0, pageSize, offset)) {
            //this.log(`read ${bytesRead} bytes`);
            offset += bytesRead;
            if (bytesRead === pageSize) {
                // make page object
                var p = new Page(buf);
                p.setNum(pageNum);
                // return                
                return p;
            }
        }
    }

    readBlocks(startBlock, endBlock) {
        var size = 64 * 4352;
        var blocks = [];
        var offset = startBlock * size;
        this.log(`reading ${endBlock - startBlock} blocks from ${startBlock} to ${endBlock} (offset: ${offset}, 0x${offset.toString(16)})`);

        for (var i = startBlock; i < endBlock; i++) {
            //this.log(`reading block ${i}`);
            var buf = new Buffer.alloc(size);
            var bytesRead = 0;
            while (bytesRead = fs.readSync(this.binfile, buf, 0, size, offset)) {
                //this.log(`read ${bytesRead} bytes`);
                offset += bytesRead;
                if (bytesRead === size) {
                    var b = new Block(buf);
                    b.setNum(i);
                    blocks.push(b);
                    break;
                }
            }
        }
        return blocks;
    }

    /**
     * Read a series of pages starting at startPage and end at endPage
     * @param {*} page 
     * @param {*} number 
     */
    readPages(startPage, endPage) {
        this.log(`reading pages ${startPage} to ${endPage}`);
        var pages = [];
        // check if endpage does not exceed data we have
        if (endPage > this.pageCount) {
            endPage = this.pageCount;
        }

        for (var i = startPage; i < endPage; i++) {
            var page = this.page(i);
            pages.push(page);
        }
        return pages
    }

    /**
     * partition table handler
     * list partition(s)
     * preferably we read this from the file but until then, we have the following
     */
    listPartitions() {
        this.log(`-------------------------------------------------------------\n`);
        this.log(`## Partition table\n`);
        for (var part of this.partitionTable) {
            //console.log(`${part.id}: ${part.name} start sector: ${part.start_sect} sectors: ${part.nr_sects}`);
            var startByte = part.start_sect * pageSize;
            var bytes = part.nr_sects * pageSize;
            this.log(`${part.id}: ${part.name} start page: ${part.start_sect} pages: ${part.nr_sects} size: ${bytes} bytes`);
        }
        this.log(`-------------------------------------------------------------\n`);
    }

    /**
     * 
     * @returns list of partition names
     */
    getPartitionNames() {
        var res = [];
        for (var part of this.partitionTable) {
            res.push(part.name);
        }
        return res;
    }

    /**
     * get data for a partition
     * @param {*} part
     */
    getPartitionData(part) {
        // handling is different depending on the partition type
        switch (part.name) {
            case 'persist':
            case 'userdata':
                this.log('handle as block for UBI');
                var blocks = this.readBlocks(part.start_sect / 64, (part.start_sect + part.nr_sects) / 64)
                var blockSize = 64 * 4096; // PEB size                
                var buf = Buffer.alloc(blocks.length * blockSize, 0xFF); // 0xFF is fine
                for (var i = 0; i < blocks.length; i++) {
                    var blockBuf = blocks[i].ubiData();
                    blockBuf.copy(buf, i * blockSize, 0, blockBuf.length);
                }
                return buf

            //case 'boot_a':
            //case 'boot_b':                
            default:
                var size = 3 * 1024;
                this.log(`get data for ${part.name} with size ${part.nr_sects} pages`)
                var pages = this.readPages(part.start_sect, part.start_sect + part.nr_sects);
                // merge data
                var buf = Buffer.alloc(pages.length * size, 0x00); // without the 0xFF stuff
                for (var i = 0; i < pages.length; i++) {
                    var pageBuf = pages[i].finalData();
                    pageBuf.copy(buf, i * size, 0, pageBuf.length);

                }

                return buf;
        }
    }

    /**
     * get data for a partition
     * @param {*} part
     */
    getRawPartitionData(part) {
        var size = 4352;
        this.log(`get raw data for ${part.name} with size ${part.nr_sects} pages`)
        var pages = this.readPages(part.start_sect, part.start_sect + part.nr_sects);
        // merge data
        var buf = Buffer.alloc(pages.length * size, 0x00);
        for (var i = 0; i < pages.length; i++) {
            var pageBuf = pages[i].raw();
            pageBuf.copy(buf, i * size, 0, pageBuf.length);
        }
        return buf;
    }

    /**
     * Get partition meta data
     * @param {*} partname 
     */
    getPartitionMeta(partname) {
        for (var part of this.partitionTable) {
            if (part.name === partname) {
                console.log(`found partition ${part.name}`);
                return part
            }
        }
        // not found if here
        console.log(`unable to find partition ${part.name}`);
    }


    /**
     * Export a buffer to a (binary) filename
     * @param {*} buf 
     * @param {*} filename 
     */
    exportBuf(buf, filename) {
        var expfile = fs.openSync(filename, 'w');
        fs.writeSync(expfile, buf, 0, buf.length, 0);
        this.log(`written ${buf.length} bytes to ${filename}`);
        fs.closeSync(expfile);
    }

    /**
     * export a partition to a (binary) file
     * oob = include oob or not
     * @param {*} part 
     */
    exportPartition(part, dir) {
        var buf = this.getPartitionData(part);
        var filename = `${dir}/${part.name}.bin`;
        this.exportBuf(buf, filename);
        this.log(`exported partition ${part.name} to ${filename} [${buf.length} bytes]`);
    }

    /**
     * Export all partitions as per the partition table
     */
    exportAllPartitions(dir) {
        for (var part of this.partitionTable) {
            this.exportPartition(part, dir);
        }
    }

    /**
     * export partition with all oob stuff
     * @param {*} part 
     * @param {*} dir 
     */
    exportRawPartition(part, dir) {
        var buf = this.getRawPartitionData(part);
        var filename = `${dir}/${part.name}_raw.bin`;
        this.exportBuf(buf, filename);
        this.log(`exported raw partition ${part.name} to ${filename} [${buf.length} bytes]`);
    }
    /**
     * 131008 and 130944 contain bad block tables
     * Bad block table found at page 131008, version 1, i 1
     * so oob or data contains something?
     */
    badBlockTable() {
        var res = [];
        res.push(this.readPage(131008));
        res.push(this.readPage(130944));
        return res;
    }

    /**
     * array of bytes to short hex formatted string
     * @param {*} byteArray 
     * @returns 
     */
    toHexString(byteArray) {
        return Array.from(byteArray, function (byte) {
            return ('0' + (byte & 0xFF).toString(16)).slice(-2);
        }).join('')
    }
    /*
    https://en.wikipedia.org/wiki/GUID_Partition_Table

    Offset 	    Length 	    Contents   
    0 (0x00) 	8 bytes 	Signature ("EFI PART", 45h 46h 49h 20h 50h 41h 52h 54h or 0x5452415020494645ULL[a] on little-endian machines)
    8 (0x08) 	4 bytes 	Revision 1.0 (00h 00h 01h 00h) for UEFI 2.8
    12 (0x0C) 	4 bytes 	Header size in little endian (in bytes, usually 5Ch 00h 00h 00h or 92 bytes)
    16 (0x10) 	4 bytes 	CRC32 of header (offset +0 to +0x5b) in little endian, with this field zeroed during calculation
    20 (0x14) 	4 bytes 	Reserved; must be zero
    24 (0x18) 	8 bytes 	Current LBA (location of this header copy)
    32 (0x20) 	8 bytes 	Backup LBA (location of the other header copy)
    40 (0x28) 	8 bytes 	First usable LBA for partitions (primary partition table last LBA + 1)
    48 (0x30) 	8 bytes 	Last usable LBA (secondary partition table first LBA − 1)
    56 (0x38) 	16 bytes 	Disk GUID in mixed endian[7]
    72 (0x48) 	8 bytes 	Starting LBA of array of partition entries (usually 2 for compatibility)
    80 (0x50) 	4 bytes 	Number of partition entries in array
    84 (0x54) 	4 bytes 	Size of a single partition entry (usually 80h or 128)
    88 (0x58) 	4 bytes 	CRC32 of partition entries array in little endian
    92 (0x5C) 	* 	        Reserved; must be zeroes for the rest of the block (420 bytes for a sector size of 512 bytes; but can be more with larger sector sizes)
    */
    // lets do some more processing
    // page 3 + 4 contain gpt
    // signature EFI PART 0x45 0x46 0x49 0x20 0x50 0x41 0x52 0x54
    efi() {
        var EFI = Buffer.from([0x45, 0x46, 0x49, 0x20, 0x50, 0x41, 0x52, 0x54]);
        var p = this.page(3);
        var data = p.finalData();

        if (this.matchingBuf(data.subarray(0, EFI.length), EFI)) {
            var lba = {
                signature: data.subarray(0, 8),
                revision: data.subarray(8, 12),
                headerSize: data.subarray(12, 16),
                headerCrc: data.subarray(16, 20),
                reserved01: data.subarray(20, 24),
                currLBA: data.subarray(24, 32),
                backupLBA: data.subarray(32, 40),
                firstLBA: data.subarray(40, 48),
                lastLBA: data.subarray(48, 56),
                guid: data.subarray(56, 72),
                startingLBA: data.subarray(72, 80),
                partitions: data.subarray(80, 84),
                partitionsSize: data.subarray(84, 88),
                partitionsCrc: data.subarray(88, 92),
                reserved02: data.subarray(92, 512),
            }
            this.log(`-------------------------------------------------------------\n`);
            this.log(`## EFI Header\n`);
            this.log(`EFI signature:  ${this.toHexString(lba.signature)}`);
            this.log(`Revision        ${this.toHexString(lba.revision)}`);
            this.log(`Header size     ${this.toHexString(lba.headerSize)}`);
            this.log(`Header CRC      ${this.toHexString(lba.headerCrc)}`);
            this.log(`Reserved 01     ${this.toHexString(lba.reserved01)}`);
            this.log(`Current LBA     ${this.toHexString(lba.currLBA)}`);
            this.log(`Backup LBA      ${this.toHexString(lba.backupLBA)}`);
            this.log(`First LBA       ${this.toHexString(lba.firstLBA)}`);
            this.log(`Last LBA        ${this.toHexString(lba.lastLBA)}`);
            this.log(`GUID            ${this.toHexString(lba.guid)}`);
            this.log(`Starting LBA    ${this.toHexString(lba.startingLBA)}`);
            this.log(`Partitions      ${this.toHexString(lba.partitions)}`);
            this.log(`Partitions Size ${this.toHexString(lba.partitionsSize)}`);
            this.log(`Partitions CRC  ${this.toHexString(lba.partitionsCrc)}`);
            this.log(`Reserved 02     ${this.toHexString(lba.reserved02)}`);
            this.log(`-------------------------------------------------------------\n`);
        }
    }

    // parse partition table
    /*
    Offset 	Length 	Contents
    0 (0x00) 	16 bytes 	Partition type GUID (mixed endian[7])
    16 (0x10) 	16 bytes 	Unique partition GUID (mixed endian)
    32 (0x20) 	8 bytes 	First LBA (little endian)
    40 (0x28) 	8 bytes 	Last LBA (inclusive, usually odd)
    48 (0x30) 	8 bytes 	Attribute flags (e.g. bit 60 denotes read-only)
    56 (0x38) 	72 bytes 	Partition name (36 UTF-16LE code units) 
    */
    gpt() {
        var p = this.page(4);
        var data = p.finalData();

        // read data and create partitions
        var partitions = [];
        var guid = data.subarray(0, 16);
        var offset = 0

        while (offset !== -1) {
            // first we know, is at 0 - rest is at offset
            var part = {
                typeGuid: data.subarray(offset, offset + 16),
                partGuid: data.subarray(offset + 16, offset + 32),
                firstLBA: data.subarray(offset + 32, offset + 40),
                lastLBA: data.subarray(offset + 40, offset + 48),
                attribs: data.subarray(offset + 48, offset + 56),
                nameBytes: data.subarray(offset + 56, offset + 108),
                name: data.subarray(offset + 56, offset + 108).toString('utf16le'),
                // now ppl values
                startPage: data.subarray(offset + 32, offset + 40),
                endPage: data.subarray(offset + 40, offset + 48)
            }
            partitions.push(part);
            var offset = this.findBinSeq(data, guid, offset + 16);
        }

        this.log(`-------------------------------------------------------------\n`);
        this.log(`## Partitions (GRP)\n`);
        this.log(`Partition guid: ${this.toHexString(guid)}`);
        for (p of partitions) {
            this.log(`${p.name}: guid ${this.toHexString(p.partGuid)} start ${this.toHexString(p.firstLBA)} end ${this.toHexString(p.lastLBA)}`);
        }
        this.log(`-------------------------------------------------------------\n`);
    }

    /**
     * find start of given sequence of bytes (all match) in buffer after offset
     * @param {*} buf 
     * @param {*} seq 
     * @param {*} offset 
     * @returns 
     */
    findBinSeq(buf, seq, offset) {
        return buf.indexOf(seq, offset);
    }

    /**
     * Compare 2 buffers and return true if matching completely byte-for-byte
     * @param {*} buf1 
     * @param {*} buf2 
     * @returns 
     */
    matchingBuf(buf1, buf2) {
        return buf1.compare(buf2);
    }

    /**
     * descramble the pages, xor and export
     * logic to set the xor data
     * we have multiple options
     * known good pages using a chunk, xor data files, or guessing
     * there are total of modSize xor chunks, 1088 bytes each 
     * modpage offset is relative tot he start of the parition ?
     * 
     */
    descramble() {
        var finalBuf = Buffer.alloc(this.pageCount * pageSize);

        for (var i = 0; i < this.pageCount; i++) {
            var page = this.page(i);
            // get relative partition offset
            //var partOffset = this.getPartitionOffset(i);
            //var modPage = partOffset % modSize;

            var modPage = i % modSize;
            //this.log(`using xor page ${modPage} based on offset ${partOffset} for page ${i} (was ${i % 64 })`);

            // try simple thing - use last chunk as xor
            // NO page.setXorData(page.chunk(3));

            var notSet = true;
            // if we have (visually/manually) identified a good one
            // we add it to the list, based on known good 
            // chunk n for a specific  pages            
            var xorMap = [
                { mod: 0, page: 0, chunk: 2 },
                { mod: 1, page: 1, chunk: 3 },
                { mod: 2, page: 2, chunk: 3 },
                { mod: 3, page: 3, chunk: 3 },
                { mod: 4, page: 4, chunk: 3 },
                //{ mod: 5, page: 5, chunk: 3 },
                //{ mod: 6, page: 6, chunk: 3 },
                //{ mod: 7, page: 7, chunk: 3 },
                //41, 42, 239, 294, 298, 299, 300, 301, 303, 677, 678, 682, 683, 684, 695];
                //{ mod: 8, page: 8, chunk: 0 },
                //{ mod: 43, page: 1835, chunk: 0 }
                //{ mod: 43, page: 2091, chunk: 0 }
            ];

            var item = xorMap.find((obj) => obj.mod === modPage);
            if (item !== undefined) {
                this.log(`getting xor data from page ${item.page} chunk ${item.chunk} (mod ${modPage})`);
                var xorPage = this.page(item.page);
                page.setXorData(xorPage.chunk(item.chunk));
                notSet = false;
            }

            // load xor files from disk if it exists
            var xorFile = `./exported/xor_chunks/${modPage}_xor.bin`;
            if (fs.existsSync(xorFile) && notSet) {
                this.log(`loading xor data from file ${xorFile} (mod ${modPage})`);
                var buf = this.loadBinFile(xorFile);
                page.setXorData(buf);
                notSet = false;
            }

            // guess it based on the data           
            if (notSet) {
                page.guessXor();
            }

            //console.log(page);            
            page.descramble();
            var data = page.data();
            data.copy(finalBuf, i * pageSize, 0, data.length);
        }
        return finalBuf;
    }

    /**
     * get relative offset in partition for given pagenumber
     * @param {*} pageno 
     * @returns 
     */
    getPartitionOffset(pageno) {
        var partitions = this.partitionTable;
        for (var part of partitions) {
            // in partition?
            if (pageno >= part.start_sect && pageno <= part.start_sect + part.nr_sects) {
                // offset is pageno - start page/sect
                var offset = pageno - part.start_sect;
                console.log(`relative offset ${offset} for ${pageno} in partition`, part);
                return offset
            }
        }
    }
    /**
     * get possible xor buffer based on identical bytes 
     * in 1088 blocks for multiple pages in scope
     * if not matching, use 0x00 as this does not change the original
     * page offsets are modSize
     * 
     * only userdata partition for now
     */
    createXorData() {
        // get chunks for all pages modSize apart

        var chunkSize = pageSize / chunks;
        var startPage = 0x3E80; // user datapages
        var endPage = 0x3E80 + 64000 // end is 0x1BF80;
        var pageCoverage = 0;
        var xorCoverage = 0;
        var minOccurs = 9;

        this.log(`loading from pages ${startPage} to ${endPage}`);
        for (var modPage = 0; modPage < modSize; modPage++) {
            this.log(`mod page ${modPage}`);
            var chunkList = [];

            for (var j = startPage; j < endPage; j++) {
                if (j % modSize === modPage) {
                    //this.log(`getting chunks for page ${j}`);
                    chunkList = chunkList.concat(this.page(j).allChunks());
                }
            }

            this.log(`got ${chunkList.length} chunks for mod page ${modPage}`);

            // loop over and add for most matches - should focus on last for each page            
            var xorBuf = Buffer.alloc(chunkSize, 0x00);
            var cnt = 0;
            for (var j = 0; j < chunkSize; j++) {
                var bytesColl = {}
                for (var chunk of chunkList) {
                    var byte = chunk[j]
                    if (bytesColl[byte] === undefined) {
                        bytesColl[byte] = 1;
                    } else {
                        bytesColl[byte] += 1;
                    }
                }
                // now we have all the bytes over the chunks in that position
                // get the max
                var occurs = 1;
                var byte = 0x00
                for (var key of Object.keys(bytesColl)) {
                    if (bytesColl[key] > occurs) {
                        occurs = bytesColl[key];
                        byte = key;
                    }
                }
                // assign only if > 9
                if (occurs > minOccurs) {
                    xorBuf[j] = byte;
                    cnt += 1;
                }
            }

            this.log(`created xor buffer for ${cnt} bytes [${100 * (cnt / chunkSize).toFixed(2)}%]`)
            console.log(xorBuf);
            console.log();

            // some coverage stats
            if (cnt === chunkSize) {
                pageCoverage += 1;
                console.log(pageCoverage);
            }

            xorCoverage += cnt;
            // export xor buffer
            this.exportBuf(xorBuf, `./exported/xor_chunks/${modPage}_xor.bin`)
        }
        this.log(`page coverage ${pageCoverage} pages at 100% [${(pageCoverage / modSize).toFixed(2)}%]`);
        this.log(`xor coverage ${(xorCoverage / (modSize * chunkSize)).toFixed(2)}%`);
    }

    /**
     * extract and list ecc blocks (the 64 bytes at the end of each 1088 chunk in a page)
     */
    listECCs() {
        var startPage = 0; //16064;
        var lastPage = this.pageCount; //startPage + 256;
        var emptyBuf = Buffer.from([0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]);
        var empty = 0;
        var fixPages = [];
        for (var i = startPage; i < lastPage; i++) {
            var p = this.page(i);
            var eccs = p.ecc();
            for (var e of eccs) {
                if (e[0] !== 0xFF) {
                    if (empty > 0) {
                        console.log(`... ${empty} empty chunks ...`);
                        empty = 0;
                    }
                    // only want ones with incorrect data
                    if (e.indexOf(emptyBuf) !== 0) {
                        console.log('p', i, 'offs', i * 4352, 'ecc', e);
                        fixPages.push(i);
                    }
                } else {
                    empty += 1
                }
            }
            if (i % 64 === 0) {
                console.log(`-- new block`);
            }
        }
        var uniquePages = [...new Set(fixPages)];
        console.log(uniquePages);
    }

    /**
     * remove all the ecc and xor chunks and export
     * @param {*} filename 
     */
    extractCleanFile(filename) {
        // we assume 4096 bytes per page, 
        this.exportBuf(finalBuf, filename);
    }

    log(msg) {
        console.log(`FlashReader: ${msg}`);
    }
}

/**
 * Page in the flash file
 */
class Page {
    // page params
    pageSize = pageSize;
    pageNum = -1;
    rawData = Buffer.alloc(pageSize);
    xorData = undefined; // data to do the xor with (chunk)
    descrambledData = undefined; // actual data after xor is applied
    chunks = chunks;

    constructor(buf) {
        this.rawData = buf;
    }

    /**
     * 
     * @returns all the (original) data eg raw
     */
    raw() {
        return this.rawData;
    }

    /**
     * 
     * @returns descramble and return data
     */
    data() {
        /*
        does NOT work for all daTa
        // descramble using last chunk of page 
        // check all are 0xFF after xor and return 3 * chunkSize only
        var xorBuf = this.chunk(3);        
        var count = this.pageSize / xorBuf.length; // asume its an int               
        var finalBuf = Buffer.alloc(3 * xorBuf.length, 0xFF); // only data we need

        // get chunks and xor
        for (var i = 0; i < count - 1; i++) {
            var chunk = this.chunk(i);
            var tmpBuf = this.xor(chunk, xorBuf);
            var offset = i * chunk.length;
            // copy to resbuf for first 3
            if(i < 3) {
                tmpBuf.copy(finalBuf, offset, 0, tmpBuf.length);
            } else {
                // last tmpBuf should all be 0xFF
                for(var j = 0; j < tmpBuf.length; j++) {
                    if(tmpBuf[j] !== 0xFF) {
                        this.log(`ERROR: descrambling failed`);
                        return undefined
                    }
                }
            }
        }        
        this.log(`descrambled`);
        return finalBuf
        */
        return this.descrambledData;
    }

    /**
     * set page number
     * @param {*} n 
     */
    setNum(n) {
        this.pageNum = n;
    }

    /**
     * get page number
     * @returns 
     */
    getNum() {
        return this.pageNum
    }

    /**
     * return buffer of chunk from page
     * @param {*} number 
     */
    chunk(number) {
        var chunkSize = this.pageSize / this.chunks;
        return this.rawData.subarray(number * chunkSize, (number + 1) * chunkSize);
    }

    /**
     * check if page starts with 0xFF which means its empty
     * @returns true if empty
     */
    empty() {
        if (this.rawData[0] === 0xFF) {
            //this.log(`empty page`);
            return true
        } else {
            return false
        }
    }

    /**
     * xor the raw data in this page and assign xor
     * only apply xor if data does not start with 0xFF
     * @param {*} xorBuf 
     * @returns 
     */
    descramble() {
        if (this.empty()) {
            this.descrambledData = this.rawData;
            return
        }

        // we apply the xor buf n times to each chunk in the data        
        var count = this.pageSize / this.xorData.length; // asume its an int        
        var finalBuf = Buffer.alloc(this.pageSize, 0xFF);

        // get chunks and xor
        for (var i = 0; i < count - 1; i++) {
            var chunk = this.chunk(i);
            var tmpBuf = this.xor(chunk, this.xorData);
            var offset = i * chunk.length;
            // copy to resbuf
            tmpBuf.copy(finalBuf, offset, 0, tmpBuf.length);
        }
        this.descrambledData = finalBuf;
        this.log(`descrambled`);
    }

    /**
     * xor the buf and return xorred-result
     * @param {*} buf 
     * @param {*} xor 
     */
    xor(buf, xor) {
        var res = Buffer.alloc(buf.length);
        for (var i = 0; i < buf.length; i++) {
            res[i] = buf[i] ^ xor[i];
        }
        return res;
    }

    /**
     * set xor data from buffer
     */
    setXorData(buf) {
        this.xorData = buf;
    }

    /**
     * overlay the data by chunks and determine the xor values 
     * based on repeating bytes and add, only if > 2 are found
     * @returns xor buf
     */
    guessXor() {
        // only if we have data
        if (this.empty()) { return }
        this.log('guessing xor');

        var chunkSize = this.pageSize / this.chunks;
        var xorBuf = Buffer.alloc(chunkSize, 0x00);

        for (var i = 0; i < chunkSize; i++) {
            // get bytes
            var b0 = this.chunk(0)[i];
            var b1 = this.chunk(1)[i];
            var b2 = this.chunk(2)[i];
            var b3 = this.chunk(3)[i];

            // b3 first, then b2, then b1
            if (b3 === b2 || b3 === b1 || b3 === b0) {
                xorBuf[i] = b3
            } else if (b2 === b1 || b2 === b0) {
                xorBuf[i] = b2
            }
        }

        this.xorData = xorBuf
        return xorBuf;
    }

    allChunks() {
        var res = [];
        if (this.empty()) { return [] }
        for (var i = 0; i < 4; i++) {
            var chunk = this.chunk(i);
            // do not want empty ones
            if (chunk[0] !== 0xFF) {
                res.push(chunk);
            }
        }
        return res;
    }

    /**
     * returns final data for given page, only 3 * 1088 bytes
     * assumes complete file is loaded, e.g. also 0xFF chunks
     * each chunk is 1088 bytes of which 1024 data, 64 bytes ecc, 
     * rest 0xff we only want to first 3x1024, no padding    
     * @returns buf with actual page data
     */
    finalData() {
        // we need the 3 * 1024 bytes only
        var size = 1024;
        var rawSize = 1088;
        var tmpBuf = this.rawData.subarray(0, 3 * rawSize);
        var resBuf = Buffer.alloc(3 * size);

        for (var i = 0; i < 3; i++) {
            var offset = i * rawSize;
            var data = tmpBuf.subarray(offset, offset + size);
            console.log(data.length);
            data.copy(resBuf, i * size, 0, data.length);
        }
        return resBuf
    }

    // page is split in 4 chunks, 1024 bytes data + 64 bytes ecc (or something)
    // we only want to 1024 * 3 and emtpy space at end (48 pages of data, 14 empty)
    // LEB size is 62 pages, PEB is 64
    ubiData() {
        // pages 0/1 4096 bytes
        var modPage = this.pageNum % 64;
        var size = 3 * 1024;
        if (modPage === 0 || modPage === 1) {
            size = 4 * 1024;
        }
        var res = Buffer.alloc(size);
        console.log(modPage, res.length);
        for (var i = 0; i < 3; i++) {
            var offset = i * 1088;
            var buf = this.rawData.subarray(offset, offset + 1024);
            //console.log(offset, buf.length, buf);
            buf.copy(res, i * 1024, 0, buf.length);
        }
        return res;
    }

    // for each first 3 * 1088, return 3 bufs with last 64 bytes
    ecc() {
        //console.log(this.rawData);
        var res = [];
        for (var i = 0; i < 3; i++) {
            //console.log(`getting bytes [${1024 + (i * 1088)}] to [${(i + 1) * 1088}]`);
            var buf = this.rawData.subarray(1024 + (i * 1088), (i + 1) * 1088);
            res.push(buf);
        }
        return res
    }

    log(msg) {
        console.log(`Page [${this.pageNum}]: ${msg}`)
    }
}

/**
 * Block in the flash file; assumes data in block is xorred
 */
class Block {
    // block params
    pagesPerBlock = 64;
    rawPageSize = 4352;
    pageSize = 4096;
    chunkSize = 1088;
    //compOffset = 8; // compensation offset

    rawBlockSize = this.pagesPerBlock * this.rawPageSize;
    blockSize = this.pagesPerBlock * this.pageSize;

    pageNum = -1;
    data = Buffer.alloc(this.rawBlockSize);

    constructor(buf) {
        this.data = buf;
    }

    /**
     * set block number
     * @param {*} n 
     */
    setNum(n) {
        this.pageNum = n;
    }

    /**
     * get block number
     * @returns 
     */
    getNum() {
        return this.pageNum
    }

    /**
     * check if block starts with 0xFF which means its empty
     * @returns true if empty
     */
    empty() {
        if (this.data[0] === 0xFF) {
            return true
        } else {
            return false
        }
    }

    /**
     * each physical UBI block (PEB) has 64 pages, 2 for the headers 
     * and 62 for the data (LEB); each page is split in 4 chunks, 
     * 1024 bytes data + 64 bytes ecc (or something) and last 1024 + 64 is 
     * xor related and we only want to the 1024 * 3 chunks. 
     * Map the data directly to the 4096 page
     * page 0 .. 1 are padded with 0 to size 4096 to ensure correct offsets
     * page 2 .. 63 are 'collapsed', no padding
     */

    // adjust pages 0 + 1 with backfill to get 4096 size
    
    ubiData() {
        // return only valid bytes in the 3 * 1024 block, skip the 0xFF chunks
        // and collapse all the data
        var res = Buffer.alloc(this.blockSize, 0x00);
        var dataSize = 1024; // part of the chunk we want
        var trgStep = 1024; // offset in target buffer for each data chunk
        var pageBackfill = 0; //1024; // backfill on each page after 3 chunks
        // chunks in block of 1088
        var chunks = this.rawBlockSize / this.chunkSize;
        console.log('');
        this.log(`have ${chunks} to handle`);
        var trgOffset = 0; // where it lands
        // process chunks
        for (var c = 0; c < chunks; c++) {
            // we need 0,1,2 + not 3, 7, 11...
            if (c % 4 === 3) {
                this.log(`skipping chunk ${c}`);
                // but we add 128 bytes after c > 7, pages 0 + 1
                if (c > 7) { trgOffset += pageBackfill; }
            } else {
                //this.log(`copying 1024 bytes to ${trgOffset}`);                
                var offset = c * this.chunkSize;
                var dataBuf = this.data.subarray(offset, offset + dataSize);
                //console.log(dataBuf);
                dataBuf.copy(res, trgOffset, 0, dataBuf.length);
                // only increase when copying 
                trgOffset += trgStep;
                // when we handle last chunk in page 0 + 1,
                // add another chunk so we have right offsets
                if (c === 2 || c === 6) {
                    trgOffset += 1024;
                }
            }
        }
        return res;
    }
    /*
    ubiData() {
        // return only valid bytes in the 3 * 1024 block, skip the 0xFF chunks
        // and collapse all the data
        var res = Buffer.alloc(this.blockSize);
        var dataSize = 1024;
        var trgStep = 1024;
        // chunks in block of 1088
        var chunks = this.rawBlockSize / this.chunkSize;
        console.log('');
        this.log(`have ${chunks} to handle`);
        var trgOffset = 0; // where it lands
        // process chunks
        for (var c = 0; c < chunks; c++) {
            // we need 0,1,2 + not 3, 7, 11...
            if(c % 4 === 3) {
                this.log(`skipping chunk ${c}`);
            } else {
                //this.log(`copying 1024 bytes to ${trgOffset}`);                
                var offset = c * this.chunkSize;                
                var dataBuf = this.data.subarray(offset, offset + dataSize);
                //console.log(dataBuf);
                dataBuf.copy(res, trgOffset, 0, dataBuf.length);
                // only increase when copying 
                trgOffset += trgStep;
                // when we handle last chunk in page 0 + 1,
                // add another chunk so we have right offsets
                if(c === 2 || c === 6) {
                    trgOffset += 1024;
                }
            }
        }
        return res;
    }
    */

    // This version breaks the ubi data as it intersperse -xFF block (the xor ones)
    // each physical UBI block has 64 pages, 2 for the headers and 62 for the data (LEB)
    // each page is split in 4 chunks, 1024 bytes data + 64 bytes ecc (or something) and last 1024 + 64 is xor related
    // we only want to the 1024 * 3 and map the data directly to the 4096 page    
    /*
    ubiData() {
        // cleanedup buffer for block        
        var res = Buffer.alloc(this.blockSize);
        var dataSize = 1024;
        // chunks in block of 1088
        var chunks = this.rawBlockSize / this.chunkSize;
        console.log('');
        this.log(`have ${chunks} to handle`);
        var trgOffset = 0; // where it lands
        // process chunks
        for (var c = 0; c < chunks; c++) {
            //this.log(`copying 1024 bytes to ${trgOffset}`);
            var offset = c * this.chunkSize;
            var trgOffset = c * dataSize
            var dataBuf = this.data.subarray(offset, offset + dataSize);
            //console.log(dataBuf);
            dataBuf.copy(res, trgOffset, 0, dataBuf.length);
        }
        return res;
    }
    */

    // OLD VERSION which had wrong mapping for ecc data etc
    /*
    ubiData() {
        // cleanedup buffer for block        
        var res = Buffer.alloc(this.blockSize);
        // chunks in block of 1088
        var chunks = this.rawBlockSize / this.chunkSize;
        console.log('');
        this.log(`have ${chunks} to handle`);
        var trgOffset = 0; // this changes variably
        // process chunks, page 0 and 1 are special as they MUST be 4096 bytes
        // so process the first 8 chunks manually
        for (var c = 0; c < 8; c++) {
            if ((c + 1) % 4 === 0) {
                //this.log(`skipping chunk ${c}`);
                // but we do add offset!
                trgOffset += 1024;
            } else {
                //this.log(`copying 1024 bytes to ${trgOffset}`);
                var offset = c * this.chunkSize;
                var chunkBuf = this.data.subarray(offset, offset + this.chunkSize);
                console.log(chunkBuf);
                chunkBuf.copy(res, trgOffset, 0, 1024);
                trgOffset += 1024;
            }
        }
        // then the rest, same concept and with a compensation offset
        // for every 3rd chunk
        for (var c = 8; c < chunks; c++) {
            // we only want 0, 1 & 2           
            if ((c + 1) % 4 === 0) {
                this.log(`skipping chunk ${c}`)
                // and change offset!
                trgOffset -= 8;
            } else {
                // read chunk data
                var offset = c * this.chunkSize;
                var chunkBuf = this.data.subarray(offset, offset + this.chunkSize);
                // we only need 1024 bytes or 1016 for every 3rd chunk
                var length = 1024;
                //console.log(chunkBuf);   
                //this.log(`chunk ${c} copying ${length} bytes to ${trgOffset}`);                
                chunkBuf.copy(res, trgOffset, 0, length);
                trgOffset += length;
            }
        }
        return res;
    }
    */

    log(msg) {
        console.log(`Block [${this.pageNum}]: ${msg}`)
    }
}

// exports
exports.Page = Page;
exports.FlashDumpReader = FlashDumpReader;


