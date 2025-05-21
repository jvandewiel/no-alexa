# Python script to read the flash

Run `python dumppages.py` to extract the flash data, page by page. The data is read multiple times for each page and compared - only when identical is it stored. It might require reading certain pages multiple times to get a complete set. Control which pages are read is in this file.

- `flashimage.py` is the file for handling reading of a page, comparison of read pages and storage - this also contains the parameter for the number of times a page is read.

- `flashdevice.py` is the "wrapper" around the hardware using the FTDI to interface with the flash chip, specifically the MX30LF4G28AD
