/* decode the dumped flash pages */

const { FlashDumpReader } = require('./dumpreader');

var pages = 131069;
var sourceDir = './pages'; // contains all pages in i.bin format
var exportDir = '../exported'; // contains exported data
var partDir = '../exported/partitions';
var rawPartDir = '../exported/raw_partitions';

// construct new reader
var dr = new FlashDumpReader();

// load the pages from the source dir, merge and store
var rawDataFile = `${exportDir}/flashdump_raw.bin`;
var dataFile = `${exportDir}/flashdump.bin`;

// only do this when needed
//dr.mergePages(sourceDir, rawDataFile, pages);

// load the merged flash dump
// descramble everything and export, when needed
/*
dr.openFile(rawDataFile);
//dr.createXorData(); // generate xor chunks
// perhaps different strategies for dirrent partitions? boot_a/_b works fin for xor-chunks but not in-page descrambling
// maybe do a full per-page descramble both ways and compare outcome?
var data = dr.descramble();
dr.exportBuf(data, dataFile);
*/
// export all partitions
dr.openFile(dataFile);
//dr.listPartitions();
//dr.efi();
//dr.gpt();

var part = dr.getPartitionMeta('userdata');
//dr.exportRawPartition(part, rawPartDir);
dr.exportPartition(part, partDir);

// eec stuff
//dr.openFile(dataFile);
//dr.listECCs();
//dr.extractCleanFile(`${exportDir}/flashdump_clean.bin`);
dr.closeFile();


