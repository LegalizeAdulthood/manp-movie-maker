#define	RGB_RED			0
#define	RGB_GREEN		1
#define	RGB_BLUE		2
#define	RGB_SIZE		3

/* Macro to determine to round off the given value to the closest byte */
#define WIDTHBYTES(i)   ((i+31)/32*4)
#define PIXELS2BYTES(n)	((n+7)/8)
#define	GREYVALUE(r,g,b) ((((r*30)/100) + ((g*59)/100) + ((b*11)/100)))
#define	ORTHOMATCH(r,g,b)  (((r) & 0x00e0) | (((g) >> 3) & 0x001c)  | (((b) >> 6) & 0x0003))

#define	LPBimage(lpbi)	((BYTE *)lpbi+lpbi->biSize+(long)(lpbi->biClrUsed*sizeof(RGBQUAD)))
#define	LPBwidth(lpbi)	(lpbi->biWidth)
#define	LPBdepth(lpbi)	(lpbi->biHeight)
#define	LPBbits(lpbi)   (lpbi->biBitCount)
#define	LPBcolours(lpbi) (lpbi->biClrUsed)
#define	LPBlinewidth(lpbi) (WIDTHBYTES((WORD)lpbi->biWidth*lpbi->biBitCount))

#define	LPBcolourmap(lpbi) (LPRGBQUAD)((LPSTR)lpbi+lpbi->biSize)

// IB 2009-05-28 these macros duplicate above (with a suitable cast)
// Macro to return pointer to DIB pixels
#define DIBPEL(pDib) LPBimage(((LPBITMAPINFOHEADER)pDib))
// Macro to return pointer to DIB palette
#define DIBPAL(pDib) LPBcolourmap(((LPBITMAPINFOHEADER)pDib))

// Macro to determine the number of bytes per line in the DIB bits. This
//  accounts for DWORD alignment by adding 31 bits, then dividing by 32,
//  then rounding up to the next highest count of 4-bytes. Then, we 
//  multiply by 4 to get the total byte count.
//#define BYTESPERLINE(Width, BPP) ((WORD)((((DWORD)(Width) * (DWORD)(BPP) + 31) >> 5)) << 2)

#define NEW_DIB_FORMAT(lpbih) (lpbih->biSize != sizeof(BITMAPCOREHEADER))

/*
#define WEIGHTING(colour) ((long)(p00.colour) * (100L - tempx) + \
			   (long)(p01.colour) * tempx) * (100L - tempy) + \
			  ((long)(p10.colour) * (100L - tempx) + \
			   (long)(p11.colour) * tempx) * tempy

typedef double	MATRIX[4][4];		// matrix of floats
typedef double	VECTOR[3];		// vector of floats
typedef double	MATRIXPTR[][4];		// fussy array warnings from HI_TECH
typedef double	VECTORPTR[];		// ditto
*/

