// convert dump from stage 2 mtk client yo binary
// stage2_dump_to_bin --source=<dump> --target=<outfile>

// load file as string
// strip first x bytes
// determine number of blocks to read (8192 block size)
// read blocks of 8192 values (00ffa1....) and convert to binary 4096 block (0x00 0xdd 0xa1)
// append to binary file

// NOT NEEDED - --filename does it 