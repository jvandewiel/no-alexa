/*
 * Security Level: Macronix Proprietary
 * COPYRIGHT (c) 2010-2020 MACRONIX INTERNATIONAL CO., LTD
 * MX30-series Parallel Flash Low Level Driver (LLD) Sample Code
 *
 * Function prototype and related definitions.
 * This sample code provides a reference, not recommand for directing using.
 *
 * $Id: MX30_CMD.h,v 1.12 2018/07/18 07:12:07 mxclldb1 Exp $
 */

#ifndef __MX30_CMD_H__
#define __MX30_CMD_H__

#include "MX30_DEF.h"

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

/* Two Plane Cache Read Mode */
typedef enum{
   Cache_SeqRead_Mode,
   Cache_RandRead_Mode,
   LastPage_Mode
}ReadMode;

/* Flash Parameter Structure */
struct fps{
    uBusWidth *Array;
    uAddr Address;
    uint32 Length;
};

typedef struct fps FlashParameter;

/* Flash Information */
struct fi{
    /* Timer Variable */
    uint32  Tus;        // time-out length in us
#ifdef Timer_8051
    uint32  TEXT0;      // extend timer
#else
    uint32  Timeout_Value;
    uint32  Timer_counter;
#endif
};

typedef struct fi FlashInfo;

/* Basic Function */
void InitFlash( void );
void SendCommand( uBusWidth CMD_code );
void SendByteAddr( uint8 Byte_addr );
void SendAddr( uint32 Address );
void SendAddrTrad( uint32 Address, BOOL LastPlane );
void FlashWrite( uBusWidth Value );
uBusWidth FlashRead( void );

/* Utility Function */
void WaitTime( uint32 TimeValue );
BOOL WaitFlashReady( void );
BOOL CheckStatus( uint8 CheckFlag );
void Set_Timer( FlashInfo *fptr );
BOOL Check_Timer( FlashInfo *fptr );
void Wait_Timer( FlashInfo *fptr );
ReturnMsg DataToggle( void );

/* Flash Command */
ReturnMsg Reset_OP( void );
ReturnMsg Read_ID_OP( uBusWidth *ID_BUF );
ReturnMsg Status_Read_OP( uBusWidth *StatusReg );
ReturnMsg Page_Read_OP( uAddr Address, uBusWidth * DataBuf, uint32 Length );
ReturnMsg Page_Random_Read_OP( uAddr Address, uBusWidth * DataBuf, uint32 Length );
ReturnMsg Page_Program_OP( uAddr Address, uBusWidth * DataBuf, uint32 Length ); 
ReturnMsg Page_Random_Program_Begin_OP( uAddr Address, uBusWidth * DataBuf, uint32 Length );
ReturnMsg Page_Random_Program_Input_OP( uAddr Address, uBusWidth * DataBuf, uint32 Length );
ReturnMsg Page_Random_Program_End_OP( void );
ReturnMsg Cache_Program_OP( uAddr Address, uBusWidth * DataBuf, uint32 Length, BOOL LastPage );
ReturnMsg Block_Erase_OP( uAddr Address );

ReturnMsg ONFI_Para_Page_Read_OP( uBusWidth * DataBuf, uint32 Length );

ReturnMsg ONFI_Unique_ID_Read_OP( uBusWidth * IDBuf );

ReturnMsg Cache_SeqRand_Read_Begin_OP( uAddr Address );
ReturnMsg Cache_Seq_Read_Another_OP(uBusWidth* DataBuf, uint32 Length, BOOL LastPage);
ReturnMsg Cache_Random_Read_OP( uAddr next_addr, uBusWidth * DataBuf, uint32 Length, BOOL LastPage );

ReturnMsg ONFI_Set_Feature_OP(uint8 Feature_Address, uBusWidth * DataBuf);
ReturnMsg ONFI_Get_Feature_OP(uint8 Feature_Address, uBusWidth * DataBuf);


ReturnMsg Status_Enhanced_Read_OP( uAddr Address, uBusWidth *StatusReg );


ReturnMsg Block_Protection_Status_Read_OP(uAddr Address, uBusWidth *StatusReg);
ReturnMsg Deep_Power_Down_OP( void );

ReturnMsg Two_plane_Program_OP( uAddr Address, uBusWidth * DataBuf, uint32 Length, BOOL LastPlane, BOOL TradFlag );

ReturnMsg Two_plane_Page_Random_Program_Begin_OP( uAddr Address, uBusWidth * DataBuf, uint32 Length );
ReturnMsg Two_plane_Page_Random_Program_Input_OP( uAddr Address, uBusWidth * DataBuf, uint32 Length );
ReturnMsg Two_plane_Page_Random_Program_Confirm_OP( BOOL LastPlane );

ReturnMsg Two_plane_Cache_Program_Plane1_OP( uAddr Address, uBusWidth * DataBuf, uint32 Length, BOOL TradFlag );
ReturnMsg Two_plane_Cache_Program_Plane2_OP( uAddr Address, uBusWidth * DataBuf, uint32 Length, BOOL LastPlane, BOOL TradFlag );

ReturnMsg Two_plane_Block_Erase_OP( uAddr Address, BOOL LastBlock, BOOL TradFlag );


#endif  /* __MX30_CMD_H__ */
