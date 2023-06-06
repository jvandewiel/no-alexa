
//const DEBUG  // Debugging

const PAGE_SIZE = 4352; // 4096 + 256 bytes, 256 bytes ECC/page
const BLOCK_SIZE = 278528; // 64 pages of4352 bytes
const MAX_WAIT_READ_BUSY = 1000000;

const PAGE_BUF_SIZE = 4352 // reading length for page buffer testing

// GPIO pins have been chosen to be compitable w/ Waveshare NandFlash Board and lost RPi SMI NAND driver
const WRITE_PROTECT = 2
const READY_BUSY = 3
const ADDRESS_LATCH_ENABLE = 4
const COMMAND_LATCH_ENABLE = 17
const READ_ENABLE = 18
const WRITE_ENABLE = 27
const CHIP_ENABLE = 22 // needed?

// mapping of the I/O pins
data_to_gpio_map[8] = { 23, 24, 25, 8, 7, 10, 9, 11 }; // 23 = I/O 0 .. 11 = I/O 7

//enum {SECS_TO_SLEEP = 0, NSEC_TO_SLEEP = 25}; //?

function SET_GPIO_INPUT(int g) {
    console.log("Setting direction of GPIO#%d to INPUT\n", g);
    (* (gpio + ((g) / 10)) &= ~(7 << (((g) % 10) * 3)));
}

function SET_GPIO_OUTPUT(int g) {
    SET_GPIO_INPUT(g);
    console.log("Setting direction of GPIO#%d to INPUT\n", g);
	* (gpio + ((g) / 10)) |= (1 << (((g) % 10) * 3));
}

function GPIO_SET_HIGH(int g) {
    console.log("Setting GPIO#%d to 1\n", g);
	* (gpio + 7)  = 1 << g;
    SHORTPAUSE();
}

function GPIO_SET_LOW(int g) {
    console.log("Setting GPIO#%d to 0\n", g);
	* (gpio + 10)  = 1 << g;
    SHORTPAUSE();
}

function GPIO_READ(int g) {
	var x = (* (gpio + 13) & (1 << g)) >> g;
    console.log("GPIO#%d reads as %d\n", g, x);
    return x;
}

function SET_DATA_DIRECTION_INPUT(void) {
    console.log("Set data direction to INPUT\n");
    for (var i = 0; i < 8; i++) {
        SET_GPIO_INPUT(data_to_gpio_map[i]);
    }
}

function SET_DATA_DIRECTION_OUTPUT(void) {
    console.log("Set data direction to OUTPUT\n");
    for (var i = 0; i < 8; i++)
        SET_GPIO_OUTPUT(data_to_gpio_map[i]);
}

// Read all bits into a single byte from IO
function GPIO_READ_BYTE() {
    var data;
    for (var i = data = 0; i < 8; i++, data = data << 1) {
        data |= GPIO_READ(data_to_gpio_map[7 - i]);
    }
    data >>= 1;
    console.log("GPIO_READ_BYTE: data=%02x\n", data);
    return data;
}

function GPIO_WRITE_BYTE(int data) {
    console.log("GPIO_WRITE_BYTE: data=%02x\n", data);
    for (var i = 0; i < 8; i++, data >>= 1) {
        if (data & 1)
            GPIO_SET_HIGH(data_to_gpio_map[i]);
        else
            GPIO_SET_LOW(data_to_gpio_map[i]);
    }
    SHORTPAUSE();
}

function CLE_HIGH() {
    GPIO_SET_HIGH(COMMAND_LATCH_ENABLE);
}

function CLE_LOW() {
    GPIO_SET_LOW(COMMAND_LATCH_ENABLE);
}

function ALE_HIGH() {
    GPIO_SET_HIGH(ADDRESS_LATCH_ENABLE);
}

function ALE_LOW() {
    GPIO_SET_LOW(ADDRESS_LATCH_ENABLE);
}


const ADDRESS_OFFSET = 0

/* Return Message */
const FlashStatus = {
    Flash_Success,
    Flash_Busy,
    Flash_OperationTimeOut,
    Flash_ProgramFailed,
    Flash_EraseFailed,
    Flash_ReadIDFailed,
    Flash_CmdInvalid,
    Flash_DataInvalid,
    Flash_AddrInvalid
}

/* BOOL Definition */
const TRUE = 1
const FALSE = 0

// system flags
const PASS = 0
const FAIL = 1
const BUSY = 0
const READY = 1
const PROTECTED = 0
const UNPROTECTED = 1

/* Target Flash Device: MX30XXXX */
const MX30LF4G28AD 

/* Timer Parameter */
const TIMEOUT = 0
const TIMENOTOUT = 1
const TPERIOD = 240             // ns, The time period of timer count one
const COUNT_ONE_MICROSECOND = 16  // us, The loop count value within one micro-second

/* Device Parameter ( Time uint: us, Condition: worst case )
   Please refer to data sheet for advanced information. */

const ID_CODE0 = 0xc2
const ID_CODE1 = 0xdc
const ID_CODE2 = 0x90
const ID_CODE3 = 0xa2
const ID_CODE4 = 0x57
const ID_CODE5 = 0x03
const READ_ID_6BYTE
const FLASH_SIZE = 0x20000000
const FLASH_TIMEOUT_VALUE = 3000
const FIRST_BYTE_LATENCY_tR = 25
const tRCBSY = 25
const tDBSY = 1
const tCBSY = 700
const BP_PROT_MODE


/*
    Address convert (MSB)
    The NAND flash address reserved A15-A12
    User can switch Address_Convert option
    to on/off address convert, default is off
*/

//const Address_Convert 1

if (Address_Convert){
    //32 bit( x8 mode): BYTE3 | BYTE2 | {4'b0, BYTE1} | BYTE0
    //32 bit(x16 mode): BYTE3 | BYTE2 | {5'b0, BYTE1} | BYTE0
    const BYTE0_OFFSET = 0
    const BYTE1_MASK  = (0x0F >> ADDRESS_OFFSET)  // Mask address A15-A12 of BYTE1
    const BYTE1_OFFSET = 8
    const BYTE2_OFFSET= 12 - ADDRESS_OFFSET
    const BYTE3_OFFSET =20 - ADDRESS_OFFSET
    const BYTE4_OFFSET= 28 - ADDRESS_OFFSET
    const PAGE_MASK   = (0xFFF >> ADDRESS_OFFSET)
} else {
    //32 bit: BYTE3 | BYTE2 | BYTE1 | BYTE0
    const BYTE0_OFFSET = 0
    const BYTE1_MASK   =0xFF
    const BYTE1_OFFSET = 8
    const BYTE2_OFFSET =16
    const BYTE3_OFFSET =24
    const BYTE4_OFFSET =32
    const PAGE_MASK    =0xFFFF
}
const BYTE_MASK =0xFF

/* Flash Information */
var FlashInfo ={
    /* Timer Variable */
    Tus,        // time-out length in us
      Timeout_Value,
      Timer_counter
}



/* Flash Parameter Structure */
var FlashParameter ={
    uBusWidth: [],
     Address,
     Length,
};


/*
 * Utility Function
 */
// fptr = FlashInfo
function Set_Timer( fptr   ) {
    fptr.Timeout_Value = (fptr.Tus)/COUNT_ONE_MICROSECOND;
    fptr.Timer_counter = 0;
}

function Check_Timer( fptr   ) {
    if( fptr.Timer_counter < fptr.Timeout_Value ){
        fptr.Timer_counter = fptr.Timer_counter + 1;
        return TIMENOTOUT;
    }else{
        return TIMEOUT;
    }
}

void Wait_Timer( fptr   ) {
    /* Wait timer until time-out */
    while( fptr.Timer_counter < fptr.Timeout_Value ){
        fptr.Timer_counter = fptr.Timer_counter + 1;
    }
}

/*
 * Function:       WaitTime
 * Arguments:      TimeValue -> The time to wait in us
 * Return Value:   READY, TIMEOUT
 * Description:    Wait flash device for a specfic time
 */
function WaitTime( TimeValue ) {
    FlashInfo flash_info;
    flash_info.Tus = TimeValue;
    Set_Timer( flash_info );
    Wait_Timer( flash_info );
}

/*
 * Function:     InitFlash
 * Arguments:    None.
 * Return Value: None.
 * Description:  Initial MX30 flash device
 */
void InitFlash() {
    // Setup GPIOs
    SET_GPIO_INPUT(READY_BUSY);

    SET_GPIO_OUTPUT(WRITE_PROTECT);
    GPIO_SET_HIGH(WRITE_PROTECT);

    SET_GPIO_OUTPUT(READ_ENABLE);
    GPIO_SET_HIGH(READ_ENABLE);

    SET_GPIO_OUTPUT(WRITE_ENABLE);
    GPIO_SET_HIGH(WRITE_ENABLE);

    SET_GPIO_OUTPUT(COMMAND_LATCH_ENABLE);
    CLE_LOW();

    SET_GPIO_OUTPUT(ADDRESS_LATCH_ENABLE);
    ALE_LOW();

    //OUT_GPIO(N_CHIP_ENABLE);
    //GPIO_SET_LOW(N_CHIP_ENABLE);
}

/*
 * Function:     FlashWrite
 * Arguments:    Value -> data to write
 * Return Value: None.
 * Description:  Write a data to flash
 */

function WriteToFlash( value) {
    SET_DATA_DIRECTION_OUTPUT(); 
    GPIO_SET_LOW(WRITE_ENABLE);
    GPIO_WRITE_BYTE(Value); // Write ID byte 1
    GPIO_SET_HIGH(WRITE_ENABLE);
    //console.log("Wrote to flash 0x%02X \n", Value);
}

/*
 * Function:     FlashRead
 * Arguments:    None
 * Return Value: Read data
 * Description:  Read a data from flash
 */
function ReadFromFlash() {
    var buffer;
    
    /* REF:
    uBusWidth buffer;
    WRITE_EN = 0;
    // set GPIO become input.
    P1 = 0xFF;
    IN_VALID = 1;
    // wait out valid signal
    while( !OUT_VALID );
    // receive data

    // send ack to notice controller
    IN_VALID = 0;
    return buffer;
    */

    GPIO_SET_LOW(READ_ENABLE);
    // GPIO_SET_LOW(WRITE_ENABLE);??
    SET_DATA_DIRECTION_INPUT();
    buffer = GPIO_READ_BYTE(); //
    GPIO_SET_HIGH(READ_ENABLE);
    return buffer;
}

/*
 * Function:     SendCommand
 * Arguments:    CMD_code -> The MX30 flash command code
 * Return Value: None.
 * Description:  Send flash command
 */
function SendCommand( CMD_code) {
    //console.log("Sending command 0x%02X\n", CMD_code);

    CLE_HIGH();             /* Enable command latch signal */
    WriteToFlash(CMD_code); /* Send commmand data */
    CLE_LOW();              /* Disable command latch signal */
}

/*
 * Function:     Status_Read_OP
 * Arguments:    StatusReg -> store status value
 * Return Value: Flash_Success
 * Description:  The status read command outputs the device status
 */
function ReadStatusOP( StatusReg ) {

    //console.log("Status before check 0x%02X\n", *StatusReg);
    /* Send status read command */
    SendCommand(0x70);

    /* Read status value */    
    StatusReg = GPIO_READ(READY_BUSY);
    
    //console.log("Status after check 0x%02X\n", *StatusReg);

    return Flash_Success;
}

/*
 * Function:     SendByteAddr
 * Arguments:    Byte_addr -> one byte address
 * Return Value: None.
 * Description:  Send one byte address
 */
function SendByteAddress(  Byte_addr ) {
    /* Enable address latch signal */
    ALE_HIGH();

    /* Send address data */
    WriteToFlash( Byte_addr );

    SET_DATA_DIRECTION_INPUT(); //??

    /* Disable address latch signal */
    ALE_LOW();
}

/*
 * Function:     Reset_OP
 * Arguments:    None
 * Return Value: Flash_Success
 * Description:  The reset command FFh resets the read/program/erase
 *               operation and clear the status register to be E0h
 */
function ResetFlashOP(  ) {
    /* Send reset command */
    SendCommand( 0xFF );

    /* Wait reset finish */
    WaitTime( FLASH_TIMEOUT_VALUE );
    return Flash_Success;
}

/*
 * Function:     SendLongAddress
 * Arguments:    Address -> 4(5) byte address
 * Return Value: None.
 * Description:  Send 4(5) byte address
 */
function SendLongAddress( Address) {
    console.log("Sending long address %lu\n", Address);
    /* Enable address latch signal */
    ALE_HIGH();  
        
    /* Send 5 byte address data */
    WriteToFlash((Address >> BYTE0_OFFSET) & BYTE_MASK );
    WriteToFlash((Address >> BYTE1_OFFSET) & BYTE1_MASK );
    WriteToFlash((Address >> BYTE2_OFFSET) & BYTE_MASK );
    WriteToFlash((Address >> BYTE3_OFFSET) & BYTE_MASK );
    WriteToFlash((Address >> BYTE4_OFFSET) & BYTE_MASK );

    SET_DATA_DIRECTION_INPUT(); //??

    /* Disable address latch signal */
    ALE_LOW();
}

/*
 * Function:     SendAddrTrad
 * Arguments:    Address -> 4(5) byte address
 *               LastPlane -> Indicate last plane or not
 * Return Value: None.
 * Description:  Send 4(5) byte address in traditional mode
 */
function SendLongAddressAndPlane(  Address,  LastPlane ) {
    console.log("Sending long address %lu and plane %c \n", Address, LastPlane);
    /* Enable address latch signal */
    ALE_HIGH();  
        
    /* Send 5 byte address data */
    FlashWrite( (Address >> BYTE0_OFFSET) & BYTE_MASK );
    FlashWrite( (Address >> BYTE1_OFFSET) & BYTE1_MASK );
    if( LastPlane ) {
        FlashWrite( (Address >> BYTE2_OFFSET) & BYTE_MASK | 0x40);
        FlashWrite( (Address >> BYTE3_OFFSET) & BYTE_MASK );
        FlashWrite( (Address >> BYTE4_OFFSET) & BYTE_MASK );
    } else {	
        FlashWrite( 0 );
        FlashWrite( 0 );
        FlashWrite( 0 );
    }

    SET_DATA_DIRECTION_INPUT(); //??

    /* Disable address latch signal */
    ALE_LOW();


}

/*
 * Function:     CheckStatus
 * Arguments:    CheckFlag -> the status bit to check
 * Return Value: READY, BUSY
 * Description:  Check status register bit 7 ~ bit 0
 */
function CheckStatus(  CheckFlag ) {
     _status;

    ReadStatusOP(_status);
    
    //console.log("CheckStatus 0x%02X\n", status);
    // two options - see page 22 in doc, use R/B 1=ready, 0=busy
    if(_status == 1 ) {       
        return TRUE;
    } else {
        return FALSE;
    }
    /* 
    if( (status & CheckFlag) == CheckFlag ) {
        console.log("CheckStatus: TRUE\n");
        return TRUE;
    } else {
        return FALSE;
        console.log("CheckStatus: FALSE\n");
    }
    */

}

/*
 * Function:     Page_Read_OP
 * Arguments:    Address -> flash address
 *               DataBuf -> data buffer to store data
 *               Length  -> the number of byte(word) to read
 * Return Value: Flash_Busy, Flash_Success
 * Description:  Read page data in sequence.
 *               Note: User can only read data in the same
 *                     page.
 */
function ReadPageOP( Address,  DataBuf,  Length ) {
    //console.log("ReadPageOP\n");
    var i;

    /* Check flash is busy or not */
    if(CheckStatus(READY_BUSY) != READY) return Flash_Busy;

    /* Send page read command */    
    SendCommand( 0x00 );

    /* Send flash address */
    SendLongAddress(Address);

    /* Send page read confirmed command */
    SendCommand(0x30);

    /* Wait flash ready and read data in a page */
    WaitTime(FIRST_BYTE_LATENCY_tR);
    //console.log("ReadPageOP: reading %d bytes starting from %d\n", Length, Address);
    for(i = 0; i < Length; i = i + 1) {
        DataBuf[i] = ReadFromFlash();
    }

    // debugging
    /*
    for (i = 0; i < Length; i++)
		console.log("0x%02X ", DataBuf[i]);
	console.log("\n");
    */
    return Flash_Success;
}

/*
 * Function:     Page_Random_Read_OP
 * Arguments:    Address -> flash address
 *               DataBuf -> data buffer to store data
 *               Length  -> the number of byte(word) to read
 * Return Value: Flash_Busy, Flash_Success
 * Description:  Read one page data randomly.
 *               Note: User needs to execute Page_Read_OP() command first before
 *                     Page_Random_Read_OP(). The number of Page_Random_Read_OP()
 *                     is not limited.
 */
function ReadRandomPageOP(  Address,   DataBuf,  Length )
{
    var i;

    /* Send random read command */
    SendCommand( 0x05 );

    /* Send page address (2 byte) */
    SendByteAddress( (Address >> BYTE0_OFFSET) & BYTE_MASK );
    SendByteAddress( (Address >> BYTE1_OFFSET) & BYTE1_MASK );

    /* Send random read confirmed command */
    SendCommand( 0xE0 );

    /* Read data in a page */
    console.log("ReadRandomPageOP: reading %d bytes starting from %d\n", Length, Address);
    for( i=0; i<Length; i=i+1 ){
        DataBuf[i] = ReadFromFlash();
    }

    return Flash_Success;
}




// see page 33 in doc for handling 2/3/4/5th bytes
function print_id(  id[6]) {
	
	console.log("Raw ID data: ");
	for (i = 0; i < 6; i++)
		console.log("0x%02X ", id[i]);
	console.log("\n");
    console.log("Expected:    0xC2 0xDC 0x90 0xA2 0x57 0x03\n");
}

function read_id(char id[6]) {
    int i;
    unsigned char buf[6]; /* Read 6-byte ID */
    
    /* Send Read ID command */
    SendCommand(0x90);

    // send address for ID   
    SendByteAddress(0x00);

    // Read bytes
    console.log("Reading bytes\n");
    for (i = 0; i < 6; i++) {
        buf[i] = ReadFromFlash();
    }

   

    return 0;
}

//
function page_to_address( page,  address_byte_index){
	switch(address_byte_index) {
	case 2:
		return page & 0xff;
	case 3:
		return (page >>  8) & 0xff;
	case 4:
		return (page >> 16) & 0xff;
	default:
		return 0;
	}
}

function send_read_command( page) {
    var i;
    /* Send page read command */    
    SendCommand(0x00);

    for (i = 0; i < 5; i++) {
        if (i < 2) {
            console.log("Col Add%d = %d\n", i + 1, page_to_address(page, i));
        } else {
            console.log("Row Add%d = %d\n", i - 1, page_to_address(page, i));
        }
        SendByteAddress(page_to_address(page, i));        
    }

    SendCommand(0x30);
    exit(0);
}




// simplified for new procedure
function read_pages(int first_page_number, int number_of_pages, char *outfile, int write_spare) {
	int page, block_no, page_nbr,  i;    
	unsigned char buf[PAGE_BUF_SIZE], id[5];
    uint32 end_page = first_page_number + number_of_pages;
    uint32 currBlock = page % 64;
    uint32 progress = 0;
    // proggress tracking
    uint32 blockNumber, blockPerc, pagePerc;

    // read first complete page
    //uBusWidth read_dat[PAGE_SIZE] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // check values
    FlashParameter fpar = {0};
    ReturnMsg rtMsg = Flash_Success;

	FILE *f = fopen(outfile, "w+");
	if (f == NULL) {
		perror("fopen output file");
		return -1;
	}
	
    // reset flash
    rtMsg = ResetFlashOP();
    // exit if fail
    if (rtMsg != (ReturnMsg)Flash_Success )  return 1;

	console.log("\nStart reading...\n\n");
	clock_t start = clock(); 


    for (page = first_page_number; page < end_page; page ++) {

            
        // 64 pages in a block
        blockNumber = page % 64;
        //blockPerc = floor(blockNumber/totalBlock)*100;

        /*
        if (end_page == page) {
            progress = 100;
        } else {
            progress = floor((page - first_page_number) * 100 / (end_page-first_page_number));
        }
        //progress = (page-start_page) * 100 / (end_page-start_page)
        */

        uBusWidth read_dat[PAGE_BUF_SIZE] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // check values
        // block_no = page / 64
        uint32 address = page << 16;

        /* Read data */
        fpar.Array = read_dat;
        fpar.Address = address; // read first page
        fpar.Length = PAGE_BUF_SIZE;

        /* before */
        /*
        console.log("Prefilled data (6 bytes): ");
        for (i = 0; i < 6; i++)
            console.log("0x%02X ", fpar.Array[i]);
        console.log("\n");
        */
        // can only read 1 page at a time like this, rest as ReadRandomPage
        //if (page == 0) {
        rtMsg = ReadPageOP( fpar.Address, fpar.Array, fpar.Length );
        //} else {
        //    rtMsg = ReadRandomPageOP( fpar.Address, fpar.Array, fpar.Length );
    // }
        //console.log("Result %d\n\n", rtMsg);

        // ok?
        if (rtMsg != (ReturnMsg)Flash_Success )  return 1;
        /*
        console.log("Returned data (6 bytes):  ");
        for (i = 0; i < 6; i++)
            console.log("0x%02X ", fpar.Array[i]);

        console.log("\n");
        */

        // write data - write as blocks, so after each 64 pages

        if (fwrite(fpar.Array, sizeof(read_dat), 1, f) != 1) {
            perror("fwrite");
            return -1;
        }
    }
    
	clock_t end = clock();
	console.log("\nReading done in %f seconds\n", (float)(end - start) / CLOCKS_PER_SEC);
	

	fflush(NULL);
	exit(0);
}


