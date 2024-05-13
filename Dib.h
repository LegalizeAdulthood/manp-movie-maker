// Dib.h: interface for the CDib class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DIB_H__955DFDBD_51F6_419C_BE14_06287AE21568__INCLUDED_)
#define AFX_DIB_H__955DFDBD_51F6_419C_BE14_06287AE21568__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Dib Error Codes
#define NODIBMEMORY  1
#define NOPIXELMEMORY  2

// Alpha channel types
#define NOALPHA		0
#define RGBA1		1	// PHD 2020-09-03 changed from RGBA to resolve conflict with WebP library
#define GREYA		2

class CDib
    {
    public:
    CDib();
    CDib(const CDib &);	// Copy Constructor
    virtual		~CDib();
    CDib & operator =(const CDib &);	// Assignment Operator

    unsigned char	*InitDib(int Width, int Height);
    unsigned char	*InitDib(int Width, int Height, WORD bits);
    unsigned char	*InitDib(int Width, int Height, BYTE *palette, WORD bits);
    unsigned char	*InitDib(LPBITMAPINFO);
    void		CloseDibPtrs(void);
    void		DibTo24(const CDib &);
    void		DibTo24(LPBITMAPINFO);
    void		DibTo24(LPBITMAPINFO, BYTE *);
    void		DibTo24(void);
//    void		InsertDib(unsigned, unsigned, BYTE *, unsigned, unsigned);
    void		InsertDib(int, int, BYTE *, unsigned, unsigned);
    void		Text2Dib(HDC, RECT *, TCHAR *);
    void		Text2Dib(HDC, RECT *, LOGFONT *, TCHAR *);
    void		Text2Dib(HDC, RECT *, COLORREF, COLORREF, LOGFONT *, int, TCHAR *);
    void		ClearDib(int Red, int Green, int Blue);
//    void		ClearDib(int);
    void		ClearDib(RGBTRIPLE);
    void		ClearDib(DWORD);
//    void		CountColours(void);
    DWORD ColourCount(void (*Progress)(int, int, char *));
    DWORD ColourCount(void);
    DWORD		Size()		{ return DibSize; };
//    WORD		Width()		{ return DibWidth; };
//    WORD		Height()	{ return DibHeight; };
//    WORD		Bits()		{ return BitsPerPixel; };
//    DWORD		NumColours()	{ return DibNumColours; };

    int			DibErrorCode;
    LPBITMAPINFO	pDibInf;		// pointer to the DIB info
    BYTE		*DibPixels;		// the device-independent bitmap  pixels
    DWORD		SizeImage;		// Size, in bytes, of the image.
    WORD		DibWidth;		// the size of the image in pixels
    WORD		DibHeight;
    WORD		BitsPerPixel;
    DWORD		WidthBytes;		// Width, in bytes, of 1 line.
    DWORD		DibNumColours;
    BYTE		AlphaChannel;		// RGB+A or Greyscale+A or off

    private:
    DWORD		DibSize;		// Size, in bytes, of the DIB
    };

#endif // !defined(AFX_DIB_H__955DFDBD_51F6_419C_BE14_06287AE21568__INCLUDED_)
