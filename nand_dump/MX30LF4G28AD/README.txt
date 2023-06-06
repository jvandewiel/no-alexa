/*
 * Security Level: Macronix Proprietary
 * COPYRIGHT (c) 2010-2020 MACRONIX INTERNATIONAL CO., LTD
 * NAND Flash Low Level Driver (LLD) Sample Codes
 *
 * This README file introduces low level driver of MX30-series.
 *
 * Filename:  README.txt
 * Issued Date: February 5, 2020
 *
 * Any questions or suggestions, please send mails to:
 *
 *     flash_model@mxic.com.tw
 */

 * Notice: All source files are saved as UNIX text format.

This README file describes the functions of MXIC MX30-series low level driver
(LLD) sample codes. It consists of several sections as follows:

1. Overview
2. File structure
3. Functions
4. Porting
5. Return messages

1. Overview
---------------------------------
The MX30-series LLD sample codes are useful for programmers to development
flash drivers or software applications of MX30-series nand flash products.
The included source codes are ANSI C compliant. Any ANSI C compliant complier
can be compiled with these codes. The MX30-series LLD sample codes have been
verified under hardware and software co-simulation platform with standard 8051
IP core.

All the contents of LLD sample code are for reference only. It might contain
some unknown problems. MACRONIX will not be liable for any claims or damages
due to the use of LLD sample codes.


2. File Structure
---------------------------------
2.1 File content
  The following files will be available after extracting the zipped file
  (or other compression format):

  MX30XXXX\
    |- README.txt
    |- MX30_CMD.c
    |- MX30_CMD.h
    |- MX30_DEF.h
    |- MX30_APP.c

  These files are key elements of LLD and function in different ways. Here are
  the details of these files:

  MX30_CMD.c (.h): Flash command sequences of flash devices. The number
                   of commands depend on the type of devices. All the
                   commands are programmed according to the data sheet of
                   flash products.
  MX30_DEF.h: Definitions of flash devices and program parameters.
  MX30_APP.c: Sample codes of applications for references. This file is not
              a major one of LLD.

  The naming rule of MX30-series LLD sample codes is as follows:

  MX30_XXX.x:
       --- -
        |  |--> c or h. Source codes or header file.
        |
        |-----> CMD, DEF or APP.
                File names represent for different functions.
       
2.2 File dependency
  MX30_APP.c
    |
    |- <stdlib.h> (*Note1)
    |- MX30_CMD.h <-> MX30_CMD.c
         |
         |- <8051.h> (*Note2)
         |- MX30_DEF.h

  *Note1:
   This header file is for MX30_APP.c to generate random data.
  *Note2:
   This header file is optional. It needs to be included if the system
   uses 8051 micro-controller and connected flash device via GPIO port.


3. Function descriptions
--------------------------------
The MX30-series LLD contains two address mode. You can switch these two
modes for different purpose. Section 3.1 gives a detailed description.
Types of function of LLD are introduced at section 3.2 and command mapping
at section 3.3.

3.1 Address Mode Switch 
    MX30-series flash include additional space of each page. For example,
    a 2KB page has 64Byte additional space. To switch these two space,
    the bit 12 of address is used (A11). Therefore, one page (2K + 64Byte)
    needs 4KB flash address space. And the flash address reserve bit 15~12.  
    The following example shows the difference between normal address and
    flash address.

    Normal address (32bit):
    A31-A24 | A23-A16 | A15-A8 | A7-A0
   ( Address_Convert is not defined )

    Nand Flash address (32bit):
    A27-A20 | A19-A12 | { 0, 0, 0, 0, A11-A8} | A7-A0
    ( Address_Convert is defined )

    You can select the suitable one depend on your system environment.
    The mode definition is in MX30_DEF.h as follows:
        #define  Address_Convert   1
    Default is off. The address is not converted.

3.2 Types of Function: There are three types of function for LLD as follows:
    a. Basic function:
       Include FlashRead() and FlashWrite(). Basic functions are highly
       hardware dependent. You usually need to modify the functions for
       different hardware environment. Please see section 4 for details.

    b. Command function:
       Command function is the major part of LLD. It is used to describe
       the command sequences of flash commands defined in the data sheet.
       Normally, the sequences from LLD and data sheet are fully matched.
       However, please follow the data sheet if any discrepancies.
       Naming rules of command function are as follows:

           ReturnMsg XXX_OP(...);

       Where XXX is a command name like Reset, Program, and so on.
       ReturnMsg report the execution result of the command. Section 5
       describes more about the return messages.

    c. Utility function:
       Those commands which are neither basic nor command functions belong to
       utility functions. Utility function provides some unique functionality
       to make LLD work smoothly.

3.3 Command mapping
    As mentioned at section 3.2.b, the command functions are implementation of
    command sequences defined in the data sheet. In this section, the mapping
    of command sequence and command function is introduced. Here takes the 
    Read_SiliconID_OP command as a example.

    a. Read_ID_OP command sequence:
    The ID READ command sequence includes one command Byte (90h), one address
    byte (00h). The Read ID command 90h may provide the manufacturer ID (C2h)
    of one-byte and device ID (F1h) of one-byte, also 3rd and 4th ID code are
    followed. 

    b. Read_ID_OP command function:
    ReturnMsg Read_ID_OP( uBusWidth *IdBuf )
    {
        uint8 i;
    
        /* Check flash is busy or not */
        if( CheckStatus( SR6_ReadyBusy ) != READY ) return Flash_Busy;
    
        /* Send Read ID command */
        SendCommand( 0x90 );
    
        /* Send one-byte address */
        SendByteAddr( 0x00 );
    
        /* Read 4-byte ID */
        for(i=0; i<4; i=i+1){
            IdBuf[i] = FlashRead();
        }
    
        return Flash_Success;
    }
    In this example, the procedures of command sequence and command function
    are matched. The other sequences and functions are similar to this example.
    Feel free to verify the relationship between different commands.


4. Porting
--------------------------------
The MX30-series LLD sample codes are verified by 8051 IP core. And it uses
8051's GPIO ports to connect flash device. With different target hardware
platforms, it is necessary to modify the LLD source codes. In this section,
porting notices are introduced. And it is divided into data types, definitions
of options and function modification for your references.

4.1 Data types
    For the ability of port LLD to different target system, the length of data
    type can be modified in the MX30_DEF.h header file. Please verify these
    data types before applying.

    Basic data type:
    typedef  unsigned char  BOOL;      /* 1 bit wide */
    typedef  unsigned char  uint8;     /* 8 bit wide */
    typedef  unsigned int   uint16;    /* 16 bit wide */
    typedef  unsigned long  uint32;    /* 32 bit wide */
    typedef  uint32  uAddr;            /* Address width */
    typedef  uint8  uBusWidth;         /* Hardware bus width */

4.2 Definition options
    There are several definition options available based on the target system.
    Please verify these definitions before applying.

    /* System Configuration */
    #define  GPIO_8051     /* Use 8051 MCU and connect flash via GPIO port */
    #define  Timer_8051    /* Use 8051 hardware timer */
    
    /* Target Flash Device: MX30XXXX */
    #define  FLASH_DEVICE

    /* Timer configuration */
    #define  TPERIOD    240    /* ns, The time period of timer count one */
    #define  COUNT_ONE_MICROSECOND  16    /* The loop count value within
                                             one micro-second */
 

4.3 Function modifications
    As mentioned at section 3.2, basic functions are highly dependent on the
    hardware. And some utility functions work the same. These functions
    should be modified based on different hardware architectures. They are
    displayed as follows:

        void FlashWrite( uBusWidth Value );
        uBusWidth FlashRead( void );   
        void InitFlash( void );
        void SendCommand( uBusWidth CMD_code );
        void SendByteAddr( uint8 Byte_addr );
        void SendAddr( uint32 Address );
 
    a. Flash writes data
       Write data to flash device. The data width is decided
       by bus width.

       void FlashWrite( uBusWidth Value )
       {
       #ifdef GPIO_8051
           WRITE_EN = 1;
       
           // send data 1 byte
           P1 = Value;
           toggle();
       
           WRITE_EN = 0;
       #else
           /* Please insert your flash write function */
           // *(FlashBaseAddr) = Value;
       #endif
       }

    b. Flash read data
       Read data from flash device. The data width is decided
       by bus width.

       uBusWidth FlashRead( void )
       {
       #ifdef GPIO_8051
           uBusWidth buffer;
       
           WRITE_EN = 0;
       
           // set GPIO become input.
           P1 = 0xFF;
           IN_VALID = 1;
       
           // wait out valid signal
           while( !OUT_VALID );
       
           // receive data 1 byte
           buffer = P1;
       
           // send ack to notice controller
           IN_VALID = 0;
       
           return buffer;
       #else
           /* Please insert your flash read function */
           // return *(FlashBaseAddr);
       #endif
       }


    c. Flash initialization
       The function initiates hardware state. You may insert some
       initial code if needed.

       void InitFlash( void )
       {
       #ifdef GPIO_8051
           /* Initial 8051 GPIO port */
           WRITE_EN = 0;
           IN_VALID = 0;
           DUMP_SIG = 0;
           CTRL1 = 1;
           CTRL0 = 1;
       #else
           /* Please insert your hardware initial code */
       #endif
       }

    d. Send flash command or flash address
       These functions are based on FlashWrite() function,
       and add the control of ALE or CLE signal.

5. Return messages
--------------------------------
The command functions return a message for operation result. These messages
are helpful for programmers to debug at development stage.

    ReturnMsg XXX( ... );

    XXX:
    Function name. Like Reset_OP, Page_Program_OP and so on.

    ReturnMsg:
    -Flash_Success
     Flash command execute successful.

    -Flash_EraseFailed
     The erase operation failed.

    -Flash_ProgramFailed
     The program operation is failed

    -Flash_OperationTimeOut
     The operation period of flash command exceed expected time length. 

    -Flash_Busy
     The flash device is busy.

    -Flash_CmdInvalid
     The flash command is invalid.

