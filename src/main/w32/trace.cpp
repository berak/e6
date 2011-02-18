
#include <stdio.h>
#include <string.h>
#include <windows.h>


//void trace_( FILE *out, char *f, int l, char *fmt, ... );

void trace_( FILE *out, char *f, int l, char *fmt, ... )
{
    static char buffer[2048] = {0};
    va_list args;
    va_start( args, fmt );
    vsprintf( buffer, fmt, args );
    va_end( args );
    fprintf(out, "%s(%d):",f,l);   
    fprintf(out, buffer);   
    fflush (out);
}


