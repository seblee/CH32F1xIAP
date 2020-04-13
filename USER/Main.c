/********************************** (C) COPYRIGHT *******************************
* File Name          : main.c
* Author             : WCH
* Version            : V1.0.0
* Date               : 2019/10/15
* Description        : Main program body.
*******************************************************************************/ 
#include "debug.h"
#include "string.h"

/* Global define */


/* Global Variable */    
__align(4) UINT8  RxBuffer[ MAX_PACKET_SIZE ] ;      // IN, must even address
__align(4) UINT8  TxBuffer[ MAX_PACKET_SIZE ] ;      // OUT, must even address
__align(4) UINT8  Com_Buffer[ 128 ];                

/*******************************************************************************
* Function Name  : main
* Description    : Main program.
* Input          : None
* Return         : None
*******************************************************************************/
int main()
{	
	UINT8	s;
    
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    Delay_Init();
	USART_Printf_Init(115200);
	printf("Start @Chip_ID:%08x\r\n", DBGMCU->IDCODE );
	
	printf("USBHD   HOST Test\r\n");
	Set_USBConfig(); 
    pHOST_RX_RAM_Addr = RxBuffer;
    pHOST_TX_RAM_Addr = TxBuffer;
	USB_HostInit();
	printf( "Wait Device In\n" );
	
	while(1)
	{
		s = ERR_SUCCESS;
		
		if ( R8_USB_INT_FG & RB_UIF_DETECT )
		{  
			R8_USB_INT_FG = RB_UIF_DETECT ; 
			printf( "Wait Device In\n" );
			s = AnalyzeRootHub( );   
			if ( s == ERR_USB_CONNECT ) FoundNewDev = 1;
		}
		
		if ( FoundNewDev || s == ERR_USB_CONNECT ) 
		{ 
			FoundNewDev = 0;
			mDelaymS( 200 ); 
			s = InitRootDevice( Com_Buffer );  
			//if ( s != ERR_SUCCESS ) 	return( s );
		}
	}
}




