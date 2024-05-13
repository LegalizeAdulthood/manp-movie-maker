/* PNGVIEW.CPP
 *
 */

/*
#ifndef PGV4
#define UNICODE
#define _UNICODE
#endif
*/

#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <conio.h>
#include <string.h>
#include <dos.h>
#include <windows.h>
#include <fcntl.h>
#include "view.h"
#define PNG_READ_oFFs_SUPPORTED
#define PNG_INTERNAL
#define	PNG_SETJMP_SUPPORTED
#include ".\pnglib\png.h"
#include ".\pnglib\pngstruct.h"
#include "FileStats.h"
#include "Dib.h"
#include "FileData.h"				// structure definitions for file in memory 
#include "FileOps.h"

// Compression factor -- this should be:
// 1 -- fastest writes, worst compression
// 9 -- slowest writes, best compression
// Z_DEFAULT_COMPRESSION (6) -- somewhere in the middle
#define	PNG_COMPRESSION	Z_DEFAULT_COMPRESSION
#define PNG_READ_tEXt_SUPPORTED
#define PNG_READ_zTXt_SUPPORTED
#define PNG_WRITE_tEXt_SUPPORTED
#define PNG_WRITE_zTXt_SUPPORTED

#define	ALLOW_PNG_WARNINGS			// comment this to disable diagnosing warnings

						// routines in this module
int	png_decoder(FILE *);
int	load_png_palette(WORD);

extern	void	ShowMessage(char *);
extern	void	ShowOutfileMessage(char *);
extern	char	*AddThumbNailData(void);
extern	int	GetThumbNailData(void);
extern	int	atox(char c);

typedef unsigned long   ULONG;

//////////////////////////////////////////////////////////////////////////////////////////////////////
extern	HWND	hwnd;					// This is the main windows handle
//////////////////////////////////////////////////////////////////////////////////////////////////////

// Global Variables:
extern	CDib		Dib;				// Device Independent Bitmap class instance
extern	CFileStats	FileStats;			// Class for file info
extern	HCURSOR		hStdCursor;
extern	TCHAR		ShortName[];			// name of file to be displayed

char	PNG_error_buffer[255];
extern "C"  jmp_buf	mark;				// Address for long jump to jump to when an error occurs
extern "C"  struct	FileDataStruct	FileMemData;	// holds the pointers to the file data in memory
extern "C"  png_text	TextData[];

extern	void	GetOrthoPalette(BYTE *);

BYTE	PNGbackground[] = {0, 0, 0};			// Back ground colour specified by the user.
BOOL	PNGBGColour = FALSE;				// Use back ground colour specified by the user. If FALSE, use default.

static	BOOL	IsAlpha;
static	WORD	number_passes;

static	png_structp	read_ptr;
static	png_infop	read_info_ptr, end_info_ptr;
static	png_structp	write_ptr = NULL;
static	png_infop	write_info_ptr = NULL;
static	png_infop	write_end_info_ptr = NULL;
static	int		num_pass, pass;
static	int		bit_depth, color_type, channels;
static	jmp_buf		jmpbuf;
static	png_textp	text_ptr;

/**************************************************************************
	Error Handling
**************************************************************************/

// This function is called when there is a warning, but the library thinks it can continue anyway.  Replacement functions don't have to do anything
// here if you don't want to.  In the default configuration, png_ptr is not used, but it is passed in case it may be useful.

static	void	png_default_warning(png_structp png_ptr, png_const_charp message)
    {
    PNG_CONST char *name = "UNKNOWN (ERROR!)";
    if (png_ptr != NULL && png_ptr->error_ptr != NULL)
	name = (char *)png_ptr->error_ptr;
    sprintf(PNG_error_buffer, "PNGLib warning: %s\n", message);
    }

// This is the default error handling function.  Note that replacements for this function MUST NOT RETURN, or the program will likely crash.  This
// function is used by default, or if the program supplies NULL for the error function pointer in png_set_error_fn().

static	void	png_default_error(png_structp png_ptr, png_const_charp message)
    {
    static	char	s[120];
    png_default_warning(png_ptr, message);
    sprintf(s, "Error: <%s> in PNG file: ", PNG_error_buffer);
    // We can return because png_error calls the default handler, which is actually OK in this case.
    }

/**************************************************************************
	File Reading
**************************************************************************/

/* START of code to validate stdio-free compilation */
/* These copies of the default read/write functions come from pngrio.c and */
/* pngwio.c.  They allow "don't include stdio" testing of the library. */
/* This is the function that does the actual reading of data.  If you are
   not reading from a standard C stream, you should create a replacement
   read_data function and use it at run time with png_set_read_fn(), rather
   than changing the library. */

static	void	png_default_read_data(png_structp read_ptr, png_bytep data, png_size_t length)
    {
    png_size_t check;

    // fread() returns 0 on error, so it is OK to store this in a png_size_t instead of an int, which is what fread() actually returns.

    check = (png_size_t)fread(data, (png_size_t)1, length, (FILE *)read_ptr->io_ptr);
    if (check != length)
	{
	png_error(read_ptr, "Read Error");
	}
    }

/**************************************************************************
	Main entry decoder
**************************************************************************/

int	PNGHeader(FILE *fp, WORD *ImageHeight, WORD *ImageWidth, WORD *Bits)
    {
    char	s[480];
    int		interlace_type, compression_type, filter_type;

    // allocate the necessary structures
    // Allocating read structures
    read_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, png_default_error, png_default_warning);
    if (read_ptr == NULL)
	return -1;

    // Allocating read_info and end_info structures
    read_info_ptr = png_create_info_struct(read_ptr);
    if (read_info_ptr == NULL)
	{
	png_destroy_read_struct(&read_ptr, (png_infopp)NULL, (png_infopp)NULL);
	return -1;
	}
    end_info_ptr = png_create_info_struct(read_ptr);
    if (end_info_ptr == NULL)
	{
	png_destroy_read_struct(&read_ptr, &read_info_ptr, (png_infopp)NULL);
	return (ERROR);
	}

    // Setting jmpbuf for read struct
    if (setjmp(png_jmpbuf(read_ptr)))
	{
	sprintf(s, "Error: %s in PNG file ", PNG_error_buffer);
	ShowMessage(s);
	png_destroy_read_struct(&read_ptr, &read_info_ptr, &end_info_ptr);
	fclose(fp);
	return (-1);
	}

    // Initializing input stream
    png_set_read_fn(read_ptr, (png_voidp)fp, png_default_read_data);

    // Reading info struct
    png_read_info(read_ptr, read_info_ptr);
    png_get_IHDR(read_ptr, read_info_ptr, (png_uint_32 *)ImageWidth, (png_uint_32 *)ImageHeight, &bit_depth, &color_type, &interlace_type, &compression_type, &filter_type);
    // flip the rgb pixels to bgr 
    if (read_ptr->color_type == PNG_COLOR_TYPE_RGB || read_ptr->color_type == PNG_COLOR_TYPE_RGB_ALPHA)
	png_set_bgr(read_ptr);

    //	PNG_COLOR_MASK_PALETTE    1
    //	PNG_COLOR_MASK_COLOR      2
    //	PNG_COLOR_MASK_ALPHA      4

    // color types.  Note that not all combinations are legal
    //	PNG_COLOR_TYPE_GRAY 0
    //	PNG_COLOR_TYPE_PALETTE  (PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_PALETTE)
    //	PNG_COLOR_TYPE_RGB        (PNG_COLOR_MASK_COLOR)
    //	PNG_COLOR_TYPE_RGB_ALPHA  (PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_ALPHA)
    //	PNG_COLOR_TYPE_GRAY_ALPHA (PNG_COLOR_MASK_ALPHA)
    // aliases
    //	PNG_COLOR_TYPE_RGBA  PNG_COLOR_TYPE_RGB_ALPHA
    //	PNG_COLOR_TYPE_GA  PNG_COLOR_TYPE_GRAY_ALPHA

    IsAlpha = (color_type & PNG_COLOR_MASK_ALPHA);

    wsprintf(FileStats.filetype, "Portable Network Graphics");
    switch (color_type)
	{
	case	PNG_COLOR_TYPE_GRAY:
	    strcat(FileStats.filetype, ", Greyscale");
	    break;
	case	PNG_COLOR_TYPE_PALETTE:
	    strcat(FileStats.filetype, ", Palette Colour");
	    break;
	case	PNG_COLOR_TYPE_RGB:
	    strcat(FileStats.filetype, ", RGB Colour");
	    break;
	case	PNG_COLOR_TYPE_RGB_ALPHA:
	    strcat(FileStats.filetype, ", RGB+Alpha Colour");
	    break;
	case	PNG_COLOR_TYPE_GRAY_ALPHA:
	    strcat(FileStats.filetype, ", Greyscale+Alpha");
	}

    FileStats.bits_per_pixel = read_ptr->pixel_depth;
    FileStats.height = read_ptr->height;
    FileStats.width = read_ptr->width;

// Set up the data transformations you want.  Note that these are all optional.  Only call them if you want/need them.  Many of the
// transformations only work on specific types of images, and many are mutually exclusive.

    if (bit_depth == 16)
	{
	png_set_strip_16(read_ptr);
	bit_depth = 8;
	}

    // Extract multiple pixels with bit depths of 1, 2 or 4 from a single byte into separate bytes (useful for paletted and grayscale images).
    if (bit_depth < 8)				// pack pixels into bytes
	png_set_packing(read_ptr);

    if (color_type & PNG_COLOR_MASK_COLOR)	// flip the rgb pixels to bgr
	png_set_bgr(read_ptr);

    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
	png_set_gray_to_rgb(read_ptr);		// grayscale to RGB

    if (color_type & PNG_COLOR_TYPE_PALETTE)	// Expand paletted colors into true RGB triplets
	png_set_palette_to_rgb(read_ptr);

    bool    IsTransparent = (png_get_valid(read_ptr, read_info_ptr, PNG_INFO_tRNS) != 0);
    if (IsTransparent)
	png_set_tRNS_to_alpha(read_ptr);

 // Set the background color to draw transparent and alpha images over
    if ((color_type == PNG_COLOR_TYPE_GRAY_ALPHA || color_type == PNG_COLOR_TYPE_RGB_ALPHA) && IsAlpha && bit_depth == 8)
	{
	png_color_16	temp_background;
	if (!PNGBGColour)
	    // initialise background colour to black
	    memset(&temp_background, 0x00, sizeof(png_color_16));
	else
	    {
	    //	    memcpy(&temp_background, &my_background, sizeof(png_color_16));
	    temp_background.red = PNGbackground[0];
	    temp_background.green = PNGbackground[1];
	    temp_background.blue = PNGbackground[2];
	    temp_background.gray = 0;
	    }
	png_set_background(read_ptr, &temp_background, PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);
	}

    number_passes = png_set_interlace_handling(read_ptr);

    if (number_passes > 1)
	strcat(FileStats.filetype, ", Interlaced Image");

    if (IsAlpha)
	strcat(FileStats.filetype, ", Alpha Channel");

    //info_ptr will return the correct transformed values.
    png_read_update_info(read_ptr, read_info_ptr);

    *ImageWidth = (WORD)(read_ptr->width);			// with of file
    *ImageHeight = (WORD)(read_ptr->height);			// height of file
    channels = png_get_channels(read_ptr, read_info_ptr);	// number of channels of info for the color type(valid values are 1 (GRAY, PALETTE), 2 (GRAY_ALPHA), 3 (RGB),4 (RGB_ALPHA or RGB + filler byte))
    bit_depth = png_get_bit_depth(read_ptr, read_info_ptr);	// number of bits per channel
    *Bits = bit_depth * channels;				// number of bits per pixel
    load_png_palette(*Bits);
    return 0;
    }

/**************************************************************************
	Decode the PNG Image
 *************************************************************************/

int   PNGDecoder(BOOL ReadThumbData, FILE *fp)

    {
    DWORD	linesize;
    char	s[480];
    long	i;
    int		num_text;
    long	bytes;
    long	bytes_per_pixel;
    BYTE	*LinePtr = Dib.DibPixels;
    png_bytep	*row_pointers;

    // Setting jmpbuf for read struct
    if (setjmp(png_jmpbuf(read_ptr)))
	{
	sprintf(s, "Error: %s in PNG file ", PNG_error_buffer);
	ShowMessage(s);
	png_destroy_read_struct(&read_ptr, &read_info_ptr, &end_info_ptr);
	return (-1);
	}

    bytes = WIDTHBYTES((DWORD)read_ptr->width * (DWORD)Dib.BitsPerPixel);
    bytes_per_pixel = bytes / (DWORD)read_ptr->width;
    linesize = (Dib.BitsPerPixel > 8) ? (WORD)WIDTHBYTES((DWORD)Dib.DibWidth * (DWORD)Dib.BitsPerPixel) : Dib.DibWidth;


    if (IsAlpha)
	{
	if (bytes_per_pixel == 4)		// RGB + Alpha
	    Dib.AlphaChannel = RGBA1;		// PHD 2020-09-03 changed from RGBA to resolve conflict with WebP library
	else if (bytes_per_pixel == 2)		// Grey + Alpha
	    Dib.AlphaChannel = GREYA;
	}
    else
	Dib.AlphaChannel = NOALPHA;


    linesize = (DWORD)WIDTHBYTES((DWORD)Dib.DibWidth * (DWORD)Dib.BitsPerPixel);
    if ((row_pointers = new png_bytep[Dib.DibHeight]) == NULL)
	{
	// free the structures 
	if (read_ptr != NULL)
	    png_destroy_read_struct(&read_ptr, &read_info_ptr, &end_info_ptr);
	fclose(fp);
	sprintf(s, "Not enough memory: %d bytes for line buffer in PNG file: ", linesize);
	ShowMessage(s);
	return -1;
	}

    for (i = Dib.DibHeight - 1; i >= 0; i--)
	{
	row_pointers[i] = (png_bytep)LinePtr;
	LinePtr += linesize;
	}
    png_read_image(read_ptr, row_pointers, hwnd);

    // read the rest of the file, getting any additional chunks in info_ptr 
    png_read_end(read_ptr, read_info_ptr);

    text_ptr = &TextData[0];
    if (png_get_text(read_ptr, read_info_ptr, &text_ptr, &num_text) > 0)
	{
	memcpy(&TextData[0], text_ptr, sizeof(TextData[0]));
	if (ReadThumbData)					// PNG text is actually Thumbnail sheet data
	    {
	    FileMemData.FileDataPtr = (BYTE *)TextData[0].text;		// use this pointer to get into the TNS database interpreter in thumbs.cpp
	    FileMemData.EndofFilePtr = FileMemData.FileDataPtr + TextData[0].text_length;
	    if (GetThumbNailData() < 0)
		ShowMessage("Can't read TNS database info from file ");
	    }
	else							// PNG text contains file comments.
	    {
	    strcpy(FileStats.CommentKey, TextData[0].key);
	    if (TextData[0].text_length < COMMENTSIZE)
		{
		strncpy(FileStats.comment, TextData[0].text, TextData[0].text_length);
		*(FileStats.comment + TextData[0].text_length) = '\0';
		}
	    else
		{
		strncpy(FileStats.comment, TextData[0].text, COMMENTSIZE - 1);
		*(FileStats.comment + COMMENTSIZE - 1) = '\0';
		}
	    }
	}

    // clean up after the read, and free any memory allocated 
    png_destroy_read_struct(&read_ptr, &read_info_ptr, &end_info_ptr);

    // free the structures and line buffer
    if (row_pointers != NULL)
	delete [] row_pointers;

    //    close the file 
    fclose(fp);

    // that's it 
    return(0);
    }

/***************************************************************************
	Load palette data registers
***************************************************************************/

int	load_png_palette(WORD Bits)

    {
    int	i;
    long	temp;

    if (read_ptr->color_type == PNG_COLOR_TYPE_GRAY_ALPHA || read_ptr->color_type == PNG_COLOR_TYPE_GRAY)
	{
	switch(Bits)
	    {
	    case 1:
		memset(VGA_PALETTE, 0, 3);
		memset(VGA_PALETTE + 3, 255, 3);
		break;

	    case 2:
		memset(VGA_PALETTE, 0, 3);
		memset(VGA_PALETTE + 3, 84, 3);
		memset(VGA_PALETTE + 6, 168, 3);
		memset(VGA_PALETTE + 9, 255, 3);
		break;

	    default:
		if (Bits <= 8)
		    {
		    for (i = 0; i < (1 << Bits); ++i)
			{
			temp = 255L * (long)i / (long) (1 << Bits);
			*(VGA_PALETTE + i * RGB_SIZE + 0) = (BYTE)temp;
			*(VGA_PALETTE + i * RGB_SIZE + 1) = (BYTE)temp;
			*(VGA_PALETTE + i * RGB_SIZE + 2) = (BYTE)temp;
			}
		    }
		break;
	    }
	}
    else if (Bits <= 8 && read_ptr->palette)
	memcpy(VGA_PALETTE, read_ptr->palette, RGB_SIZE * read_ptr->num_palette);
    else
	GetOrthoPalette(VGA_PALETTE);

    return 0;
    }

/**************************************************************************
	Write Bitmap Image to Portable Network Graphics (PNG) File
**************************************************************************/

int	write_png_file(FILE *fp, BOOL WriteThumbFile)

    {
    char		s[MAXLINE];
    BYTE		**row_pointers = NULL;
    //FILE		*fp = NULL;
    WORD		i;
    WORD		width, height;
    png_text		TextInfo;

    static	HCURSOR	hOldCursor;

    hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    width = Dib.DibWidth;
    height = Dib.DibHeight;

    write_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (write_ptr == NULL)
	return -1;

    // Allocating write_info and end_info structures
    write_info_ptr = png_create_info_struct(write_ptr);
    if (write_info_ptr == NULL)
	{
	png_destroy_write_struct(&write_ptr, NULL);
	return -1;
	}
    write_end_info_ptr = png_create_info_struct(write_ptr);
    if (write_end_info_ptr == NULL)
	{
	png_destroy_read_struct(&write_ptr, &write_info_ptr, &write_end_info_ptr);
	return -1;
	}

    // Setting jmpbuf for write struct
    if (setjmp(png_jmpbuf(write_ptr)))
	{
	sprintf(s, "Error: %s in PNG file ", PNG_error_buffer);
	ShowMessage(s);
	png_destroy_write_struct(&write_ptr, &write_info_ptr);
	return (-1);
	}

    png_init_io(write_ptr, fp);
    png_set_compression_level(write_ptr, PNG_COMPRESSION);
    bit_depth = 8;
    color_type = PNG_COLOR_TYPE_RGB;
    png_set_bgr(write_ptr);
    png_set_IHDR(write_ptr, write_info_ptr, width, height, bit_depth, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    write_ptr->width = width;
    write_ptr->height = height;
    if (Dib.BitsPerPixel <= 8)
	{
	png_set_PLTE(write_ptr, write_info_ptr, (struct png_color_struct *)VGA_PALETTE, 1 << Dib.BitsPerPixel);
	write_ptr->bit_depth = (BYTE)Dib.BitsPerPixel;
	write_ptr->color_type = PNG_COLOR_TYPE_PALETTE;
	write_ptr->channels = 1;
	}
    else
	{
	write_ptr->bit_depth = 8;
	write_ptr->color_type = PNG_COLOR_TYPE_RGB;
	write_ptr->channels = 3;
	png_set_bgr(write_ptr);
	}

    png_write_info(write_ptr, write_info_ptr);
    if ((row_pointers = (BYTE **) new BYTE[height * sizeof(BYTE *)]) == NULL)

 //   if ((row_pointers = (BYTE **)FixedGlobalAlloc(height * sizeof(BYTE *))) == NULL)
	{
	if(fp != NULL)
	    fclose(fp);
	png_destroy_write_struct(&write_ptr, &write_info_ptr);
	wsprintf(s, "Error: can't allocate memory for row pointer in PNG file ", PNG_error_buffer);
	ShowOutfileMessage(s);
	return -1;
	}

    for (i = 0; i < height; ++i)
	row_pointers[height - i - 1] = Dib.DibPixels + (long)i * (long)(WIDTHBYTES(width * Dib.BitsPerPixel));

    png_write_image(write_ptr, row_pointers, hwnd);

	     /* write uncompressed chunk */
    if (WriteThumbFile)					// we are using this as a thumb nail database
	{
	TextInfo.compression = PNG_TEXT_COMPRESSION_NONE;	// compression value
	TextInfo.key = "TNS Data";				// keyword, 1-79 character description of "text"
	TextInfo.text = AddThumbNailData();			// comment, may be an empty string (ie "")
	TextInfo.text_length = strlen(TextInfo.text);	// length of "text" field
	png_set_text(write_ptr, write_info_ptr, &TextInfo, 1);
	}

    TextInfo.compression = PNG_TEXT_COMPRESSION_NONE;	// compression value
    if (FileStats.CommentKey[0])
	TextInfo.key = FileStats.CommentKey;
    else
	TextInfo.key = "Comment";			// keyword, 1-79 character description of "text"
    TextInfo.text = FileStats.comment;			// comment, may be an empty string (ie "")
    TextInfo.text_length = strlen(FileStats.comment);	// length of "text" field

    png_set_text(write_ptr, write_info_ptr, &TextInfo, 1);

    png_write_end(write_ptr, write_info_ptr);
    png_destroy_write_struct(&write_ptr, &write_info_ptr);
    SetCursor(hStdCursor);

    if(fp != NULL)
	fclose(fp);
    if (row_pointers != NULL)
	delete [] row_pointers; row_pointers = NULL;
    return 0;
    }

/*-----------------------------------------
	Update PNG background Colour Dialogue Box
  -----------------------------------------*/

DLGPROC PASCAL PNGBackgroundDlg (HWND hDlg, UINT message, UINT wParam, LONG lParam)
    {
    HWND	hCtrl ;
    static	HANDLE  hCursor;
    static	BYTE	temp;
    static	UINT	tempParam;
    static	char	PNGColourString[20];		// hold the RGB pixel info in here

    switch(message)
	{
	case WM_INITDIALOG:
	    sprintf(PNGColourString, "%02X%02X%02X", PNGbackground[0], PNGbackground[1], PNGbackground[2]);
//	    hCtrl = GetDlgItem (hDlg, IDC_BACKGROUND);
	    SetDlgItemText(hDlg, IDC_BACKGROUND, PNGColourString);
	    hCtrl = GetDlgItem (hDlg, IDC_CHANGEBG) ;
	    SendMessage(hCtrl, BM_SETCHECK, PNGBGColour, 0L);
	    return (DLGPROC)TRUE;

	case WM_COMMAND:
	    switch(wParam)
		{
		case IDOK:
		    GetDlgItemText(hDlg, IDC_BACKGROUND, PNGColourString, 10);
		    PNGbackground[0] = atox(*PNGColourString) * 16 + atox(*(PNGColourString + 1));
		    PNGbackground[1] = atox(*(PNGColourString + 2)) * 16 + atox(*(PNGColourString + 3));
		    PNGbackground[2] = atox(*(PNGColourString + 4)) * 16 + atox(*(PNGColourString + 5));
//		    *ZTreeColourString = '\0';			// off by default
		    hCtrl = GetDlgItem (hDlg, IDC_CHANGEBG) ;
		    PNGBGColour = (BYTE)SendMessage(hCtrl, BM_GETCHECK, 0, 0L);
		    EndDialog (hDlg, IDOK);
		    return (DLGPROC)TRUE;

		case IDCANCEL:
		    EndDialog (hDlg, IDCANCEL);
		    return FALSE;
	    	}
	    break;
	}
    return FALSE ;
    }

