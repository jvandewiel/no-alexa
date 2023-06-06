// extract image and oob data from dump file
var fs = require('fs');
const { exit } = require('process');

var sourceFile = 'pages0_150000.bin';
var targetFile = 'flash_001.img';

let readStream = fs.createReadStream(sourceFile);
let writeStream = fs.createWriteStream(targetFile);

// page parameters
var pageSize = 4096;
var oobSize = 256;
var rawSize = pageSize + oobSize;
var pagesPerBlock = 64;
var blockSize = pageSize * 64;
var leftOver;

var writeSize = pageSize + oobSize;
var page = Buffer.alloc(pageSize);

// on open
readStream.on('open', function () {
    console.log('readstream is open');    
});


// read chunks
pageNum = 0;
readStream.on('data', function (buf) {    
    console.log(`received ${buf.length} bytes`);
    // add leftovers - new buf of size leftover + page, process, the next

    // have a chunk of data, calc number of pages and move leftover
    var numPages = Math.floor(buf.length / rawSize);
    var pageBufSize = numPages * pageSize;
    var leftBufSize = buf.length % rawSize;

    console.log(`extracting ${numPages} pages into ${pageBufSize} bytes `);  
    var pageBuf = Buffer.alloc(pageBufSize);    
    leftOver = Buffer.alloc(leftBufSize);    
    
    console.log(`allocated ${pageBufSize} bytes for ${numPages} pages, allocated ${leftBufSize} bytes for leftovers`);
    
    // copy leftover
    // buffer.copy(target, targetStart, sourceStart, sourceEnd);
    buf.copy(leftOver, 0, buf.length - leftBufSize, leftBufSize);
    console.log(`copied ${leftBufSize} to leftover`);

    var offset = 0;
    for(var page = 0; page < numPages; page ++) {
        offsetSrc = page * rawSize;
        offsetTrg = page * pageSize;
        buf.copy(pageBuf, offsetTrg, offsetSrc, offsetSrc + rawSize);
    }
    console.log(`copied ${pageBufSize} to pageBuf`);

    //newBuf = Buffer.alloc(pageSize * pages);
    //pages = 
    //buf.copy(page, 0, 0, 4096);
    writeStream.write(pageBuf);    
});

// write some data with a base64 encoding
//writeStream.write('aef35ghhjdk74hja83ksnfjk888sfsf', 'base64');

// the finish event is emitted when all data has been flushed from the stream
writeStream.on('finish', () => {
    console.log('WS: wrote all data to file');
});

// close the stream
writeStream.end();
