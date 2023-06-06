/*
 * Security Level: Macronix Proprietary
 * COPYRIGHT (c) 2010-2020 MACRONIX INTERNATIONAL CO., LTD
 * MX30-series Parallel Flash Low Level Driver (LLD) Sample Code
 *
 * Command functions source code.
 * This sample code provides a reference, not recommand for direct using.
 *
 * $Id: MX30_CMD.c,v 1.16 2018/07/18 07:11:02 mxclldb1 Exp $
 */

#include "MX30_CMD.h"

/*
 * Function:     Reset_OP
 * Arguments:    None
 * Return Value: Flash_Success
 * Description:  The reset command FFh resets the read/program/erase
 *               operation and clear the status register to be E0h
 */
ReturnMsg Reset_OP( void )
{
    /* Send reset command */
    SendCommand( 0xFF );

    /* Wait reset finish */
    WaitTime( FLASH_TIMEOUT_VALUE );

    return Flash_Success;
}

/*
 * Function:     Read_ID_OP
 * Arguments:    IdBuf -> The data buffer to store ID
 * Return Value: Flash_Busy, Flash_Success
 * Description:  The Read ID command read the manufacturer ID, device ID, 3rd and 4th ID code.
 */
ReturnMsg Read_ID_OP( uBusWidth *IdBuf )
{
    uint8 i;

    /* Check flash is busy or not */
    if( CheckStatus( GPIO_ReadyBusy ) != READY ) return Flash_Busy;

    /* Send Read ID command */
    SendCommand( 0x90 );

    /* Send one-byte address */
    SendByteAddr( 0x00 );

    #ifdef READ_ID_5BYTE
        /*Read 5-byte ID */
        for(i=0; i<5; i++)
        {
            IdBuf[i] = FlashRead();
        }
    #else
		#ifdef READ_ID_6BYTE
		/* Read 6-byte ID */
        for(i=0; i<6; i=i+1) {
            IdBuf[i] = FlashRead();
        }
		#else
        /* Read 4-byte ID */
        for(i=0; i<4; i=i+1)
      		  {
            IdBuf[i] = FlashRead();
        	  }
		#endif
    #endif


    return Flash_Success;
}

/*
 * Function:     Status_Read_OP
 * Arguments:    StatusReg -> store status value
 * Return Value: Flash_Success
 * Description:  The status read command outputs the device status
 */
ReturnMsg Status_Read_OP( uBusWidth *StatusReg )
{
    /* Send status read command */
    SendCommand( 0x70 );

    /* Read status value */
    *StatusReg = FlashRead();
    

    return Flash_Success;
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
ReturnMsg Page_Read_OP( uAddr Address, uBusWidth * DataBuf, uint32 Length )
{
    uint32 i, status;

    /* Check flash is busy or not */
    if( CheckStatus( &status ) != READY ) return Flash_Busy;

    /* Send page read command */
    SendCommand( 0x00 );

    /* Send flash address */
    SendAddr( Address );

    /* Send page read confirmed command */
    SendCommand( 0x30 );

    /* Wait flash ready and read data in a page */
    WaitTime( FIRST_BYTE_LATENCY_tR );
    for( i=0; i<Length; i=i+1 ){
        DataBuf[i] = FlashRead();
    }

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
ReturnMsg Page_Random_Read_OP( uAddr Address, uBusWidth * DataBuf, uint32 Length )
{
    uint32 i;

    /* Send random read command */
    SendCommand( 0x05 );

    /* Send page address (2 byte) */
    SendByteAddr( (Address >> BYTE0_OFFSET) & BYTE_MASK );
    SendByteAddr( (Address >> BYTE1_OFFSET) & BYTE1_MASK );

    /* Send random read confirmed command */
    SendCommand( 0xE0 );

    /* Read data in a page */
    for( i=0; i<Length; i=i+1 ){
        DataBuf[i] = FlashRead();
    }

    return Flash_Success;
}


/*
 * Function:     Page_Program_OP
 * Arguments:    Address -> flash address
 *               DataBuf -> provide data to program
 *               Length  -> the number of byte(word) to program
 * Return Value: Flash_Busy, Flash_Success, Flash_ProgramFailed,
 *               Flash_OperationTimeOut
 * Description:  Program data in one page.
 */
ReturnMsg Page_Program_OP( uAddr Address, uBusWidth * DataBuf, uint32 Length )
{
    uint32 i;
    uBusWidth status;

    /* Check flash is busy or not */
    if( CheckStatus( GPIO_ReadyBusy ) != READY ) return Flash_Busy;

    /* Send page program command */
    SendCommand( 0x80 );

    /* Send flash address */
    SendAddr( Address );

    /* Send data to program */
    for( i=0; i<Length; i=i+1 ){
       FlashWrite( DataBuf[i] );
    }

    /* Send page program confirm command */
    SendCommand( 0x10 );

    /* Wait page program finish */
    if( WaitFlashReady() == READY ){
        /* Check program result */
        Status_Read_OP( &status );
        if( (status & SR0_ChipStatus ) == SR0_ChipStatus )
            return Flash_ProgramFailed;
        else
            return Flash_Success;
    }
    else
        return Flash_OperationTimeOut;
}

/*
 * Function:     Page_Random_Program_Begin_OP
 * Arguments:    Address -> flash address
 *               DataBuf -> provide data to program
 *               Length  -> the number of byte(word) to program
 * Return Value: Flash_Busy, Flash_Success
 * Description:  Program data randomly in one page.
 *               Note: User needs to execute Page_Random_Program_Input_OP() (if need)
 *                     and Page_Random_Program_End_OP() to finish this command.
 */
ReturnMsg Page_Random_Program_Begin_OP( uAddr Address, uBusWidth * DataBuf, uint32 Length )
{
    uint32 i;

    /* Check flash is busy or not */
    if( CheckStatus( GPIO_ReadyBusy ) != READY ) return Flash_Busy;

    /* Send page program command */
    SendCommand( 0x80 );

    /* Send flash address */
    SendAddr( Address );

    /* Send data to program */
    for( i=0; i<Length; i=i+1 ){
       FlashWrite( DataBuf[i] );
    }

    return Flash_Success;

}

/*
 * Function:     Page_Random_Program_Input_OP
 * Arguments:    Address -> flash address
 *               DataBuf -> provide data to program
 *               Length  -> the number of byte(word) to program
 * Return Value: Flash_Success
 * Description:  Input data during page random program operation.
 *               Note: User needs to execute Page_Random_Program_Begin_OP()
 *                     command first before Page_Random_Program_Input_OP().
 *                     And end with Page_Random_Program_End_OP().
 *                     The number of Page_Random_Program_Input_OP()
 *                     is not limited.
 */
ReturnMsg Page_Random_Program_Input_OP( uAddr Address, uBusWidth * DataBuf, uint32 Length )
{
    uint32 i;

    /* Send page data input command */
    SendCommand( 0x85 );

    /* Send page address (2 byte) */
    SendByteAddr( (Address >> BYTE0_OFFSET) & BYTE_MASK );
    SendByteAddr( (Address >> BYTE1_OFFSET) & BYTE1_MASK );

    /* Send data to program */
    for( i=0; i<Length; i=i+1 ){
       FlashWrite( DataBuf[i] );
    }

    return Flash_Success;
}
/*
 * Function:     Page_Random_Program_End_OP
 * Arguments:    None.
 * Return Value: Flash_Success, Flash_ProgramFailed
 * Description:  Start page program after data input is finished.
 *               Note: User needs to execute Page_Random_Program_Begin_OP()
 *                     and Page_Random_Program_Input_OP() (if need) commands
 *                     before Page_Program_Confirm_OP().
 */
ReturnMsg Page_Random_Program_End_OP( void )
{
    /* Send page program confirm command */
    SendCommand( 0x10 );

    /* Wait page program finish */
    if( WaitFlashReady() == READY ){
        return Flash_Success;
    }
    else
        return Flash_ProgramFailed;
}

/*
 * Function:     Cache_Program_OP
 * Arguments:    Address -> flash address
 *               DataBuf -> provide data to program
 *               Length  -> the number of byte(word) to program
 *               LastPage -> Indicate the last page or not
 *                           0: False, 1: True
 * Return Value: Flash_Success, Flash_ProgramFailed
 * Description:  Cache program can enhances the program performance
 *               by using the cache buffer. The last page needs
 *               confirmed with 0x10 commad to end cache program.
 */
ReturnMsg Cache_Program_OP( uAddr Address, uBusWidth * DataBuf, uint32 Length, BOOL LastPage )
{
    uint32 i;

    /* Send page program command */
    SendCommand( 0x80 );

    /* Send flash address */
    SendAddr( Address );

    /* Send data to program */
    for( i=0; i<Length; i=i+1 ){
       FlashWrite( DataBuf[i] );
    }

    /* Send page program confirm command */
    if( LastPage )
        SendCommand( 0x10 );    // program last page
    else
        SendCommand( 0x15 );    // continue cache program

    /* Wait program status */
    if( WaitFlashReady() == READY ){
        return Flash_Success;
    }
    else
        return Flash_ProgramFailed;
}

/*
 * Function:     Block_Erase_OP
 * Arguments:    Address -> flash address
 * Return Value: Flash_Busy, Flash_EraseFailed,
 *               Flash_Success, Flash_OperationTimeOut
 * Description:  Erase data in a block
 */
ReturnMsg Block_Erase_OP( uAddr Address )
{
    uBusWidth status;

    /* Check flash is busy or not */
    if( CheckStatus( GPIO_ReadyBusy ) != READY ) return Flash_Busy;

    /* Send block erase command */
    SendCommand( 0x60 );

    /* Send block address (2 or 3 bytes) */
	#if ( FLASH_SIZE <= 0x8000000 )
	       SendByteAddr( (Address >> BYTE2_OFFSET) & BYTE_MASK );
	       SendByteAddr( (Address >> BYTE3_OFFSET) & BYTE_MASK );
	#else
	       SendByteAddr( (Address >> BYTE2_OFFSET) & BYTE_MASK );
	       SendByteAddr( (Address >> BYTE3_OFFSET) & BYTE_MASK );
	       SendByteAddr( (Address >> BYTE4_OFFSET) & BYTE_MASK );
	#endif	

    /* Send random read confirmed command */
    SendCommand( 0xD0 );

    /* Wait block erase finish */
    if( WaitFlashReady() == READY ){
        /* Check erase result */
        Status_Read_OP( &status );
        if( (status & GPIO_ReadyBusy ) == SR0_ChipStatus )
            return Flash_EraseFailed;
        else
            return Flash_Success;
    }
    else
        return Flash_OperationTimeOut;
}

/*
 * Utility Function
 */
/*
 * Function:       WaitTime
 * Arguments:      TimeValue -> The time to wait in us
 * Return Value:   READY, TIMEOUT
 * Description:    Wait flash device for a specfic time
 */
void WaitTime( uint32 TimeValue )
{
    FlashInfo flash_info;
    flash_info.Tus = TimeValue;
    Set_Timer( &flash_info );
    Wait_Timer( &flash_info );
}

/*
 * Function:       WaitFlashReady
 * Arguments:      None
 * Return Value:   READY, TIMEOUT
 * Description:    Wait flash device until time-out.
 *                 Check SR6_ReadyBusy of status register.
 */
BOOL WaitFlashReady( void )
{
    FlashInfo flash_info;
    flash_info.Tus = FLASH_TIMEOUT_VALUE;
    Set_Timer( &flash_info );

    while( Check_Timer( &flash_info ) != TIMEOUT )
    {
        if( CheckStatus( GPIO_ReadyBusy ) == READY )
            return READY;
    }

    return TIMEOUT;
}


/*
 * Function:     CheckStatus
 * Arguments:    CheckFlag -> the status bit to check
 * Return Value: READY, BUSY
 * Description:  Check status register bit 7 ~ bit 0
 */
BOOL CheckStatus( uint8 CheckFlag )
{
    uBusWidth status;
    Status_Read_OP( &status );
    if( (status & CheckFlag) == CheckFlag )
        return TRUE;
    else
        return FALSE;
}


/*
 * Function:     Cache_Read_SeqRand_Begin_OP
 * Arguments:    Address -> flash address
 * Return Value: Flash_Success
 * Description:  Cache Read Seq/Rand function
 *               Note: User needs to execute  Cache_Read_Seq_Another_OP()
 *                     or Cache_Read_Random_OP() to finish this command.
 */
ReturnMsg Cache_SeqRand_Read_Begin_OP( uAddr Address )
{

    /* Check the address  is valid or invalid*/
    if (Address & PAGE_MASK)
        return Flash_AddrInvalid;

    /* Check flash is busy or not */
    if( CheckStatus( GPIO_ReadyBusy ) != READY ) return Flash_Busy;

    /* Send cache read command */
    SendCommand( 0x00 );

    /* Send flash address (4 byte) */
    SendAddr( Address );

    /* Send cache read confirmed command */
    SendCommand( 0x30 );

    /* Wait flash ready */
    WaitTime( FIRST_BYTE_LATENCY_tR );

    return Flash_Success;

}

/*
 * Function:   Cache_Read_Seq_Another_OP
 * Arguments:  DataBuf  -> data buffer to store data,
 *             Length   -> the number of byte(word) to read
 *             LastPage -> Indicate the last page or not
 *                         0: False, 1: True
 * Return Value:  Flash_Success
 * Description:   read another cache buffer
 */
ReturnMsg Cache_Seq_Read_Another_OP(uBusWidth* DataBuf, uint32 Length, BOOL LastPage)
{
    uint32 i;

    /* Send cache read command */
    if (LastPage)
        SendCommand( 0x3F );
    else
        SendCommand( 0x31 );

    /* Wait flash ready and read data in a cache buffer */
    WaitTime( tRCBSY );

    for(i=0; i<Length; i++)
        DataBuf[i] = FlashRead();

    return Flash_Success;
}

/*
 * Function:     Cache_Read_Random_OP
 * Arguments:    next_addr -> next to read cache address,
 *               DataBuf   -> data buffer to store data,
 *               Length    -> the number of byte(word) to read
 *               LastPage  -> Indicate the last page or not
 *                         0: False, 1: True
 * Return Value: Flash_Success
 * Description:  read random cache buffer
 */
ReturnMsg Cache_Random_Read_OP( uAddr next_addr, uBusWidth * DataBuf, uint32 Length, BOOL LastPage )
{
    uint32 i;

    /* Send cache read command */
    SendCommand( 0x00 );

    /* Send flash address (4 byte) */
    SendAddr( next_addr );

    /* Send cache read confirmed command */
    if (LastPage)
      SendCommand( 0x3F );
    else
      SendCommand( 0x31 );

    /* Wait flash ready and read data in a cache buffer */
    WaitTime( tRCBSY );
     for( i=0; i<Length; i=i+1 )
    {
      DataBuf[i] = FlashRead();
    }

    return Flash_Success;
}

/*
 * Function:     ONFI_Para_Page_Read_OP
 * Arguments:    DataBuf -> data buffer to store data,
 *               Length  -> the number of byte(word) to read
 * Return Value: Flash_Success
 * Description:  read ONFI Para Page data
 */
ReturnMsg ONFI_Para_Page_Read_OP( uBusWidth * DataBuf, uint32 Length )
{
    uint32 i;

    /* Check flash is busy or not */
    if( CheckStatus( GPIO_ReadyBusy ) != READY ) return Flash_Busy;

    /* Send ONFI para page read command */
    SendCommand( 0xEC );

    /* Send one-byte address */
    SendByteAddr( 0x00 );

    /* Wait flash ready and read parameter data */
    WaitTime( FIRST_BYTE_LATENCY_tR );
    for(i=0; i<Length; i++)
    {
        DataBuf[i] = FlashRead();
    }
    return Flash_Success;

}

/*
 * Function:     ONFI_Unique_ID_Read_OP
 * Arguments:    IDBuf -> The data buffer to store ID
 * Return Value: Flash_Success
 * Description:  read ONFI Unique ID
 */
 ReturnMsg ONFI_Unique_ID_Read_OP(uBusWidth * IDBuf)
{
    uint8 i;

    /* Check flash is busy or not */
    if( CheckStatus( GPIO_ReadyBusy ) != READY ) return Flash_Busy;

    /* Send Read ID command */
    SendCommand( 0xED );

    /* Send one-byte address */
    SendByteAddr( 0x00 );

    /* Wait flash ready and read unique ID*/
    WaitTime( FIRST_BYTE_LATENCY_tR );
    for(i=0; i<32; i++)
    {
        IDBuf[i] = FlashRead();
    }

   return Flash_Success;
}

/*
 * Function:     ONFI_Set_Feature_OP
 * Arguments:    Feature_Address -> Set Feature Address
 *               DataBuf         -> data buffer to program data,
 * Return Value: Flash_Success
 * Description:  Change the power-on default feature set
 */
ReturnMsg ONFI_Set_Feature_OP(uint8 Feature_Address, uBusWidth *DataBuf)
{
    uint8 i;

    /* Send Set Feature command */
    SendCommand( 0xEF );

    /* Send one-byte feature address */
    SendByteAddr( Feature_Address );

    /* Send data to program */
    for( i=0; i<4; i=i+1 )
    {
       FlashWrite( DataBuf[i] );
    }

    /* Wait Set Feature Finish */
    if( WaitFlashReady() == READY )
       return Flash_Success;
    else
       return Flash_OperationTimeOut;

}

/*
 * Function:     ONFI_Get_Feature_OP
 * Arguments:    Feature_Address -> Set Feature Address
 *               DataBuf         -> data buffer to store data,
 * Return Value: Flash_Success
 * Description:  read the sub-feature parameter
 */
ReturnMsg ONFI_Get_Feature_OP(uint8 Feature_Address, uBusWidth *DataBuf)
{
    uint8 i;

    /* Send Get Feature command */
    SendCommand( 0xEE );

    /* Send one-byte feature address */
    SendByteAddr( Feature_Address );

    /* Wait flash ready and get feature data*/
    WaitTime( FIRST_BYTE_LATENCY_tR );
    for(i=0; i<4; i++)
    {
        DataBuf[i] = FlashRead();
    }

    return Flash_Success;

}


/*
 * Function:     Status_Enhanced_Read_OP
 * Arguments:    Address -> flash address
 *               StatusReg ->  status register buffer to store data,
 * Return Value: Flash_Success
 * Description:  Read the Status of specific plane in the same NAND Flash device.
 */
 ReturnMsg Status_Enhanced_Read_OP(uAddr Address, uBusWidth *StatusReg)
{

    /* Send Status Enhanced Read command */
    SendCommand( 0x78 );

    /* Send block address */
    SendByteAddr( (Address >> BYTE2_OFFSET) & BYTE_MASK );
    SendByteAddr( (Address >> BYTE3_OFFSET) & BYTE_MASK );
    SendByteAddr( (Address >> BYTE4_OFFSET) & BYTE_MASK );

    /* Read ERE status Register data */
    *StatusReg = FlashRead();

    return Flash_Success;
}


/*
 * Function:     Block_Protection_Status_Read_OP
 * Arguments:    Address -> flash address
 *               StatusReg ->  status register buffer to store data,
 * Return Value: Flash_Success
 * Description:  Read the protection status of individual blocks and the PBL status.
   */
ReturnMsg Block_Protection_Status_Read_OP(uAddr Address, uBusWidth *StatusReg)
{

    /* Send block protection status read command */
    SendCommand( 0x7A );

    /* Send BA address */
    SendByteAddr( (Address >> BYTE2_OFFSET) & BYTE_MASK );
    SendByteAddr( (Address >> BYTE3_OFFSET) & BYTE_MASK );
    SendByteAddr( (Address >> BYTE4_OFFSET) & BYTE_MASK );

    /* Read Block Protection status Register data */
    *StatusReg = FlashRead();

    return Flash_Success;
}

/*
 * Function:     Deep_Power_Down_OP
 * Return Value: Flash_Success
 * Description:  Enter the deep power down mode to place the device
 *               into a minimum power consumption state.
   */
ReturnMsg Deep_Power_Down_OP( void )
{

    /* Send Deep Power Down command */
    SendCommand( 0xB9 );

    return Flash_Success;
}

/*
 * Function:     Two_plane_Program_OP
 * Arguments:    Address -> flash address
 *               DataBuf -> provide data to program
 *               Length  -> the number of byte(word) to program
 *               LastPlane -> Indicate the last plane or not
 *                           0: False, 1: True
 *			        TradFlag -> Indicate ONFI or Tradional
 *                           0: ONFI, 1: Tradional
 * Return Value: Flash_Success, Flash_ProgramFailed
 * Description:  Two-plane program can enhances the program performance
 *               by using two planes.
 * Notice:       Two_plane_Program_OP can only be used with two times.
 *               The second plane needs to be confirmed with 0x10 commad to end two-plane program.
   */
ReturnMsg Two_plane_Program_OP( uAddr Address, uBusWidth * DataBuf, uint32 Length, BOOL LastPlane, BOOL TradFlag )
{
    uint32 i;
    uBusWidth status;

	  if( !TradFlag )
	{
		    /* Send Two-Plane program command */
		    SendCommand( 0x80 );

		    /* Send flash address */
		    SendAddr( Address );

		    /* Send data to program */
		    for( i=0; i<Length; i=i+1 ){
		       FlashWrite( DataBuf[i] );
		    }

		    /* Send Two-plane program confirm command */
		    if (LastPlane)
		    {
		        SendCommand( 0x10 );

		       /* Wait page program finish */
		        if( WaitFlashReady() == READY ){
		            /* Check program result */
		            Status_Enhanced_Read_OP( Address, &status );
		            if( (status & SR0_ChipStatus ) == SR0_ChipStatus )
		                return Flash_ProgramFailed;
		            else
		                return Flash_Success;
		        }
		        else
		            return Flash_OperationTimeOut;
		    }
		    else
		    {
		        SendCommand( 0x11 );
		        /* Wait flash ready*/
		        WaitTime( tDBSY );

		        return Flash_Success;
		    }
	}
	else
	{	
		if (!LastPlane)
		{
			/* Send Two-Plane program command */
		    SendCommand( 0x80 );

		    /* Send flash address */
			 SendAddrTrad( Address, LastPlane );

		    /* Send data to program */
		    for( i=0; i<Length; i=i+1 ){
		       FlashWrite( DataBuf[i] );
			  }

		  SendCommand( 0x11 );
        /* Wait flash ready*/
        WaitTime( tDBSY );

        return Flash_Success;
		}
		else
		{
			/* Send Two-Plane program command */
		    SendCommand( 0x81 );

		    /* Send flash address */
			 SendAddrTrad( Address, LastPlane );

		    /* Send data to program */
		    for( i=0; i<Length; i=i+1 ){
		       FlashWrite( DataBuf[i] );
			 }

				/* Send Two-plane program confirm command */
				SendCommand( 0x10 );

		       /* Wait page program finish */
		        if( WaitFlashReady() == READY ){
		            /* Check program result */
		            Status_Enhanced_Read_OP( Address, &status );
		            if( (status & SR0_ChipStatus ) == SR0_ChipStatus )
		                return Flash_ProgramFailed;
		            else
		                return Flash_Success;
		        }
		        else
		            return Flash_OperationTimeOut;
		}
	}
}

/*
 * Function:     Two_plane_Random_Program_Begin_OP
 * Arguments:    Address -> flash address
 *               DataBuf -> provide data to program
 *               Length  -> the number of byte(word) to program
 * Return Value: Flash_Busy, Flash_Success
 * Description:  Program data randomly in one page.
 *               Note: User needs to execute Two_plane_Page_Random_Program_Input_OP() (if need)
 *                     and Two_plane_Page_Random_Program_Confirm_OP() to finish this command.
 */
ReturnMsg Two_plane_Page_Random_Program_Begin_OP( uAddr Address, uBusWidth * DataBuf, uint32 Length )
{
    uint32 i;

    /* Check flash is busy or not */
    if( CheckStatus( SR6_ReadyBusy ) != READY ) return Flash_Busy;

    /* Send page random program command */
    SendCommand( 0x80 );

    /* Send flash address */
    SendAddr( Address );

    /* Send data to program */
    for( i=0; i<Length; i=i+1 ){
       FlashWrite( DataBuf[i] );
    }

    return Flash_Success;

}

/*
 * Function:     Two_plane_Random_Program_Input_OP
 * Arguments:    Address -> flash address
 *               DataBuf -> provide data to program
 *               Length  -> the number of byte(word) to program
 * Return Value: Flash_Success
 * Description:  Input data during page random program operation.
 *               Note: User needs to execute Two_plane_Page_Random_Program_Begin_OP()
 *                     command first before Two_plane_Page_Random_Program_Input_OP().
 *                     And end with Two_plane_Page_Random_Program_Confirm_OP().
 *                     The number of Two_plane_Page_Random_Program_Input_OP()
 *                     is not limited.
 */
ReturnMsg Two_plane_Page_Random_Program_Input_OP( uAddr Address, uBusWidth * DataBuf, uint32 Length )
{
    uint32 i;

    /* Send page random data input command */
    SendCommand( 0x85 );

    /* Send page address (2 byte) */
    SendByteAddr( (Address >> BYTE0_OFFSET) & BYTE_MASK );
    SendByteAddr( (Address >> BYTE1_OFFSET) & BYTE1_MASK );

    /* Send data to program */
    for( i=0; i<Length; i=i+1 ){
        FlashWrite( DataBuf[i] );
    }

    return Flash_Success;
}


/*
 * Function:     Two_plane_Page_Random_Program_Confirm_OP
 * Arguments:    LastPlane -> Indicate last plane or not.
 * Return Value: Flash_Success, Flash_ProgramFailed
 * Description:  Start page random program after data input is finished.
 *               Note: User needs to execute Two_plane_Page_Random_Program_Begin_OP()
 *                     and Two_plane_Page_Random_Program_Input_OP() (if need) commands
 *                     before Two_plane_Page_Program_Confirm_OP().
 */
ReturnMsg Two_plane_Page_Random_Program_Confirm_OP( BOOL LastPlane )
{
    if( LastPlane) {
        /* Send page random program confirm command */
        SendCommand( 0x10 );

        /* Wait page random program finish */
        if( WaitFlashReady() != READY ) {
            return Flash_ProgramFailed;
        }
    }
    else {
        /* Send page random program confirm command */
        SendCommand( 0x11 );
        /* Wait flash ready*/
        WaitTime( tDBSY );
	}
    return Flash_Success;
}

/*
 * Function:     Two_plane_Cache_Program_Plane1_OP
 * Arguments:    Address -> flash address
 *               DataBuf -> provide data to program
 *               Length  -> the number of byte(word) to program
 *			        TradFlag -> Indicate ONFI or Tradional
 *                           0: ONFI, 1: Tradional
 * Return Value: Flash_Success
 * Description:  Input data during cache program operation.
 *               Note: Two-plane cache program can enhances the program performance
 *               by using the cache buffer.
 */
ReturnMsg Two_plane_Cache_Program_Plane1_OP( uAddr Address, uBusWidth * DataBuf, uint32 Length, BOOL TradFlag )
{
    uint32 i;

    /* Send Two-Plane cache program command */
    SendCommand( 0x80 );

    /* Send flash address */
	 if( !TradFlag ){
    	SendAddr( Address );
	  }
	 else {
		SendAddrTrad( Address, FALSE );
	}

    /* Send data to program */
    for( i=0; i<Length; i=i+1 ){
        FlashWrite( DataBuf[i] );
    }

    /* Send cahce program confirm command */
    SendCommand( 0x11 );

    /* Wait flash ready*/
    WaitTime( tDBSY );

    return Flash_Success;
}

/*
 * Function:     Two_plane_Cache_Program_Plane2_OP
 * Arguments:    Address -> flash address
 *               DataBuf -> provide data to program
 *               Length  -> the number of byte(word) to program
 *               LastPlane -> Indicate last plane or not
 *			        TradFlag -> Indicate ONFI or Tradional
 *                           0: ONFI, 1: Tradional
 * Return Value: Flash_Success
 * Description:  Input data during cache program operation.
 *               Note: Two-plane cache program can enhances the program performance
 *               by using the cache buffer. The last page needs
 *               confirmed with 0x10 commad to end cache program.
 */
ReturnMsg Two_plane_Cache_Program_Plane2_OP( uAddr Address, uBusWidth * DataBuf, uint32 Length, BOOL LastPlane, BOOL TradFlag )
{
    uint32 i;

	 if ( !TradFlag )
	 {
	    /* Send Two-Plane cache program command */
	    SendCommand( 0x80 );

	    /* Send flash address */
	    SendAddr( Address );
	}
	else
	{	
		 /* Send Two-Plane cache program command */
	    SendCommand( 0x81 );
		
		/* Send flash address */
		 SendAddrTrad( Address, TRUE );
	}

    /* Send data to program */
    for( i=0; i<Length; i=i+1 ){
        FlashWrite( DataBuf[i] );
    }

    if ( LastPlane ) {
        /* Send cache program confirm command */
        SendCommand( 0x10 );

        /* Wait cache program finish */
        if( WaitFlashReady() != READY )
            return Flash_ProgramFailed;
    }
    else {
        /* Send cache program confirm command */
        SendCommand( 0x15 );

        /* Wait flash ready*/
        WaitTime( tCBSY );
    }

    return Flash_Success;
}

/*
 * Function:     Two-plane_Block_Erase_OP
 * Arguments:    Address -> flash address
 *               LastBlock -> last block erase command or not 
 *			        TradFlag -> Indicate ONFI or Tradional
 *                           0: ONFI, 1: Tradional
 * Return Value: Flash_Busy, Flash_EraseFailed,
 *               Flash_Success, Flash_OperationTimeOut
 * Description:  Erase data in a block. The last plane needs
 *               confirmed with 0xD0 commad to end two-plane block erase op.
 */
ReturnMsg Two_plane_Block_Erase_OP( uAddr Address, BOOL LastBlock, BOOL TradFlag )
{
    uBusWidth status;

    /* Check flash is busy or not */
    if(!( TradFlag && LastBlock ))
		if( CheckStatus( SR6_ReadyBusy ) != READY ) return Flash_Busy;

    /* Send block erase command */
    SendCommand( 0x60 );

	 if( !TradFlag )
	{
		    /* Send block address (2 or 3 bytes) */
		#if ( FLASH_SIZE <= 0x8000000 )
		    SendByteAddr( (Address >> BYTE2_OFFSET) & BYTE_MASK );
		    SendByteAddr( (Address >> BYTE3_OFFSET) & BYTE_MASK );
		#else
		    SendByteAddr( (Address >> BYTE2_OFFSET) & BYTE_MASK );
		    SendByteAddr( (Address >> BYTE3_OFFSET) & BYTE_MASK );
		    SendByteAddr( (Address >> BYTE4_OFFSET) & BYTE_MASK );
		#endif

		    /* Send block erase confirmed command */
		    if ( LastBlock ) {
		       SendCommand( 0xD0 );
		      /* Wait block erase finish */
		      if( WaitFlashReady() == READY ){
		          /* Check erase result */
		          Status_Enhanced_Read_OP( Address, &status );
		          if( (status & SR0_ChipStatus ) == SR0_ChipStatus )
		              return Flash_EraseFailed;
		      }
		      else
		          return Flash_OperationTimeOut;
		    }
		    else {
		         SendCommand( 0xD1 );
		         /* Wait flash ready*/
		         WaitTime( tDBSY );
		    }
	}
	else
	{
		/* Send block erase confirmed command */
		  if ( LastBlock ) {
				/* Send block address (2 or 3 bytes) */
				#if ( FLASH_SIZE <= 0x8000000 )
				    SendByteAddr( (Address >> BYTE2_OFFSET) & BYTE_MASK );
				    SendByteAddr( (Address >> BYTE3_OFFSET) & BYTE_MASK );
				#else
				    SendByteAddr( (((Address >> BYTE2_OFFSET) & BYTE_MASK) & 0x80) | 0x40 );
				    SendByteAddr( (Address >> BYTE3_OFFSET) & BYTE_MASK );
				    SendByteAddr( (Address >> BYTE4_OFFSET) & BYTE_MASK );
				#endif				
				
		       SendCommand( 0xD0 );
		      /* Wait block erase finish */
		      if( WaitFlashReady() == READY ){
		          /* Check erase result */
		          Status_Enhanced_Read_OP( Address, &status );
		          if( (status & SR0_ChipStatus ) == SR0_ChipStatus )
		              return Flash_EraseFailed;
		      }
		      else
		          return Flash_OperationTimeOut;
		    }
		 else {
		      /* Send block address (2 or 3 bytes) */
				#if ( FLASH_SIZE <= 0x8000000 )
				    SendByteAddr( 0 );
				    SendByteAddr( 0 );
				#else
				    SendByteAddr( 0 );
				    SendByteAddr( 0 );
				    SendByteAddr( 0 );
				#endif
		    }
	}

    return Flash_Success;
}

/*
 * Function:     Flash Timer Group
 * Arguments:    fptr -> Pointer of flash infomation structure
 * Return Value: TIMEOUT, TIMENOTOUT
 * Description:  Set_Timer()   : Set timer for specific value and run
 *               Check_Timer() : Check timer is exceed (TIMEOUT) or not.
 *               Wait_Timer()  : Wait timer until time out.
 *
 * Usage:
 *     Set_Timer()
 *     Check_Timer() or Wait_Timer()
 *
 */
#ifdef Timer_8051
void Set_Timer( FlashInfo *fptr )
{
    uint16 timer_reg;
    uint32 timer_value;
    TR0 = 0;    // make sure timer0 is stopped
    TF0 = 0;    // clear timeout flag
    TMOD = 0x01;    // Set timer 0 to mode 1 ( 16 bit cascade timer )
    fptr->TEXT0 = 0;      // clear extend timer count

    timer_value = (fptr->Tus/TPERIOD)*1000;
    if( timer_value > 0xffff ){
        fptr->TEXT0 = (timer_value >> 16) + 1;
        timer_reg = 0x0000;
    }
    else{
        fptr->TEXT0 = 0;
        timer_reg = 0xffff - timer_value;
    }

    // Set timer initial value
    TL0 = timer_reg;
    TH0 = timer_reg >> 8 ;

    TCON = 0x10;    // start timer
}

BOOL Check_Timer( FlashInfo *fptr )
{
    if( TF0 == 1 ){
        if( fptr->TEXT0 == 0){
            TF0 = 0;    // clear timeout flag
            TR0 = 0;    // stop timer 0
            return TIMEOUT;
        }
        else{
            fptr->TEXT0 = fptr->TEXT0 - 1;
            TF0 = 0;
            return TIMENOTOUT;
        }
    }
    else
        return TIMENOTOUT;
}

void Wait_Timer( FlashInfo *fptr )
{
    while( 1 ){
        if( TF0 == 1 && fptr->TEXT0 == 0){
            TF0 = 0;
            break;
        }
        else if( TF0 == 1 && fptr->TEXT0 > 0 ){
            fptr->TEXT0 = fptr->TEXT0 - 1;
            TF0 = 0;
        }
    }
    TF0 = 0;  // clear timeout flag
    TR0 = 0;  // stop timer 0
}
#else
void Set_Timer( FlashInfo *fptr  )
{
    fptr->Timeout_Value = (fptr->Tus)/COUNT_ONE_MICROSECOND;
    fptr->Timer_counter = 0;
}

BOOL Check_Timer( FlashInfo *fptr  )
{
    if( fptr->Timer_counter < fptr->Timeout_Value ){
        fptr->Timer_counter = fptr->Timer_counter + 1;
        return TIMENOTOUT;
    }else{
        return TIMEOUT;
    }
}

void Wait_Timer( FlashInfo *fptr  )
{
    /* Wait timer until time-out */
    while( fptr->Timer_counter < fptr->Timeout_Value ){
        fptr->Timer_counter = fptr->Timer_counter + 1;
    }
}

#endif  /* End Timer_8051 */
/*
 * Basic function
 */

/*
 * Function:     InitFlash
 * Arguments:    None.
 * Return Value: None.
 * Description:  Initial MX30 flash device
 */
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

/*
 * Function:     SendCommand
 * Arguments:    CMD_code -> The MX30 flash command code
 * Return Value: None.
 * Description:  Send flash command
 */
void SendCommand( uBusWidth CMD_code )
{
#ifdef GPIO_8051
    /* Enable command latch signal */
    CLE_HIGH();

    /* Send commmand data */
    FlashWrite( CMD_code );

    /* Disable command latch signal */
    CLE_LOW();
#else
    /* Please insert your flash command function */
    // *(FlashBaseAddr + CMD_reg_offset) = CMD_code;
#endif

}

/*
 * Function:     SendByteAddr
 * Arguments:    Byte_addr -> one byte address
 * Return Value: None.
 * Description:  Send one byte address
 */
void SendByteAddr( uint8 Byte_addr )
{
#ifdef GPIO_8051
    /* Enable address latch signal */
    ALE_HIGH();

    /* Send address data */
    FlashWrite( Byte_addr );

    /* Disable address latch signal */
    ALE_LOW();
#else
    /* Please insert your flash address function */
    // *(FlashBaseAddr + Addr_reg_offset) = Byte_addr;
#endif

}

/*
 * Function:     SendAddr
 * Arguments:    Address -> 4(5) byte address
 * Return Value: None.
 * Description:  Send 4(5) byte address
 */
void SendAddr( uint32 Address )
{
#ifdef GPIO_8051
    /* Enable address latch signal */
    ALE_HIGH();

    /* Send four byte address data */
 #if (FLASH_SIZE <= 0x8000000)
	FlashWrite( (Address >> BYTE0_OFFSET) & BYTE_MASK );
	FlashWrite( (Address >> BYTE1_OFFSET) & BYTE1_MASK );
	FlashWrite( (Address >> BYTE2_OFFSET) & BYTE_MASK );
	FlashWrite( (Address >> BYTE3_OFFSET) & BYTE_MASK );

 #else
	FlashWrite( (Address >> BYTE0_OFFSET) & BYTE_MASK );
	FlashWrite( (Address >> BYTE1_OFFSET) & BYTE1_MASK );
	FlashWrite( (Address >> BYTE2_OFFSET) & BYTE_MASK );
	FlashWrite( (Address >> BYTE3_OFFSET) & BYTE_MASK );
	FlashWrite( (Address >> BYTE4_OFFSET) & BYTE_MASK );
 #endif

    /* Disable address latch signal */
    ALE_LOW();
#else
    /* Please insert your flash address function */
    // *(FlashBaseAddr + Addr_reg_offset) = Address;
#endif

}

/*
 * Function:     SendAddrTrad
 * Arguments:    Address -> 4(5) byte address
 *               LastPlane -> Indicate last plane or not
 * Return Value: None.
 * Description:  Send 4(5) byte address in traditional mode
 */
void SendAddrTrad( uint32 Address, BOOL LastPlane )
{
#ifdef GPIO_8051
    /* Enable address latch signal */
    ALE_HIGH();

    /* Send four byte address data */
#if (FLASH_SIZE <= 0x8000000)

    FlashWrite( (Address >> BYTE0_OFFSET) & BYTE_MASK );
    FlashWrite( (Address >> BYTE1_OFFSET) & BYTE1_MASK );
	 if( LastPlane ) 
	 {
    	FlashWrite( (Address >> BYTE2_OFFSET) & BYTE_MASK | 0x40 );
    	FlashWrite( (Address >> BYTE3_OFFSET) & BYTE_MASK );
	 }
	else
	{	
		FlashWrite( 0 );
    	FlashWrite( 0 );
	}

#else

    FlashWrite( (Address >> BYTE0_OFFSET) & BYTE_MASK );
    FlashWrite( (Address >> BYTE1_OFFSET) & BYTE1_MASK );
	 if( LastPlane ) 
	 {
    	FlashWrite( (Address >> BYTE2_OFFSET) & BYTE_MASK | 0x40);
    	FlashWrite( (Address >> BYTE3_OFFSET) & BYTE_MASK );
    	FlashWrite( (Address >> BYTE4_OFFSET) & BYTE_MASK );
	 }
	 else
	{	
		FlashWrite( 0 );
    	FlashWrite( 0 );
		FlashWrite( 0 );
	}

#endif
    /* Disable address latch signal */
    ALE_LOW();
#else
    /* Please insert your flash address function */
    // *(FlashBaseAddr + Addr_reg_offset) = Address;
#endif

}


/*
 * Function:     FlashWrite
 * Arguments:    Value -> data to write
 * Return Value: None.
 * Description:  Write a data to flash
 */
void FlashWrite( uBusWidth Value )
{
#ifdef GPIO_8051
    WRITE_EN = 1;

    // send data 1 byte
    P1 = Value;
    toggle();

#ifdef X16_DEVICE
    P2 = Value >> 8;
    toggle();
#endif

    WRITE_EN = 0;
#else
    /* Please insert your flash write function */
    // *(FlashBaseAddr) = Value;
#endif
}

/*
 * Function:     FlashRead
 * Arguments:    None
 * Return Value: Read data
 * Description:  Read a data from flash
 */
uBusWidth FlashRead( void )
{
#ifdef GPIO_8051
    uBusWidth buffer;

    WRITE_EN = 0;

    // set GPIO become input.
    P1 = 0xFF;
#ifdef X16_DEVICE
    P2 = 0xFF;
#endif
    IN_VALID = 1;

    // wait out valid signal
    while( !OUT_VALID );

    // receive data
#ifdef X16_DEVICE
	buffer = (P2 << 8) | P1;
#else
    buffer = P1;
#endif

    // send ack to notice controller
    IN_VALID = 0;

    return buffer;
#else
    /* Please insert your flash read function */
    // return *(FlashBaseAddr);
#endif
}


