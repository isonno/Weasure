//
// Scale.c
// Implementation for the Weasure scale firmware
//
// Copyright 2005 John W Peterson
// Renesas M16C Design Contest 2005
//
// This file is originally based on the SKP_DEMO supplied by the HEW
// workshop by Renesas.
//

/* Include the required header files */
#include "uart.h"		// UART Serial I/O support
#include "length.h"		// Length measurement routines
#include "skp_bsp.h"	// include SKP board support package

// Timer register control bits for Pulse Width Measurement

#define WIDTH_MEASURE 0x8A	/* 10001010 value to load into timer B1 mode register
							   ||||||||_ TMOD0,TMOD1: PULSE MEASUREMENT MODE
							   ||||||____ MR0,MR1: PULSE PERIOD MODE
							   ||||_______MR2: = 0 FOR PULSE MEASUREMENT
							   |||_______ MR3: OVERFLOW FLAG
							   ||________ TCK0,TCK1: F DIVIDED BY 00:f1,01:f8,10:f32 SELECTED */

//
// These constants convert the PWM timer data to weight in pounds and ounces
//
#define MAX_OVERFLOWS 4		// If too many overflows, probably no signal from scale

#define COUNTS_PER_LBS	174	// Actually, it's about 174.4
#define COUNTS_PER_OZ	11	// Is really COUNTS_PER_LBS/16, 174.4/16 = 10.9 =~= 11

// Format string for displaying the weight
_far char * weight_format = "  p,  oz";	// Spaces are filled in w/numbers

// Function prototypes
void mcu_init(void);
void ta0_irq(void);
char * IntToAsciiHex(char * dest_string,int min_digits,unsigned int value);
char * IntToAsciiDec(char * dest_string,int min_digits,unsigned int value);

// global variables
char update_lcd;				// flag to update LCD when the weight has changed.

// Structure for maintaining the state of the pulse width measure ISR
struct {
	unsigned int num_samples;
	unsigned int sum;
	int average;
	unsigned int overflows;
} pw_sample;

// used in converting AD value to characters for LCD
const char num_to_char[11] = {'0','1','2','3','4','5','6','7','8','9',' '};
const unsigned int base_ten[5] = {10000,1000,100,10,1};

// This value will be incemented every 100 ms when Timer A0 runs
unsigned int ticks_100ms = 0;

// Delay by specified number of ticks (100ms each)
static void Delay( unsigned int ticks )
{
	unsigned int i;
	DISABLE_IRQ
	ta0ic = 2;					// Enable Timer A0 interrupts
	ENABLE_IRQ
	ta0s = 1;					// Start timer A0

	i = ticks_100ms + ticks;	// get current system time and add in 2 second 
								// delay (100ms x 20)
	while( i < ticks_100ms );	// if counter roll over, wait till back to 0
	while( ticks_100ms < i);	// now wait till timer rolls past our marker

	DISABLE_IRQ
	ta0ic = 0;					// Now that delays are done, shut off this timer
	ENABLE_IRQ
}

//
// Display at least min_digits of number starting at position in the LCD display
//
static void DisplayDigits( unsigned char position, int min_digits, int number )
{
	int i;
	char lcd_text[9];			// used to create text strings to send to the LCD
	if (number < 0)				// If negative, just draw "---"
	{
		for (i = 0; i < min_digits; ++i)
			lcd_text[i] = '-';
		lcd_text[i] = '\0';
	}
	else
		IntToAsciiDec( lcd_text, min_digits, number );

	// Blank leading zeros
	for (i = 0; (i < min_digits-1) && (lcd_text[i] == '0'); ++i)
		lcd_text[i] = ' ';

	DisplayString( position, lcd_text);
}

//
// Main
//
// This initializes the hardware, then loops continously, updating the
// display and responding to requests on the serial port.
//
void main(void) {

	unsigned int i;
	char ad_result;				// calculated ADC char for LCD display 
	char msg_text[15];			// used to create text strings to send to the UART
	unsigned char p0byte, lastp0, t;
	int zero_offset = 0;
	unsigned char was_off = 0;
	char cmd = NO_DATA;
	int count, lbs, oz, height, width, depth;

	// Initialize Timer B4 ISR state
	pw_sample.num_samples = 0;
	pw_sample.sum = 0;
	pw_sample.average = 0;
	pw_sample.overflows = 0;

	/* Initializations */
	mcu_init();					// Initialize MCU clock
	uart_init( BAUD_RATE );		// Init serial I/O
	length_init();				// Initialize ports for reading length data

    /* Start the 32Khz crystal sub clock */
   	prc0 = 1;  		// Unlock CM0 and CM1
   	pd8_7 = 0;		// setting GPIO to inputs (XCin/XCout)
   	pd8_6 = 0;
   	cm04 = 1; 		// Start the 32KHz crystal

 	ENABLE_SWITCHES 	/* Switch initialization - macro defined in skp_bsp.h */
 	ENABLE_LEDS 		/* LED initialization - macro defined in skp_bsp.h */

	pd6_2 = 0;			// Enable as input port (serial I/O)

	/* Timer A0 initialization */
	/* Configure Timer A0 - 100ms (millisecond) counter */
	ta0mr = 0xc0;	// Timer mode, fc32, no pulse output
	ta0 = (int)(( 1023*.1)-1);
   

	/* Change interrupt priority levels to enable maskable interrupts */
	DISABLE_IRQ		// disable irqs before setting irq registers - macro defined in skp_bsp.h
	tb4ic |= 0x03;	// Set timer B4 interrupt priority to level 3
	tb4mr = WIDTH_MEASURE;	// Set up Timer B4 to measure the pulse period
	ENABLE_IRQ		// enable interrupts macro defined in skp_bsp.h

	/* Start timers */
	tb4s = 1;		/* start counting */

	update_lcd = 1;				// display first weight value

	InitDisplay();				// clear LCD

	/* Display Renesas Splash Screen for 1.5 seconds */
	DisplayString(LCD_LINE1,RENESAS_LOGO);	// Display Renesas bitmapped logo on Line 1
	DisplayString(LCD_LINE2,"Weasure!");	
	Delay( 15 );

	/* Display SKP Splash Screen for 1.5 seconds */
	DisplayString(LCD_LINE1, "Weasure!");
	DisplayString(LCD_LINE2, "  v1.0  ");
	Delay(15);

	/* Display lables for measurements */
	DisplayString(LCD_LINE1, weight_format);
	DisplayString(LCD_LINE2, "  x  x  ");
	lastp0 = 0;

	while(1)					   		// infinite loop
	{
		p0byte = p0;

		if (update_lcd || (lastp0 != p0byte))		// p0 changed?
		{
			// Check PWM measurements to see if we have a 
			if (pw_sample.overflows >= MAX_OVERFLOWS)
			{
				DisplayString( LCD_LINE1, "  Off   ");
				was_off = 1;
			}
			else
			{
				// Establish the zero value
				if ((!S1) || (zero_offset == 0))
					zero_offset = pw_sample.average;

				// If scale was turned off, "Off" was displayed.
				// Need to re-display the weight format
				if (was_off)
				{
					DisplayString(LCD_LINE1, weight_format);
					was_off = 0;
				}
				count = pw_sample.average - zero_offset;
				lbs = count / COUNTS_PER_LBS;
				oz  = (count - (lbs*COUNTS_PER_LBS)) / COUNTS_PER_OZ;

				DisplayDigits( (char)(LCD_LINE1 + 0), 2, lbs );
				DisplayDigits( (char)(LCD_LINE1 + 4), 2, oz  );
			}

//			DisplayDigits( (char)(LCD_LINE2 + 3), 5, count );
			height = measure_length( HEIGHT );
			width = measure_length( WIDTH );
			depth = measure_length( DEPTH );
			DisplayDigits( LCD_LINE2, 2, height );
			DisplayDigits( (char)(LCD_LINE2 + 3), 2, width );
			DisplayDigits( (char)(LCD_LINE2 + 6), 2, depth );

			// If we get a request from the host, write the data out the serial port
			cmd = uart_get_char();
			switch (cmd)
			{
			case 's':
				{
					IntToAsciiDec( msg_text, 2, height );
					msg_text[2] = ',';
					IntToAsciiDec( msg_text+3, 2, width );
					msg_text[5] = ',';
					IntToAsciiDec( msg_text+6, 2, depth );
					msg_text[8] = '\r';
					msg_text[9] = '\n';
					msg_text[10] = 0;
					uart_text_write( msg_text );
					break;
				}
			case 'w':
			case 'c':
				{
					if (was_off)
						uart_text_write( "OFF\n\r" );
					else
					{
						IntToAsciiDec( msg_text, 2, lbs );
						msg_text[2] = ',';
						IntToAsciiDec( msg_text+3, 2, oz );
						i = 5;
						if (cmd == 'c')
						{
							msg_text[i++] = ',';
							IntToAsciiDec( msg_text+i, 5, count );
							i += 5;
						}
						msg_text[i++] = '\r';
						msg_text[i++] = '\n';
						msg_text[i++] = 0;
						uart_text_write( msg_text );
					}
					break;
				}
			case NO_DATA:			// Nothing to read
			default:	break;
			}

			// Clear our update Flag
			update_lcd = 0;
			lastp0 = p0byte;

			/* Debug: Light up LEDs depending on which bits are set */
			RED_LED = ~(p0byte & 1);
			YLW_LED = ~((p0byte >> 1) & 1);
			GRN_LED = ~((p0byte >> 2) & 1); /**/
		}
   	}
}

//
// This is the timer A0 interrupt routine. It occurs every 100 ms. 
//

#pragma INTERRUPT 		ta0_irq
void ta0_irq(void)
{
   	ticks_100ms++;
	update_lcd = 1;				// LCD needs to be updated
}

//
// This is the timer B4 interrupt routine.  It is invoked every time the
// signal TB4in (port p9_4) changes state.  On the low-high transition, we
// record the pulse width count
//
#pragma INTERRUPT /B	tb4_irq
void tb4_irq(void)
{
	unsigned int tb4_data;
	if (mr3_tb4mr == 1)			// Check for timer overflow
	{
		if (pw_sample.overflows < MAX_OVERFLOWS)
			pw_sample.overflows++;
		else
			update_lcd = 1;
		tb4mr = WIDTH_MEASURE;	// If so, clear flag, and
		return;					// data invalid, so leave.
	}
	if (p9_4 == 1)				// tb4in now hi, so just measured a low width
	{
		// In order to stablize the count, we average the last four counts.
		// When the average is complete (the fourth count) we record the final
		// averaged sample, reset it, then set a flag indicating the display
		// needs updating.
		tb4_data = tb4;
		tb4_data = tb4_data >> 2;
		pw_sample.sum += tb4_data;
		pw_sample.num_samples++;
		if (pw_sample.num_samples == 4)
		{
			pw_sample.average = (int) (pw_sample.sum >> 3);	// Throw away an extra bit too
			pw_sample.sum = 0;
			pw_sample.num_samples = 0;
			update_lcd = 1;
		}
		pw_sample.overflows = 0;
	}
}		

/*****************************************************************************
Name:	IntToAsciiHex   
Parameters:	
		dest_string
			Pointer to a buffer will the string will reside
		min_digits
			Specifies the minimum number of characters the output string will
			have. Leading zeros will be written as '0' characters.
Returns:
		A pointer to the string's NULL character in the string that was just
		created.
Description: 
		This function is used to convert a passed unsigned int into a ASCII
		string represented in Hexidecimal format.
*****************************************************************************/
char * IntToAsciiHex(char * dest_string,int min_digits,unsigned int value)
{
	unsigned int i, total_digits = 0;
	char buff[4];
	
	for(i=0;i<4;i++)
	{
		buff[i] = (char)(value & 0x0F);
		value = value >> 4;
		if( buff[i] <= 9)
			buff[i] += '0';
		else
			buff[i] = (char)(buff[i] - 0xA + 'A');

		if(buff[i] != '0')
			total_digits = i+1;
	}

	if( total_digits < 	min_digits)
		total_digits = min_digits;

	i = total_digits;
	while(i)
	{
		*dest_string++ = buff[i-1];
		i--;
	}

	*dest_string = 0;

	return dest_string;
}
/*****************************************************************************
Name:	IntToAsciiDec   
Parameters:	
		dest_string
			Pointer to a buffer will the string will reside
		min_digits
			Specifies the minimum number of characters the output string will
			have. Leading zeros will be written as '0' characters.
Returns:
		A pointer to the string's NULL character in the string that was just
		created.
Description: 
		This function is used to convert a passed unsigned int into a ASCII
		string represented in base 10 decimal format.
*****************************************************************************/
char * IntToAsciiDec(char * dest_string,int min_digits,unsigned int value)
{
	const unsigned long base10[] = {1,10,100,1000,10000,100000};

	unsigned int tmp;
	unsigned int i, total_digits = 0;
	char buff[5];
	
	for(i=0;i<5;i++)
	{
		tmp = (int)( value % base10[i+1] );
		value -= tmp;

		buff[i] = (char)( tmp / base10[i] );
		buff[i] += '0';

		if(buff[i] != '0')
			total_digits = i+1;
	}

	if( total_digits < 	min_digits)
		total_digits = min_digits;

	i = total_digits;
	while(i)
	{
		*dest_string++ = buff[i-1];
		i--;
	}

	*dest_string = 0;

	return dest_string;
}

