/*
 * Security Level: Macronix Proprietary
 * COPYRIGHT (c) 2010-2020 MACRONIX INTERNATIONAL CO., LTD
 * MX30-series Parallel Flash Low Level Driver (LLD) Sample Code
 *
 * Application example.
 * This sample code provides a reference, not recommand for direct using.
 *
 * $Id: MX30_APP.c,v 1.1.1.1 2012/02/23 06:14:37 modelqa Exp $
 */

#include "MX30_CMD.h"
#include <stdlib.h>
#include <string.h>

#define TRANS_LENGTH 64

/* Command definition, some optional commands
   are supported by specific products. */
typedef enum{
    Read_ID,
    Program,
    Read,
    BlockErase
}CmdType;

ReturnMsg FlashAPI( CmdType Cmd, FlashParameter *fptr );

/*
 * Function:     main
 * Arguments:    None.
 * Return Value: 0 -> execute successful
 *               1 -> execute failed
 * Description:  Entry of program.
 */
int main( void )
{
    FlashParameter fpar = {0};
    uAddr flash_init_addr ;
    uBusWidth device_id[4] = {0};
    uBusWidth exp_dat[TRANS_LENGTH] = {0};
    uBusWidth read_dat[TRANS_LENGTH] = {0};
    int i;

    /* Initial of MX30-series LLD applcation */
    InitFlash();
    srand( 123 );  /* Initial random seed of rand() function */

    /* Read device ID. */
    fpar.Array = device_id;
    fpar.Length = 4;
    if( FlashAPI( Read_ID, &fpar ) != (ReturnMsg)Flash_Success )  return 1;
    for( i=0; i<4; i=i+1 ){
        if( device_id[i] != ID_CODE[i] ) return 1;
    }

    /* Program data to flash device */
    flash_init_addr = 0x00000000;
    for( i=0 ; i< TRANS_LENGTH ; i=i+1 ){
        exp_dat[i] = rand()&0xFF;    /* Generate random number */
    }
    fpar.Array = exp_dat;
    fpar.Address = flash_init_addr;
    fpar.Length = TRANS_LENGTH;
    if( FlashAPI( Program, &fpar ) != (ReturnMsg)Flash_Success ){  /* Program data */
        return 1;
    }

    /* Read and verify data */
    fpar.Array = read_dat;
    if( FlashAPI( Read, &fpar ) != (ReturnMsg)Flash_Success )  return 1; /* Read data */
    for( i=0 ; i< TRANS_LENGTH ; i=i+1 ){
        if( read_dat[i] != exp_dat[i] )  return 1;  /* Verify data */
    }

    /* Erase flash data */
    fpar.Address = flash_init_addr;
    if( FlashAPI( BlockErase, &fpar ) != (ReturnMsg)Flash_Success )  return 1;  /* Erase data */

    /* Read and verify data */
    fpar.Array = read_dat;
    if( FlashAPI( Read, &fpar ) != (ReturnMsg)Flash_Success ){  /* Read data */
        return 1;
    }
    for( i=0 ; i< TRANS_LENGTH ; i=i+1 ){
        if( read_dat[i] != 0xFF )  return 1;  /* Verify data, should equal to 0xFF */
    }
    return 0;  /* Execute success */
}


/*
 * Function:     FlashAPI
 * Arguments:    Cmd -> Input command code
 *               fptr -> Flash parameter pointer
 * Return Value: Flash command returned message.
 * Description:  This function provide the interface to access flash commands.
 *
 * Include Command:
 *     Read_ID
 */
ReturnMsg FlashAPI( CmdType Cmd, FlashParameter *fptr )
{
  ReturnMsg rtMsg = Flash_Success;

    switch( Cmd ){
        case Read_ID:
            rtMsg = Read_ID_OP( fptr->Array );
            break;
        case Program:
            rtMsg = Page_Program_OP( fptr->Address, fptr->Array, fptr->Length );
            break;
        case Read:
            rtMsg = Page_Read_OP( fptr->Address, fptr->Array, fptr->Length );
            break;
        case BlockErase:
            rtMsg = Block_Erase_OP( fptr->Address );
            break;
        default:
            rtMsg = Flash_CmdInvalid;
            break;
    }

    return rtMsg;
}
