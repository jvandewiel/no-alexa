/*
    Raspberry Pi / 

    GPIO RAW NAND flasher
    (made out of "360-Clip based 8-bit NAND reader" by pharos)  

    Copyright (C)	2016 littlebalup
					2019 skypiece
					2020 jvandewiel - specifically adapted for the MX30LFxG28AD nand chip in the echo dot 3rd gen

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

//---------------------------

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

// ------------------------------ START OF RESTRUCTURED ------------------------------ 

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
    printf("Sending command 0x%02X\n", CMD_code);

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

    printf("Status before check 0x%02X\n", *StatusReg);
    /* Send status read command */
    SendCommand(0x70);

    /* Read status value */    
    *StatusReg = GPIO_READ(READY_BUSY);
    
    printf("Status after check 0x%02X\n", *StatusReg);

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
Address Allocation (For 4G)

Addresses                   I/O7    I/O6    I/O5    I/O4    I/O3    I/O2    I/O1    I/O0
Column address - 1st cycle  A7      A6      A5      A4      A3      A2      A1      A0
Column address - 2nd cycle  L       L       L       A12     A11     A10     A9      A8
Row address - 3rd cycle     A20     A19     A18     A17     A16     A15     A14     A13
Row address - 4th cycle     A28     A27     A26     A25     A24     A23     A22     A21
Row address - 5th cycle     L       L       L       L       L       L       L       A29

A19 = plane
The A[11:8] must be 0 when the A12 value is 1.
*/

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
 * Function:     CheckStatus
 * Arguments:    CheckFlag -> the status bit to check
 * Return Value: READY, BUSY
 * Description:  Check status register bit 7 ~ bit 0
 */
BOOL CheckStatus( uint8 CheckFlag ) {
    uBusWidth status;

    ReadStatusOP(&status);
    
    printf("CheckStatus 0x%02X\n", status);
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
    printf("ReadPageOP\n");
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
    printf("ReadPageOP: reading %d bytes starting from %d\n", Length, Address);
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



// ------------------------------ END OF RESTRUCTURED ------------------------------ 

// void shortpause()
// {
//     struct timespec ts;
//     ts.tv_sec = delay / 1000;
//     ts.tv_nsec = (delay % 1000) * 1000000;
//     nanosleep(&ts, NULL);
// }

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

void print_id(unsigned char id[6])
{
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
    printf("Expected:    0xc2 0xdc 0x90 0xa2 0x57 0x03\n");

 	switch(id[0]) {
 		case 0xEC: {
 			strcpy(maker, "Samsung");
 			switch(id[1]) {
 				case 0xA1: strcpy(device, "K9F1G08R0A"); break;
 				case 0xD5: strcpy(device, "K9GAG08U0M"); break;
 				case 0xF1: strcpy(device, "K9F1G08U0A/B"); break;
 				default: strcpy(device, "unknown");
 			}
 			break;
 		}
 		case 0xAD: {
 			strcpy(maker, "Hynix");
 			switch(id[1]) {
 				case 0x73: strcpy(device, "HY27US08281A"); break;
 				case 0xD7: strcpy(device, "H27UBG8T2A"); break;
 				case 0xDA: strcpy(device, "HY27UF082G2B"); break;
 				case 0xDC: strcpy(device, "H27U4G8F2D"); break;
 				default: strcpy(device, "unknown");
 			}
 			break;
 		}
 		case 0x2C: {
 			strcpy(maker, "Micron");
 			switch(id[1]) {
 				default: strcpy(device, "unknown");
 			}
 			break;
 		}
 		case 0xC2: {
 			strcpy(maker, "Macronix");
 			switch(id[1]) {
				case 0xF1: strcpy(device, "MX30LF1G28AD"); break;
 				case 0xDA: strcpy(device, "MX30LF2G28AD"); break;
 				case 0xDC: strcpy(device, "MX30LF4G28AD"); break;
 				default: strcpy(device, "unknown");
 			}
 			break;
 		}
 		default: strcpy(maker, "unknown"); strcpy(device, "unknown");
 	}

	/* all sizes in bytes */
	for(bit = 0; bit < 8; ++bit)
		thirdbits[bit] = (id[2] >> bit) & 1;

	for(bit = 0; bit < 8; ++bit)
		fourthbits[bit] = (id[3] >> bit) & 1;
	switch(fourthbits[1] * 10 + fourthbits[0]) {
		case 00: page_size = 1024; break;
		case 01: page_size = 2048; break;
		case 10: page_size = 4096; break;
		case 11: page_size = 8192; break;
	}
	switch(fourthbits[5] * 10 + fourthbits[4]) {
		case 00: block_size = 64 * 1024; break;
		case 01: block_size = 128 * 1024; break;
		case 10: block_size = 256 * 1024; break;
		case 11: block_size = 521 * 1024; break;
	}
	switch(fourthbits[2]) {
		case 0: ras_size = 8; break; // for 512 bytes
		case 1: ras_size = 16; break; // for 512 bytes
	}
	switch(fourthbits[6]) {
		case 0: orga = 8; break; // bits
		case 1: orga = 16; break; // bits
	}
	switch(fourthbits[7] * 10 + fourthbits[3]) {
		case 00: strcpy(serial_access, "50ns/30ns minimum"); break;
		case 10: strcpy(serial_access, "25ns minimum"); break;
		case 01: strcpy(serial_access, "unknown (reserved)"); break;
		case 11: strcpy(serial_access, "unknown (reserved)"); break;
	}

	for(bit = 0; bit < 8; ++bit)
		fifthbits[bit] = (id[4] >> bit) & 1;
	switch(fifthbits[3] * 10 + fifthbits[2]) {
		case 00: plane_number = 1; break;
		case 01: plane_number = 2; break;
		case 10: plane_number = 4; break;
		case 11: plane_number = 8; break;
	}
	switch(fifthbits[6] * 100 + fifthbits[5] * 10 + fifthbits[4]) {
		case 000: plane_size = 64 / 8 * 1024 * 1024; break; // 64 megabits
		case 001: plane_size = 128 / 8 * 1024 * 1024; break; // 128 megabits
		case 010: plane_size = 256 / 8 * 1024 * 1024; break; // 256 megabits
		case 011: plane_size = 512 / 8 * 1024 * 1024; break; // 512 megabits
		case 100: plane_size = 1024 / 8 * 1024 * 1024; break; // 1 gigabit
		case 101: plane_size = 2048 / 8 * 1024 * 1024; break; // 2 gigabits
		case 110: plane_size = 4096 / 8 * 1024 * 1024; break; // 4 gigabits
		case 111: plane_size = 8192 / 8 * 1024 * 1024; break; // 8 gigabits
	}

	nand_size = plane_number * plane_size;
	nandras_size = nand_size + ras_size * nand_size / 512;

	printf("\n");
	printf("NAND manufacturer:  %s (0x%02X)\n", maker, id[0]);
	printf("NAND model:         %s (0x%02X)\n", device, id[1]);
	printf("\n");

	printf("             I/O |7|6|5|4|3|2|1|0|\n");
	printf("3rd ID data:     |");
	for(bit = 8; bit--;)
        printf("%u|", thirdbits[bit]);
    printf(" (0x%02X)\n", id[2]);
	printf("4th ID data:     |");
	for(bit = 8; bit--;)
        printf("%u|", fourthbits[bit]);
    printf(" (0x%02X)\n", id[3]);
	printf("5th ID data:     |");
	for(bit = 8; bit--;)
        printf("%u|", fifthbits[bit]);
    printf(" (0x%02X)\n", id[4]);
    printf("6th ID data:     |");
	for(bit = 8; bit--;)
        printf("%u|", sixthbits[bit]);
    printf(" (0x%02X)\n", id[5]);

	printf("\n");
	printf("Page size:          %d bytes\n", page_size);
	printf("Block size:         %lu bytes\n", block_size);
	printf("RAS (/512 bytes):   %d bytes\n", ras_size);
	// printf("RAS (per page):  %d bytes\n", ras_size * page_size / 512);
	// printf("RAS (per block): %d bytes\n", ras_size * block_size / 512);
	printf("Organisation:       %d bit\n", orga);
	printf("Serial access:      %s\n", serial_access);
	printf("Number of planes:   %d\n", plane_number);
	printf("Plane size:         %lu bytes\n", plane_size);
	printf("\n");
	printf("NAND size:          %lu MB\n", nand_size / (1024 * 1024));
	printf("NAND size + RAS:    %lu MB\n", nandras_size / (1024 * 1024));
	printf("Number of blocks:   %lu\n", nand_size / block_size);
	printf("Number of pages:    %lu\n", nand_size / page_size);
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
inline int page_to_address(int page, int address_byte_index)
{
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

    
    ALE_HIGH();
	for (i = 0; i < 5; i++) {
		GPIO_SET_LOW(WRITE_ENABLE);
		if (i < 2) {
		 	printf("Col Add%d = %d\n", i + 1, page_to_address(page, i));
		} else {
		 	printf("Row Add%d = %d\n", i - 1, page_to_address(page, i));
		}

		GPIO_WRITE_BYTE(page_to_address(page, i));
		GPIO_SET_HIGH(WRITE_ENABLE);
	}
	
	ALE_LOW();
    
    SendCommand(0x30);
    /*
	CLE_HIGH();	
	GPIO_SET_LOW(WRITE_ENABLE);
	GPIO_WRITE_BYTE(0x30);
	GPIO_SET_HIGH(WRITE_ENABLE);
	CLE_LOW();
    */
	return 0;
}



int send_write_command(int page, unsigned char data[PAGE_SIZE]) {
	
	int i;

	SET_DATA_DIRECTION_OUTPUT();

	CLE_HIGH();
	GPIO_SET_LOW(WRITE_ENABLE);

	GPIO_WRITE_BYTE(0x80);
	GPIO_SET_HIGH(WRITE_ENABLE);
	CLE_LOW();

	ALE_HIGH();

	for (i = 0; i < 5; i++) {
		GPIO_SET_LOW(WRITE_ENABLE);

		// if (i < 2) {
		// 	printf("Col Add%d = %d\n", i + 1, page_to_address(page, i));
		// }
		// else {
		// 	printf("Row Add%d = %d\n", i - 1, page_to_address(page, i));
		// }

		GPIO_WRITE_BYTE(page_to_address(page, i));
		GPIO_SET_HIGH(WRITE_ENABLE);
	}
	ALE_LOW();

	for (i = 0; i < PAGE_SIZE; i++) {
		GPIO_SET_LOW(WRITE_ENABLE);
		GPIO_WRITE_BYTE(data[i]); //
		GPIO_SET_HIGH(WRITE_ENABLE);
	}

	CLE_HIGH();
	GPIO_SET_LOW(WRITE_ENABLE);
	GPIO_WRITE_BYTE(0x10);
	GPIO_SET_HIGH(WRITE_ENABLE);
	CLE_LOW();
	
	
	return 0;
}

int send_eraseblock_command(int block) {
	
	int i;

	SET_DATA_DIRECTION_OUTPUT();

	CLE_HIGH();
    
	GPIO_SET_LOW(WRITE_ENABLE);
	GPIO_WRITE_BYTE(0x60);
	GPIO_SET_HIGH(WRITE_ENABLE);
    
	CLE_LOW();

	ALE_HIGH();
	for (i = 2; i < 5; i++) {
		GPIO_SET_LOW(WRITE_ENABLE);

		// printf("Row Add%d = %d\n", i - 1, page_to_address(block, i));

		GPIO_WRITE_BYTE(page_to_address(block, i));
		GPIO_SET_HIGH(WRITE_ENABLE);

	}
	ALE_LOW();
	CLE_HIGH();
	GPIO_SET_LOW(WRITE_ENABLE);

	GPIO_WRITE_BYTE(0xD0);
	GPIO_SET_HIGH(WRITE_ENABLE);
	CLE_LOW();

	return 0;
}

int read_status()
{
	int data;
	//unsigned char buf[5];

	SET_DATA_DIRECTION_OUTPUT();
	CLE_HIGH();
	
	GPIO_SET_LOW(WRITE_ENABLE);
	GPIO_WRITE_BYTE(0x70);
	GPIO_SET_HIGH(WRITE_ENABLE);
	
	CLE_LOW();

	SET_DATA_DIRECTION_INPUT();
	GPIO_SET_LOW(READ_ENABLE);
	data = GPIO_READ_BYTE(); //
	GPIO_SET_HIGH(READ_ENABLE);

	printf("Status data = %d\n", data);
	return data & 1; // I/O0=0 success , I/O0=1 error
}

/*
int read_pages(int first_page_number, int number_of_pages, char *outfile, int write_spare) {

	int page, page_no, block_no, page_nbr, percent, i, n, retry_count;
	unsigned char id[5], id2[5];
	unsigned char buf[PAGE_SIZE * 2];
	 
	FILE *badlog, *f = fopen(outfile, "w+");
	if (f == NULL) {
		perror("fopen output file");
		return -1;
	}
	if ((badlog = fopen("bad.log", "w+")) == NULL) {
		perror("fopen bad.log");
		return -1;
	}
	if (GPIO_READ(READY_BUSY) == 0) {
		error_msg((char*)"READY_BUSY should be 1 (pulled up), but reads as 0. Make sure the NAND is powered on");
		return -1;
	}

	if (read_id(id) < 0)
		return -1;
	print_id(id);
	printf("If this ID is incorrect, press Ctrl-C NOW to abort (3s timeout)\n");
	sleep(3);

	printf("\nStart reading...\n");
	clock_t start = clock();

	for (retry_count = 0, page = first_page_number*2; page < (first_page_number + number_of_pages)*2; page++) {

	  retry_all:
		page_no = page >> 1;

		// printf("page = %d, n = %d\n",page, n);

		if (page % 2 == 0 && retry_count == 0) {
			// page_no = page / 2;
			page_nbr = page_no - first_page_number + 1;
			percent = (100 * page_nbr) / number_of_pages;
			block_no = page_no / 64;
			printf("Reading page n° %d in block n° %d (page %d of %d), %d%%\r", page_no, block_no, page_nbr, number_of_pages, percent);
			fflush(stdout);
		}
		// else {
		// 	printf("Reading the page again to ensure correct operation\n");
		// }

	  retry:
		read_id(id2);
		if (memcmp(id, id2, 5) != 0) {
			printf("\nNAND ID has changed! retrying");
			goto retry;
		}
		//send_read_command(page_no);
		//for (i = 0; i < MAX_WAIT_READY_BUSY; i++) {
		//	if (GPIO_READ(N_READY_BUSY) == 0)
		//		break;
		//}
        
		while (GPIO_READ(READY_BUSY) == 0) {
			// printf("Busy\n");
			//waitUntilReady();
			shortpause();
		}
        
		// if (i == MAX_WAIT_READY_BUSY) {
		// 	// #ifdef DEBUG
		// 		printf("N_READY_BUSY was not brought to 0 by NAND in time, retrying\n");
		// 	// #endif
		// 	goto retry;
		// }
        
		SET_DATA_DIRECTION_INPUT();
        
		// for (i = 0; i < MAX_WAIT_READY_BUSY; i++) {
		// 	if (GPIO_READ(N_READY_BUSY) == 1)
		// 		break;
		// }
		// if (i == MAX_WAIT_READY_BUSY) {
		// 	// #ifdef DEBUG
		// 		printf("N_READY_BUSY was not brought to 1 by NAND in time, retrying\n");
		// 	// #endif
		// 	goto retry;
		// }
        
		n = PAGE_SIZE*(page & 1);
		for (i = 0; i < PAGE_SIZE; i++) {
			GPIO_SET_LOW(READ_ENABLE);
			buf[i + n] = GPIO_READ_BYTE(); //
			GPIO_SET_HIGH(READ_ENABLE);
		}

		if (!n) // read the page again to ensure correct operation, bit 0 in page used for this purpose
			// printf("RE LOOP    | page = %d, n = %d\n",page, n);
			// printf("Reading the page n° %d again to ensure correct operation\n", page_no);
			continue;

		if (memcmp(buf, buf + PAGE_SIZE, PAGE_SIZE) != 0) {
			if (retry_count == 0) printf("\n");
			if (retry_count < 5) {
				printf("Page failed to read correctly! retrying\n");
				retry_count++;
				page = page & ~1;
				goto retry_all;
			}
			printf("Too many retries. Perhaps bad block?\n");
			fprintf(badlog, "Page %d seems to be bad\n", page_no);
		}
		if (write_spare) {
			if (fwrite(buf, PAGE_SIZE, 1, f) != 1) {
				perror("fwrite");
				return -1;
			}
		}
		else {
			if (fwrite(buf, 512 * (PAGE_SIZE / 512), 1, f) != 1) {
				perror("fwrite");
				return -1;
			}
		}
		retry_count = 0;
        
	}
	
	
	clock_t end = clock();
	printf("\n\nReading done in %f seconds\n", (float)(end - start) / CLOCKS_PER_SEC);

	//show cursor
	// printf("\e[?25h");
	fflush(NULL);
	exit(0);

}
*/

// simplified for new procedures 
int read_pages(int first_page_number, int number_of_pages, char *outfile, int write_spare) {

	int page, block_no, page_nbr,  i;    
	unsigned char buf[PAGE_SIZE], id[5];

    // read first complete page
    uBusWidth read_dat[PAGE_SIZE] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // check values
    FlashParameter fpar = {0};
    ReturnMsg rtMsg = Flash_Success;

	FILE *f = fopen(outfile, "w+");
	if (f == NULL) {
		perror("fopen output file");
		return -1;
	}
	
	printf("\nStart reading...\n\n");
	clock_t start = clock();

    for (page = 0; page < number_of_pages; page ++) {
        /* Read data */
        fpar.Array = read_dat;
        fpar.Address = page * PAGE_SIZE; //??
        fpar.Length = PAGE_SIZE;

        /* before */
        
        printf("Prefilled data (6 bytes): ");
        for (i = 0; i < 6; i++)
            printf("0x%02X ", fpar.Array[i]);
        printf("\n");
        
        // can only read 1 page at a time like this, rest as ReadRandomPage
        if (page == 0) {
            rtMsg = ReadPageOP( fpar.Address, fpar.Array, fpar.Length );
        } else {
            rtMsg = ReadRandomPageOP( fpar.Address, fpar.Array, fpar.Length );
        }
        printf("Result %d\n\n", rtMsg);

        // ok?
        if (rtMsg != (ReturnMsg)Flash_Success )  return 1;
        
        printf("Returned data (6 bytes):  ");
        for (i = 0; i < 6; i++)
            printf("0x%02X ", fpar.Array[i]);

        printf("\n");
        

        // write data
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
	
	int page, block_no, page_nbr, percent, retry_count;
	unsigned char buf[PAGE_SIZE], id[5], id2[5];;

	if (read_id(id) < 0)
		return -1;
	print_id(id);
	printf("if this ID is incorrect, press Ctrl-C NOW to abort (3s timeout)\n");
	sleep(3);

	printf("\nStart writing...\n");
	clock_t start = clock();


	FILE *f = fopen(infile, "rb");
	if (f == NULL) {
		perror("fopen input file");
		return -1;
	}

	// printf("first_page_number = %d\n", first_page_number);
	// printf("number of pages = %d\n", number_of_pages);


	for (retry_count = 0, page = first_page_number; page < first_page_number + number_of_pages; page++) {

	  retry_all:

		if (retry_count == 0) {
			// page_no = page / 2;
			page_nbr = page - first_page_number + 1;
			percent = (100 * page_nbr) / number_of_pages;
			block_no = page / 64;
			printf("Writing page n° %d in block n° %d (page %d of %d), %d%%\r", page, block_no, page_nbr, number_of_pages, percent);
			fflush(stdout);
		}

		fseek(f, page * PAGE_SIZE, SEEK_SET);
		fread(buf, PAGE_SIZE, 1, f);

		// printf("\nwriting page n°%d\n", page);

	  retry:
		read_id(id2);
		if (memcmp(id, id2, 5) != 0) {
			printf("\nNAND ID has changed! retrying");
			goto retry;
		}

		send_write_command(page, buf);
		while (GPIO_READ(READY_BUSY) == 0) {
			// printf("Busy\n");
			shortpause();
		}
		// read_status();
		if (read_status()) {
			if (retry_count == 0) printf("\n");
			if (retry_count < 5) {
				printf("Failed to write page correctly! retrying\n");
				retry_count++;
				goto retry_all;
			}
			printf("Too many retries. Perhaps bad block?\n");
			// retry_count = 0;
		}
		retry_count = 0;
	}

	clock_t end = clock();
	printf("\nWrite done in %f seconds\n", (float)(end - start) / CLOCKS_PER_SEC);
	fflush(NULL);
	exit(0);
}

int erase_blocks(int first_block_number, int number_of_blocks) {

	int block, block_no, block_nbr, percent, i, n, retry_count;
	unsigned char id[5], id2[5];

	if (read_id(id) < 0)
		return -1;
	print_id(id);
	printf("if this ID is incorrect, press Ctrl-C NOW to abort (3s timeout)\n");
	sleep(3);

	printf("\nStart erasing...\n");
	clock_t start = clock();

	for (retry_count = 0, block = first_block_number; block < (first_block_number + number_of_blocks); block++) {

	  retry_all:
			
		block_nbr = block - first_block_number + 1;
		percent = (100 * block_nbr) / number_of_blocks;

		if (retry_count == 0) {
			printf("Erasing block n° %d at adress 0x%02X (block %d of %d), %d%%\r", block, block * BLOCK_SIZE, block_nbr, number_of_blocks, percent);
			fflush(stdout);
			// printf("Block address : %d (0x%02X)\n", block * BLOCK_SIZE, block * BLOCK_SIZE);
		}

	  retry:
		read_id(id2);
		if (memcmp(id, id2, 5) != 0) {
			printf("\nNAND ID has changed! retrying");
			goto retry;
		}

		send_eraseblock_command(block * 64); // 64 = pages per block
		while (GPIO_READ(READY_BUSY) == 0) {
			// printf("Busy\n");
			shortpause();
		}

		if (read_status()) {
			if (retry_count == 0) printf("\n");
			if (retry_count < 5) {
				printf("Failed to erase block correctly! retrying\n");
				retry_count++;
				goto retry_all;
			}
			printf("Too many retries. Perhaps bad block?\n");
			// retry_count = 0;
		}
		retry_count = 0;
	}

	clock_t end = clock();
	printf("\nErasing done in %f seconds\n", (float)(end - start) / CLOCKS_PER_SEC);	
	return 0;
}

