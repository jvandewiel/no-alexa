/*
    Raspberry Pi / 

    GPIO RAW NAND flasher
    (made out of "360-Clip based 8-bit NAND reader" by pharos)  

    Copyright (C)	2016 littlebalup, 2019 skypiece, 2021 jvandewiel
    specifically adapted for the MX30LFxG28AD nand chip in the echo dot 3rd gen

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    
    Note that the last changes were done by a none-C coder - you are warned
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

//#define DEBUG  // Debugging

#define PAGE_SIZE 4352 // 4096 + 256 bytes, 256 bytes ECC/page
#define BLOCK_SIZE 278528 // 64 pages of4352 bytes
#define MAX_WAIT_READ_BUSY	1000000

#define PAGE_BUF_SIZE 4352 // reading length for page buffer testing
/* For Raspberry 2B and 3B :*/
#define BCM2736_PERI_BASE        0x3F000000
#define GPIO_BASE                (BCM2736_PERI_BASE + 0x200000) /* GPIO controller */

// IMPORTANT: BE VERY CAREFUL TO CONNECT VCC TO P1-01 (3.3V) AND *NOT* P1-02 (5V) !!
// IMPORTANT: MAY BE YOU NEED EXTERNAL 1.8V for modern NANDs

// GPIO pins have been chosen to be compitable w/ Waveshare NandFlash Board and lost RPi SMI NAND driver
#define WRITE_PROTECT           2
#define READY_BUSY              3
#define ADDRESS_LATCH_ENABLE    4
#define COMMAND_LATCH_ENABLE    17
#define READ_ENABLE             18
#define WRITE_ENABLE            27
#define CHIP_ENABLE             22 // needed?

// mapping of the I/O pins
int data_to_gpio_map[8] = { 23, 24, 25, 8, 7, 10, 9, 11 }; // 23 = I/O 0 .. 11 = I/O 7


volatile unsigned int *gpio;

int read_id(unsigned char id[5]);
int read_pages(int first_page_number, int number_of_pages, char *outfile, int write_spare);
int write_pages(int first_page_number, int number_of_pages, char *infile);
int erase_blocks(int first_block_number, int number_of_blocks);

enum {SECS_TO_SLEEP = 0, NSEC_TO_SLEEP = 25}; //?

inline void SHORTPAUSE() {
    struct timespec remaining, request = {SECS_TO_SLEEP, NSEC_TO_SLEEP};
	nanosleep(&request, &remaining);
}

inline void SET_GPIO_INPUT(int g) {
#ifdef DEBUG
	printf("Setting direction of GPIO#%d to INPUT\n", g);
#endif
	(*(gpio+((g)/10)) &= ~(7<<(((g)%10)*3)));
}

inline void SET_GPIO_OUTPUT(int g) {
	SET_GPIO_INPUT(g);
#ifdef DEBUG
	printf("Setting direction of GPIO#%d to INPUT\n", g);
#endif
	*(gpio+((g)/10)) |= (1<<(((g)%10)*3));
}

inline void GPIO_SET_HIGH(int g) {
#ifdef DEBUG
	printf("Setting GPIO#%d to 1\n", g);
#endif
	*(gpio +  7)  = 1 << g;
	SHORTPAUSE();
}

inline void GPIO_SET_LOW(int g) {
#ifdef DEBUG
	printf("Setting GPIO#%d to 0\n", g);
#endif
	*(gpio + 10)  = 1 << g;
	SHORTPAUSE();
}

inline int GPIO_READ(int g) {
	int x = (*(gpio + 13) & (1 << g)) >> g;
#ifdef DEBUG
	printf("GPIO#%d reads as %d\n", g, x);
#endif
	return x;
}

inline void SET_DATA_DIRECTION_INPUT(void) {
	int i;
#ifdef DEBUG
	printf("Set data direction to INPUT\n");
#endif
	for (i = 0; i < 8; i++)
		SET_GPIO_INPUT(data_to_gpio_map[i]);
}

inline void SET_DATA_DIRECTION_OUTPUT(void) {
	int i;
#ifdef DEBUG
	printf("Set data direction to OUTPUT\n");
#endif
	for (i = 0; i < 8; i++)
		SET_GPIO_OUTPUT(data_to_gpio_map[i]);
}

// Read all bits into a single byte from IO
inline int GPIO_READ_BYTE(void) {
	int i, data;
	for (i = data = 0; i < 8; i++, data = data << 1) {
		data |= GPIO_READ(data_to_gpio_map[7 - i]);
	}
	data >>= 1;
#ifdef DEBUG
	printf("GPIO_READ_BYTE: data=%02x\n", data);
#endif
	return data;
}

inline void GPIO_WRITE_BYTE(int data) {
	int i;
#ifdef DEBUG
	printf("GPIO_WRITE_BYTE: data=%02x\n", data);
#endif
	for (i = 0; i < 8; i++, data >>= 1) {
		if (data & 1)
			GPIO_SET_HIGH(data_to_gpio_map[i]);
		else
			GPIO_SET_LOW(data_to_gpio_map[i]);
	}
	SHORTPAUSE();
}

inline void CLE_HIGH() { 
    GPIO_SET_HIGH(COMMAND_LATCH_ENABLE);
}

inline void CLE_LOW() {
    GPIO_SET_LOW(COMMAND_LATCH_ENABLE);
}

inline void ALE_HIGH() {
    GPIO_SET_HIGH(ADDRESS_LATCH_ENABLE);
}

inline void ALE_LOW() {
    GPIO_SET_LOW(ADDRESS_LATCH_ENABLE);
}
// sleep function w/ nanoseconds, use input param
// replace this with wait until ready function?

int delay = 1; // in nanosec

void shortpause() {
	int i;
	volatile static int dontcare = 0;
	for (i = 0; i < delay; i++) {
		dontcare++;
	}
}

// ------------ borrowed from macronix ref code ------------

/* Basic Data Type Definition */

typedef  unsigned char  BOOL;
typedef  unsigned char  uint8;
typedef  unsigned int   uint16;
typedef  unsigned long  uint32;
typedef  uint32  uAddr;
// 8 bits
typedef  uint8  uBusWidth;
#define ADDRESS_OFFSET 0

/* Return Message */
typedef enum{
    Flash_Success,
    Flash_Busy,
    Flash_OperationTimeOut,
    Flash_ProgramFailed,
    Flash_EraseFailed,
    Flash_ReadIDFailed,
    Flash_CmdInvalid,
    Flash_DataInvalid,
    Flash_AddrInvalid
}ReturnMsg;

/* BOOL Definition */
#define  TRUE   1
#define  FALSE  0

// system flags
#define  PASS        0
#define  FAIL        1
#define  BUSY        0
#define  READY       1
#define  PROTECTED   0
#define  UNPROTECTED 1

/* Target Flash Device: MX30XXXX */
#define MX30LF4G28AD 

/* Timer Parameter */
#define  TIMEOUT    0
#define  TIMENOTOUT 1
#define  TPERIOD    240             // ns, The time period of timer count one
#define  COUNT_ONE_MICROSECOND  16  // us, The loop count value within one micro-second

/* Device Parameter ( Time uint: us, Condition: worst case )
   Please refer to data sheet for advanced information. */
#ifdef MX30LF4G28AD        
#define ID_CODE0              0xc2
#define ID_CODE1              0xdc
#define ID_CODE2              0x90
#define ID_CODE3              0xa2
#define ID_CODE4              0x57
#define ID_CODE5              0x03
#define READ_ID_6BYTE
#define FLASH_SIZE            0x20000000
#define FLASH_TIMEOUT_VALUE   3000
#define FIRST_BYTE_LATENCY_tR 25
#define tRCBSY                25
#define tDBSY                 1
#define tCBSY                 700
#define BP_PROT_MODE
#endif

/*
    Address convert (MSB)
    The NAND flash address reserved A15-A12
    User can switch Address_Convert option
    to on/off address convert, default is off
*/

//#define Address_Convert 1

#ifdef Address_Convert
    //32 bit( x8 mode): BYTE3 | BYTE2 | {4'b0, BYTE1} | BYTE0
    //32 bit(x16 mode): BYTE3 | BYTE2 | {5'b0, BYTE1} | BYTE0
    #define BYTE0_OFFSET  0
    #define BYTE1_MASK   (0x0F >> ADDRESS_OFFSET)  // Mask address A15-A12 of BYTE1
    #define BYTE1_OFFSET  8
    #define BYTE2_OFFSET 12 - ADDRESS_OFFSET
    #define BYTE3_OFFSET 20 - ADDRESS_OFFSET
    #define BYTE4_OFFSET 28 - ADDRESS_OFFSET
    #define PAGE_MASK    (0xFFF >> ADDRESS_OFFSET)
#else
    //32 bit: BYTE3 | BYTE2 | BYTE1 | BYTE0
    #define BYTE0_OFFSET  0
    #define BYTE1_MASK   0xFF
    #define BYTE1_OFFSET  8
    #define BYTE2_OFFSET 16
    #define BYTE3_OFFSET 24
    #define BYTE4_OFFSET 32
    #define PAGE_MASK    0xFFFF
#endif
#define BYTE_MASK 0xFF

/* Flash Information */
struct flashinf {
    /* Timer Variable */
    uint32  Tus;        // time-out length in us
    uint32  Timeout_Value;
    uint32  Timer_counter;
};

typedef struct flashinf FlashInfo;

/* Flash Parameter Structure */
struct fps{
    uBusWidth *Array;
    uAddr Address;
    uint32 Length;
};

typedef struct fps FlashParameter;

/*
 * Utility Function
 */

void Set_Timer( FlashInfo *fptr  ) {
    fptr->Timeout_Value = (fptr->Tus)/COUNT_ONE_MICROSECOND;
    fptr->Timer_counter = 0;
}

BOOL Check_Timer( FlashInfo *fptr  ) {
    if( fptr->Timer_counter < fptr->Timeout_Value ){
        fptr->Timer_counter = fptr->Timer_counter + 1;
        return TIMENOTOUT;
    }else{
        return TIMEOUT;
    }
}

void Wait_Timer( FlashInfo *fptr  ) {
    /* Wait timer until time-out */
    while( fptr->Timer_counter < fptr->Timeout_Value ){
        fptr->Timer_counter = fptr->Timer_counter + 1;
    }
}

/*
 * Function:       WaitTime
 * Arguments:      TimeValue -> The time to wait in us
 * Return Value:   READY, TIMEOUT
 * Description:    Wait flash device for a specfic time
 */
void WaitTime( uint32 TimeValue ) {
    FlashInfo flash_info;
    flash_info.Tus = TimeValue;
    Set_Timer( &flash_info );
    Wait_Timer( &flash_info );
}

/*
 * Function:     InitFlash
 * Arguments:    None.
 * Return Value: None.
 * Description:  Initial MX30 flash device
 */
void InitFlash(void) {
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

void WriteToFlash(uBusWidth Value) {
    SET_DATA_DIRECTION_OUTPUT(); 
    GPIO_SET_LOW(WRITE_ENABLE);
    GPIO_WRITE_BYTE(Value); // Write ID byte 1
    GPIO_SET_HIGH(WRITE_ENABLE);
    //printf("Wrote to flash 0x%02X \n", Value);
}

/*
 * Function:     FlashRead
 * Arguments:    None
 * Return Value: Read data
 * Description:  Read a data from flash
 */
uBusWidth ReadFromFlash(void) {
    uBusWidth buffer;
    
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
void SendCommand(uBusWidth CMD_code) {
    //printf("Sending command 0x%02X\n", CMD_code);

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
ReturnMsg ReadStatusOP( uBusWidth *StatusReg ) {

    //printf("Status before check 0x%02X\n", *StatusReg);
    /* Send status read command */
    SendCommand(0x70);

    /* Read status value */    
    *StatusReg = GPIO_READ(READY_BUSY);
    
    //printf("Status after check 0x%02X\n", *StatusReg);

    return Flash_Success;
}

/*
 * Function:     SendByteAddr
 * Arguments:    Byte_addr -> one byte address
 * Return Value: None.
 * Description:  Send one byte address
 */
void SendByteAddress( uint8 Byte_addr ) {
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
ReturnMsg ResetFlashOP( void ) {
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
void SendLongAddress(uint32 Address) {
    printf("Sending long address %lu\n", Address);
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
void SendLongAddressAndPlane( uint32 Address, BOOL LastPlane ) {
    printf("Sending long address %lu and plane %c \n", Address, LastPlane);
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
BOOL CheckStatus( uint8 CheckFlag ) {
    uBusWidth status;

    ReadStatusOP(&status);
    
    //printf("CheckStatus 0x%02X\n", status);
    // two options - see page 22 in doc, use R/B 1=ready, 0=busy
    if(status == 1 ) {       
        return TRUE;
    } else {
        return FALSE;
    }
    /* 
    if( (status & CheckFlag) == CheckFlag ) {
        printf("CheckStatus: TRUE\n");
        return TRUE;
    } else {
        return FALSE;
        printf("CheckStatus: FALSE\n");
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
ReturnMsg ReadPageOP(uAddr Address, uBusWidth * DataBuf, uint32 Length ) {
    //printf("ReadPageOP\n");
    uint32 i;

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
    //printf("ReadPageOP: reading %d bytes starting from %d\n", Length, Address);
    for(i = 0; i < Length; i = i + 1) {
        DataBuf[i] = ReadFromFlash();
    }

    // debugging
    /*
    for (i = 0; i < Length; i++)
		printf("0x%02X ", DataBuf[i]);
	printf("\n");
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
ReturnMsg ReadRandomPageOP( uAddr Address, uBusWidth * DataBuf, uint32 Length )
{
    uint32 i;

    /* Send random read command */
    SendCommand( 0x05 );

    /* Send page address (2 byte) */
    SendByteAddress( (Address >> BYTE0_OFFSET) & BYTE_MASK );
    SendByteAddress( (Address >> BYTE1_OFFSET) & BYTE1_MASK );

    /* Send random read confirmed command */
    SendCommand( 0xE0 );

    /* Read data in a page */
    printf("ReadRandomPageOP: reading %d bytes starting from %d\n", Length, Address);
    for( i=0; i<Length; i=i+1 ){
        DataBuf[i] = ReadFromFlash();
    }

    return Flash_Success;
}

int main(int argc, char **argv) { 
	
	int mem_fd;
	printf("Raspberry GPIO raw NAND flasher by pharos, littlebalup, skypiece, jvandewiel\n\n");

	if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC)) < 0) {
		perror("Open /dev/mem, are you root?");
		return -1;
	}

	if ((gpio = (volatile unsigned int *) mmap((caddr_t) 0x13370000, 4096, PROT_READ|PROT_WRITE,
						MAP_SHARED|MAP_FIXED, mem_fd, GPIO_BASE)) == MAP_FAILED) {
		perror("mmap GPIO_BASE");
		close(mem_fd);
		return -1;
	}

	InitFlash();
    //GPIO_SET_HIGH(N_CHIP_ENABLE);

	if (argc < 3) {
usage:
		
		printf("usage: sudo %s <delay> <command> ...\n\n" \
		    " <delay> used to slow down operations (50 should work, increase if bad reads)\n\n" \
		    "Commands:\n" \
		    " read_id (no arguments)                        : read and decrypt chip ID\n" \
		    " read_full <page #> <# of pages> <output file> : read N pages including spare\n" \
		    " read_data <page #> <# of pages> <output file> : read N pages, discard spare\n" \
		    " write_full <page #> <# of pages> <input file> : write N pages, including spare\n" \
		    " write_data <page #> <# of pages> <input file> : write N pages, discard spare\n" \
		    " erase_blocks <block number> <# of blocks>     : erase N blocks\n\n" \
		    "Notes:\n" \
		    " This program assumes PAGE_SIZE == %d\n" \
		    " Run as root (sudo) required (for /dev/mem access)\n\n",
			argv[0], PAGE_SIZE);
		close(mem_fd);
		return -1;
	}

	//delay = atoi(argv[1]);

	// parse params
	if (strcmp(argv[2], "read_id") == 0) {
		return read_id(NULL);
	}

	if (strcmp(argv[2], "read_full") == 0) {
		if (argc != 6) goto usage;
		if (atoi(argv[4]) <= 0) {
			printf("# of pages must be > 0\n");
			return -1;
		}
		return read_pages(atoi(argv[3]), atoi(argv[4]), argv[5], 1);
	}
    
	if (strcmp(argv[2], "read_data") == 0) {
		if (argc != 6) goto usage;
		if (atoi(argv[4]) <= 0) {
			printf("# of pages must be > 0\n");
			return -1;
		}
		return read_pages(atoi(argv[3]), atoi(argv[4]), argv[5], 0);
	}

	if (strcmp(argv[2], "write_full") == 0) {
		if (argc != 6) goto usage;
		if (atoi(argv[4]) <= 0) {
			printf("# of pages must be > 0\n");
			return -1;
		}
		return write_pages(atoi(argv[3]), atoi(argv[4]), argv[5]);
	}

	if (strcmp(argv[2], "erase_blocks") == 0) {
		if (argc != 5) goto usage;
		if (atoi(argv[4]) <= 0) {
			printf("# of blocks must be > 0\n");
			return -1;
		}
		return erase_blocks(atoi(argv[3]), atoi(argv[4]));
	}

	printf("unknown command '%s'\n", argv[2]);
	goto usage;
	return 0;
}

void error_msg(char *msg)
{
	printf("%s\nBe sure to check wiring, and check that pressure is applied on clip (if used)\n", msg);
}

// see page 33 in doc for handling 2/3/4/5th bytes
void print_id(unsigned char id[6]) {
	unsigned int i, bit, page_size, ras_size, orga, plane_number;
	unsigned long block_size, plane_size, nand_size, nandras_size;
	char maker[16], device[16], serial_access[20];
	unsigned *thirdbits = (unsigned*)malloc(sizeof(unsigned) * 8);
	unsigned *fourthbits = (unsigned*)malloc(sizeof(unsigned) * 8);
	unsigned *fifthbits = (unsigned*)malloc(sizeof(unsigned) * 8);
  unsigned *sixthbits = (unsigned*)malloc(sizeof(unsigned) * 8);

	printf("Raw ID data: ");
	for (i = 0; i < 6; i++)
		printf("0x%02X ", id[i]);
	printf("\n");
    printf("Expected:    0xC2 0xDC 0x90 0xA2 0x57 0x03\n");
}

int read_id(unsigned char id[6]) {
    int i;
    unsigned char buf[6]; /* Read 6-byte ID */
    
    /* Send Read ID command */
    SendCommand(0x90);

    // send address for ID   
    SendByteAddress(0x00);

    // Read bytes
    printf("Reading bytes\n");
    for (i = 0; i < 6; i++) {
        buf[i] = ReadFromFlash();
    }

    if (id != NULL)
        memcpy(id, buf, 6);
    else
        print_id(buf);
    if (buf[0] == buf[1] && buf[1] == buf[2] && buf[2] == buf[3] && buf[3] == buf[4] && buf[4] == buf[5]) {
        error_msg((char*)"All 6 ID bytes are identical, this is not normal");
        return -1;
    }

    return 0;
}

//
inline int page_to_address(int page, int address_byte_index){
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

int send_read_command(int page) {
    int i;
    /* Send page read command */    
    SendCommand(0x00);

    for (i = 0; i < 5; i++) {
        if (i < 2) {
            printf("Col Add%d = %d\n", i + 1, page_to_address(page, i));
        } else {
            printf("Row Add%d = %d\n", i - 1, page_to_address(page, i));
        }
        SendByteAddress(page_to_address(page, i));        
    }

    SendCommand(0x30);
    exit(0);
}

int send_write_command(int page, unsigned char data[PAGE_SIZE]) {
    int i;
    /*
    SendCommand(0x80);
    SendLongAddress(page);
    for (i = 0; i < PAGE_SIZE; i++) {
        WriteToFlash(data[i]);
    }
    SendCommand(0x10);
    */
    return 0;
}

int send_eraseblock_command(int block) {
    exit(0);
}

// simplified for new procedure
int read_pages(int first_page_number, int number_of_pages, char *outfile, int write_spare) {
	int page, block_no, page_nbr,  i;    
	unsigned char buf[PAGE_BUF_SIZE], id[5];
    uint32 end_page = first_page_number + number_of_pages;
    uint32 currBlock = page % 64;
    uint32 progress = 0;
    // proggress tracking
    uint32 blockNumber, blockPerc, pagePerc;

    // read first complete page
    // uBusWidth read_dat[PAGE_SIZE] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // check values
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

	printf("\nStart reading...\n\n");
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
        printf("Prefilled data (6 bytes): ");
        for (i = 0; i < 6; i++)
            printf("0x%02X ", fpar.Array[i]);
        printf("\n");
        */
        // can only read 1 page at a time like this, rest as ReadRandomPage
        //if (page == 0) {
        rtMsg = ReadPageOP( fpar.Address, fpar.Array, fpar.Length );
        //} else {
        //    rtMsg = ReadRandomPageOP( fpar.Address, fpar.Array, fpar.Length );
    // }
        //printf("Result %d\n\n", rtMsg);

        // ok?
        if (rtMsg != (ReturnMsg)Flash_Success )  return 1;
        /*
        printf("Returned data (6 bytes):  ");
        for (i = 0; i < 6; i++)
            printf("0x%02X ", fpar.Array[i]);

        printf("\n");
        */

        // write data - write as blocks, so after each 64 pages

        if (fwrite(fpar.Array, sizeof(read_dat), 1, f) != 1) {
            perror("fwrite");
            return -1;
        }
    }
    
	clock_t end = clock();
	printf("\nReading done in %f seconds\n", (float)(end - start) / CLOCKS_PER_SEC);
	

	fflush(NULL);
	exit(0);
}


int write_pages(int first_page_number, int number_of_pages, char *infile) {
    exit(0);
}

int erase_blocks(int first_block_number, int number_of_blocks) {
    exit(0);
}
