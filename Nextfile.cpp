/*------------------------------------------
   NEXTFILE.CPP --   Find first and next Files
  ------------------------------------------*/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <direct.h>
#include <time.h>
#include "view.h"

#define SORT_ALPHA	1		// full path

static	HANDLE	FindHandle = (HANDLE)NULL;	// maintain these
static	BYTE	FilesFound = FALSE;
static FILETIME	FileSaveTime;			// grab it while we can
static	char	FileSpec[255];			// hold file spec
static	WORD	usMask = 0;
static	DWORD	Filesize;

int	PointMethod = SORT_ALPHA;		// 0 = no sort, 1 = alpha full path.
						// 2 alpha filename only
						// 3 = ext+alpha, 4 = random
int	CurrentFileFromList = 0;

//////////////////////////////////////////////////////////////////////////////////////////////////////
extern	HWND	hwnd;					// This is the main windows handle
//////////////////////////////////////////////////////////////////////////////////////////////////////
extern HCURSOR	hStdCursor;

  	struct FileListStuff FileList[MAXFILES];	// info for sorting file list
extern	int	filecount;
	char	LastReadDir[MAX_PATH];			// remember last SaveAs dir
extern		void	user_data(void);		// user interrupt long loops

/***************************************************************************
	Initialise file list routines - especially if directory change
***************************************************************************/

void	InitFileList(void)
    {
    FindHandle = (HANDLE)NULL;
    FilesFound = FALSE;
    CurrentFileFromList = -1;
    srand((WORD)time(NULL));
    }

/***************************************************************************
	Display Message
***************************************************************************/

void	ShowMessage(HWND hwnd, char *s)
    {
    MessageBox(hwnd, s, "Viewer", MB_ICONEXCLAMATION | MB_OK);
    }

/***************************************************************************
	Find first and following files in a directory
***************************************************************************/

int	MyGetNextFilename(int usMask, char *szSearchMask, char *szFilename)

    {
    static	WIN32_FIND_DATA	aFindBuffer;		// between calls
    DWORD	rc;

    if (!FindHandle)
	{
	FindHandle = FindFirstFile(szSearchMask, &aFindBuffer);

	if (FindHandle == INVALID_HANDLE_VALUE)
	    {
	    rc = GetLastError();
	    FindHandle = (HANDLE)NULL;
	    if (rc != ERROR_FILE_NOT_FOUND)
		return(-1);                             // Error - return
	    return(0);                                  // No matches - return
	    }
	else
	    {
	    strcpy(szFilename, aFindBuffer.cFileName);  // Copy first filename
	    memcpy(&FileSaveTime, &aFindBuffer.ftLastWriteTime, sizeof(FILETIME));		// grab it while we can
	    Filesize = aFindBuffer.nFileSizeLow;

				    // let's look for directories or special files
	    if (strcmp(szFilename, ".") == 0 || strcmp(szFilename, "..") == 0)
		return 0;					// . or .. filename
	    if (aFindBuffer.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return (10);				// No more entries
	    FilesFound = TRUE;
	    return(1);					// good return code
	    }
	}
    else
	{
	if (FindNextFile(FindHandle, &aFindBuffer))
	    {
	    strcpy(szFilename, aFindBuffer.cFileName);		// Copy next filename
	    memcpy(&FileSaveTime, &aFindBuffer.ftLastWriteTime, sizeof(FILETIME));		// grab it while we can
	    Filesize = aFindBuffer.nFileSizeLow;
					// let's look for directories or special files

	    if (strcmp(szFilename, ".") == 0 || strcmp(szFilename, "..") == 0)
		return 0;					// . or .. filename
	    if (aFindBuffer.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return (10);					// No more entries
	    FilesFound = TRUE;
	    return(1);					// good return code
	    }
	else							// no more files
	    {
	    if (FindHandle)
		FindClose(FindHandle);				// Close handle
	    FindHandle = (HANDLE)NULL;
	    return (0);						// No more entries
	    }
	}
    }

/***************************************************************************
	Display status
***************************************************************************/
/*
void	DisplayStatus(HWND hwnd)

    {
    char	s[100];
    char	wheel[] = {'|','\\','-','/'};
    static	BYTE	i;

    wsprintf (s, "Loading Files into Memory %c", wheel[++i % 4]);
    SetWindowText (hwnd, s);		// Show formatted text in the caption bar
    }
*/

/***************************************************************************
	Read the next file in the current directory
***************************************************************************/

int	LoadFileList(HWND hwnd, char *Filename)
    {
    int		flag = FALSE;
    char	filename[460];
    char	s[460];
    int	i;
    static	HCURSOR	hOldCursor;
    char	*FileMask;

    char	drive0[_MAX_DRIVE], dir0[_MAX_DIR], name0[_MAX_FNAME], ext0[_MAX_EXT];

    hOldCursor = SetCursor(LoadCursor((HINSTANCE)NULL, IDC_WAIT));		// Load hour-glass cursor
    _splitpath(Filename, drive0, dir0, name0, ext0);
    sprintf(LastReadDir, "%s%s", drive0, dir0);

    if (filecount != 0)		// cleanup any oustanding pointers from previous list
	{
	for (i = 0; i < filecount; ++i)
	    {
	    if (FileList[i].FileName != NULL)
		{
		delete [] FileList[i].FileName;
		FileList[i].FileName = NULL;
		}
	    }
	}
    filecount = 0;

    _chdir(LastReadDir);

    while (filecount < MAXFILES)
	{
	user_data();
	strcpy(FileSpec, "*.png");
	FileMask = FileSpec;				// user defined file spec
	flag = MyGetNextFilename(usMask, FileMask, filename);

	if (flag < 0)
	    {
	    sprintf(s, "File not found for line %d in list file ", filecount);
	    ShowMessage(hwnd, s);
	    return -1;
	    }

	if (flag == 0)
	    {
	    return 1;
	    }

	if((FileList[filecount].FileName = new char[strlen(filename) + 1]) == NULL)   // allocate storage for next filename
	    {
	    sprintf(s, "Not enough memory for line %d in list file ", filecount);
	    ShowMessage(hwnd, s);
	    return -1;
	    }

	strcpy(FileList[filecount].FileName, filename);
	memcpy(&FileList[filecount].LastWriteTime, &FileSaveTime, sizeof(FILETIME));
	FileList[filecount].FileSize = Filesize;
	++filecount;
	}
    if (filecount <= 0)
	{
	ShowMessage(hwnd, "There are no valid filenames in list file ");
	return -1;
	}

    SetCursor(hOldCursor);
    return 0;
    }

/**************************************************************************
	Strip extensions from filenames
**************************************************************************/

int	StripPath(char *p)
    {
    static	char	*pstr;

    pstr = p + lstrlen(p) - 1;
    while ((*pstr != '\\') && (*pstr != ':') && (pstr >= p))
	pstr--;
    return (int)(pstr - p + 1);
    }

/**************************************************************************
	Compare function for sorting on filenames, date and time
**************************************************************************/

int	file_cmp(const void *p1, const void *p2)
    {
    register int	diff, pm, width1, height1, width2, height2;
    char		*a, *b;
    DWORD		size1, size2;		// compare file sizes
    FILETIME		t1, t2;			// compare save times
//    char		drive0[_MAX_DRIVE], dir0[_MAX_DIR], name0[_MAX_FNAME], ext0[_MAX_EXT];
//    char		drive1[_MAX_DRIVE], dir1[_MAX_DIR], name1[_MAX_FNAME], ext1[_MAX_EXT];

    struct FileListStuff *elem1, *elem2;
    elem1 = (struct FileListStuff *)p1;
    elem2 = (struct FileListStuff *)p2;
    
    a = elem1->FileName;
    b = elem2->FileName;
    size1 =   elem1->FileSize;
    size2 =   elem2->FileSize;
    t1 =     elem1->LastWriteTime;
    t2 =     elem2->LastWriteTime;
    width1 = elem1->width;
    width2 = elem2->width;
    height1 = elem1->height;
    height2 = elem2->height;
    pm = abs(PointMethod);
    diff = (int) (_stricmp(a, b));
    return ((PointMethod > 0) ? diff : -diff);
    }

/***************************************************************************
	Sort File List
***************************************************************************/

void	SortFileList(void)
    {
    SetCursor(LoadCursor(NULL, IDC_WAIT));			        // Reload wait cursor.
    qsort((void *)FileList, filecount, sizeof(FileList[0]), file_cmp);
    SetCursor(LoadCursor(NULL, IDC_ARROW));			        // Reload arrow cursor.
    }

