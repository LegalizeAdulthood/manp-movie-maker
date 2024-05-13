/*-----------------------------------------
	ManpMovieMaker.CPP 
	ManpWIN Movie File Sequence Generation Program Written by Paul de Leeuw
  -----------------------------------------*/

#include <windows.h>
#include <Winuser.h>
#include <commdlg.h>
#include <commctrl.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <setjmp.h>
#include <string.h>
#include "resource.h"
#include "Dib.h"
#include "View.h"
#include "Screen.h"
#include "DibMacro.h"

#define	 MAX_INSERTED_FRAMES	10

extern	BOOL	ScrollImage(HWND hwnd, int nScrollBar, WORD wScrollCode);
extern	void	InitializeScrollBars(HWND, RECT *, RECT *);
extern	int	decode_png_header(HWND hwnd, char *infile, char *szAppName);
extern	int	png_decoder(HWND hwnd, char *szAppName, char *infile, double  *Zoom, bool UseComment);
extern "C" void	HandleJPEGError(void);
extern "C" int	SaveJPEG(HWND, FILE *, BYTE *, BOOL, int, WORD *, WORD *, WORD *);
extern "C"	jmp_buf	mark;				// Address for long jump to jump to when an error occurs

//extern	void InitText(void);
//	int  SaveData(HWND hwnd, char *szFileName, char *szAppName, char *lpText, char *Time, char *Name);

extern	void	InitFileList(void);
extern	int	LoadFileList(HWND hwnd, char *Filename);
extern	void	SortFileList(void);
//extern	void	GetPNGComment(HWND hwnd, char *infile, double *Zoom);
extern	int	ResizeDib(CDib *LocalDib, ResizeStruct RESIZE, BOOL InterpFlag);
extern	int	Crop(HWND hwnd, RECT & SelectRect);
extern	void	WINAPI	NormalizeRect(RECT &);

long FAR PASCAL WndProc (HWND, UINT, UINT, LONG);
BOOL FAR PASCAL AboutBoxDlg (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL FAR PASCAL SpecifyImageFileDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL FAR PASCAL CreateFFMPEGCommandDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void	ChangeView(HWND hwnd, int xdest, int ydest, int widthdest, int heightdest, int xsrc, int ysrc, int widthsrc, int heightsrc, char stretchflag);
char 	szAppName [100] = "ManpMovieMaker";
static	HANDLE	hBitmap = NULL;					// opening bitmap
static	HDC	hdcMem;
static	BITMAP	bm;
static	LOGFONT	lf;

int	GenerateFileSequence(HWND hwnd, char *Filename, int NumInsertedFrames);

enum	JustifyKind	{LEFT, CENTRE, RIGHT, JUSTIFY};
static	JustifyKind	Justification;

int	NumInsertedFrames = 0;
int	NumEndFrames = 50;		// number of times to repeat last frame. Assume 2 seconds at 25 frames per second
int	filecount, FrameCount;
bool	UseScaling = true;

//#define min(a,b) (((a) < (b)) ? (a) : (b))
//#define max(a,b) (((a) > (b)) ? (a) : (b))
#define RWIDTH(rect) (rect.right - rect.left)	// IB 2009-04-14
#define RHEIGHT(rect) (rect.bottom - rect.top)

//int	type = 0;   // 0 = Pi, 1 = e, 2 = e using FFT, 3 = log 2, 4 = sqrt 2

char		szFileName[MAX_PATH] = " ";
char		Name[160];
char		Time[160];

PAINTSTRUCT 	ps;
RECT 		r;
HPALETTE 	hpal = NULL;
StateType	State = NORMAL;

// Styles of app. window 
DWORD		dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_THICKFRAME;
//DWORD		dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
int		AppIndex;
char		Filename[MAX_PATH];
char		OutputPath[MAX_PATH] = "";
char		OutputFilename[MAX_PATH] = "";
char		InputPath[MAX_PATH] = "";
char		AudioPath[MAX_PATH] = "";
char		ffmpegCommand[1200];
char		ffmpegAudioCommand[1200];
static	short	nNumLines;

int		xdots, ydots, height, width, bits_per_pixel;
int		caption, scroll_width, display_width, display_height, HorOffset, VertOffset, screenx, screeny;

CDib		Dib;				// Device Independent Bitmap class instance
HWND		hwnd;

extern	struct FileListStuff FileList[MAXFILES];	// info for sorting file list

/*-----------------------------------------
	Main Windows Entry Point
  -----------------------------------------*/

int PASCAL WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
		    LPSTR lpszCmdLine, int nCmdShow)
     {
     MSG      msg;
     WNDCLASS wndclass;

     if (!hPrevInstance)
	  {
	  wndclass.style	 = CS_HREDRAW | CS_VREDRAW;
//	  wndclass.style	 = CS_HREDRAW | CS_VREDRAW | CS_SAVEBITS;
	  wndclass.lpfnWndProc	 = (WNDPROC)WndProc;
	  wndclass.cbClsExtra	 = 0;
	  wndclass.cbWndExtra	 = 0;
	  wndclass.hInstance	 = hInstance;
//	  wndclass.hIcon	 = LoadIcon (NULL, IDI_APPLICATION);
	  wndclass.hIcon	 = LoadIcon (hInstance, szAppName);
	  wndclass.hCursor	 = LoadCursor (NULL, IDC_ARROW);
	  wndclass.hbrBackground = (HBRUSH)GetStockObject (WHITE_BRUSH);
	  wndclass.lpszMenuName  = szAppName;
	  wndclass.lpszClassName = szAppName;

	  RegisterClass (&wndclass);
	  }
     
     hwnd = CreateWindow (szAppName, "ManpWIN Frame Generator",
     			  dwStyle, 
			  160, 120, 				// nice initial location
			  500, 300, 				// nice initial size
			  NULL, NULL, hInstance, NULL);

     ShowWindow (hwnd, nCmdShow);
     UpdateWindow (hwnd);

     while (GetMessage (&msg, NULL, 0, 0))
	  {
	  TranslateMessage (&msg);
	  DispatchMessage (&msg);
	  }

     return (int)msg.wParam;
     }

/*-----------------------------------------
	Draw Opening Bitmap
  -----------------------------------------*/

void	DrawBitmap(HWND hwnd, HDC hdc, HANDLE hBitmap)

    {
    DWORD	ErrorCode;
    char	s[180];

    hdcMem = CreateCompatibleDC(hdc);
    if (SelectObject(hdcMem, hBitmap) == NULL)
	{
	ErrorCode = GetLastError();
	sprintf(s, "SelectObject Error type = %ld", ErrorCode);
	MessageBox (hwnd, s, szAppName, MB_ICONEXCLAMATION | MB_OK);
	}
    SetMapMode(hdcMem, GetMapMode(hdc));
    if (GetObject(hBitmap, sizeof(BITMAP), (LPSTR) &bm) == 0)
	{
	ErrorCode = GetLastError();
	sprintf(s, "GetObject Error type = %ld", ErrorCode);
	MessageBox (hwnd, s, szAppName, MB_ICONEXCLAMATION | MB_OK);
	}
    if (hBitmap)
	{
	DeleteObject(hBitmap);
	hBitmap = NULL;
	}
    MoveWindow (hwnd, 160, 120, bm.bmWidth, bm.bmHeight, TRUE);
    ShowWindow (hwnd, SW_SHOWNORMAL);
    InvalidateRect(hwnd, NULL, FALSE);      
    }

/*-----------------------------------------
	Let's Process a few messages
  -----------------------------------------*/

long FAR PASCAL WndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
    {
    static HINSTANCE	hInst;
    static HCURSOR	hCursor;
    static HBRUSH	hBrush;
    HDC			hdc;
    DWORD		ErrorCode;
//    int			result;
     static HWND    hScroll ;
     PAINTSTRUCT     ps ;
//     char	    *p;

    char		s[180];
//    static char    szSaveFileName  [480];

    switch (message)
	{
	case WM_CREATE:
	    hInst = ((LPCREATESTRUCT)(_int64)lParam)->hInstance;
	    hdc = GetDC(hwnd);
	    hBitmap = LoadImage(hInst, MAKEINTRESOURCE(MANPMOVIEMAKER_BMP), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR/*LR_LOADFROMFILE*/);
	    DrawBitmap(hwnd, hdc, hBitmap);		// cute opening bitmap
	    BeginPaint(hwnd, &ps);
	    if (BitBlt(ps.hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY) == 0)
		{
		ErrorCode = GetLastError();
		sprintf(s, "Error type = %ld", ErrorCode);
		MessageBox(hwnd, s, szAppName, MB_ICONEXCLAMATION | MB_OK);
		}
	    //		sprintf(s, "Image size = %d, %d", bm.bmWidth, bm.bmHeight);
	    //		MessageBox (hwnd, s, szAppName, MB_ICONEXCLAMATION | MB_OK);
	    MoveWindow(hwnd, 160, 120, bm.bmWidth, bm.bmHeight, TRUE);
	    ShowWindow(hwnd, SW_SHOWNORMAL);
	    EndPaint(hwnd, &ps);
	    return 0;

	case WM_INITMENUPOPUP:

	    switch (lParam)
		{
		case 0:	   // File menu
		    EnableMenuItem((HMENU)(_int64)wParam, IDM_SETUPFILEINFO, MF_ENABLED);
		    EnableMenuItem((HMENU)(_int64)wParam, IDM_FFMPEG, MF_ENABLED);
		    EnableMenuItem ((HMENU)(_int64)wParam, IDM_EXIT, MF_ENABLED);
		    break;

		case 1:	   // Help menu
		    EnableMenuItem ((HMENU)(_int64)wParam, IDM_ABOUT, MF_ENABLED);
		    break;
		}
	    return 0;

	  case WM_COMMAND:

	       switch (wParam)
		    {
		    case IDM_EXIT:
			SendMessage (hwnd, WM_CLOSE, 0, 0L);
			return 0;

		    case IDM_SETUPFILEINFO:
			if (DialogBox (hInst, "SpecifyImageFileDlg", hwnd, (DLGPROC)SpecifyImageFileDlg) != IDOK)
			    return 0;
			else
			    GenerateFileSequence(hwnd, Filename, NumInsertedFrames);
			return 0;

		    case IDM_FFMPEG:
			DialogBox(hInst, "CreateFFMPEGCommandDlg", hwnd, (DLGPROC)CreateFFMPEGCommandDlg);
			return 0;

			// Messages from Help menu
		    case IDM_ABOUT:
			 DialogBox (hInst, "AboutBoxDlg", hwnd, (DLGPROC)AboutBoxDlg);
			 return 0;
		    }
	       break;

	  case WM_SIZE:

		InvalidateRect(hwnd, NULL, FALSE);      
		return 0;

	  case WM_PAINT:
/*
		BeginPaint(hwnd, &ps);
		if (BitBlt(ps.hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY) == 0)
		    {
		    ErrorCode = GetLastError();
		    sprintf(s, "Error type = %ld", ErrorCode);
		    MessageBox (hwnd, s, szAppName, MB_ICONEXCLAMATION | MB_OK);
		    }
//		sprintf(s, "Image size = %d, %d", bm.bmWidth, bm.bmHeight);
//		MessageBox (hwnd, s, szAppName, MB_ICONEXCLAMATION | MB_OK);
		MoveWindow (hwnd, 160, 120, bm.bmWidth, bm.bmHeight, TRUE);
		ShowWindow (hwnd, SW_SHOWNORMAL);
		EndPaint(hwnd, &ps);
*/
		ChangeView(hwnd, -GetScrollPos(hwnd, SB_HORZ), -GetScrollPos(hwnd, SB_VERT), width, height, 0, 0, width, height, TRUE);
		return 0;

        case WM_KEYDOWN:					// Handle any keyboard messages
            switch (wParam) 
                {
                case VK_F1:						// Help    
		    PostMessage (hwnd, WM_COMMAND, IDM_ABOUT,   0L);
                    break;
                case 'f':						// Select Constant to display
                case 'F': 
		     SendMessage (hwnd, WM_COMMAND, IDM_FFMPEG, 0L);
                     break;
                case 's':						// Select Constant to display
                case 'S': 
		     SendMessage (hwnd, WM_COMMAND, IDM_SETUPFILEINFO, 0L);
                     break;
/*
                case 'o':						// open file  
                case 'O': 
		     SendMessage (hwnd, WM_COMMAND, IDM_OPEN, 0L);
                     break;
*/
                case VK_RETURN:						// Let's get out of here
                case VK_ESCAPE: 
		    State = NORMAL;
		    SendMessage (hwnd, WM_CLOSE, 0, 0L);
                    break;
                }
            break;

	  case WM_DESTROY :
		PostQuitMessage (0);
	       	return 0;
	  }
     return (long)DefWindowProc (hwnd, message, wParam, lParam);
     }

/**************************************************************************
	Image files
**************************************************************************/

BOOL FAR PASCAL SpecifyImageFileDlg (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
     {
     BOOL		bTrans ;
     char		*ptr;
     HWND		hCtrl;
     char		drive0[_MAX_DRIVE], dir0[_MAX_DIR], name0[_MAX_FNAME], ext0[_MAX_EXT];

     switch (message)
	  {
	  case WM_INITDIALOG:
		SetDlgItemText(hDlg, IDC_FIRSTFILE, Filename);
		SetDlgItemText(hDlg, IDC_OUTPUTPATH, OutputPath);
		SetDlgItemText(hDlg, IDC_OUTPUTFILENAME, OutputFilename);
		SetDlgItemInt(hDlg, IDC_NUMBER_INSERTED_FRAMES, NumInsertedFrames, TRUE);
		SetDlgItemInt(hDlg, IDC_ADDITIONALFRAMES, NumEndFrames, TRUE);
		hCtrl = GetDlgItem(hDlg, IDC_USESCALING);
		SendMessage(hCtrl, BM_SETCHECK, UseScaling, 0L);
	        return FALSE ;

	  case WM_COMMAND:
	        switch ((int) LOWORD(wParam))
//	        switch (wParam)
		    {
		    case IDOK:
			NumEndFrames = GetDlgItemInt(hDlg, IDC_ADDITIONALFRAMES, &bTrans, TRUE);
			NumInsertedFrames = GetDlgItemInt(hDlg, IDC_NUMBER_INSERTED_FRAMES, &bTrans, TRUE);
			if (NumInsertedFrames < 0)
			    NumInsertedFrames = 0;
			if (NumInsertedFrames > MAX_INSERTED_FRAMES)
			    NumInsertedFrames = MAX_INSERTED_FRAMES;
			GetDlgItemText(hDlg, IDC_FIRSTFILE, Filename, MAX_PATH);
			GetDlgItemText(hDlg, IDC_OUTPUTPATH, OutputPath, MAX_PATH);
			GetDlgItemText(hDlg, IDC_OUTPUTFILENAME, OutputFilename, MAX_PATH);

			_splitpath(Filename, drive0, dir0, name0, ext0);
			_makepath(InputPath, drive0, dir0, "", "");

			if (!*OutputPath)
			    strcpy(OutputPath, InputPath);

			ptr = OutputPath + strlen(OutputPath) - 1;		// remove final backslash if it exists so we can add it later
			if (*ptr = '\\')
			    *ptr = '\0';
			ptr = InputPath + strlen(InputPath) - 1;		// remove final backslash if it exists so we can add it later
			if (*ptr = '\\')
			    *ptr = '\0';
			hCtrl = GetDlgItem(hDlg, IDC_USESCALING);
			UseScaling = (BOOL)SendMessage(hCtrl, BM_GETCHECK, 0, 0L);
			EndDialog (hDlg, TRUE);
			return TRUE;

		    case IDCANCEL:
			EndDialog (hDlg, FALSE);
			return FALSE;
		   }
		   break;
	    }
      return FALSE ;
      }

/**************************************************************************
	Copy text to clipboard
**************************************************************************/

int	CopyTextToClipboard(HWND hwnd, char *text)
    {
    HGLOBAL hMem;

    if (text == NULL)
	return FALSE;
    //    if (!(hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, lstrlenA(text)+1)))
    if (!(hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, (strlen(text) + 1))))
	return -1;
    //    strcpy((char *)GlobalLock(hMem), text);
    strcpy((char *)GlobalLock(hMem), text);
    GlobalUnlock(hMem);
    OpenClipboard(hwnd);
    EmptyClipboard();
#ifdef UNICODE
    SetClipboardData(CF_UNICODETEXT, hMem);
#else
    SetClipboardData(CF_TEXT, hMem);
#endif

    CloseClipboard();
    return 0;
    }

/**************************************************************************
	ffmpeg Commands Dialog Box
**************************************************************************/

BOOL FAR PASCAL CreateFFMPEGCommandDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
    {
//    BOOL		bTrans;
//    char		*ptr;
    char		drive0[_MAX_DRIVE], dir0[_MAX_DIR], name0[_MAX_FNAME], ext0[_MAX_EXT];

    switch (message)
	{
	case WM_INITDIALOG:
//	    SetDlgItemText(hDlg, IDC_FIRSTFILE, Filename);
//	    SetDlgItemText(hDlg, IDC_OUTPUTPATH, OutputPath);
	    SetDlgItemText(hDlg, IDC_AUDIOPATH, AudioPath);
	    SetDlgItemText(hDlg, IDC_OUTPUTFILENAME, OutputFilename);
	    SetDlgItemText(hDlg, IDC_FFMPEG, ffmpegCommand);
	    SetDlgItemText(hDlg, IDC_FFMPEGAUDIO, ffmpegAudioCommand);
	    return FALSE;

	case WM_COMMAND:
	    switch ((int)LOWORD(wParam))
		{
		case IDC_CREATEFFMPEG:
		    GetDlgItemText(hDlg, IDC_AUDIOPATH, AudioPath, MAX_PATH);
		    GetDlgItemText(hDlg, IDC_OUTPUTFILENAME, OutputFilename, MAX_PATH);
		    _splitpath(OutputFilename, drive0, dir0, name0, ext0);		// just in case we forgot to add the output file extension
		    _makepath(OutputFilename, drive0, dir0, name0, "mp4");
		    sprintf(ffmpegCommand, "ffmpeg -i \"%s\\Frame%%05d.jpg\" -c:v libx264 -crf 18 \"%s\\temp.mp4\" ", OutputPath, OutputPath);
		    sprintf(ffmpegAudioCommand, "ffmpeg -i \"%s\" -i \"%s\\temp.mp4\" -acodec copy -vcodec copy -shortest \"%s\\%s\"", AudioPath, OutputPath, OutputPath, OutputFilename);
		    SetDlgItemText(hDlg, IDC_FFMPEG, ffmpegCommand);
		    SetDlgItemText(hDlg, IDC_FFMPEGAUDIO, ffmpegAudioCommand);
		    return FALSE;
		case IDCOPY1:
		    CopyTextToClipboard(hDlg, ffmpegCommand);
		    return FALSE;
		case IDCOPY2:
		    CopyTextToClipboard(hDlg, ffmpegAudioCommand);
		    return FALSE;

		case IDOK:
		    EndDialog(hDlg, TRUE);
		    return TRUE;
		}
	    break;
	}
    return FALSE;
    }

/**************************************************************************
	About Constant Dialog Box
**************************************************************************/

BOOL FAR PASCAL AboutBoxDlg (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
     {
     switch (message)
	  {
	  case WM_COMMAND:
	        switch ((int) LOWORD(wParam))
		    {
		    case IDOK:
			EndDialog (hDlg, TRUE);
			return TRUE;
		   }
		   break;
	    }
      return FALSE ;
      }

/*-----------------------------------------
	Handle View Options
  -----------------------------------------*/

void	ChangeView(HWND hwnd, int xdest, int ydest, int widthdest, int heightdest, int xsrc, int ysrc, int widthsrc, int heightsrc, char stretchflag)
    { 
    HPALETTE hpalT;

    BeginPaint(hwnd, &ps);
    hpalT = SelectPalette (ps.hdc, hpal, FALSE);
    RealizePalette(ps.hdc);
    SetMapMode(ps.hdc,MM_TEXT);
    SetBkMode(ps.hdc,TRANSPARENT);
    SetStretchBltMode (ps.hdc, STRETCH_DELETESCANS) ;	// correct stretch mode for zoom etc
    StretchDIBits (ps.hdc, xdest, ydest, widthdest, heightdest, xsrc, ysrc, widthsrc, heightsrc,
			       (LPSTR)Dib.DibPixels, (LPBITMAPINFO)Dib.pDibInf, DIB_PAL_COLORS, SRCCOPY);
    SelectPalette(ps.hdc,hpalT,FALSE);
    EndPaint(hwnd, &ps);
    }

/*-----------------------------------------
    Setup View Options
-----------------------------------------*/

void	SetupView(HWND hwnd)
    {
//    static	BOOL	first = TRUE;				// start window maximised
    int		xFrame, yFrame;
    RECT 	WARect;						// this is the usable screen taking taskbar into account

    SystemParametersInfo(SPI_GETWORKAREA, 0, &WARect, 0);	// let's get usable area including task bar

    screenx = WARect.right - WARect.left;
    screeny = WARect.bottom - WARect.top;

    //#ifdef _WIN64
    xFrame = GetSystemMetrics(SM_CXFRAME) * 2;			// 2017 version of development environment reports only half the size of 2008 version. Not sure why
    yFrame = GetSystemMetrics(SM_CYFRAME) * 2;
    InvalidateRect(hwnd, &r, FALSE);
    caption = GetSystemMetrics(SM_CYCAPTION) + xFrame * 2 + GetSystemMetrics(SM_CYMENU);
    scroll_width = yFrame * 2;
    display_width = width + scroll_width;
    display_height = height + caption;					// include caption
    display_width = (display_width < screenx) ? display_width : screenx;	// large pictures
    display_height = (display_height < screeny) ? display_height : screeny;
    MoveWindow(hwnd, (screenx - display_width) / 2, (screeny - display_height) / 2, display_width, display_height, TRUE);

    HorOffset = (screenx - display_width) / 2 + yFrame;
    VertOffset = (screeny - display_height) / 2 + GetSystemMetrics(SM_CYCAPTION) + yFrame + GetSystemMetrics(SM_CYMENU);
    }


//////////////////////////////////////////////////////////////////////
//	Save JPEG File
//////////////////////////////////////////////////////////////////////

int	write_jpg_file(HWND hwnd, TCHAR *szSaveFileName, BYTE *JPEGPixels, BOOL WriteThumbFile, WORD ImageWidth, WORD ImageHeight, WORD Bits)
    {
    char	t[200];
    FILE	*fp;
    int		quality = 90;

    if (setjmp(mark))				// PNG and JPEG error handling
	{
	HandleJPEGError();
	return -1;
	}
    if ((fp = fopen(szSaveFileName, "wb")) == NULL)
	{
	wsprintf(t, "Unable to open file: %s", szSaveFileName);
	MessageBox(hwnd, t, szAppName, MB_ICONEXCLAMATION | MB_OK);
	MessageBeep(0);
	return -1;
	}

    if (SaveJPEG(hwnd, fp, JPEGPixels, WriteThumbFile, quality, &ImageHeight, &ImageWidth, &Bits) < 0)
	{
	sprintf(t, TEXT("Error in Opening %s"), szSaveFileName);
	MessageBox(hwnd, t, "Write JPEG", MB_ICONEXCLAMATION | MB_OK);
	if (fp)
	    fclose(fp);
	return -1;
	}
    if (fp)
	fclose(fp);
    return 0;
    }

/////////////////////////////////////////////////////////////////
//	take user keyboard input
/////////////////////////////////////////////////////////////////

void user_data(void)

    {
    MSG msg;

    if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
	TranslateMessage(&msg);
	DispatchMessage(&msg);
	}
    }

/**************************************************************************
	Here's where we do all the work
**************************************************************************/

int	GenerateFileSequence(HWND hwnd, char *Filename, int NumInsertedFrames)
    {
    char    JPEGFilename[MAX_PATH];
//    char    *fileptr;
    char    status[240];
    double  RezizeValue[MAX_INSERTED_FRAMES];
    double  Zoom1, Zoom2;
    double ratio;
    ResizeStruct	RESIZE = { 1.0, 1.0, 1, 1, false };
    CDib    TempDib;
    RECT    SelectRect;
    int	    FrameCount = 0;

    State = BATCH;

    InitFileList();							// init view next file if directory changes
    LoadFileList(hwnd, Filename);
    SortFileList();
    if (NumInsertedFrames > 0)						// we need to calulate the relative zoom level for each inserted frame
	{
	if (decode_png_header(hwnd, FileList[0].FileName, "ManpWIN Sequence Generator") < 0)		// get zoom for first frame
	    return -1;
	if (Dib.InitDib(width, height, bits_per_pixel) == NULL)
	    {
	    switch (Dib.DibErrorCode)
		{
		case NODIBMEMORY:
		    MessageBox(hwnd, "Can't allocate DIB memory. Using windows default palette", szAppName, MB_ICONEXCLAMATION | MB_OK);
		    break;
		case NOPIXELMEMORY:
		    MessageBox(hwnd, "Can't allocate memory for pixels", szAppName, MB_ICONEXCLAMATION | MB_OK);
		    break;
		}
	    return -1;
	    }
	else
	    memset(Dib.DibPixels, 0, WIDTHBYTES((DWORD)width * (DWORD)bits_per_pixel) * (DWORD)height);
	if (png_decoder(hwnd, FileList[0].FileName, "ManpWIN Sequence Generqator", &Zoom1, true) < 0)
	    return -1;

	if (decode_png_header(hwnd, FileList[1].FileName, "ManpWIN Sequence Generator") < 0)		// get zoom for second frame
	    return -1;

	if (png_decoder(hwnd, FileList[1].FileName, "ManpWIN Sequence Generqator", &Zoom2, true) < 0)
	    return -1;

	if (Zoom1 <= 0.0 || Zoom2 <= 0.0 || Zoom1 == Zoom2)
	    UseScaling = false;
	else
	    {
	    ratio = Zoom1 / Zoom2;
	    for (int i = 0; i < NumInsertedFrames; i++)
		RezizeValue[i] = pow(ratio, (double)i / NumInsertedFrames);
	    }
	}
    for (int i = 0; i < filecount; ++i)
//    for (int i = 0; i < 25; ++i)
	{
	user_data();
	if (State == NORMAL)
	    break;
	strcpy(Filename, FileList[i].FileName);
	if (decode_png_header(hwnd, Filename, "ManpWIN Sequence Generator") < 0)
	    return -1;
	xdots = width;
	ydots = height;

	SetupView(hwnd);
	if (Dib.InitDib(width, height, bits_per_pixel) == NULL)
	    {
	    switch (Dib.DibErrorCode)
		{
		case NODIBMEMORY:
		    MessageBox(hwnd, "Can't allocate DIB memory. Using windows default palette", szAppName, MB_ICONEXCLAMATION | MB_OK);
		    break;
		case NOPIXELMEMORY:
		    MessageBox(hwnd, "Can't allocate memory for pixels", szAppName, MB_ICONEXCLAMATION | MB_OK);
		    break;
		}
	    return -1;
	    }
	else
	    memset(Dib.DibPixels, 0, WIDTHBYTES((DWORD)width * (DWORD)bits_per_pixel) * (DWORD)height);

	double	dummy;
	if (png_decoder(hwnd, Filename, "ManpWIN Sequence Generator", &dummy, false) < 0)
	    return -1;

	if (NumInsertedFrames > 0)
	    {
	    TempDib = Dib;
	    for (int j = 0; j < NumInsertedFrames; j++)
		{
		if (j > 0)	// don't resize original frame
		    { 
		    if (UseScaling)
			{
			RESIZE.HorResizeValue = RESIZE.VertResizeValue = RezizeValue[j];
			ResizeDib(&Dib, RESIZE, true);
			SelectRect.left = (Dib.DibWidth - TempDib.DibWidth) / 2;
			SelectRect.right = SelectRect.left + TempDib.DibWidth;
			SelectRect.top = (Dib.DibHeight - TempDib.DibHeight) / 2;
			SelectRect.bottom = SelectRect.top + TempDib.DibHeight;
			NormalizeRect(SelectRect);
			Crop(hwnd, SelectRect);
			}
		    }
		FrameCount = i * NumInsertedFrames + j;
		sprintf(status, "Creating Frame %d", FrameCount);
		SetWindowText(hwnd, status);					// Show formatted text in the caption bar
		sprintf(JPEGFilename, "%s\\Frame%05d.jpg", OutputPath, i * NumInsertedFrames + j);
		if (write_jpg_file(hwnd, JPEGFilename, Dib.DibPixels, FALSE, Dib.DibWidth, Dib.DibHeight, Dib.BitsPerPixel) < 0)
		    return -1;
		Dib = TempDib;
		}
	    }
	else
	    {
	    FrameCount = i;
	    sprintf(status, "Creating Frame %d", FrameCount);
	    SetWindowText(hwnd, status);					// Show formatted text in the caption bar
	    sprintf(JPEGFilename, "%s\\Frame%05d.jpg", OutputPath, i);
	    if (write_jpg_file(hwnd, JPEGFilename, Dib.DibPixels, FALSE, Dib.DibWidth, Dib.DibHeight, Dib.BitsPerPixel) < 0)
		return -1;
	    }
	InvalidateRect(hwnd, &r, FALSE);
	}

    if (NumEndFrames > 0)
	{
	for (int j = 0; j < NumEndFrames; j++)
	    {
	    sprintf(status, "Creating Frame %d", j + FrameCount);
	    SetWindowText(hwnd, status);					// Show formatted text in the caption bar
	    sprintf(JPEGFilename, "%s\\Frame%05d.jpg", OutputPath, j + FrameCount);
	    if (write_jpg_file(hwnd, JPEGFilename, Dib.DibPixels, FALSE, Dib.DibWidth, Dib.DibHeight, Dib.BitsPerPixel) < 0)
		return -1;
	    }
	}

    return 0;
    }
