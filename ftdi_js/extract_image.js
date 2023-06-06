// extract image and oob data from dump file
var fs = require('fs');
const { exit } = require('process');

var sourceFile = 'dump_008.bin';
var targetFile = 'flash_001.img';

// page data
var pageSize = 4096;
var oobSize = 256;
var pagesPerBlock = 64;
var blockSize = pageSize * 64;

var writeSize = pageSize + oobSize;

// open file, read page + oob, extract page buf and append to bin file
var stats = fs.statSync(sourceFile);
console.log(`File size ${stats.size}`);
/*
var fs = require('fs');

fs.open('file.txt', 'r', function(status, fd) {
    if (status) {
        console.log(status.message);
        return;
    }
    var buffer = Buffer.alloc(100);
    fs.read(fd, buffer, 0, 100, 0, function(err, num) {
        console.log(buffer.toString('utf8', 0, num));
    });
});
*/

fs.open(sourceFile, 'r', function(status, fd) {
    if (status) {
        console.log(status.message);
        return;
    }
    
    var rawPageSize = pageSize + oobSize;
    var pageCount = 100; //stats.size/rawPageSize;
    console.log(`Reading ${pageCount} pages from ${sourceFile}`);

    var buffer = Buffer.alloc(rawPageSize);
    var pageBuf = Buffer.alloc(pageSize);
    var oobBuf = Buffer.alloc(4096);

    var offset = 0; // num page

    var options = {
        flags: 'as', //'as': Open file for appending in synchronous mode. The file is created if it does not exist.
        encoding: 'binary',
    }
    pageFile = fs.createWriteStream(targetFile, options);

    for(var page = 0; page < pageCount; page++) {
        offset = page * rawPageSize;
        console.log(`Reading page #${page} of total ${pageCount} at offset ${offset} of length ${rawPageSize} [${Math.round(page/pageCount*100)}%]`);
            // fs.read(fd, buffer, offset in buffer, length, position, callback)
        var numRead = fs.readSync(fd, buffer, 0, rawPageSize, offset);
        //console.log(`Read data ${numRead} [${buffer[0]}, ${buffer[1]}, ... ${buffer[4094]}, ${buffer[4095]}]`);
        // copy buffer
        buffer.copy(pageBuf, 0, 0, 4096);
        //console.log(`Appending page bytes (first 5) ${pageBuf[0]} ${pageBuf[1]} ${pageBuf[2]} ${pageBuf[3]} ${pageBuf[4]}`);
        // append to target file      
        //fs.writeFile(targetFile, pageBuf, { flag:'a+'});
        let result = pageFile.write(pageBuf, 'binary', function() {
            console.log('flushed');
        });   
        if (result) {
            console.log('write result true');
        } else {
            console.log('waiting for drain');
            pageFile.once('drain', function() {
                console.log('drained');
            });
        }
    } 
    pageFile.end();
    
    pageFile.on('finish', () => {
        console.log('Written...');        
    });
    
    pageFile.on('drain', () => {
        console.log('resuming');
    });

    pageFile.on('error', (err) => {
        console.error(err.message)
    }); 
    
    process.exit(0);
});

