/*------------------------
   VIEW.H header file
  ------------------------*/

#include    "resource.h"
#include    "DibMacro.h"

// comment out the following line for registered version
//#define SHAREWARE		1

// comment out the following line for 16 bit version
#define WIN95			1			// 32 bit code

#ifdef	WIN95						// 32 bit code
#undef  huge
#define huge      
#undef  _export
#define _export
#endif

//#define	WINHELP		
//#undef	WINHELP		// comment this one out for WINHELP

// comment out the following line if the force single instance code is not needed
#define	SINGLEINSTANCE

#define DONTUPDATEHISTORY  	0		// we don't update history if making a collage or a thumbnail file
#define UPDATEHISTORY  		1
#define ONLYUPDATEDIRHISTORY  	2		// there's no point in updating file history if we are just viewing all files in a directory

#define FILE_BMP      		0
#define FILE_RLE     		1
#define FILE_GIF     		2
#define FILE_PCX     		3
#define FILE_JPG		4
#define FILE_TGA     		5
#define FILE_TIF     		6
#define FILE_CUR     		7
#define FILE_ICO     		8
#define FILE_ANI     		9
#define FILE_PNG     		10
#define FILE_RAW     		11		// all raw formats
#define FILE_THM     		12
#define FILE_CLG     		19
#define FILE_DB     		20
#define FILE_TNS     		21
#define FILE_LST     		22
#define FILE_MPG    		23
#define FILE_WEBP    		24		// Google web pictures format

#define TRUE			1
#define FALSE			0
#define EVER			(;;)
#define ERR			-1

#define CANCELBUTTON		0
#define OKBUTTON		1
#define GETCOLBUTTON		2

#define	CSIZE			8		// character size for text

#define NONE			0		// opening bitmap loading flags
#define DEFAULT			1
#define NEW			2
#define FILE_ERROR		3

#define SLIDESHOWTIMER		1
#define GIFANIMTIMER		2
#define EXITTIMER		3
#define BATCHTIMER		4
#define ZTWTIMER		5

#define PGVREGISTERED		0		// registration flags
#define UNREGISTERED		1
#define BADKEY			2

//#define	RGB_RED			0
//#define	RGB_GREEN		1
//#define	RGB_BLUE		2
//#define	RGB_SIZE		3

#define	WRGB_RED		2
#define	WRGB_GREEN		1
#define	WRGB_BLUE		0
			
#define	STRINGSIZE		260

#define	MAXFILES		100000
#define	LONG_PATH		MAX_PATH*2
#define	MAXHORIZONTAL		2500
#define	MAXVERTICAL		1000
#define	VGA_PAL_SIZE		768
#define	EGA_PAL_SIZE		16
#define VGA_COLOURS		256

#define	DITHERBRIGHTNESS	20
#define	DITHERCONTRAST		20

#define	MAXHISTORY		36	// how much directory history can we have?

//#define	PREVIEW_HEIGHT		177
//#define	PREVIEW_WIDTH		220

//#define	MAXANIM			1024	// maximum GIF animation frames
#define MAXLINE			640	// maximum length of message line (error or otherwise)

#define MINWINDOW		280	// Don't wrap menu around and lose part of the image

#define	MAXMESSAGE		10240	// big enough for System Resources message.
#define	MAXRESIZESTR		24	// big enough for System Resources message.

#ifdef __cplusplus
#define	FixedGlobalAlloc(n) 	new char[n]
#define	FixedGlobalFree(p)	delete [] p
//#define	FixedGlobalRealloc(p,n)	// not used
#else
#define	FixedGlobalAlloc(n) 	(char *)GlobalAlloc(GMEM_FIXED, n)
#define	FixedGlobalFree(p)	GlobalFree(p)
//#define	FixedGlobalRealloc(p,n) GlobalRealloc(p,n)
#endif

/* Macro to restrict a given value to an upper or lower boundary value */
#define BOUND(x,min,max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

/* Macro to align given value to the closest DWORD (unsigned long ) */
#define ALIGNULONG(i)   ((i+3)/4*4)

#define GET_2B(array,offset)  ((unsigned int) (BYTE)(array[offset]) + \
			       (((unsigned int) (BYTE)(array[offset+1])) << 8))
#define GET_4B(array,offset)  ((long) (BYTE)(array[offset]) + \
			       (((long) (BYTE)(array[offset+1])) << 8) + \
			       (((long) (BYTE)(array[offset+2])) << 16) + \
			       (((long) (BYTE)(array[offset+3])) << 24))

// the following macros allow conversion to and from wide characters to multibyte chars
#define	UTF8ToName(UTFString, Name)  MultiByteToWideChar(CP_UTF8,0,UTFString,-1, Name, MAX_PATH * sizeof(TCHAR));
#define	NameToUTF8(Name, UTFString)  WideCharToMultiByte(CP_UTF8,0,Name,(int)_tcslen(Name)+1,UTFString,MAX_PATH,NULL,NULL);

#ifndef	VIEWMAIN
#define VIEWMAIN

//extern	int		xdots, ydots;
extern	WORD		numcolors;			// colours in the file
extern	BYTE		VGA_PALETTE[]; 			// VGA colour palette
#ifdef __cplusplus
extern	LPBITMAPINFO	CreateEmptyDib(int Width, int Height, WORD bits);
extern	LPBITMAPINFO	CreateEmptyDib(int Width, int Height, BYTE *palette, WORD bits);
#endif
extern	void	SetDibPalette(LPBITMAPINFO pDib, BYTE *palette);

#ifdef __cplusplus
extern "C" {
#endif
extern	int	PutJPEGScanline(BYTE *, BYTE *, WORD, WORD, WORD, WORD, BOOL, BOOL);
#ifdef __cplusplus
    }
#endif

//extern	int	fdin;					// FILE descripter

//extern	DWORD 	PASCAL	lread (int, VOID far *, DWORD);
#endif

#define PI	3.14159265358979323846
#define	TWO_PI	6.28318530717958
#define	HALF_PI	1.570796326794895
#define	MAG_PI	4+20

#define RWIDTH(rect) (rect.right - rect.left)		// IB 2009-04-14
#define RHEIGHT(rect) (rect.bottom - rect.top)

#ifdef __cplusplus
extern "C" {
#endif

extern	void	HandleMessages(HWND, char *, char *, int);
extern	void	HandleWarnings(HWND, char *, char *, int);

#ifdef __cplusplus
    }
#endif

struct FileListStuff	// info for sorting file list
    {
    char	*FileName;
    DWORD	FileSize;
    int		width;
    int		height;
    FILETIME	LastWriteTime;
    };

struct AppData
   {
   BYTE *header;
   WORD size;
   BOOL active;
   };

// Let's set up a few state variables. These are mutually exclusive states that PGV can be in
enum	ControlType   {VIEW, REDEYE, CROP_RESIZE, ZOOMING, GETBKGRNDCOL, READPIXEL, ROTATE, 
		    TRAPEZOID, PARALLEL, BARREL, LOSSLESSCROP, MAKEICON, WRITEANIMFRAME, LOADANIMGIF, ADDTEXT, ADDTEXTREADY, INSERTIMAGE, INSERTIMAGEREADY, READMPEG};
enum	StateType   {NORMAL, ZAAPACTIVE, THUMBSHEET, THUMBNAIL, BATCH, SLIDESHOW, NEXTFILE, DITHER, READRAW, PREPAREINSERTIMAGE};

enum	SaveMethod  {USERSAVE, SAVELASTREAD, SAVECURRENTDIR};

enum	ExifKind  {NO_EXIF, JPEG_EXIF, RAW_EXIF, WEBP_EXIF};

#define AllowTIFF    // PHD 2020-09-19
