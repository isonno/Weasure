//
// Length.h - Definition for length measurement code
//
// Copyright 2005 John W Peterson
// Renesas M16C Design Contest 2005
//

#ifndef LENGTH_H
#define LENGTH_H

/* Function Prototypes */
int measure_length( int axis_id );
void length_init(void);

// Axis ID definitions
#define HEIGHT	0
#define WIDTH	1
#define DEPTH	2

#endif
