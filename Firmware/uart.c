/***********************************************************************
 * DISCLAIMER:                                                         *
 * The software supplied by Renesas Technology America Inc. is         *
 * intended and supplied for use on Renesas Technology products.       *
 * This software is owned by Renesas Technology America, Inc. or       *
 * Renesas Technology Corporation and is protected under applicable    *
 * copyright laws. All rights are reserved.                            *
 *                                                                     * 
 * THIS SOFTWARE IS PROVIDED "AS IS". NO WARRANTIES, WHETHER EXPRESS,  *
 * IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO IMPLIED 		   *
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE  *
 * APPLY TO THIS SOFTWARE. RENESAS TECHNOLOGY AMERICA, INC. AND        *
 * AND RENESAS TECHNOLOGY CORPORATION RESERVE THE RIGHT, WITHOUT       *
 * NOTICE, TO MAKE CHANGES TO THIS SOFTWARE. NEITHER RENESAS           *
 * TECHNOLOGY AMERICA, INC. NOR RENESAS TECHNOLOGY CORPORATION SHALL,  * 
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR         *
 * CONSEQUENTIAL DAMAGES FOR ANY REASON WHATSOEVER ARISING OUT OF THE  *
 * USE OR APPLICATION OF THIS SOFTWARE.                                *
 ***********************************************************************/

/************************************************************************
 *											   						 	*
 *    DESCRIPTION: main routine for SIO - UART mode 		 	 	 	*
 *											  						 	*
			   						 	*
 *											   						 	*
 *	This program communicates from the M16C/62P to a terminal program	*
 * 	via UART0 and RS232.  UART0 configuration is:						*
 *	19200 baud, 8-bit data, 1 stop bit, no parity, no flow control. 	*
 *											   						 	*
 *											   						 	*
 *	An incrementing data (0 to 9) is sent to the Hyperterminal window.	*
 *	To stop receiving data, press z on the keyboard. To resume, press	*
 * 	any key.															*
 ************************************************************************/
/*=========================================================================
	When running this demo on the SKP16C62P Board must be connected to 
	the expansion board (i.e. COM Board) for RS-232C connectivity. The
	expansion board has the RS-232C driver/receiver and DB9 D-Sub connector.
    Use a straight through serial connector 
*=========================================================================*/
/*===========================================================================
*   Revision history: 
*
*   1.1 Removed mcu_init function from this file - it is now contained in 
        separate file "mcu_init.c"
*   1.0 SKP Release 
*==========================================================================*/


/* Include the required header files */
#include "skp_bsp.h"	// include SKP board support package
#include "uart.h"

/* Setup interrupt routine for UART0 receive. This must also be setup in the 
   vector table in SECT30_UART.INC */

#pragma INTERRUPT U0rec_ISR
void U0rec_ISR (void);

/*  Global variables */
char U0_in;		 		// declare UART0 recieve variable
char U0_char_available = 0;	// If 1, a char has been read

/* String constants used for screen output **********************************/
const char cmd_clr_scr[] = {27,'[','2','J',0};
const char cmd_cur_home[] = {27,'[','H',0};

/*****************************************************************************
Name:       Main    
Parameters:  none                   
Returns:     none   
Description: This is the main program    
*****************************************************************************/
#if 0
void main(void) {

	int count;			// declare count variable
	int convert;		// declare ASCII variable
	unsigned int delay;	// declare delay variable
	int i;				// declare string pointer variable

	mcu_init();			// MCU initialization
 	ENABLE_LEDS 		// LED initialization
	uart_init();		// UART initialization

/* Text to be displayed at the beginning of the program output
   to the terminal window (\r\n = carriage return & linefeed) */

	text_write(cmd_clr_scr);	 	// clear screen
	text_write(cmd_cur_home);	 	// home cursor
	text_write("Renesas Technology America, Inc. \r\n");	   
	text_write("SKP16C62P UART demo. \r\n");	   
	text_write("Press z to stop, any key to continue. \r\n");
	
/************** MAIN PROGRAM LOOP ***********************/
  while (1){
  	
// setup program loop to count from 0 to 9, stop when "z" is received

	while (U0_in != 'z'){		// count as long as "z" wasn't pressed
		text_write("\r\n");		// send carrige return and line feed
 		RED_LED = LED_OFF;			
 		GRN_LED = LED_ON;

		for (count=0;(count<=9)&&(U0_in!='z');count ++){	// count 0 to 9
 			convert = count + 0x30;	  				// convert count data to ASCII
			while(ti_u0c1 == 0);	  				// wait for previous transmission to complete 
	 		u0tb = convert;			  				// put ASCII data in transmit buffer
			for (delay=0x3fff; delay>0; delay--); 	// Count Delay
		}
	}
	_asm("NOP");			// Do nothing while stopped
  }							//  (after "z" is pressed)
}
#endif

/*****************************************************************************
Name:    UART0 Receive Interrupt Routine       
Parameters:  none                   
Returns:     none   
Description: Interrupt routine for UART0 receive
			 Reads character received from keyboard and stores U0_in variable	   
*****************************************************************************/
void U0rec_ISR(void){
	while(ri_u0c1 == 0);	// make sure receive is complete
	U0_in = (char) u0rb;	// read in received data
	U0_char_available = 1;	// flag data ready
}							

/*****************************************************************************
Name:		uart_init   
Parameters:	None				 
Returns:	None
Description: Uart0 initialization - 19200 baud, 8 data bits, 1 stop bit, no parity.
*****************************************************************************/
void uart_init(int baud_rate) {

	u0brg = (unsigned char)(((f1_CLK_SPEED/16)/baud_rate)-1);	// set UART0 bit rate generator
		 /*
	  	  bit rate can be calculated by:
	  	  bit rate = ((BRG count source / 16)/baud rate) - 1

		  in this example: BRG count source = f1 (12MHz - Xin in SKP16C62P)
						   baud rate = 19200
						   bit rate = ((12MHz/16)/19200) - 1 = 38 

	  	  ** one has to remember that the value of BCLK does not affect BRG count source */

  	ucon = 0x00; 		// UART transmit/receive control register 2
		 /*
		  00000000; 	// transmit irq not used
		  ||||||||______UART0 transmit irq cause select bit, U0IRS
	   	  |||||||_______UART1 transmit irq cause select bit, U1IRS
	   	  ||||||________UART0 continuous receive mode enable bit, U0RRM - set to 0 in UART mode
	  	  |||||_________UART1 continuous receive mode enable bit, U1RRM	- set to 0 in UART mode
	   	  ||||__________CLK/CLKS select bit 0, CLKMD0 - set to 0 in UART mode
	   	  |||___________CLK/CLKS select bit 1, CLKMD1 - set to 0 in UART mode
	   	  ||____________Separate CTS/RTS bit, RCSP
	  	  |_____________Reserved, set to 0 */

  	u0c0 = 0x10; 		// UART0 transmit/receive control register 1
		 /*
		  00010000;		// f1 count source, CTS/RTS disabled, CMOS output  
		  ||||||||______BRG count source select bit, CLK0
	   	  |||||||_______BRG count source select bit, CLK1
	   	  ||||||________CTS/RTS function select bit, CRS
	  	  |||||_________Transmit register empty flag, TXEPT
	   	  ||||__________CTS/RTS disable bit, CRD
	   	  |||___________Data output select bit, NCH
	   	  ||____________CLK polarity select bit, CKPOL 		- set to 0 in UART mode
	  	  |_____________Transfer format select bit, UFORM 	- set to 0 in UART mode */

  	u0c1 = 0x00; 		// UART0 transmit/receive control register 1
		 /*
		  00000000;		// disable transmit and receive 
		  ||||||||______Transmit enable bit, TE
	   	  |||||||_______Transmit buffer empty flag, TI
	   	  ||||||________Receive enable bit, RE
	  	  |||||_________Receive complete flag, RI
	   	  ||||__________Reserved, set to 0
	   	  |||___________Reserved, set to 0
	   	  ||____________Data logic select bit, U0LCH
	  	  |_____________Error signal output enable bit, U0ERE */

  	u0mr = 0x05;		// UART0 transmit/receive mode register, not reversed
		 /*
		  00000101;		// 8-bit data, internal clock, 1 stop bit, no parity
		  ||||||||______Serial I/O Mode select bit, SMD0
	   	  |||||||_______Serial I/O Mode select bit, SMD1
	   	  ||||||________Serial I/O Mode select bit, SMD2
	  	  |||||_________Internal/External clock select bit, CKDIR
	   	  ||||__________Stop bit length select bit, STPS
	   	  |||___________Odd/even parity select bit, PRY
	   	  ||____________Parity enable bit, PRYE
	  	  |_____________TxD, RxD I/O polarity reverse bit */

  	u0tb = u0rb;		// clear UART0 receive buffer by reading
  	u0tb = 0;			// clear UART0 transmit buffer

    DISABLE_IRQ			// disable irqs before setting irq registers
	s0ric = 0x04;		// Enable UART0 receive interrupt, priority level 4	
	ENABLE_IRQ			// Enable all interrupts

  	u0c1 = 0x05; 		// UART0 transmit/receive control register 1
		 /*
		  00000101;		// enable transmit and receive 
		  ||||||||______Transmit enable bit, TE
	   	  |||||||_______Transmit buffer empty flag, TI
	   	  ||||||________Receive enable bit, RE
	  	  |||||_________Receive complete flag, RI
	   	  ||||__________Reserved, set to 0
	   	  |||___________Reserved, set to 0
	   	  ||____________Data logic select bit, U0LCH
	  	  |_____________Error signal output enable bit, U0ERE */
}

/*****************************************************************************
Name:   	 text_write        
Parameters:  msg_string -> the text string to output                    
Returns:     none   
Description: The following sends a text string to the terminal program     
*****************************************************************************/
void uart_text_write ( _far char * msg_string)
{
	char i;

	for (i=0; msg_string[i]; i++){		// This loop reads in the text string and 
		while(ti_u0c1 == 0); 			//  puts it in the UART 0 transmit buffer 
		u0tb = msg_string[i];
	}
}

/*
 * Return a char if one is available, else return -1
 * Does no buffering
 */

char uart_get_char() 
{
	if (U0_char_available)
	{
		U0_char_available = 0;
		return U0_in;
	}
	else
		return NO_DATA;
}