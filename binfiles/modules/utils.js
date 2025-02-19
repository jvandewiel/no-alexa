// util functions for handling binary files and buffers

import * as fs from 'fs';

let binfile;
let fname; // filename

/**
 * load binary file
 * @param {*} filename 
 * @returns buffer object
 */
export function loadBinFile(filename) {    
    try {
        let size = fs.statSync(filename).size;        
        if (size > 0) {
            log(`loading file ${filename}, size ${size} bytes`);
            let data = new Buffer.alloc(size);
            binfile = fs.openSync(filename, 'r');
            fname = filename;
            let bytesRead = fs.readSync(binfile, data, 0, size, 0);
            log(`read ${bytesRead} bytes from ${filename}`);
            return data
        } else {
            return undefined
        }
    } catch (err) {
        console.log(err);
        return undefined
    }
}

/**
     * Close the file
     */
export function closeFile() {
    log(`closing binary file ${fname}`)
    fs.closeSync(binfile);
}

/**
 * parse a buffer into values based on a map object
 * assumes map starts at 0 in the buffer
 * @param {*} map
 */
export function parseBuf(bytes, map) {
    let offset = 0;
    let res = {};
    let buf = Buffer.from(bytes);
    log("buffer: " + buf.toString());
    for (let key in map) {
        let tmpBuf = buf.subarray(offset, offset + map[key]);        
        // convert            
        switch (map[key]) {
            case 1:
                res[key] = tmpBuf.readUInt8();
                break
            case 2:
                res[key] = tmpBuf.readUInt16LE();
                break
            case 4:
                res[key] = tmpBuf.readUInt32LE();
                break
            default:
                res[key] = tmpBuf;
        }
        offset += map[key];
    }
    console.log(res);
    return res
}

/**
 * Export a buffer to a (binary) filename
 * @param {*} buf 
 * @param {*} filename 
 */
export function exportBuf(buf, filename) {
    let expfile = fs.openSync(filename, 'w');
    fs.writeSync(expfile, buf, 0, buf.length, 0);
    log(`written ${buf.length} bytes to ${filename}`);
    fs.closeSync(expfile);
}

/**
 * return size of all elements in a map
 * @param {*} map 
 * @returns 
 */
export function structSize(map) {
    let size = 0;
    for (let key in map) { size += map[key] }
    return size;
}

/**
 * simple logger
 * @param {*} msg 
 */
function log(msg) {
    console.log(`[Utils] ${msg}`);
}