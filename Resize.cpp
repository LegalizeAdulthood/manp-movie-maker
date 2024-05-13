/*-----------------------------------------------------
   RESIZE.CPP --   Resize the Picture according to scale.
  -----------------------------------------------------*/

#include <windows.h>
#include <stdio.h>
#include "view.h"
#include "Dib.h"
#include "screen.h"

void	interpolate(DWORD, DWORD, double, DWORD, BYTE *, BYTE *, WORD, WORD);
extern	void	UpdateName(void);

static	BOOL	InterpFlag = TRUE;	// default is to interpolate pixels when scaling up
static	BOOL	MaintainAspect = TRUE;	// horizontal scale = vertical scale
static	BOOL	ResizeFlag = FALSE;	// the following prevent generation of messages
					// while these are active

// Global Variables:
extern	CDib	Dib;			// Device Independent Bitmap class instance

/////////////////////////////////////////////////////////////////////////////////////////////////////
extern	HWND	hwnd;			// This is the main windows handle
/////////////////////////////////////////////////////////////////////////////////////////////////////

extern	char	Name[];			// name of file to be displayed
ResizeStruct	RESIZE = {1.0, 1.0, 1, 1, false};

extern	ControlType	PGVControl;	// PGV state. Behaviour depends on current value of this variable
extern	StateType	PGVState;	// what state is pgv in?

extern	void	SetupView(void);
extern	int	SetupUndo(void);
extern	void	CalcWindowSize(void);
extern	void	UpdateTitleBar(HWND);
extern	void	CreateTitleBar(char *, char *, int, int, int);
//extern	void	ProgressDisplay(int, int, char *);

#define PROGRESSLINES 100
#define PROGRESSINTERVAL 1000	// 1000 mSec

/*---------------------------------------------------------------------
	Display progress
  -------------------------------------------------------------------*/
/*
void ProgressDisplay(int max, int current, char *label)
    {
    TCHAR	s[MAXLINE];
    int 	percent = (current * 100) / max;
    sprintf(s, "%s %d%%", label, percent);
    SetWindowText(hwnd, s);		// Show formatted text in the caption bar
    }
*/
//	Resize the image in Dib
int	ResizeDib(CDib *LocalDib, ResizeStruct RESIZE, BOOL InterpFlag/*, void (*Progress)(int, int, char *)*/)
    {
    if(RESIZE.HorResizeValue <= 0.0 || RESIZE.VertResizeValue <= 0.0 || (RESIZE.HorResizeValue == 1.0 && RESIZE.VertResizeValue == 1.0))
	return 0;					// no point in running resize

    DWORD	dw1st = GetTickCount();
    WORD	OrigWidth  = LocalDib->DibWidth;
    WORD	OrigHeight = LocalDib->DibHeight;

    DWORD	NewWidth;
    DWORD	NewHeight;

    // allocate a temp line buffer
    UINT	bits_per_pixel = 24;
    UINT	bytes_per_pixel = bits_per_pixel / 8;
    DWORD 	i, j, j3;
    DWORD	OrigWidthBytes = LocalDib->WidthBytes;
    DWORD	NewWidthBytes;
    long	CalcVertPosition;	// to ensure the pixel exists in the source Dib
    BYTE	*linebuf;
    BYTE	* Dest, * Source;
    CDib	TempDIB = *LocalDib;	// Create TempDIB initialised to current Dib


    if (RESIZE.UseIntVersion)		// flag to tell algorithm to use values given by user - removes rounding errors
	{
	NewWidth = RESIZE.NewWidth;
	NewHeight = RESIZE.NewHeight;
	if (NewWidth <= 0 || NewHeight <= 0)
	    return 0;
	}
    else
	{
	NewWidth = (DWORD)((double)OrigWidth  * RESIZE.HorResizeValue);
	NewHeight = (DWORD)((double)OrigHeight * RESIZE.VertResizeValue);
	}

    // Construct a new device-independent bitmap for the rescaled image
    if (LocalDib->InitDib(NewWidth, NewHeight, 24) == NULL)
	{
	return -1;
	}
    NewWidthBytes = LocalDib->WidthBytes;		// note that the size of Dib has already been changed
    linebuf = new BYTE [NewWidthBytes];		// row buffer
    if(linebuf == NULL)
	{
	return -1;
	}

    for(i = 0; i < NewHeight; ++i)
	{
	if(dw1st / PROGRESSINTERVAL - GetTickCount() / PROGRESSINTERVAL)
	    {
	    dw1st = GetTickCount();
//	    if(Progress)	Progress(NewHeight, i, "Resizing Image:");
	    }
	    
	// can't interpolate with line or column past last or before the first pixel
	Dest = LocalDib->DibPixels + NewWidthBytes * i;
	CalcVertPosition = (DWORD) ((double)i / RESIZE.VertResizeValue);
//	if(InterpFlag && i < NewHeight - 1 && i > 1/* && i < (long)CalcVertPosition*/)
	if (InterpFlag && CalcVertPosition < OrigHeight - 1 && CalcVertPosition > 1)
	    {
	    Source = TempDIB.DibPixels + CalcVertPosition * OrigWidthBytes;
	    for(j = j3 = 0; j < NewWidth; ++j, j3+=3)
		{
		interpolate(i, j, RESIZE.HorResizeValue, OrigWidthBytes, (linebuf + j3), Source, TempDIB.DibWidth, TempDIB.DibHeight);
		}
	    memcpy(Dest, linebuf, NewWidthBytes);  // this row clear of filter
	    }
	else
	    {
	    Source = TempDIB.DibPixels + OrigWidthBytes * ((i * OrigHeight) / NewHeight);
	    for(j = j3 = 0; j < NewWidth; ++j, j3+=3)
		{
		memcpy(Dest + j3, Source + ((j * OrigWidth) / NewWidth)*3, 3);
		}
	    }
	}
    delete [] linebuf;
    RESIZE.UseIntVersion = false;
    return 0;
    }

int	ResizeDib(CDib *LocalDib, ResizeStruct RESIZE)
    {
    return ResizeDib(LocalDib, RESIZE, InterpFlag);
    }

/*
int	ResizeDib(CDib *LocalDib, ResizeStruct RESIZE, BOOL InterpFlag)
    {
    return ResizeDib(LocalDib, RESIZE, InterpFlag);
    }
*/

/*---------------------------------------------------------------------
	Interpolate new pixels by factor in type
  -------------------------------------------------------------------*/

void	interpolate(DWORD i, DWORD j, double type, DWORD LineWidth, BYTE *linebuf, BYTE *pix, WORD OrigWidth, WORD OrigHeight)
    {
    DWORD	Three_J, three_j;
    WORD	k;
    DWORD	tempx, tempy, value, TypeInt;
    double	multiplier;

    multiplier = 100.0 / type;
    TypeInt = 100L;
    Three_J = j + j + j;
    three_j = (DWORD)((double)Three_J * multiplier / 300.0);
    three_j = three_j + three_j + three_j;		// convert to multiple of 3
    tempx = ((DWORD)((double)j * multiplier)) % 100L;
    tempy = ((DWORD)((double)i * multiplier)) % 100L;

    value=0L;

    if(tempy == 0L)					// This column exists in orig picture
	{
	if(tempx == 0L)					// this pixel just copies across
	    memcpy(linebuf, (pix + three_j), 3);
	else
	    {
	    for(k = 0; k < 3; ++k)				// 3 colour channels
		{
		value = ((DWORD)(*(pix + three_j + k)) * (TypeInt - tempx) +
		    (DWORD)(*(pix + three_j + k + 3)) * tempx);
		*(linebuf + k) = (BYTE)(value / TypeInt);
		}
	    }
	}
    else
	{
	if(tempx == 0L)			// this pixel just copies across
	    {
	    for(k = 0; k < 3; ++k)				// 3 colour channels
		{
		value = ((DWORD)(*(pix + three_j + k)) * (TypeInt - tempy) +
		    (DWORD)(*(pix + three_j + k + LineWidth)) * tempy);
		*(linebuf + k) = (BYTE)(value / TypeInt);
		}
	    }
	else
	    {
	    for(k = 0; k < 3; ++k)				// 3 colour channels
		{
		value = (DWORD)(*(pix + three_j + k)) * (TypeInt - tempx) * (TypeInt - tempy)
		+ (DWORD)(*(pix + three_j + k + 3)) * tempx * (TypeInt - tempy)
		+ (DWORD)(*(pix + three_j + k + LineWidth)) * (TypeInt - tempx) * tempy
		+ (DWORD)(*(pix + three_j + k + LineWidth + 3)) * tempx * tempy;
		*(linebuf + k) = (BYTE)(value / (TypeInt * TypeInt));
		}
	    }
	}
    }

