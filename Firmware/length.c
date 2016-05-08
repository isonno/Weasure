//
// Length.c - code for reading the photocell sensors to report the length
//
// Copyright 2005 John W. Peterson
// Renesas M16C Design Contest 2005
//

#include "skp_bsp.h"
#include "length.h"

// These define the port bits connected to the various light sensors
unsigned char a1_table[24];
unsigned char a2_table[20];
// Axis 3 has the "leftover" port bits, so it's defined explictly
unsigned char a3_table[18] = { 0x56, 0x57, 0x84, 0x70, 0x71, 0x73, 0x75, 0x76, 0x77,
									0x95, 0x96, 0x97, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7 };

//
// For each axis, there is a table enumerating the port bits corresponding to
// the photosensors.  This walks the table (from furthest to closest) and stops
// when it finds a bit that is dark (logic 1)
//
int measure_axis( unsigned int table_length, unsigned char * port_table  )
{
	int length = table_length-1;
	unsigned char pdat;
	unsigned char sensor_port;
	while ((length >= 0)) {
		sensor_port = port_table[length];
		// High nybble has port number, low nybble has port bit.  If the
		// photocell is lit, the light lowers the resistance and pulls 
		// the port to ground.
		switch (sensor_port >> 4)		
		{
		case 0: pdat = p0; break;
		case 1: pdat = p1; break;
		case 2: pdat = p2; break;
		case 3: pdat = p3; break;
		case 4: pdat = p4; break;
		case 5: pdat = p5; break;
		case 6: pdat = p6; break;
		case 7: pdat = p7; break;
		case 8: pdat = p8; break;
		case 9: pdat = p9; break;
		case 10:pdat = p10; break;
		}
		
		// Found a dark sensor, so we've hit the edge of the box
		if ((pdat & (1 << (sensor_port & 7))) != 0)
			break;

		length--;

	}
	return length + 1;
}

//
// Measure the length of the specified axis

int measure_length( int axis_id )
{
	int len, offset;

	switch (axis_id)
	{
	case HEIGHT: len = measure_axis( (unsigned int) sizeof( a1_table ), a1_table ); 
				 offset = 0;
				 break;
	case WIDTH:  len = measure_axis( (unsigned int) sizeof( a2_table ), a2_table ); 
				 offset = 2;
				 break;
	case DEPTH:	 len = measure_axis( (unsigned int) sizeof( a3_table ), a3_table );
				 offset = 1;
				 break;
	}

	// If the length is non-zero, then add in the starting offset
	if (len > 0)
		len += offset;

	return len;
}

//
// Initialize the ports and the port tables used for reading the photo sensors
//
void length_init(void)
{
	int i;
	unsigned char port;

	// Enable sense line inputs */
	pd0 = 0;			// All of port 0 
	pd1 = 0;			// All of port 1
	pd2 = 0;			// All of port 2
	pd3 = 0;			// All of port 3
	pd4 = 0;			// All of port 4
	pd5_1 = 0;			// Most of port 5 (p5_0 is used for the ICD)
	pd5_2 = 0;
	pd5_3 = 0;
	pd5_4 = 0;
	pd5_6 = 0;
	pd5_7 = 0;
						// Port 6 enables the LCD display, debug port, and serial I/O
	pd7_0 = 0;			// Port 7 has LEDs (7_2 & 7_4)
	pd7_1 = 0;
	pd7_3 = 0;
	pd7_5 = 0;
	pd7_6 = 0;
	pd7_7 = 0;
	pd8_4 = 0;			// Port 8 has LEDs, switches, the RTC clock (8_4 is free)
	pd9_5 = 0;			// Port 9_0..3 is the LCD, 9_4 is the scale PWM timer B input
	pd9_6 = 0;
	pd9_7 = 0;
	pd10_2 = 0;			// Port 10_0,1 are analog input
	pd10_3 = 0;
	pd10_4 = 0;
	pd10_5 = 0;
	pd10_6 = 0;
	pd10_7 = 0;

	// Initialize the first two axis tables (the third is defined explictly above)
	for (i = 0; i < sizeof(a1_table); ++i)
	{
		port = ((i/8) << 4) | (i & 7);
		a1_table[i] = port;
		// a2 is shorter, and port bit 5_0 is skipped
		if (i < sizeof(a2_table))
			a2_table[i] = (((i/8)+3) << 4) | (i & 7) + ((i > 15) ? 1 : 0);
	}
}

