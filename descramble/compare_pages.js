var fs = require('fs');

// load binary file if it exists
function loadBinFile(filename) {
    try {
        var size = fs.statSync(filename).size;
        if (size > 0) {
            var buf = Buffer.alloc(size);
            var binfile = fs.openSync(filename, 'r');
            bytesRead = fs.readSync(binfile, buf, 0, size, 0);
            //console.log(`read ${bytesRead} bytes from ${filename}`);
            return buf;
        } else {
            return undefined
        }
    } catch (err) {
        //console.log(err);
        return undefined
    }
}

// create buffer of size and fill with val
function getFilledBuf(size, val) {
    var buf = Buffer.alloc(size)
    for (var i = 0; i < size; i++) { buf[i] = val }
    return buf;
}
// compare exported pages, merge if identical
var orgDir = '/home/joost/Projects/gitea/echodotv3/flashdump/pages_0/';
var cmpDir = '/home/joost/Projects/gitea/echodotv3/flashdump/pages_1/';
var mergefile = '/home/joost/Projects/gitea/echodotv3/flashdump/merged_pages.bin';

// prepare target buffer 
var pageCount = 16000;
var pageSize = 4352;

var buf = Buffer.alloc(pageCount * pageSize);
console.log(`allocated buffer ${buf.length}`);

for (var i = 0; i < pageCount; i++) {
    // calc offset
    offset = i * pageSize;
    //console.log(`offset in buf ${offset}`);
    // get names
    var orgFilename = `${orgDir}${i}.bin`;
    var cmpFilename = `${cmpDir}${i}.bin`;
    // load data, if not possible buf will be undefined
    var orgBuf = loadBinFile(orgFilename);
    var cmpBuf = loadBinFile(cmpFilename);

    // compare
    if (orgBuf !== undefined && cmpBuf !== undefined) {
        var identical = true;
        for (var j = 0; j < orgBuf.length; j++) {
            if (orgBuf[j] !== cmpBuf[j]) {
                identical = false;
                break;
            }
        }
        if (!identical) {
            console.log(`${cmpFilename} different @${j}`);
            // remove both files
            //fs.unlinkSync(cmpFilename);
            //fs.unlinkSync(orgFilename);
            //console.log(`removed files`)
            // append 0xFF buf
            var tmp = getFilledBuf(pageSize, '0xFF');
        } else {
            // console.log(`${files[i]} identical`);
            // append to merge file
            orgBuf.copy(buf, offset, 0, pageSize);
        }
    } else {
        console.log(`compare ${cmpFilename} does not exists`);
        // append 0xFF buf
        var tmp = getFilledBuf(pageSize, '0xFF');
        tmp.copy(buf, offset, 0, pageSize);
    }
}

// write buffer
var exportFile = fs.openSync(mergefile, 'w');
fs.writeSync(exportFile, buf, 0, buf.length, 0);
console.log(`written ${buf.length} bytes to ${mergefile}`);
fs.closeSync(exportFile);

console.log(`done`)