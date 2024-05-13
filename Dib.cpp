// Dib.cpp: implementation of the CDib class.
//
//////////////////////////////////////////////////////////////////////

#ifndef PGV4
#define UNICODE
#define _UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include "Dib.h"
#include "DibMacro.h"

#define BITMAPCOMPRESSION(lpbi)     (*(LPDWORD)lpbi >= sizeof(BITMAPINFOHEADER) ? lpbi->biCompression : BI_RGB)
#define BITMAPREDMASK(lpbi)         ( (*(LPDWORD)lpbi >= sizeof(BITMAPV4HEADER)) || (BITMAPCOMPRESSION(lpbi) == BI_BITFIELDS) ?         \
                                    ((PBITMAPV4HEADER)lpbi)->bV4RedMask : 0)
#define BITMAPGREENMASK(lpbi)       ( (*(LPDWORD)lpbi >= sizeof(BITMAPV4HEADER)) || (BITMAPCOMPRESSION(lpbi) == BI_BITFIELDS) ?         \
                                    ((PBITMAPV4HEADER)lpbi)->bV4GreenMask : 0)
#define BITMAPBLUEMASK(lpbi)        ( (*(LPDWORD)lpbi >= sizeof(BITMAPV4HEADER)) || (BITMAPCOMPRESSION(lpbi) == BI_BITFIELDS) ?         \
                                    ((PBITMAPV4HEADER)lpbi)->bV4BlueMask : 0)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDib::CDib()			// device independent bitmap
{
    DibPixels = NULL;
    pDibInf = NULL;
    DibErrorCode = 0;
    DibHeight = BitsPerPixel = 0;
    WidthBytes = 0L;
    DibWidth = 0L;
    AlphaChannel = NOALPHA;
}

CDib::CDib(const CDib & Dib1)	// Copy Constructor
    {
    SizeImage = Dib1.SizeImage;
    DibHeight = Dib1.DibHeight;
    DibWidth = Dib1.DibWidth;
    DibSize = Dib1.DibSize;
    AlphaChannel = Dib1.AlphaChannel;
    BitsPerPixel = Dib1.BitsPerPixel;
    WidthBytes = Dib1.WidthBytes;
    DibErrorCode = 0;
    pDibInf = (LPBITMAPINFO)new BYTE[DibSize];
    if(pDibInf == NULL)
	throw "Can't allocate DIB memory.";	// IB 2009-05-18 C++ error handling
    memcpy(pDibInf, Dib1.pDibInf, DibSize);
    DibPixels = DIBPEL(pDibInf);
    }

CDib::~CDib()
    {
    CloseDibPtrs();
    }

CDib& CDib::operator=(const CDib & Dib1)	// Assignment Operator
    {
    if(this == & Dib1)	    return * this;	// Attempt to assign to self
    CloseDibPtrs();				// release memory for target DIB
    SizeImage = Dib1.SizeImage;
    DibHeight = Dib1.DibHeight;
    DibWidth = Dib1.DibWidth;
    DibSize = Dib1.DibSize;
    AlphaChannel = Dib1.AlphaChannel;
    BitsPerPixel = Dib1.BitsPerPixel;
    WidthBytes = Dib1.WidthBytes;
    DibErrorCode = 0;
    pDibInf = (LPBITMAPINFO)new BYTE[DibSize];
    if(pDibInf == NULL)
	throw "Can't allocate DIB memory.";	// IB 2009-05-18 C++ error handling
    memcpy(pDibInf, Dib1.pDibInf, DibSize);
    DibPixels = DIBPEL(pDibInf);

    return * this;
    }

// Convert DIB into 24 bit - for unsuported sizes, just copy pixels into CDib
void	CDib::DibTo24(LPBITMAPINFO pDib, BYTE * SourcePixels)
    {
    int 	width = pDib->bmiHeader.biWidth;
    int 	height = pDib->bmiHeader.biHeight;
    WORD	bits = pDib->bmiHeader.biBitCount;

    RGBQUAD *	Palette = DIBPAL(pDib);
    BYTE *	Source;
    BYTE	SourcePel, SourcePix;
    RGBTRIPLE *	Dest;

    DWORD	SourceWidthBytes = WIDTHBYTES((DWORD)width * bits);
    DWORD	DestWidthBytes   = WIDTHBYTES((DWORD)width * 24L);
    int 	i, j, k, wbytes, wpart;

    InitDib(width, height, 24);		// Create an empty 24 bit DIB

    switch(bits)
	{
	case 1:
	    wbytes = width / 8;		// whole bytes in source line
	    wpart = width % 8;		// leftover bits in source line
	    for(i = 0; i < height; ++i)
	    	{
	    	Dest = (RGBTRIPLE *)((BYTE *)DibPixels + i * DestWidthBytes);
	    	Source = SourcePixels + i * SourceWidthBytes;
	    	for(j = 0; j < wbytes; ++j)
		    {
		    SourcePix = *Source;
		    for(k = 0; k < 8; ++k)
		    	{
		    	SourcePel = SourcePix & 0x80 ? 1 : 0;
		    	Dest->rgbtBlue  = Palette[SourcePel].rgbBlue;
		    	Dest->rgbtGreen = Palette[SourcePel].rgbGreen;
		    	Dest->rgbtRed   = Palette[SourcePel].rgbRed;
		    	SourcePix <<= 1;
		    	Dest++;
		    	}
		    Source++;
		    }
	    	SourcePix = *Source;
	    	for(k = 0; k < wpart; ++k)
		    {
		    SourcePel = SourcePix & 0x80 ? 1 : 0;
		    Dest->rgbtBlue  = Palette[SourcePel].rgbBlue;
		    Dest->rgbtGreen = Palette[SourcePel].rgbGreen;
		    Dest->rgbtRed   = Palette[SourcePel].rgbRed;
		    SourcePix <<= 1;
		    Dest++;
		    }
	    	}
	    break;
	case 4:
	    wbytes = width / 2;		// whole bytes in source line
	    wpart = width % 2;		// leftover nibbles in source line
	    for(i = 0; i < height; ++i)
	    	{
	    	Dest = (RGBTRIPLE *)((BYTE *)DibPixels + i * DestWidthBytes);
	    	Source = SourcePixels + i * SourceWidthBytes;

	    	for(j = 0; j < wbytes; ++j)
		    {
		    SourcePel = (*Source & 0xF0) >> 4;
		    for(k = 0; k < 2; ++k)
		    	{
			Dest->rgbtBlue  = Palette[SourcePel].rgbBlue;
			Dest->rgbtGreen = Palette[SourcePel].rgbGreen;
			Dest->rgbtRed   = Palette[SourcePel].rgbRed;
			Dest++;
			SourcePel = *Source & 0x0F;
			}
		    Source++;
		    }
		SourcePel = (*Source & 0xF0) >> 4;
		for(k = 0; k < wpart; ++k)
		    {
		    Dest->rgbtBlue  = Palette[SourcePel].rgbBlue;
		    Dest->rgbtGreen = Palette[SourcePel].rgbGreen;
		    Dest->rgbtRed   = Palette[SourcePel].rgbRed;
		    Dest++;
		    SourcePel = *Source & 0x0F;
		    }
	    	}
	    break;
	case 8:
	    for(i = 0; i < height; ++i)
	    	{
	    	Dest = (RGBTRIPLE *)((BYTE *)DibPixels + i * DestWidthBytes);
	    	Source = SourcePixels + i * SourceWidthBytes;
	    	for(j = 0; j < width; ++j)
		    {
		    SourcePel = *Source;
		    Dest->rgbtBlue  = Palette[SourcePel].rgbBlue;
		    Dest->rgbtGreen = Palette[SourcePel].rgbGreen;
		    Dest->rgbtRed   = Palette[SourcePel].rgbRed;
		    Dest++;
		    Source++;
		    }
	    	}
	    break;
	case 16:
	    if(AlphaChannel == GREYA)	// Greyscale + Alpha (non-standard DIB)
		{
		BYTE	Alpha;
		for(i = 0; i < height; ++i)
		    {
		    Dest = (RGBTRIPLE *)((BYTE *)DibPixels + i * DestWidthBytes);
		    Source = SourcePixels+ i * SourceWidthBytes;
		    for(j = 0; j < width; ++j)
			{
			SourcePel = *Source;
			Alpha  = *(++Source);
			Dest->rgbtBlue  = Dest->rgbtGreen = Dest->rgbtRed   = (SourcePel * Alpha) / 256;
			Dest++;
			Source++;
			}
		    }
		break;
		}
	    {
	    WORD	SourcePel16;
	    WORD *	Source16;
	    DWORD	RedMask   =  0x7C00;
	    DWORD	GreenMask =  0x03E0;
	    DWORD	BlueMask  =  0x001F;
	    if (pDib->bmiHeader.biCompression == BI_BITFIELDS)
		{
		// If this is a 16 or 32 bit bitmap and BI_BITFIELDS is used,
		// bmiColors member contains three DWORD color masks.
		RedMask   =  BITMAPREDMASK  (((LPBITMAPINFOHEADER)pDib));
		GreenMask =  BITMAPGREENMASK(((LPBITMAPINFOHEADER)pDib));
		BlueMask  =  BITMAPBLUEMASK (((LPBITMAPINFOHEADER)pDib));
		}
	    // Calculate the magnitude of the shift from the respective Mask (Blue is assumed to be LSD)
	    int 	bs =  0, rs =  0, gs = 0;
	    DWORD	Mask;

	    Mask = BlueMask;
	    while((Mask&0x80) == 0)     { Mask <<= 1; bs++; }
	    Mask = RedMask;
	    while((Mask&1) == 0)	{ Mask >>= 1; rs++; }
	    while((Mask&0x80) == 0)     { Mask <<= 1; rs--; }
	    Mask = GreenMask;
	    while((Mask&1) == 0)	{ Mask >>= 1; gs++; }
	    while((Mask&0x80) == 0)     { Mask <<= 1; gs--; }

     	    for(i = 0; i < height; ++i)
	    	{
	    	Dest = (RGBTRIPLE *)((BYTE *)DibPixels + i * DestWidthBytes);
	    	Source16 = (WORD *)(SourcePixels+ i * SourceWidthBytes);
	    	for(j = 0; j < width; ++j)
		    {
		    SourcePel16 = *Source16;
		    Dest->rgbtBlue  = (BYTE)((SourcePel16 & BlueMask)  << bs);
		    Dest->rgbtGreen = (BYTE)((SourcePel16 & GreenMask) >> gs);
		    Dest->rgbtRed   = (BYTE)((SourcePel16 & RedMask)   >> rs);
		    Dest++;
		    Source16++;
		    }
	    	}
	    }
	    break;
	case 24:
	    memcpy(DibPixels, SourcePixels, SizeImage);
	    break;
	case 32:
	    if(AlphaChannel == RGBA1)	// PHD 2020-09-03 changed from RGBA to resolve conflict with WebP library
		{
		RGBQUAD *	Source32;
		BYTE	Alpha;
		for(i = 0; i < height; ++i)
		    {
		    Dest = (RGBTRIPLE *)((BYTE *)DibPixels + i * DestWidthBytes);
		    Source32 = (RGBQUAD *)(SourcePixels + i * SourceWidthBytes);
		    for(j = 0; j < width; ++j)
			{
			Alpha  = Source32->rgbReserved;	// MicroSoft DIB do not support AlphaChannel, but the structure is used to support RGBA data
			Dest->rgbtBlue  = (Source32->rgbBlue * Alpha) / 256;
			Dest->rgbtGreen = (Source32->rgbGreen * Alpha) / 256;
			Dest->rgbtRed   = (Source32->rgbRed * Alpha) / 256;
			Dest++;
			Source32++;
			}
		    }
		break;
		}

	    else if(AlphaChannel == GREYA)	// Greyscale + Alpha (non-standard DIB)
		{
		WORD	Alpha, Grey;
		WORD *	SourceGA;
		for(i = 0; i < height; ++i)
		    {
		    Dest = (RGBTRIPLE *)((BYTE *)DibPixels + i * DestWidthBytes);
		    SourceGA = (WORD *)(SourcePixels + i * SourceWidthBytes);
		    for(j = 0; j < width; ++j)
			{
			Grey = *SourceGA;
			Alpha  = *(SourceGA++);
			Dest->rgbtBlue  = Dest->rgbtGreen = Dest->rgbtRed = BYTE((Grey * Alpha) / (256 * 256 * 256));
			Dest++;
			SourceGA++;
			}
		    }
		break;
		}
	    {
	    DWORD	SourcePel32;
	    DWORD *	Source32;
	    DWORD	RedMask   =  0xFF0000;
	    DWORD	GreenMask =  0x00FF00;
	    DWORD	BlueMask  =  0x0000FF;
	    if (pDib->bmiHeader.biCompression == BI_BITFIELDS)
		{
		// If this is a 16 or 32 bit bitmap and BI_BITFIELDS is used,
		// bmiColors member contains three DWORD color masks.
		RedMask   =  BITMAPREDMASK  (((LPBITMAPINFOHEADER)pDib));
		GreenMask =  BITMAPGREENMASK(((LPBITMAPINFOHEADER)pDib));
		BlueMask  =  BITMAPBLUEMASK (((LPBITMAPINFOHEADER)pDib));
		}
	    // Calculate the magnitude of the shift from the respective Mask (Blue is assumed to be LSD)
	    // This is to conform to MicroSoft DIB V4 & V5 specification
	    // I have NEVER seen a DIB with other than the standard Masks
	    int 	bs =  0, rs =  0, gs = 0;
	    DWORD	Mask;
	    
	    Mask = BlueMask;
	    while((Mask&0x80) == 0)     { Mask <<= 1; bs++; }
	    Mask = RedMask;
	    while((Mask&1) == 0)	{ Mask >>= 1; rs++; }
	    while((Mask&0x80) == 0)     { Mask <<= 1; rs--; }
	    Mask = GreenMask;
	    while((Mask&1) == 0)	{ Mask >>= 1; gs++; }
	    while((Mask&0x80) == 0)     { Mask <<= 1; gs--; }

     	    for(i = 0; i < height; ++i)
	    	{
	    	Dest = (RGBTRIPLE *)((BYTE *)DibPixels + i * DestWidthBytes);
	    	Source32 = (DWORD *)(SourcePixels+ i * SourceWidthBytes);
	    	for(j = 0; j < width; ++j)
		    {
		    SourcePel32 = *Source32;
		    Dest->rgbtBlue  = (BYTE)((SourcePel32 & BlueMask)  << bs);
		    Dest->rgbtGreen = (BYTE)((SourcePel32 & GreenMask) >> gs);
		    Dest->rgbtRed   = (BYTE)((SourcePel32 & RedMask)   >> rs);
		    Dest++;
		    Source32++;
		    }
	    	}
	    }
//	case 32:
//	    {
//	    RGBQUAD *	Source32;
//	    if (pDib->bmiHeader.biCompression == BI_BITFIELDS)
//		{
//		// If this is a 16 or 32 bit bitmap and BI_BITFIELDS is used,
//		// bmiColors member contains three DWORD color masks.
//		if (pDib->bmiHeader.biSize == sizeof(BITMAPINFOHEADER))
//		    {
//		    // For V4 or V5 headers, this info is included in the header
//			SourcePixels += 3 * sizeof(DWORD);
//		    }
//		}
//
//     	    for(i = 0; i < height; ++i)
//	    	{
//	    	Dest = (RGBTRIPLE *)((BYTE *)DibPixels + i * DestWidthBytes);
//	    	Source32 = (RGBQUAD *)(SourcePixels+ i * SourceWidthBytes);
//	    	for(j = 0; j < width; ++j)
//		    {
//		    Dest->rgbtBlue  = Source32->rgbBlue;
//		    Dest->rgbtGreen = Source32->rgbGreen;
//		    Dest->rgbtRed   = Source32->rgbRed;
//		    Dest++;
//		    Source32++;
//		    }
//	    	}
//	    }
	    break;
	case 64:
	    {
	    struct RGBOCT{
		BYTE    rgbBlue;
		BYTE    rgbBlueHi;
		BYTE    rgbGreen;
		BYTE    rgbGreenHi;
		BYTE    rgbRed;
		BYTE    rgbRedHi;
		WORD    rgbReserved;
		}  *	Source64;

     	    for(i = 0; i < height; ++i)
	    	{
	    	Dest = (RGBTRIPLE *)((BYTE *)DibPixels + i * DestWidthBytes);
	    	Source64 = (RGBOCT *)(SourcePixels+ i * SourceWidthBytes);
	    	for(j = 0; j < width; ++j)
		    {
		    Dest->rgbtBlue  = Source64->rgbBlue;
		    Dest->rgbtGreen = Source64->rgbGreen;
		    Dest->rgbtRed   = Source64->rgbRed;
		    Dest++;
		    Source64++;
		    }
	    	}
	    }
	    break;
	default:
	    InitDib(width, height, bits);		// Create an empty bit DIB of existing size
	    memcpy(DibPixels, SourcePixels, SizeImage);	// just copy pixels
	    break;
	}
    }

void	CDib::DibTo24(LPBITMAPINFO pDib)
    {
    WORD	bits = pDib->bmiHeader.biBitCount;
    if(bits <= 8 && pDib->bmiHeader.biClrUsed == 0)
	pDib->bmiHeader.biClrUsed = 1 << bits;	// biClrUsed is permitted to be 0, but DIBPEL assumes it is set

    BYTE *	SourcePixels = DIBPEL(pDib);
    if (pDib->bmiHeader.biCompression == BI_BITFIELDS)
	{
	// If this is a 16 or 32 bit bitmap and BI_BITFIELDS is used,
	// bmiColors member contains three DWORD color masks.
	if (pDib->bmiHeader.biSize == sizeof(BITMAPINFOHEADER))
	    {
	    SourcePixels += 3 * sizeof(DWORD);    // For V4 or V5 headers, this info is included in the header
	    }
	}
    DibTo24(pDib, SourcePixels);
}

void	CDib::DibTo24(const CDib & Dib1)
    {
    DibTo24(Dib1.pDibInf);
    }

void	CDib::DibTo24()
    {
    CDib	TempDIB(*this);	// Create TempDIB initialised to current Dib
    DibTo24(TempDIB);
    }

// Insert pixels into 24 bit CDib at x,y (source MUST be 24 bit pixel array)
//void	CDib::InsertDib(unsigned x, unsigned y, BYTE * SourcePixels, unsigned width, unsigned height)
//    {
    //	NOTE DIB origin lies at the lower left corner. 
//    unsigned 	i;
//    unsigned		height2move = height;
//    BYTE *	Source;
//    BYTE *	Dest;
//    int		DestOffset	= x * 3;	// NOTE this is the actual number of bytes for pixels offset
//    DWORD	DestWidthBytes   = WIDTHBYTES((DWORD)DibWidth * 24L);
//    WORD &	DestWidthBytes   = WidthBytes;
//    DWORD	SourceWidthBytes = WIDTHBYTES((DWORD)width * 24L);
//    unsigned		SourceWidth;    // NOTE SourceWidth is the actual number of bytes used for pixels (less any pixels which would overflow)

//    if(x >= DibWidth || y >= DibHeight)	return;	// Nothing to insert

//	Discard any lines which would overflow the DIB
//    if(height > DibHeight - y)	height2move = DibHeight - y;
//    SourceWidth	= ((width > DibWidth - x) ? DibWidth - x : width) * 3;

//    Dest = DibPixels + (DibHeight - height2move - y) * DestWidthBytes + DestOffset;
//    Source = SourcePixels + (height - height2move) * SourceWidthBytes;
//    for(i = height - height2move; i < height; ++i)
//	{
//	memcpy(Dest, Source, SourceWidth);
//	Dest += DestWidthBytes;
//	Source += SourceWidthBytes;
//	}
//    }

// Insert pixels into 24 bit CDib at x,y (source MUST be 24 bit pixel array) 
// signed offset allows for source Dib origin to be above and left of the destination Dib
void	CDib::InsertDib(int x, int y, BYTE * SourcePixels, unsigned width, unsigned height)
    {
    //	NOTE DIB origin lies at the lower left corner. 
    unsigned 	i;
    unsigned		height2move = height;
    BYTE *	Source;
    BYTE *	Dest;
    int		DestOffset	= x * 3;	// NOTE this is the actual number of bytes for pixels offset
//    DWORD	DestWidthBytes   = WIDTHBYTES((DWORD)DibWidth * 24L);
    DWORD &	DestWidthBytes   = WidthBytes;
    DWORD	SourceWidthBytes = WIDTHBYTES((DWORD)width * 24L);
    unsigned		SourceWidth;    // NOTE SourceWidth is the actual number of bytes used for pixels (less any pixels which would overflow)

    if(x >= DibWidth || y >= DibHeight || x < -DibWidth || y < -DibHeight)	return;	// Nothing to insert

//	Discard any lines which would overflow the DIB
    if((int)height > DibHeight - y)	height2move = DibHeight - y;

    if (x < 0)							// inserted Dib origin left of destination Dib
	{
	SourceWidth = (width + x > DibWidth) ? WIDTHBYTES((DWORD)DibWidth * 24L) : (width + x) * 3;
	Dest = DibPixels + (DibHeight - height2move - y) * DestWidthBytes;
	Source = SourcePixels + (height - height2move) * SourceWidthBytes - x * 3;
	}
    else
	{
	SourceWidth = (((int)width > DibWidth - x) ? DibWidth - x : width) * 3;
	Dest = DibPixels + (DibHeight - height2move - y) * DestWidthBytes + DestOffset;
	Source = SourcePixels + (height - height2move) * SourceWidthBytes;
	}

    for(i = height - height2move; i < height; ++i)
	{
	if (y < 0 && i >= y + height)				// inserted Dib origin above destination Dib 
	    break;
	memcpy(Dest, Source, SourceWidth);
	Dest += DestWidthBytes;
	Source += SourceWidthBytes;
	}
    }

// Insert text into 24 bit CDib at the location rect 

void	CDib::Text2Dib(HDC hDc, RECT *rect, TCHAR *text)
    {
    Text2Dib(hDc, rect, GetTextColor(hDc), GetBkColor(hDc), NULL, GetBkMode(hDc),text);
    }

void	CDib::Text2Dib(HDC hDc, RECT *rect, LOGFONT *lf, TCHAR *text)
    {
    Text2Dib(hDc, rect, GetTextColor(hDc), GetBkColor(hDc), lf, GetBkMode(hDc), text);
    }
void	CDib::Text2Dib(HDC hDc, RECT *rect, COLORREF FontColour, COLORREF BkgColour, LOGFONT *lf, int TranparentText, TCHAR *text)
    {
    HDC	hdc;
    HBITMAP	hBitmap;

//    HDC	hDc = GetDC(hwnd); 
    if ((hdc = CreateCompatibleDC(hDc)) == NULL)
	return;
    SetTextColor(hdc, FontColour);
    SetBkColor(hdc, BkgColour);
    SetBkMode(hdc, TranparentText);
    if (lf)
	SelectObject (hdc, CreateFontIndirect (lf)) ;
    hBitmap = CreateCompatibleBitmap (hDc, DibWidth, DibHeight);
    SelectObject (hdc, hBitmap);
    if (SetDIBits(hDc, hBitmap, 0, DibHeight, DibPixels, (LPBITMAPINFO)pDibInf, DIB_RGB_COLORS) == 0)
	return;
    DrawText(hdc, text, -1, rect, DT_CENTER);
    if (GetDIBits(hDc, hBitmap, 0, DibHeight, DibPixels, (LPBITMAPINFO)pDibInf, DIB_RGB_COLORS) == 0)
	return;
    DeleteObject(hBitmap);
//    ReleaseDC(hwnd, hDc);
    }

//	Initialise DIB Palette (This is not a member of CDib)
void	SetDibPalette(LPBITMAPINFO pDib, BYTE *palette)
    {
    RGBQUAD *pRgb;
    WORD i, i3;
    if(palette != NULL)
	{
	pRgb = DIBPAL(pDib);
	for(i = i3 = 0; i < pDib->bmiHeader.biClrUsed; ++i, i3+=RGB_SIZE)
	    {
	    pRgb[i].rgbRed   = (char)palette[i3+RGB_RED];
	    pRgb[i].rgbGreen = (char)palette[i3+RGB_GREEN];
	    pRgb[i].rgbBlue  = (char)palette[i3+RGB_BLUE];
	    pRgb[i].rgbReserved = 0;
	    }
	}
    }

//	Round bit depths up to legal values for a bitmap
// IB 2009-05-28 add 16 bit (and tidy up)
static WORD	AdjustDIBits(WORD n)
{
if (n == 1)	return(1);
if (n <= 4)	return(4);
if (n <= 8)	return(8);
if (n <= 16)	return(16);
if (n <= 24)	return(24);
if (n <= 32)	return(32);
    return(64);			// found 64 bit PNG files recently PHD 2009-05-16
}

//	Create an empty bitmap (This is not a member of CDib)
//	NOTE it is the responsibility of the caller to release memory allocated by this function
//	NOTE does not currently handle 16 or 32 bit DIB (which require bit field masks for BI_BITFIELD)
LPBITMAPINFO	CreateEmptyDib(int Width, int Height, WORD bits)
    {
    LPBITMAPINFO	pDib;		// pointer to the DIB info
    bits = AdjustDIBits(bits);
    DWORD	colours_used = bits > 8 ? 0L : (DWORD)((1<<bits) & 0xffff);
    DWORD	SizeImage = WIDTHBYTES(Width * bits) * Height;	// Size, in bytes, of the image made of RGB triplets.
    DWORD	DibSize = sizeof(BITMAPINFOHEADER) + colours_used*sizeof(RGBQUAD) + SizeImage;

    // allocate a device-independent bitmap header + palette + pixel array
    if(pDib = (LPBITMAPINFO)new BYTE[DibSize] )
	{
	// fill in the header
	pDib->bmiHeader.biSize	= (DWORD)sizeof(BITMAPINFOHEADER);
	pDib->bmiHeader.biWidth  	= (DWORD)Width;
	pDib->bmiHeader.biHeight 	= (DWORD)Height;
	pDib->bmiHeader.biPlanes 	= 1;
	pDib->bmiHeader.biBitCount 	= bits;
	pDib->bmiHeader.biCompression 	= BI_RGB;
	pDib->bmiHeader.biSizeImage	= SizeImage;	// Size, in bytes, of the image. May be set to zero for BI_RGB bitmaps.
	pDib->bmiHeader.biXPelsPerMeter = 0L;
	pDib->bmiHeader.biYPelsPerMeter = 0L;
	pDib->bmiHeader.biClrUsed 	= colours_used;
	pDib->bmiHeader.biClrImportant 	= 0;
	}
    else
	{
//	throw "Can't allocate DIB memory.";	// IB 2009-05-18 C++ error handling
	return NULL;
	}
    return pDib;
    }

//	Create an empty bitmap, Initialise Palette (This is not a member of CDib)
//	NOTE it is the responsibility of the caller to release memory allocated by this function
LPBITMAPINFO	CreateEmptyDib(int Width, int Height, BYTE *palette, WORD bits)
    {
    // allocate a device-independent bitmap header + palette + pixel array
    LPBITMAPINFO pDib = CreateEmptyDib(Width, Height, bits);
    if(pDib == NULL )
	{
	return NULL;
	}
    SetDibPalette(pDib, palette);
    return pDib;
    }

//////////////////////////////////////////////////////////////////////
//	Initialise Windows Device Independent Bitmap
//////////////////////////////////////////////////////////////////////

unsigned char	*CDib::InitDib(int Width, int Height, WORD bits)
    {
    CloseDibPtrs();			// give back memory for new DIB

    // allocate a device-independent bitmap header + palette + pixel array
    pDibInf = CreateEmptyDib(Width, Height, bits);
    if(pDibInf == NULL )
	{
	DibErrorCode = NODIBMEMORY;
	throw "Can't allocate DIB memory.";	// IB 2009-05-18 C++ error handling
	return NULL;
	}

    // Initialise CDib members
    DibPixels = DIBPEL(pDibInf);
    SizeImage = pDibInf->bmiHeader.biSizeImage;	// Size, in bytes, of the image made of RGB triplets.
    DibHeight = Height;			// dimensions in pixels, useful in undo() and redo()
    DibWidth = Width;			// may be useful to replace these as globals in the future
    BitsPerPixel = pDibInf->bmiHeader.biBitCount ;
    WidthBytes = WIDTHBYTES(Width * bits);
    DibSize = sizeof(BITMAPINFOHEADER) + pDibInf->bmiHeader.biClrUsed*sizeof(RGBQUAD) + SizeImage;
    DibNumColours = 0L;
//    memset(DibPixels, 0x55, SizeImage);	// this is code for debugging
    return DibPixels;
    }

//	Initialise Windows Device Independent Bitmap and Palette
unsigned char	*CDib::InitDib(int Width, int Height, BYTE *palette, WORD bits)
{
    InitDib(Width, Height, bits);
    SetDibPalette(pDibInf, palette);
    return DibPixels;
}

// Currently only supports BI_RGB
unsigned char	*CDib::InitDib(LPBITMAPINFO pDib)
    {
    CloseDibPtrs();			// give back memory for new DIB
    if(pDib->bmiHeader.biCompression != BI_RGB)
	{
	throw "DIB Not supported";
	return NULL;
	}

    // allocate a device-independent bitmap header + palette + pixel array
    InitDib(pDib->bmiHeader.biWidth, pDib->bmiHeader.biHeight, pDib->bmiHeader.biBitCount);
    // Copy palette + pixel array
    memcpy(DIBPAL(pDibInf), DIBPAL(pDib), pDib->bmiHeader.biClrUsed*sizeof(RGBQUAD));	// palette
    memcpy(DibPixels, DIBPEL(pDib), pDib->bmiHeader.biSizeImage);	// picture

    return DibPixels;
    }


//////////////////////////////////////////////////////////////////////
//	Close Dib Pointers
//////////////////////////////////////////////////////////////////////

void	CDib::CloseDibPtrs(void)
    {
    if (DibPixels)
	{
	DibPixels = NULL;
	}
    if (pDibInf)
	{
	delete [] pDibInf;
	pDibInf = NULL;
	}
    }

int compare( const void *arg1, const void *arg2 )
    {
    return *(unsigned*)arg2 - *(unsigned*)arg1;
    }

DWORD CDib::ColourCount(void (*Progress)(int, int, char *))
    {
    int		i, j;
//    int		display_count;
//    int		percent = 0;
//    struct	tnode	*root = NULL;
//    TCHAR	s[120];
    DWORD	WidthBytes   = WIDTHBYTES((DWORD)DibWidth * 24L);	// assumes 24 bit
    RGBTRIPLE *	Source;
//    DWORD	dw1st = GetTickCount();

    if(DibNumColours != 0)	return	DibNumColours;

//    SetWindowText (hwnd, TEXT("Counting total colours in the image"));
//    SetCursor(LoadCursor(NULL, IDC_WAIT));
    unsigned pel = DibHeight*DibWidth;
    unsigned *colour = new unsigned[pel];
    unsigned *cp = colour;
    if(colour == NULL)	return	0;
    for (i = 0; i < DibHeight; i++)
	{
//	if(dw1st / PROGRESSINTERVAL - GetTickCount() / PROGRESSINTERVAL)
////	if((dw1st - GetTickCount()) % PROGRESSINTERVAL)
//	    {
//	    dw1st = GetTickCount();
//	    if(Progress)	Progress(DibHeight, i, "Counting colours: ");
//	    }
//	display_count = (10 * i) / DibHeight;
//	if(display_count > percent)
//	    {
//	    percent = display_count;
//	    _stprintf (s, TEXT("Counting total colours in the image: %d%%"), percent * 10);
//	    SetWindowText (hwnd, s);		// Show formatted text in the caption bar
//	    }
	Source = (RGBTRIPLE *)((BYTE *)DibPixels + i * WidthBytes);
	for (j = 0; j < DibWidth; j++)
	    {
//	    root = tree(root, (DibPixels + ((DWORD) i * WidthBytes) + j * 3));
//	    root = tree(root, (unsigned char *)Source);
//	    root = tree(root, Source->rgbtBlue << 16 | Source->rgbtGreen << 8 | Source->rgbtRed);
	    *cp++ = Source->rgbtBlue << 16 | Source->rgbtGreen << 8 | Source->rgbtRed;
	    Source++;
	    }
	}
    qsort(colour, pel, sizeof(unsigned), compare);
    unsigned count = *colour;
    cp = colour;
    DibNumColours = 1;
    for (; pel>0; pel--, cp++)
	{
	if(count != *cp)
	    {
	    count = *cp;
	    DibNumColours++;
	    }
	}
    delete[] colour;

//    TotalColours = 0;
//    HowManyColours(root);
//    freetree(root);
//    UpdateTitleBar(hwnd);	// Show formatted text in the caption bar
//    SetCursor(hStdCursor);
//    DibNumColours = TotalColours;
    return	DibNumColours;
    }

DWORD CDib::ColourCount(void)
    {
    int		i, j;
//    DWORD	WidthBytes   = WIDTHBYTES((DWORD)DibWidth * 24L);	// assumes 24 bit
    RGBTRIPLE *	Source;

    if(DibNumColours != 0)	return	DibNumColours;

    unsigned pel = DibHeight*DibWidth;
    unsigned *colour = new unsigned[pel];
    unsigned *cp = colour;
    if(colour == NULL)	return	0;
    for (i = 0; i < DibHeight; i++)
	{
	Source = (RGBTRIPLE *)((BYTE *)DibPixels + i * WidthBytes);
	for (j = 0; j < DibWidth; j++)
	    {
	    *cp++ = Source->rgbtBlue << 16 | Source->rgbtGreen << 8 | Source->rgbtRed;
	    Source++;
	    }
	}
    qsort(colour, pel, sizeof(unsigned), compare);
    unsigned count = *colour;
    cp = colour;
    DibNumColours = 1;
    for (; pel>0; pel--, cp++)
	{
	if(count != *cp)
	    {
	    count = *cp;
	    DibNumColours++;
	    }
	}
    delete[] colour;
    return	DibNumColours;
    }

//void CDib::ClearDib(int Red, int Green, int Blue)
//    {
//    int		i, j;
//    RGBTRIPLE *	Dest;
//    DWORD	DestWidthBytes   = WIDTHBYTES((DWORD)DibWidth * 24L);
//    for(i = 0; i < DibHeight; ++i)
//	{
//	Dest = (RGBTRIPLE *)((BYTE *)DibPixels + i * DestWidthBytes);
//	for(j = 0; j < DibWidth; ++j)
//	    {
//	    Dest->rgbtBlue  = Blue;
//	    Dest->rgbtGreen = Green;
//	    Dest->rgbtRed   = Red;
//	    Dest++;
//	    }
//	}
//    }

void CDib::ClearDib(int Red, int Green, int Blue)
    {
    int		i, j;

    RGBTRIPLE *	Dest;
    DWORD	DestWidthBytes   = WIDTHBYTES((DWORD)DibWidth * 24L);

    switch(BitsPerPixel)
	{
	case 24:
	    for(i = 0; i < DibHeight; ++i)
		{
		Dest = (RGBTRIPLE *)((BYTE *)DibPixels + i * DestWidthBytes);
		for(j = 0; j < DibWidth; ++j)
		    {
		    Dest->rgbtBlue  = Blue;
		    Dest->rgbtGreen = Green;
		    Dest->rgbtRed   = Red;
		    Dest++;
		    }
		}
		break;
	default:
	    memset((BYTE *)DibPixels, 0, (size_t)SizeImage);	// if not 24 bit set to 0
	    break;
	}
    }


void CDib::ClearDib(RGBTRIPLE colour)
    {
    ClearDib(colour.rgbtRed, colour.rgbtGreen, colour.rgbtBlue); 
    }

void CDib::ClearDib(DWORD colour)
    {
    ClearDib((colour>>16)&0xFF, (colour>>8)&0xFF, colour&0xFF); 
    }
