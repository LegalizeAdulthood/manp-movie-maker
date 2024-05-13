// crop.h: interface for cropping code.
//
// These include defines to determine the meaning of the fFlags variable.  
// The low byte is used for the various types of "boxes" to draw.  The high byte is
// available for special commands.
//
//////////////////////////////////////////////////////////////////////

#define SL_BOX		1	// Draw a solid border around the rectangle
#define SL_BLOCK	2	// Draw a solid rectangle                    

#define CROP_OFF	-1	// no crop
#define CROP_CANCEL	0	// cancel crop
#define CROP_OK		1	// accept crop
#define CROP_MOVE	2	// move crop area
#define CROP_MOVERESIZE	3	// resize crop area
#define CROP_REDEYE	4	// redeye crop area

#define CROP_INACTIVE	0	// crop not active
#define CROP_ACTIVE	1	// normal crop
#define CROP_MAX	2	// max crop area

#define TOP_LEFT	0       // segment types
#define TOP_RIGHT	1
#define BOTTOM_LEFT	2
#define BOTTOM_RIGHT	3
#define LEFT		4
#define TOP		5
#define BOTTOM		6
#define RIGHT		7
#define CENTRE		8

#define SL_EXTEND	256	// Extend the current pattern

#define SL_TYPE		0x00FF	// Mask out everything but the type flags
#define SL_SPECIAL	0xFF00	// Mask out everything but the special flags

// macro that forces the aspect ratio defined by xmin and ymin
#define FORCE_ASPECT(a, b, s)			\
    if (xmin > 0 && ymin > 0)			\
	{					\
	s = ((a > 0) == (b > 0)) ? 1 : -1;	\
	a /= xmin;				\
	b /= ymin;				\
	if (abs(a) <= abs(b))			\
	    {					\
	    b = a * ymin * s;			\
	    a *= xmin;				\
	    }					\
	else					\
	    {					\
	    a = b * xmin * s;			\
	    b *= ymin;				\
	    }					\
	}

