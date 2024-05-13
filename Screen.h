////////////////////////////
// SCREEN.H header file
//
// Screen and display specific stuff
////////////////////////////

struct	ScreenStruct				// screen specific stuff
    {
    RECT	WARect;				// this is the usable screen taking taskbar into account
    RECT	DesktopRect;			// usable area including task bar
    int		caption;			// size of windows caption and scroll bars
    int		scroll_width;			// size of horizontal scroll bars
    int		screenx, screeny;		// screen size
    int		xFrame, yFrame;			// frame size
    int		MinWidth;			// minimum width of PGV to prevent double menu
    };

struct	ZoomStruct				// stuff related to how we display the image on the screen
    {
    POINT	ptSize;				// Stores DIB dimensions scaled for zoom type
    int		xdots, ydots;			// describes the rectangle size for zoomflags 'Z' and 'O'
    char	OldZoomFlag;			// Back it up for 'ron
    char	zoomflag;			// how are we displaying image (double, half, full screen)?
						// 'W' = variable zoom using mouse wheel
						// 'Z' = variable zoom by dragging corner
						// 'O' = variable zoom by dragging corner and maintaining aspect ratio
						// 'F' = full screen
						// 'I' = fit screen
						// 'B' = fit screen with no border
						// 'Q' = quarter
						// 'H' = half
						// 'D' = double
						// '4' = four times
						// 'N' = normal size
    };

struct	ResizeStruct				// stuff related to how we resize the image
    {
    double	VertResizeValue;		// vert scaling value for resize option
    double	HorResizeValue;			// hor scaling value for resize option
    long	NewWidth;			// integer versions to allow accurate pixel count without rounding errors
    long	NewHeight;
    bool	UseIntVersion;			// flag to tell whether to use float or integer version
    };

