//--------------------------------------------------------------------------
// Program to pull the information out of various types of EXIF digital
// camera files and show it in a reasonably consistent way
//
// Version 2.82
//
// Compiling under Windows:
//   Make sure you have Microsoft's compiler on the path, then run make.bat
//
// Dec 1999 - Apr 2008
//
// by Matthias Wandel   www.sentex.net/~mwandel
//--------------------------------------------------------------------------
// This file is extracted from jhead.c of jhead project
//   by hialan <hialan.liu@gmail.com>
//--------------------------------------------------------------------------
#include "jhead.h"
#include <time.h>    // Explicitly include for struct tm and strftime
#include <stdlib.h>  // Explicitly include for exit() and EXIT_FAILURE

gboolean ShowTags   = FALSE; 
gboolean DumpExifMap = FALSE;

//--------------------------------------------------------------------------
// Error exit handler
//--------------------------------------------------------------------------
void ErrFatal(char * msg)
{
    // Muted to keep the application GUI quiet on image parsing failures
    (void)msg; // Prevents unused parameter warnings from compiler
    exit(EXIT_FAILURE);
}

//--------------------------------------------------------------------------
// Report non fatal errors.
//--------------------------------------------------------------------------
void ErrNonfatal(char * msg, int a1, int a2)
{
    (void)msg; // Prevents unused parameter warnings
    (void)a1;
    (void)a2;
    return;
}

//--------------------------------------------------------------------------
// Set file time as exif time.
//--------------------------------------------------------------------------
void FileTimeAsString(char * TimeStr)
{
    struct tm ts;
    // Safely fetch local time from the ImageInfo struct tracked in jhead.h
    ts = *localtime(&ImageInfo.FileDateTime);
    strftime(TimeStr, 20, "%Y:%m:%d %H:%M:%S", &ts);
}