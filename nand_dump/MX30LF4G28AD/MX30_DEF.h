/*
 * Security Level: Macronix Proprietary
 * COPYRIGHT (c) 2010-2020 MACRONIX INTERNATIONAL CO., LTD
 * MX30-series Parallel Flash Low Level Driver (LLD) Sample Code
 *
 * Flash device information.
 * This sample code provides a reference, not recommand for directing using.
 *
 * $Id: MX30_DEF.h,v 1.85 2019/10/23 01:45:13 mxclldb1 Exp $
 */
#ifndef __MX30_DEF_H__
#define __MX30_DEF_H__

//#include <8051.h>  /* Use MCU8051 */ DISABLED

/* BOOL Definition */
#define  TRUE   1
#define  FALSE  0

/* System Configuration */
//#define  GPIO_8051  1 // DON'T HAVE
//#define  Timer_8051 1
//#define  NonSynchronousIO  1

/* Target Flash Device: MX30XXXX */
#define MX30LF4G28AD 

/* Timer Parameter */
#define  TIMEOUT    0
#define  TIMENOTOUT 1
#define  TPERIOD    240             // ns, The time period of timer count one
#define  COUNT_ONE_MICROSECOND  16  // us, The loop count value within one micro-second

/* Flash Register Parameter */

// system flags
#define  PASS        0
#define  FAIL        1
#define  BUSY        0
#define  READY       1
#define  PROTECTED   0
#define  UNPROTECTED 1

// GPIO specific for RPI Raspberry 2B and 3B
#define BCM2736_PERI_BASE        0x3F000000
#define GPIO_BASE                (BCM2736_PERI_BASE + 0x200000) /* GPIO controller */

// GPIO pins have been chosen to be compitable w/ Waveshare NandFlash Board and lost RPi SMI NAND driver
#define WRITE_PROTECT	        2
#define READY_BUSY		        3
#define ADDRESS_LATCH_ENABLE    4
#define COMMAND_LATCH_ENABLE    17
#define READ_ENABLE             18
#define WRITE_ENABLE            27
#define CHIP_ENABLE             22 // needed?

// mapping of the I/O pins
//int data_to_gpio_map[8] = { 23, 24, 25, 8, 7, 10, 9, 11 }; // 23 = I/O 0 .. 11 = I/O 

#define  GPIO_ChipStatus         0x01
#define  GPIO_CacheProgramResult 0x02
#define  GPIO_ReadyBusyForCtrl   0x20
#define  GPIO_ReadyBusy          3
#define  GPIO_WriteProtect       2

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

/* Basic Data Type Definition */
typedef  unsigned char  BOOL;
typedef  unsigned char  uint8;
typedef  unsigned int   uint16;
typedef  unsigned long  uint32;
typedef  uint32  uAddr;
#ifdef X16_DEVICE
	typedef  uint16  uBusWidth;
	#define ADDRESS_OFFSET 1
#else
	typedef  uint8  uBusWidth;
	#define ADDRESS_OFFSET 0
#endif

/*
    Address convert (MSB)
    The NAND flash address reserved A15-A12
    User can switch Address_Convert option
    to on/off address convert, default is off
*/
#define  Address_Convert   1

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

/* GPIO port mapping */
#ifdef GPIO_8051

#define  IN_VALID       P3_0
#define  OUT_VALID      P3_1
#define  WRITE_EN       P3_2
#define  DUMP_SIG       P3_3
#define  CTRL0          P3_4
#define  CTRL1          P3_5

#define CLE_LOW();  CTRL1 = 0; CTRL0 = 0;
#define CLE_HIGH(); CTRL1 = 1; CTRL0 = 0;

#define ALE_LOW();  CTRL1 = 0; CTRL0 = 0;
#define ALE_HIGH(); CTRL1 = 0; CTRL0 = 1;

#define DDR_DISABLE();   CTRL1 = 1; CTRL0 = 1;WRITE_EN = 0;
#define DDR_ENABLE();    CTRL1 = 1; CTRL0 = 1;WRITE_EN = 1;

#define  toggle(); IN_VALID = 1; \
                   IN_VALID = 0;

#define  dump_sig(); DUMP_SIG = 1; \
                     DUMP_SIG = 0;
#endif  /* End of GPIO_8051 */

/* Program Parameter */
#ifdef READ_ID_5BYTE
static const uint8 ID_CODE[5] = {ID_CODE0, ID_CODE1, ID_CODE2, ID_CODE3, ID_CODE4};
#else  
	#ifdef READ_ID_6BYTE
       static const uint8 ID_CODE[6] = {ID_CODE0, ID_CODE1, ID_CODE2, ID_CODE3, ID_CODE4, ID_CODE5};
	#else
		static const uint8 ID_CODE[4] = {ID_CODE0, ID_CODE1, ID_CODE2, ID_CODE3};
#endif
#endif

#ifdef F_MX30LM
	static const uint8 ONFI_PARA_SIGN[] = { 0x4A, 0x45, 0x53, 0x44 };
#else 
	static const uint8 ONFI_PARA_SIGN[] = { 0x4F, 0x4E, 0x46, 0x49 };
#endif 

#endif  // end of __MX30_DEF_H__

