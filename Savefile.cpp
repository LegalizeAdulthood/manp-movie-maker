/*------------------------------------------
   SAVEFILE.CPP --   File Write Functions
  ------------------------------------------*/

//	PHD 2008-01-18  (Unicode)

#ifndef PGV4
#define UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <commdlg.h>
#include <stdlib.h>
#include <direct.h>
#include <string.h>
#include <stdio.h>
#include <tchar.h>
#include "view.h"
#include "FileStats.h"
#include "FileOps.h"
#include "Dib.h"
#include "Thumbs.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
extern	HWND	hwnd;					// This is the main windows handle
//////////////////////////////////////////////////////////////////////////////////////////////////////

extern	CDib		Dib;			// Device Independent Bitmap class instance
extern	CFileStats	FileStats;		// Class for file info
extern	ThumbStruct	THUMB;			// thumbnail control info 
extern	StateType	PGVState;		// what state is pgv in?

//extern	WORD	ThumbFlag;			// Are we currently displaying a thumbnail file?
extern	int	UndoPtr;			// is Undo buffer loaded?
//extern	int	FileChanged;			// Compare to undo pointer. If not the same, image has changed
int		save_file_type = FILE_JPG;
extern	TCHAR	LastReadDir[];			// remember last SaveAs dir
extern	TCHAR	SaveAsDir[];			// remember last SaveAs dir
//extern	char	SaveAsDirFlag;			// SaveAs dir:	C = same as file just viewed
//						//		D = use "last file" to init Save Dir
//						//		U = User defined
extern	SaveMethod	PGVSaveMethod;		// SaveAs dir type
extern	ControlType	PGVControl;		// PGV behaviour depends on current value of this variable

extern	BOOL	SaveDate;			// TRUE if the saved file keeps the date of the read file
extern	int	SaveFileDate(TCHAR *);

int	SaveFile (TCHAR *, LPSTR);

extern	int	write_bmp_file(FILE *);
extern	int	write_webp_file(FILE *);
extern	int	write_jpg_file(FILE *, TCHAR *, BYTE *, BOOL, WORD, WORD, WORD);
extern	int	write_gif_file(FILE *);
extern	int	write_mpg_file(FILE *);
extern	int	write_png_file(FILE *, BOOL);
extern	int	write_tga_file(FILE *);
extern	int	write_ico_file(FILE *);
//extern	int	write_tns_file(TCHAR *, char *);
extern	void	FileErrors(DWORD, TCHAR *);
extern	void	ShowOutfileMessage(char *);
extern	TCHAR	*Trailing(TCHAR *);
extern	void	ClearAllUndo(void);
extern	void	UpdateTitleBar(HWND);
extern	void	ShowMessage(char *);

static TCHAR *szFilter =
TEXT("JPEG Files (*.JPG)\0*.jpg\0Bitmap Files (*.BMP)\0*.bmp\0GIF Files (*.GIF)\0*.gif\0Targa Files (*.TGA)\0*.tga\0PiNG Files (*.PNG)\0*.png\0WebP Files (*.WEBP)\0*.webp\0MPEG Files (*.MPG)\0*.mpg\0");
static TCHAR *szThumbFilter =
TEXT("JPEG Files (*.JPG)\0*.jpg\0Bitmap Files (*.BMP)\0*.bmp\0GIF Files (*.GIF)\0*.gif\0Targa Files (*.TGA)\0*.tga\0PiNG Files (*.PNG)\0*.png\0MPEG Files (*.MPG)\0*.mpg\0ThumbNail Files (*.TNS)\0*.tns\0");
static TCHAR *szIconFilter = TEXT("Icon Files (*.ICO)\0*.ico\0");
static TCHAR *szCollageFilter = TEXT("Collage Script Files (*.CLG)\0*.clg\0");

/*------------------------------------------
   Initialise File Write Functions
  ------------------------------------------*/

void SaveFileInitialize (OPENFILENAME *SaveOfn)
    {
    memset(SaveOfn, 0, sizeof(OPENFILENAME));
    SaveOfn->lStructSize = sizeof(OPENFILENAME);
    SaveOfn->hwndOwner = hwnd;
    SaveOfn->hInstance = NULL;
    SaveOfn->lpstrFilter = szFilter;
    SaveOfn->lpstrCustomFilter = NULL;
    SaveOfn->nMaxCustFilter = 0;
    SaveOfn->nFilterIndex = 0;
    SaveOfn->nMaxFile = 480;
    SaveOfn->lpstrFileTitle = NULL;
    SaveOfn->nMaxFileTitle = 0;
    SaveOfn->lpstrInitialDir = NULL;
    SaveOfn->lpstrTitle = NULL;
    SaveOfn->lpstrFileTitle = NULL;
    SaveOfn->Flags =  OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_LONGNAMES;
    SaveOfn->nFileOffset = 0;
    SaveOfn->nFileExtension = 1;
    SaveOfn->lpstrDefExt = NULL;
    SaveOfn->lCustData = 0L;
    SaveOfn->lpfnHook = NULL;
    SaveOfn->lpTemplateName = NULL;
    save_file_type = FILE_JPG;
    }

/*------------------------------------------
   Generate SaveFile Name from SaveAsDir
  ------------------------------------------*/

void	GenerateSaveFileName (HWND hwnd, TCHAR *lpstrFileName, TCHAR *infile)

    {
    TCHAR	*fileptr;
    TCHAR	temp[_MAX_PATH + 1];

    _tcscpy(temp, infile);
    fileptr = temp + _tcslen(temp);
    while (fileptr > temp && *fileptr != '\\')
       fileptr--;						// remove path from input file
							    // use previous path
    _stprintf(lpstrFileName, TEXT("%s%s"), SaveAsDir, fileptr + ((*(fileptr) == '\\') ? 1 : 0));
    fileptr = lpstrFileName + _tcslen(lpstrFileName);
    while (fileptr > temp && *fileptr != '.')
       fileptr--;							// remove extension
    *fileptr = '\0';
    }

/*------------------------------------------
   General File Save Dialogue
  ------------------------------------------*/

int	SaveFileOpenDlg (HWND hwnd, TCHAR *lpstrFileName, TCHAR *infile, LPSTR lpstrTitleName)
     {
     char	s[MAXLINE];
     TCHAR	u[MAXLINE];
//     TCHAR	CurrentDir[_MAX_PATH + 1];
     TCHAR	*t;
     TCHAR	*fileptr;
     FILE	*fp;
     DWORD	ErrorType;
     OPENFILENAME SaveOfn;
     TCHAR	*jpgptr = TEXT(".jpg");
     TCHAR	*bmpptr = TEXT(".bmp");
     TCHAR	*gifptr = TEXT(".gif");
     TCHAR	*tgaptr = TEXT(".tga");
     TCHAR	*icoptr = TEXT(".ico");
     TCHAR	*pngptr = TEXT(".png");
     TCHAR	*tnsptr = TEXT(".tns");
     TCHAR	*clgptr = TEXT(".clg");
     TCHAR	*mpgptr = TEXT(".mpg");
     TCHAR	*webpptr = TEXT(".webp");

     SaveFileInitialize (&SaveOfn);
     if (*SaveAsDir)
	 SaveOfn.lpstrInitialDir = SaveAsDir;
     else
	 {
	    {
	    _tcscpy(SaveAsDir, LastReadDir);		// use last file to initialise Save Dir
	    SaveOfn.lpstrInitialDir = LastReadDir;
	    }
	 }

     GenerateSaveFileName(hwnd, lpstrFileName, infile);

     SaveOfn.hwndOwner	   	= hwnd ;
     SaveOfn.lpstrFile	   	= lpstrFileName ;
    if (PGVState == THUMBSHEET)
//     if (THUMB.ThumbFlag == 1)			// if ThumbNail image, setup TNS files as default
	 {
	 SaveOfn.lpstrFilter	= szThumbFilter;
	 SaveOfn.lpstrDefExt	= TEXT("tns");
	 SaveOfn.nFilterIndex	= 7;
	 save_file_type		= FILE_TNS;
	 }
     else
	 {
	 SaveOfn.lpstrFilter	= szFilter;
	 SaveOfn.lpstrDefExt	= TEXT("jpg");
	 SaveOfn.nFilterIndex	= 1;
	 save_file_type		= FILE_JPG;
	 }

     if (GetSaveFileName (&SaveOfn) == 0)
	 {
	 ErrorType = CommDlgExtendedError();
	 FileErrors(ErrorType, lpstrFileName);
         return -1;
	 }

     fileptr = lpstrFileName + _tcslen(lpstrFileName);
     while (fileptr > lpstrFileName && *fileptr != '.')
	 fileptr--;							// remove extension
     *fileptr = '\0';

    _tcscpy(SaveAsDir, lpstrFileName);		// remove filename and copy to SaveAs Dir
    t = SaveAsDir + _tcslen(SaveAsDir);
    while (*t != '\\' && t > SaveAsDir)
	*t-- = '\0';

    switch (SaveOfn.nFilterIndex)
	{
	case 1:
	    save_file_type = FILE_JPG;
	    _tcscat(lpstrFileName, jpgptr);
	    break;
	case 2:
	    save_file_type = FILE_BMP;
	    _tcscat(lpstrFileName, bmpptr);
	    break;
	case 3:
	    save_file_type = FILE_GIF;
	    _tcscat(lpstrFileName, gifptr);
	    break;
	case 4:
	    save_file_type = FILE_TGA;
	    _tcscat(lpstrFileName, tgaptr);
	    break;
	case 5:
	    save_file_type = FILE_PNG;
	    _tcscat(lpstrFileName, pngptr);
	    break;
	case 6:
	    save_file_type = FILE_WEBP;
	    _tcscat(lpstrFileName, webpptr);
	    break;
	case 7:
	    save_file_type = FILE_MPG;
	    _tcscat(lpstrFileName, mpgptr);
	    break;
	case 8:
	    save_file_type = FILE_TNS;
	    _tcscat(lpstrFileName, tnsptr);
	    break;
	default:
	    if (PGVState == THUMBSHEET)
//	    if (THUMB.ThumbFlag == 1)		// allow for thumbnail file
		sprintf(s, "Error: Only File types JPG, BMP, GIF, TGA and TNS Supported in file ");
	    else
		sprintf(s, "Error: Only File types JPG, BMP, GIF, WEBP and TGA Supported in file ");
	    ShowOutfileMessage(s);
	    return -1;
	}
    ClearAllUndo();
//    FileChanged = UndoPtr;				// we have loaded a new file so reset pointer so that we are the same undo level
    if (!(fp = _tfopen(lpstrFileName, TEXT("rb"))))	// file does not exits
	return 0;					// so return OK and use file name
    else
	{
	fclose(fp);
	_stprintf(u, TEXT("File: <%s> Already exists. Overwrite?"), lpstrFileName);
	if (MessageBox (hwnd, u, TEXT("SaveAs"), MB_ICONEXCLAMATION | MB_YESNO) == IDYES)
	    return 0;
	else
	    return -1;
	}
    return 0;
    }

/*------------------------------------------
   Special Icon File Save Dialogue
  ------------------------------------------*/

int	SaveIconOpenDlg (TCHAR *lpstrFileName, TCHAR *infile, char *lpstrTitleName)
     {
     TCHAR	s[MAXLINE];
     TCHAR	*fileptr;
     FILE	*fp;
     TCHAR	*icoptr = TEXT(".ico");
     DWORD	ErrorType;
     OPENFILENAME SaveOfn;

     _tcscpy(lpstrFileName, infile);
     fileptr = lpstrFileName;
     fileptr = lpstrFileName + _tcslen(lpstrFileName);
     while (fileptr > lpstrFileName && *fileptr != '.')
	 fileptr--;							// remove extension
     *fileptr = '\0';

     SaveFileInitialize (&SaveOfn);
     SaveOfn.hwndOwner	   	= hwnd ;
     SaveOfn.lpstrFile	   	= lpstrFileName ;
     SaveOfn.lpstrFileTitle    	= NULL;
     SaveOfn.lpstrFilter	= szIconFilter;
     SaveOfn.lpstrDefExt	= TEXT("ico");
     SaveOfn.nFileExtension	= 1;
     save_file_type		= FILE_ICO;

     if (GetSaveFileName (&SaveOfn) == 0)
	 {
	 ErrorType = CommDlgExtendedError();
	 FileErrors(ErrorType, lpstrFileName);
         return -1;
	 }

    fileptr = lpstrFileName + _tcslen(lpstrFileName);
    while (fileptr > lpstrFileName && *fileptr != '.')
       fileptr--;							// remove extension
    *fileptr = '\0';

    save_file_type = FILE_ICO;
    _tcscat(lpstrFileName, icoptr);

    ClearAllUndo();
//    FileChanged = UndoPtr;				// we have loaded a new file so reset pointer so that we are the same undo level
    if (!(fp = _tfopen(lpstrFileName, TEXT("rb"))))		// file does not exits
	return 0;					// so return OK and use file name
    else
	{
	fclose(fp);
	_stprintf(s, TEXT("File: <%s> Already exists. Overwrite?"), lpstrFileName);
	if (MessageBox (hwnd, s, TEXT("SaveAs"), MB_ICONEXCLAMATION | MB_YESNO) == IDYES)
	    return 0;
	else
	    return -1;
	}
    save_file_type = FILE_JPG;				// ready for next save
    return 0;
    }

/*------------------------------------------
   Special Collage Script File Save Dialogue
  ------------------------------------------*/

int	SaveCollageOpenDlg (TCHAR *lpstrFileName, TCHAR *infile, char *lpstrTitleName)
     {
     TCHAR	s[MAXLINE];
     TCHAR	*fileptr;
     FILE	*fp;
     TCHAR	*clgptr = TEXT(".clg");
     DWORD	ErrorType;
     OPENFILENAME SaveOfn;

     _tcscpy(lpstrFileName, infile);
     fileptr = lpstrFileName;
     fileptr = lpstrFileName + _tcslen(lpstrFileName);
     while (fileptr > lpstrFileName && *fileptr != '.')
	 fileptr--;							// remove extension
     *fileptr = '\0';

     SaveFileInitialize (&SaveOfn);
     SaveOfn.hwndOwner	   	= hwnd ;
     SaveOfn.lpstrFile	   	= lpstrFileName ;
     SaveOfn.lpstrFileTitle    	= NULL;
     SaveOfn.lpstrFilter	= szCollageFilter;
     SaveOfn.lpstrDefExt	= TEXT("CLG");
     SaveOfn.nFileExtension	= 1;
     save_file_type		= FILE_CLG;

     if (GetSaveFileName (&SaveOfn) == 0)
	 {
	 ErrorType = CommDlgExtendedError();
	 FileErrors(ErrorType, lpstrFileName);
         return -1;
	 }

    fileptr = lpstrFileName + _tcslen(lpstrFileName);
    while (fileptr > lpstrFileName && *fileptr != '.')
       fileptr--;							// remove extension
    *fileptr = '\0';

    save_file_type = FILE_CLG;
    _tcscat(lpstrFileName, clgptr);

    ClearAllUndo();
//    FileChanged = UndoPtr;				// we have loaded a new file so reset pointer so that we are the same undo level
    if (!(fp = _tfopen(lpstrFileName, TEXT("rb"))))		// file does not exits
	return 0;					// so return OK and use file name
    else
	{
	fclose(fp);
	_stprintf(s, TEXT("File: <%s> Already exists. Overwrite?"), lpstrFileName);
	if (MessageBox (hwnd, s, TEXT("SaveAs"), MB_ICONEXCLAMATION | MB_YESNO) == IDYES)
	    return 0;
	else
	    return -1;
	}
    save_file_type = FILE_JPG;				// ready for next save
    return 0;
    }

/*------------------------------------------
   File Save Routines
  ------------------------------------------*/

int	SaveFile (TCHAR * lpstrFileName, LPSTR lpstrTitleName)

    {
    TCHAR	t[MAXLINE];
    FILE	*output_file = NULL;
    CFileOps	outFileOps;			// Class for output filename operations
    ControlType	OldControl;
#ifdef	DEBUG
    errno_t	err;
#endif
    if ((output_file = outFileOps.FileOpsOpenWrite(lpstrFileName, TEXT("wb"))) == NULL)	
	{
#ifdef	DEBUG
	_get_errno(&err);
	_stprintf(t, TEXT("Error type %ld in Opening %s"), err, lpstrFileName);
	MessageBox (hwnd, t, TEXT("File Write"), MB_ICONEXCLAMATION | MB_OK);
#else
	ShowMessage("Error in Opening output file ");
#endif
	return -1;
	}

    switch (save_file_type)
	{
	case FILE_BMP:
	    if (write_bmp_file(output_file) < 0)
		return - 1;
	    break;
	case FILE_GIF:
	    OldControl = PGVControl;
	    PGVControl = WRITEANIMFRAME;					// suppress warnings in dithering if part of GIF animation run (Dl3quant.cpp)
	    if (write_gif_file(output_file) < 0)
		{
		PGVControl = OldControl;
		return - 1;
		}
	    PGVControl = OldControl;
	    break;
	case FILE_MPG:
	    OldControl = PGVControl;
	    PGVControl = WRITEANIMFRAME;					// suppress warnings in dithering if part of GIF animation run (Dl3quant.cpp)
	    if (write_mpg_file(output_file) < 0)
		{
		PGVControl = OldControl;
		return - 1;
		}
	    PGVControl = OldControl;
	    break;
	case FILE_PNG:
	    if (write_png_file(output_file, FALSE) < 0)
		return - 1;
	    break;
	case FILE_JPG:
	    if (write_jpg_file(output_file, lpstrFileName, Dib.DibPixels, FALSE, Dib.DibWidth, Dib.DibHeight, Dib.BitsPerPixel) < 0)
		return - 1;
	    break;
	case FILE_TGA:
	    if (write_tga_file(output_file) < 0)
		return -1;
	    break;
	case FILE_WEBP:
	    if (write_webp_file(output_file) < 0)
		return -1;
	    break;
	case FILE_ICO:
	    if (write_ico_file(output_file) < 0)
		return - 1;
	    break;
	case FILE_TNS:								// use JPEG format for thumbnails
	    if (write_png_file(output_file, TRUE) < 0)
		return - 1;
	    break;
	default:
	     _stprintf(t, TEXT("Error: File type <%d> not supported in file: %s"), save_file_type, lpstrFileName);
	    MessageBox (hwnd, t, TEXT("File Write"), MB_ICONEXCLAMATION | MB_OK);
	    return - 1;
	}
    FileStats.bits_per_pixel = Dib.BitsPerPixel;				// these are now the stats for the most recent file
    FileStats.height = Dib.DibHeight;
    FileStats.width = Dib.DibWidth;
    outFileOps.CreateTitleBarInfo("", "", Dib.DibWidth, Dib.DibHeight, (WORD)((save_file_type == FILE_GIF) ? 8 : Dib.BitsPerPixel));
    UpdateTitleBar(hwnd);
    InvalidateRect(hwnd, NULL, FALSE);
    if (output_file)
	fclose(output_file);
    if (SaveDate)
	SaveFileDate(lpstrFileName);
    return 0;
    }

/*-----------------------------------------
	Update Save Path Dialogue Box
  -----------------------------------------*/

DLGPROC FAR PASCAL SavePathDlg (HWND hDlg, UINT message, UINT wParam, LONG lParam)
     {
//     HWND	hCtrl ;
     static	SaveMethod	temp;
     static     UINT	tempParam;
     TCHAR	*t;

     switch (message)
	 {
	 case WM_INITDIALOG:
//	     hCtrl = GetDlgItem (hDlg, IDC_EDIT_SAVEPATH) ;
	     SetDlgItemText(hDlg, IDC_EDIT_SAVEPATH, SaveAsDir);
	     temp = PGVSaveMethod;
	     switch (PGVSaveMethod)
		 {
		 case USERSAVE:
		     tempParam = IDC_USER_PATH;
		     break;
		 case SAVELASTREAD:
		     tempParam = IDC_INPUT_PATH;
		     break;
		 default:
		     tempParam = IDC_INPUT_PATH;
		     break;
		 }
	     CheckRadioButton(hDlg, IDC_USER_PATH, IDC_INPUT_PATH, tempParam);
	     SetFocus(GetDlgItem(hDlg, tempParam));
	     return (DLGPROC)TRUE;

	  case WM_COMMAND:
	        switch (wParam)
		    {
		    case IDC_USER_PATH:
		    case IDC_CURRENT_PATH:
		    case IDC_INPUT_PATH:
		        switch ((int) LOWORD(wParam))
			    {
			    case IDC_USER_PATH:
				temp = USERSAVE;
				break;
			    case IDC_CURRENT_PATH:
				temp = SAVECURRENTDIR;
				break;
			    case IDC_INPUT_PATH:
				temp = SAVELASTREAD;
				break;
			    }
			CheckRadioButton(hDlg, IDC_USER_PATH, IDC_CURRENT_PATH, (int) LOWORD(wParam));
		        return (DLGPROC)TRUE ;

		    case IDOK:
			PGVSaveMethod = temp;
			switch (PGVSaveMethod)
			    {
			    case USERSAVE:
				GetDlgItemText(hDlg, IDC_EDIT_SAVEPATH, SaveAsDir, _MAX_PATH + 1);
				Trailing(SaveAsDir);		// remove trailing spaces or newlines
				t = SaveAsDir + _tcslen(SaveAsDir) - 1;
				if (*t != '\\')			// add a backslash to the path if required
				    {
				    *(t + 1) = '\\';
				    *(t + 2) = '\0';
				    }
				break;
			    case SAVELASTREAD:
				_tcscpy(SaveAsDir, LastReadDir);   // use last file to init SaveDir
				break;
			    case SAVECURRENTDIR:
				_tgetcwd(SaveAsDir, _MAX_PATH);	
//				_tcscpy(SaveAsDir, LastReadDir);   // use last file to init SaveDir
				break;
			    }
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

