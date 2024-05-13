/*
* JPGVIEW.C a module to read *.JPG files.
*
* Written in Microsoft Visual C++ 'C' by Paul de Leeuw.
*
* This file illustrates how to use the IJG code as a subroutine library
* to read or write JPEG image files.  You should look at this code in
* conjunction with the documentation file libjpeg.doc.
*
* This code will not do anything useful as-is, but it may be helpful as a
* skeleton for constructing routines that call the JPEG library.
*
* We present these routines in the same coding style used in the JPEG code
* (ANSI function definitions, etc); but you are of course free to code your
* routines in a different style if you prefer.
*/

//#define MODIFYORIENTATION	// allow PGV the ability to modify the orientation flag in the EXIF header

//	2008-02-04  IB added Exif processor
//	2009-11-17  IB Save Exif to file

#include <stdio.h>
#include <windows.h>
#include <setjmp.h>
#include "view.h"
//#include "FileData.h"		// structure definitions for file in memory 

/*
* Include file for users of JPEG library.
* You will need to have included system headers that define at least
* the typedefs FILE and size_t before you can include jpeglib.h.
* (stdio.h is sufficient on ANSI-conforming systems.)
* You may also wish to include "jerror.h".
*/

#include ".\jpeglib\jpeglib.h"

/*
* <setjmp.h> is used for the optional error recovery mechanism shown in
* the second part of the example.
*/

#include ".\jpeglib\jerror.h"
#define MAXPIXELSIZE 160000000	// This is a bit of a cludge. When the number of pixels is greater, operator "new" crashes when allocating memory
#define COMMENTSIZE 1024	// IB why is it so big? PHD: what is the biggest comment you have ever seen?
				// I've seen some pretty long ones. PGV allows editing up to 1K of comment.

//////////////////////////////////////////////////////////////////////////////////////////////////////
//extern	HWND	hwnd;					// This is the main windows handle
//////////////////////////////////////////////////////////////////////////////////////////////////////
//extern HCURSOR	hStdCursor;

static	char	JPEGComment[COMMENTSIZE];	// assemble comment to pass to CFileInfo object
static	char	JPEGFiletype[100];		// assemble filetype to pass to CFileInfo object
static	BOOL	LargeImage = FALSE;		// When the number of pixels is too large, operator "new" crashes when allocating memory

	long	MaxPixelSize = MAXPIXELSIZE;	// see comment above for MAXPIXELSIZE
	int	sof_marker = 0;			// what sort of processing?
extern struct	AppData EXIFData;
	BOOL	ExifValid;			// PHD 2008-04-28 added ExifValid to prevent crash on lossless crop
	BYTE	*ExifHeader = NULL;		// pointer to Exif data
	int	SaveExifHeaderToFile;		// Do we save Exif header to JPEG file?
extern struct	AppData XMPData;
//	BOOL	XmpValid;
//extern	BYTE	*XmpHeader;			// pointer to XMP data
						// 1 = yes, 0 = no, -1 = error
extern	BOOL	GetExif(const BYTE *);
extern	BOOL	GetXMP(const BYTE *);
static	char	error_buffer[400];

extern	void	JPEGShowCaption(char *s, char *t);
extern	void	UpdateInfoCommentFiletypeBPS(char *, char *, int);
	char	PutInfoComment[64] = "JPEG File";
//	WORD	Orientation_Offset;		// how far into the EXIF data is the orientation flag?
extern	void	JPEGShowMessage(char *);
extern	void	ModifyOrientationFlag(WORD);

/* This struct contains the JPEG decompression parameters and pointers to
* working space (which is allocated as needed by the JPEG library).
*/
static	struct jpeg_decompress_struct cinfo;
// More stuff
JSAMPARRAY buffer;				// Output row buffer
//struct	FileDataStruct	FileMemData;		// holds the pointers to the file data in memory

/******************** JPEG DECOMPRESSION SAMPLE INTERFACE *******************/

/* This half of the example shows how to read data from the JPEG decompressor.
* It's a bit more refined than the above, in that we show:
*   (a) how to modify the JPEG library's standard error-reporting behavior;
*   (b) how to allocate workspace using the library's memory manager.
*
* Just to make this example a little different from the first one, we'll
* assume that we do not intend to put the whole image into an in-memory
* buffer, but to send it line-by-line someplace else.  We need a one-
* scanline-high JSAMPLE array as a work buffer, and we will let the JPEG
* memory manager allocate it for us.  This approach is actually quite useful
* because we don't need to remember to deallocate the buffer separately: it
* will go away automatically when the JPEG object is cleaned up.
*/

/*
* ERROR HANDLING:
*
* The JPEG library's standard error handler (jerror.c) is divided into
* several "methods" which you can override individually.  This lets you
* adjust the behavior without duplicating a lot of code, which you might
* have to update with each future release.
*
* Our example here shows how to override the "error_exit" method so that
* control is returned to the library's caller when a fatal error occurs,
* rather than calling exit() as the standard error_exit method does.
*
* We use C's setjmp/longjmp facility to return control.  This means that the
* routine which calls the JPEG library must first execute a setjmp() call to
* establish the return point.  We want the replacement error_exit to do a
* longjmp().  But we need to make the setjmp buffer accessible to the
* error_exit routine.  To do this, we make a private extension of the
* standard JPEG error handler object.  (If we were using C++, we'd say we
* were making a subclass of the regular error handler.)
*
* Here's the extended error handler struct:
*/

// setjmp now moved to viewmain.c to enable function to be active if error occurs. We only want to
// abort image, not exit the program PHD 28/4/2000

struct	my_error_mgr
    {
    struct	jpeg_error_mgr pub;			// "public" fields
    jmp_buf setjmp_buffer;				// for return to caller
    };

typedef struct	my_error_mgr	*my_error_ptr;

	jmp_buf	mark;			// Address (in viewmain.c) for long jump to jump to when an error occurs
	jmp_buf	ExifMark;		// Address (in exifview.c) for long jump to jump to when an error occurs

// We use our private extension JPEG error handler.
static	struct my_error_mgr jerr;

extern	BOOL	IsExifThumbnail;	// does the Exif header contain a thumbnail?
	BOOL	IsExifThumbnail = FALSE;	// does the Exif header contain a thumbnail?

void	    JPEGShowMessage(char *s)
    {

    }

/**************************************************************************
Handle JPEG Errors
**************************************************************************/

void	HandleJPEGError(void)
    {
    static  char	s[360];

    jpeg_destroy_decompress(&cinfo);	    // free the structures
    wsprintf(s, "Error: %s in JPEG file", error_buffer);
    JPEGShowMessage (s);
    }

/**************************************************************************
    Here's the routine that will replace the standard error_exit method
**************************************************************************/


BOOL	SetupExifJump(void)
    {
    return setjmp(ExifMark);
    }

void	my_error_exit(j_common_ptr cinfo)

    {
    // cinfo->err really points to a my_error_mgr struct, so coerce pointer
				// Always display the message.
				// We could postpone this until after returning, if we chose.

    (*cinfo->err->format_message) (cinfo, error_buffer);	// Create the message
    if (IsExifThumbnail)			// choose appropriate jump location
	longjmp(ExifMark, 1);		// Return control to the setjmp point
    else
	longjmp(mark, 1);			// Return control to the setjmp point
    }

/**************************************************************************
* Marker processor for COM and interesting APPn markers.
* This replaces the library's built-in processor, which just skips the marker.
* We want to print out the marker as text, to the extent possible.
* Note this code relies on a non-suspending data source.
**************************************************************************/

/**************************************************************************
    Generate an output line
**************************************************************************/

int	GenerateLinebuf(BYTE *decoderline, BYTE *JPEGPixels, int line, short *percent, WORD *height, WORD *width, WORD *bits_per_pixel)
    {
    int	i, localwidth;
    DWORD	startaddr;
    int	display_count;
    BYTE	c;

    display_count = (10 * line) / *height;
    localwidth = (*bits_per_pixel < 24) ? *width : (*bits_per_pixel == 24) ? *width * 3 : *width * 4;
    startaddr = WIDTHBYTES((DWORD)*width * (DWORD)*bits_per_pixel) * (*height - line - 1);
    memcpy(decoderline, JPEGPixels + startaddr, localwidth);
    for (i = 0; i < *width; ++i)	    // swap dem colours
	{
	c = *(decoderline + i * 3 + 2);
	*(decoderline + i * 3 + 2) = *(decoderline + i * 3);
	*(decoderline + i * 3) = c;
	}
    return 0;
    }

/**************************************************************************
	Write Bitmap Image to JPEG File
**************************************************************************/

    int	SaveJPEG(HWND hwnd, FILE *outfp, BYTE *JPEGPixels, BOOL WriteThumbFile, int quality, WORD *height, WORD *width, WORD *bits_per_pixel)
	{
	char		s[MAXLINE];
	short		percent;		// loading sequence
	//int		fh;
	//OFSTRUCT	of;

	/******************** JPEG COMPRESSION SAMPLE INTERFACE *******************/

	/* This half of the example shows how to feed data into the JPEG compressor.
	* We present a minimal version that does not worry about refinements such
	* as error recovery (the JPEG code will just exit() if it gets an error).
	*/

	/*
	* IMAGE DATA FORMATS:
	*
	* The standard input image format is a rectangular array of pixels, with
	* each pixel having the same number of "component" values (color channels).
	* Each pixel row is an array of JSAMPLEs (which typically are unsigned chars).
	* If you are working with color data, then the color values for each pixel
	* must be adjacent in the row; for example, R,G,B,R,G,B,R,G,B,... for 24-bit
	* RGB color.
	*
	* For this example, we'll assume that this data structure matches the way
	* our application has stored the image in memory, so we can just pass a
	* pointer to our image buffer.  In particular, let's say that the image is
	* RGB color and is described by:
	*/

	//extern JSAMPLE * image_buffer;	/* Points to large array of R,G,B-order data */
	//extern int image_height;	/* Number of rows in image */
	//extern int image_width;		/* Number of columns in image */

	/*
	* Sample routine for JPEG compression.  We assume that the target file name
	* and a compression quality factor are passed in.
	*/

	//GLOBAL(void)
	//write_JPEG_file (char * filename, int quality)

	/* This struct contains the JPEG compression parameters and pointers to
	* working space (which is allocated as needed by the JPEG library).
	* It is possible to have several such structures, representing multiple
	* compression/decompression processes, in existence at once.  We refer
	* to any one struct (and its associated working data) as a "JPEG object".
	*/
	struct jpeg_compress_struct cinfo;
	/* This struct represents a JPEG error handler.  It is declared separately
	* because applications often want to supply a specialized error handler
	* (see the second half of this file for an example).  But here we just
	* take the easy way out and use the standard error handler, which will
	* print a message on stderr and call exit() if compression fails.
	* Note that this struct must live as long as the main JPEG parameter
	* struct, to avoid dangling-pointer problems.
	*/
	struct jpeg_error_mgr jerr;
	/* More stuff */

	//FILE * outfp;			/* target file */
	//JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
	//int row_stride;		/* physical row width in image buffer */

	BYTE	*decoderline;        		// decoded line goes here
	DWORD	linesize;
#ifdef MODIFYORIENTATION	// allow modification of orientation flag in the EXIF header before saving
	short	NewOrientation;
#endif MODIFYORIENTATION	// allow modification of orientation flag in the EXIF header before saving

	/* Step 1: allocate and initialize JPEG compression object */

	/* We have to set up the error handler first, in case the initialization
	* step fails.  (Unlikely, but it could happen if you are out of memory.)
	* This routine fills in the contents of struct jerr, and returns jerr's
	* address which we place into the link field in cinfo.
	*/
	cinfo.err = jpeg_std_error(&jerr);
	// We set up the normal JPEG error routines, then override error_exit.
	jerr.error_exit = my_error_exit;
	/* Now we can initialize the JPEG compression object. */
	jpeg_create_compress(&cinfo);

	linesize = *width * 3;		// even if 8 bits/pixel, need to alloc enough for 24 bit output!!
	if((decoderline = (BYTE *)FixedGlobalAlloc(linesize)) == NULL)
	    {
	    wsprintf(s, "Not enough memory: %d bytes for line buffer in JPEG file ", linesize);
	    JPEGShowMessage(s);
	    return -1;
	    }

	/* Step 2: specify data destination (eg, a file) */
	/* Note: steps 2 and 3 can be done in either order. */

	/* Here we use the library-supplied code to send compressed data to a
	* stdio stream.  You can also write your own code to do something else.
	* VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
	* requires it in order to write binary files.
	*/

	if (WriteThumbFile)					// we are using this as a thumb nail database
	    {
	    wsprintf(s, "JPEG file no longer supports TNS format", linesize);
	    JPEGShowMessage(s);
	    return -1;
	    }
//	    AddThumbNailData(outfp);

	jpeg_stdio_dest(&cinfo, outfp);

	/* Step 3: set parameters for compression */

	/* First we supply a description of the input image.
	* Four fields of the cinfo struct must be filled in:
	*/
	cinfo.image_width = *width; 		/* image width and height, in pixels */
	cinfo.image_height = *height;
	//  cinfo.image_width = image_width; 	/* image width and height, in pixels */
	//  cinfo.image_height = image_height;
	cinfo.input_components = 3;		/* # of color components per pixel */
	cinfo.in_color_space = JCS_RGB; 	/* colorspace of input image */

	/* Now use the library's routine to set default compression parameters.
	* (You must set at least cinfo.in_color_space before calling this,
	* since the defaults depend on the source color space.)
	*/
	jpeg_set_defaults(&cinfo);
	/* Now you can set any non-default parameters you wish to.
	* Here we just illustrate the use of quality (quantization table) scaling:
	*/
	jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);

	/* Step 4: Start compressor */

	// OK, now if we just happen to change the orientation of the image, we need to be able to reflect this in
	// the EXIF header.
#ifdef MODIFYORIENTATION	// allow modification of orientation flag in the EXIF header before saving
	NewOrientation = 5;	// just a test for now.
	ModifyOrientationFlag(NewOrientation);		// update the orientation flag in the EXIF header
#endif MODIFYORIENTATION	// allow modification of orientation flag in the EXIF header before saving

	// TRUE ensures that we will write a complete interchange-JPEG file.
	// Pass TRUE unless you are very sure of what you're doing.

	jpeg_start_compress(&cinfo, TRUE);

	/* Step 4 and a bit: Add comment */
	// IB 2009-11-17 Save Exif to file
/*
	if(SaveExifHeaderToFile == 1 && EXIFData.active)	// PHD 2008-04-28 added ExifValid to prevent crash on lossless crop
//	if(SaveExifHeaderToFile == 1 && ExifValid)	// PHD 2008-04-28 added ExifValid to prevent crash on lossless crop
	    {
//	    int	length = *(ExifHeader + 2) * 256 + *(ExifHeader + 3) - 2; // data size (excluding APP1 & length)
//	    jpeg_write_marker(&cinfo, JPEG_APP0+1, (ExifHeader + 4), length);
	    jpeg_write_marker(&cinfo, JPEG_APP0+1, (EXIFData.header + 4), EXIFData.size);
	    }
*/
	/* Write a user comment string pointed to by comment_text.
	* Pass marker type, comment and length of comment.
	*/
	jpeg_write_marker(&cinfo, JPEG_COM, PutInfoComment, (UINT)strlen(PutInfoComment));

	/* Step 5: while (scan lines remain to be written) */
	/*           jpeg_write_scanlines(...); */

	/* Here we use the library's state variable cinfo.next_scanline as the
	* loop counter, so that we don't have to keep track ourselves.
	* To keep things simple, we pass one scanline per call; you can pass
	* more if you wish, though.
	*/
//	row_stride = width * 3;	/* JSAMPLEs per row in image_buffer */
	percent = 0;			// initialise how much of the file is loaded

	while (cinfo.next_scanline < cinfo.image_height) {
	/* jpeg_write_scanlines expects an array of pointers to scanlines.
	* Here the array is only one element long, but you could pass
	* more than one scanline at a time if that's more convenient.
	    */
	    //    row_pointer[0] = & image_buffer[cinfo.next_scanline * row_stride];
	    //decoderline = (pixels + (height - cinfo.next_scanline) * row_stride);
	    if (GenerateLinebuf(decoderline, JPEGPixels, cinfo.next_scanline, &percent, height, width, bits_per_pixel) < 0)
		{
		wsprintf(s, "JPEG write failed in line %d ", cinfo.next_scanline);
		JPEGShowMessage(s);
		break;
		}

	    (void) jpeg_write_scanlines(&cinfo, &decoderline, 1);
	    }

//	UpdateTitleBar(hwnd);		// moved to JPEG.CPP

	/* Step 6: Finish compression */

	jpeg_finish_compress(&cinfo);
	/* After finish_compress, we can close the output file. */

//	fclose(outfp);
	if(decoderline != NULL)
	    FixedGlobalFree(decoderline);

	/* Step 7: release JPEG compression object */

	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_compress(&cinfo);

	/* And we're done! */
	return 0;

	/*
	* SOME FINE POINTS:
	*
	* In the above loop, we ignored the return value of jpeg_write_scanlines,
	* which is the number of scanlines actually written.  We could get away
	* with this because we were only relying on the value of cinfo.next_scanline,
	* which will be incremented correctly.  If you maintain additional loop
	* variables then you should be careful to increment them properly.
	* Actually, for output to a stdio stream you needn't worry, because
	* then jpeg_write_scanlines will write all the lines passed (or else exit
	* with a fatal error).  Partial writes can only occur if you use a data
	* destination module that can demand suspension of the compressor.
	* (If you don't know what that's for, you don't need it.)
	*
	* If the compressor requires full-image buffers (for entropy-coding
	* optimization or a multi-scan JPEG file), it will create temporary
	* files for anything that doesn't fit within the maximum-memory setting.
	* (Note that temp files are NOT needed if you use the default parameters.)
	* On some systems you may need to set up a signal handler to ensure that
	* temporary files are deleted if the program is interrupted.  See libjpeg.doc.
	*
	* Scanlines MUST be supplied in top-to-bottom order if you want your JPEG
	* files to be compatible with everyone else's.  If you cannot readily read
	* your data in that order, you'll need an intermediate array to hold the
	* image.  See rdtarga.c or rdbmp.c for examples of handling bottom-to-top
	* source data using the JPEG code's internal virtual-array mechanisms.
	*/
}

/*-----------------------------------------
JPEG Lossless Transforms
-----------------------------------------*/

#include ".\jpeglib\transupp.h"
static JCOPY_OPTION copyoption;	/* -copy switch */
static jpeg_transform_info transformoption; /* image transformation options */

void	select_transform (JXFORM_CODE transform)
// Silly little routine to detect multiple transform options, which we can't handle.
    {
    if (transformoption.transform == JXFORM_NONE || transformoption.transform == transform)
	transformoption.transform = transform;
    }

// error handling structures

struct win_jpeg_error_mgr {
  struct jpeg_error_mgr pub;		// public fields
  jmp_buf setjmp_buffer;		// for return to caller
};

typedef struct win_jpeg_error_mgr *error_ptr;

static void win_jpeg_error_exit( j_common_ptr cinfo )
{
  error_ptr myerr = (error_ptr)cinfo->err;
//  (*cinfo->err->output_message)(cinfo);
  (*cinfo->err->format_message) (cinfo, error_buffer);	// Create the message
  longjmp( myerr->setjmp_buffer, 1);
}

