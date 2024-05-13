/*-----------------------------------------------------
   CROP.CPP --   Crop the Picture according to User scan.
  ---------------------------------------------------*/

#include <windows.h>
#include <commdlg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "view.h"
#include "crop.h"
#include "screen.h"
#include "Dib.h"

#define SWAP(x,y)   ((x)^=(y)^=(x)^=(y))

// Global Variables:
extern	CDib		Dib;			// Device Independent Bitmap class instance - main image

extern	void	SetupView(void);

long	xOffset, yOffset;			// offset for full screen with no borders
int	xMax, yMax;				// max allowable crop size for full screen with no borders

// Paul, this was copied from init_windows_dib(szTitle)
// It would make more sense to use a separate function, although I am not sure that the messages are sensible

void	DisplayDibErrorMessage(HWND hwnd, const CDib & Dib, char * szTitle)
    {
    MessageBox(hwnd, "Can't allocate DIB memory.", szTitle, MB_ICONEXCLAMATION | MB_OK);
    }

/*---------------------------------------------------------------------
	Resize the Picture by factor in type
  -------------------------------------------------------------------*/

DWORD	CalcAddress(HWND hwnd, WORD i, WORD OrigWidth, WORD OrigHeight, RECT & SelectRect)

    {
    DWORD	offset, offset1, address;
    long	ScanBottom, ScanLeft;
    int	VertScrollPos, HorScrollPos;

    VertScrollPos = GetScrollPos (hwnd, SB_VERT);
    HorScrollPos = GetScrollPos (hwnd, SB_HORZ);

    ScanLeft = (SelectRect.left);
    ScanBottom = ((SelectRect.bottom));
    offset = (DWORD) (OrigHeight - ScanBottom + (i - VertScrollPos));
    offset1 = (DWORD) (ScanLeft + HorScrollPos);
    address = offset * WIDTHBYTES((DWORD)OrigWidth * (DWORD)Dib.BitsPerPixel) + offset1 * 3L;
	return address;
    }

//*************************************************************************
//
//  FUNCTION   : NormalizeRect(RECT *prc)
//
//  PURPOSE    : If the rectangle coordinates are reversed, swaps them.
//               This is used to make sure that the first coordinate of
//               our rect is the upper left, and the second is lower right.
//
//*************************************************************************

void WINAPI NormalizeRect(RECT & prc)
    {
    if (prc.right < prc.left) SWAP(prc.right, prc.left);
    if (prc.bottom < prc.top)  SWAP(prc.bottom, prc.top);
    }

/*---------------------------------------------------------------------
	Resize the Picture by factor in type
	
	Crop the Image to the area selected in SelectRect

	globals used in this function
	    long xOffset, yOffset;	// offset for full screen with no borders
	    int	xMax, yMax;		// max allowable crop size for full screen with no borders
	    double VertRatio, HorRatio;
	    int	xmin, ymin;
  -------------------------------------------------------------------*/

int	Crop(HWND hwnd, RECT & SelectRect)
    {
//    char	s[MAXLINE];
    char	szCropTitle [] = TEXT("Cropping Image");
    WORD	OrigWidth = Dib.DibWidth;
    WORD	OrigHeight = Dib.DibHeight;
    WORD	NewHeight, NewWidth;

    // Remap the height and width after forcing selection to be within picture
    NormalizeRect(SelectRect);
    NewWidth = (WORD)(RWIDTH(SelectRect));
    NewHeight = (WORD)(RHEIGHT(SelectRect));

    try
	{
	BYTE *	Dest, * Source;
	DWORD	NewWidthBytes;
	int	i;
//	int	display_count;
//	int	percent = 0;

// IB 2009-05-18 The existing Crop (and the code above) copies the existing DIB, then rescales back into Dib
// This rescales into a temporary DIB (which is thus smaller) then copies the result
	// Construct a temporary DIB for the rescaled image
	CDib	TempDIB;	// Copy current Dib to TempDIB
	if(TempDIB.InitDib(NewWidth, NewHeight, 24) == NULL)
	    {
	    DisplayDibErrorMessage(hwnd, TempDIB, "Resizing Image");
	    return -1;
	    }

	NewWidthBytes = TempDIB.WidthBytes;
	for(i = 0; i < NewHeight; ++i)			// new height
	    {
/*
	    display_count = (10 * i) / NewHeight;
	    if(display_count > percent)
		{
		percent = display_count;
		sprintf (s, "Cropping Image: %d%%", percent * 10);
		SetWindowText (hwnd, s);		// Show formatted text in the caption bar
		}
*/
	    Dest = TempDIB.DibPixels + NewWidthBytes * i;
	    Source = Dib.DibPixels + CalcAddress(hwnd, i, OrigWidth, OrigHeight, SelectRect);
	    memcpy(Dest, Source, NewWidthBytes);
	    }
	// Show formatted text in the caption bar
//	UpdateTitleBar(hwnd);
	Dib = TempDIB;	// Copy the rescaled image
	}
    catch (char * emmg)
	{
	MessageBox(hwnd, emmg, "Crop", MB_ICONEXCLAMATION | MB_OK);
	}

    InvalidateRect(hwnd, NULL, FALSE);
    return 0;
    }


