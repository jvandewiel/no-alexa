


// add based on xor guess
for (var p = 0; p < 64; p++) {
    if (goodBufs[p] === undefined) {
        xorBuf = getXorBuf(p);
        console.log(`adding xor guess ${p}`);
        goodBufs[p] = xorBuf;
    }
}

// convert to binary sequence, we want the first 64 as , seperated string
// could be reverse
/*
binBuf = xorBuf.subarray(0, 32);
console.log('xorBuf', binBuf);
const binary = BigInt('0x' + binBuf.toString('hex')).toString(2).padStart(binBuf.length * 8, '0')
// seq = (1,0,1,1,1,0,0,1)
for (var i = 0; i < 4; i++) {
    var orgArr = binary.substring(i * 8, (i + 1) * 8).split('');
    console.log('org', orgArr);
    // reverse
    var arr = orgArr.reverse();
    console.log('rev', arr);
    console.log('seq = (', arr.join(','), ')');
}
*/


/* 
visual inspection learns pages 38337-38370 contain 
a lot of words/strings in uppercase, so lets use that 
to define a better set of xor data
*/

/*
pagesStart = 38337;
pagesEnd = 38370;
pagesCount = pagesEnd - pagesStart;
tmpBuf = Buffer.alloc(pagesCount * pageSize)
console.log(`exporting ${pagesCount} xorred pages`);

// start with writing xor buf to file
// modify the file manually and use that combined with xor to get better xor
// store the xor files so we can modify and reuse them
// we only need the chunks (!) so 1088 bytes, 64 xor_chunks
// export the chunks by page number, import to xor and done

// what we do is:
// export the xorred pages by chunk, (page.chunk.bin, e.g. 1_0, 1_1 etc)
// fix the contents for 32 of these chunk, whatever is easy
// load these to determine the xor chunks (we know the page and chunk)
// and use these xors on the rest
// maybe apply similar logic or put 0xff in spaces we do not know??

// strip out all the is not ascci capital letters    
            for (var i = 0; i < data.length; i++) {
                //console.log(data[i].toString(16), data[i]);
                if (data[i] >= 0x41 && data[i] < 0x5A) {
                    // keep
                } else {
                    data[i] = 0xFF
                }
            }
        
            // we use this to determine the xor values and store those, needs to be done by chunk
            writeBuf(data, `./xored_pages/${modP}_stripped.bin`);
            console.log(data);

*/



// resulting buffer with xorred data
var resBuf = Buffer.alloc(pageSize);
// check if we want to process
if (pageBuf[0] !== 0xFF) {
    // check if we have a good xor page
    var xorPage = i % 64;
    if (goodBufs[xorPage] !== undefined) {
        //console.log(`page ${i} applying known xor ${xorPage} (offset ${i * pageSize})`);
        var xorBuf = goodBufs[xorPage];
    } else {
        // if not a good page, do matching approach -> does not work very well
        // console.log(`page ${i} applying dumb xor (offset ${i * pageSize})`);
        // var xorBuf = getXorBuf(pageBuf);
        // best result so far
        var xorBuf = pageBuf.subarray(3 * chunkSize, 4 * chunkSize);
    }

    for (j = 0; j < chunks; j++) {
        // get org and xor
        var xorOffset = j * chunkSize;
        tmpBuf = xor(pageBuf.subarray(xorOffset, xorOffset + chunkSize), xorBuf);
        // copy to resbuf
        tmpBuf.copy(resBuf, xorOffset, 0, tmpBuf.length);
    }

    // copy resBuf to finalBuf
    resBuf.copy(finalBuf, i * pageSize, 0, resBuf.length);
    // also put this in shortBuf, but with different offset
    resBuf.copy(shortBuf, nonEmptyPages * pageSize, 0, resBuf.length);
    nonEmptyPages += 1;
} else {
    // 0xFF page
    //console.log(`page ${i} 0xFF`);
    pageBuf.copy(finalBuf, i * pageSize, 0, resBuf.length);
}



//console.log('selected', byteVal, bytesColl);

/**
* get the byte that occurs the most or 0x00 if only 1 found
* @param {*} bytes 
* @returns 
*/

function handleFoundValues(bytes) {


    // handle
    var values = {};

    for (var i = 0; i < bytes.length; i++) {
        // make hashtable      
        var key = bytes[i];
        if (key !== 0xFF) {
            if (values[key] === undefined) {
                values[key] = 1
            } else {
                values[key] += 1
            }
        }
    }
    console.log('xor bytes object', values);

    // make array
    var array = [];
    for (var key in values) {
        array.push({
            byte: key,
            occurs: values[key]
        });
    }

    var sorted = array.sort(function (a, b) {
        return (a.occurs > b.occurs) ? 1 : ((b.occurs > a.occurs) ? -1 : 0)
    });

    console.log('sorted xor bytes array', sorted);
    // return first
    return sorted[0];
}