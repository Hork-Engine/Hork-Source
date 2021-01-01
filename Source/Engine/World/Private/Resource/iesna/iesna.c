/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

This file is part of the Angie Engine Source Code.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

/*

IESNA LM-63 Photometric Data Module

Based on iesna.c by Ian Ashdown:
Copyright 1995-1998 byHeart Consultants Limited
Permission: The following source code is copyrighted. However, it may be freely
copied, redistributed, and modified for personal use or for royalty-free inclusion
in commercial programs.

*/

#include "iesna.h"

#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>

#define FALSE   0
#define TRUE    1

#define ARRAY_SIZE( a ) (sizeof(a)/sizeof((a)[0]))

static char IE_TextBuf[4096];        /* Input text buffer    */
static char IE_ValueBuf[4096];       /* Input value buffer   */

static IE_Bool ReadArray( IE_Context const * context, float *, int );
static IE_Bool ReadList( IE_Context const * context, char *, ... );
static IE_Bool ReadTilt( IE_Context const * context, IE_DATA * );
static char * ReadLine( IE_Context const * context, char *, int );

IE_Bool IES_Load( IE_Context const * context, IE_DATA *pdata )
{
    char *tilt_str;
    int i;

    memset( pdata, 0, sizeof( *pdata ) );

    if ( !ReadLine( context, IE_TextBuf, ARRAY_SIZE( IE_TextBuf ) ) ) {
        return FALSE;
    }

    if ( !strcmp( IE_TextBuf, "IESNA:LM-63-1995" ) ) {
        pdata->format = IESNA_95;
    }
    else if ( !strcmp( IE_TextBuf, "IESNA91" ) ) {
        pdata->format = IESNA_91;
    }
    else {
        pdata->format = IESNA_86;

        // First line is a label line or "TILT="
        context->rewind( context->userData );
    }

    // Read label lines
    for ( ; ; ) {
        if ( !ReadLine( context, IE_TextBuf, ARRAY_SIZE( IE_TextBuf ) ) ) {
            IES_Free( context, pdata );
            return FALSE;
        }

        // Check for "TILT" keyword indicating end of label lines
        if ( !strncmp( IE_TextBuf, "TILT=", 5 ) ) {
            break;
        }
    }

    // Point to TILT line parameter
    tilt_str = IE_TextBuf + 5;

    // Check for TILT data
    if ( !strcmp( tilt_str, "NONE" ) ) {
        IE_Bool status = TRUE;
        if ( !strcmp( tilt_str, "INCLUDE" ) ) {
#if 0
            // Open the TILT data file
            ptilt = fopen( tilt_str, "r" );
            if ( !ptilt ) {
                IES_Free( context, pdata );
                return FALSE;
            }

            // Read the TILT data from the TILT data file
            status = ReadTilt( context, pdata, ptilt );

            // Close the TILT data file
            fclose( ptilt );
#endif
        }
        else {
            // Read the TILT data from the IESNA data file
            status = ReadTilt( context, pdata );
        }
        if ( !status ) {
            IES_Free( context, pdata );
            return FALSE;
        }
    }

    // Read in next two lines
    if ( !ReadList( context, "%d%f%f%d%d%d%d%f%f%f",
                    &(pdata->lamp.num_lamps), &(pdata->lamp.lumens_lamp),
                    &(pdata->lamp.multiplier), &(pdata->photo.num_vert_angles),
                    &(pdata->photo.num_horz_angles), &(pdata->photo.gonio_type),
                    &(pdata->units), &(pdata->dim.width), &(pdata->dim.length),
                    &(pdata->dim.height) ) ) {
        IES_Free( context, pdata );
        return FALSE;
    }

    if ( !ReadList( context, "%f%f%f", &(pdata->elec.ball_factor),
                    &(pdata->elec.blp_factor), &(pdata->elec.input_watts) ) ) {
        IES_Free( context, pdata );
        return FALSE;
    }

    // Allocate space for the vertical and horizontal angles array
    pdata->photo.vert_angles = (float *)context->calloc( pdata->photo.num_vert_angles, sizeof( float ) );
    pdata->photo.horz_angles = (float *)context->calloc( pdata->photo.num_horz_angles, sizeof( float ) );
    if ( !pdata->photo.vert_angles || !pdata->photo.horz_angles ) {
        IES_Free( context, pdata );
        return FALSE;
    }

    // Read in vertical angles array
    if ( !ReadArray( context, pdata->photo.vert_angles, pdata->photo.num_vert_angles ) ) {
        IES_Free( context, pdata );
        return FALSE;
    }

    // Read in horizontal angles array
    if ( !ReadArray( context, pdata->photo.horz_angles, pdata->photo.num_horz_angles ) ) {
        IES_Free( context, pdata );
        return FALSE;
    }

    // Allocate space for the candela values array pointers
    pdata->photo.pcandela = (float **)context->calloc( pdata->photo.num_horz_angles, sizeof( float * ) );
    if ( !pdata->photo.pcandela ) {
        IES_Free( context, pdata );
        return FALSE;
    }

    // Read in candela values arrays
    for ( i = 0; i < pdata->photo.num_horz_angles; i++ ) {
        // Allocate space for the candela values array
        pdata->photo.pcandela[i] = (float *)context->calloc( pdata->photo.num_vert_angles, sizeof( float ) );
        if ( !pdata->photo.pcandela[i] ) {
            IES_Free( context, pdata );
            return FALSE;
        }

        // Read in candela values
        if ( !ReadArray( context, pdata->photo.pcandela[i], pdata->photo.num_vert_angles ) ) {
            IES_Free( context, pdata );
            return FALSE;
        }
    }

    return TRUE;
}

void IES_Free( IE_Context const * context, IE_DATA *pdata )
{
    int i;

    if ( pdata->lamp.tilt.angles ) {
        context->free( pdata->lamp.tilt.angles );
        pdata->lamp.tilt.angles = NULL;
    }

    if ( pdata->lamp.tilt.mult_factors ) {
        context->free( pdata->lamp.tilt.mult_factors );
        pdata->lamp.tilt.mult_factors = NULL;
    }

    if ( pdata->photo.vert_angles ) {
        context->free( pdata->photo.vert_angles );
        pdata->photo.vert_angles = NULL;
    }

    if ( pdata->photo.horz_angles ) {
        context->free( pdata->photo.horz_angles );
        pdata->photo.horz_angles = NULL;
    }

    if ( pdata->photo.pcandela ) {
        for ( i = 0; i < pdata->photo.num_horz_angles; i++ ) {
            if ( pdata->photo.pcandela[i] ) {
                context->free( pdata->photo.pcandela[i] );
            }
        }

        context->free( pdata->photo.pcandela );
        pdata->photo.pcandela = NULL;
    }
}

static IE_Bool ReadTilt( IE_Context const * context, IE_DATA *pdata )
{
    IE_Bool status = TRUE;   /* Status flag                                  */

    /* Read the lamp-to-luminaire geometry line                           */
    if ( ReadLine( context, IE_ValueBuf, ARRAY_SIZE( IE_ValueBuf ) )  == NULL )
        status = FALSE;

    if ( status == TRUE )
    {
        /* Get the lamp-to-luminaire geometry value                         */
        pdata->lamp.tilt.orientation = atoi( IE_ValueBuf );

        /* Read the number of angle-multiplying factor pairs line           */
        if ( ReadLine( context, IE_ValueBuf, ARRAY_SIZE( IE_ValueBuf ) )  == NULL )
            status = FALSE;
    }

    if ( status == TRUE )
    {
        /* Get the number of angle-multiplying factor pairs value           */
        pdata->lamp.tilt.num_pairs = atoi( IE_ValueBuf );

        if ( pdata->lamp.tilt.num_pairs > 0 )
        {
            /* Allocate space for the angle and multiplying factors arrays    */
            if ( (pdata->lamp.tilt.angles = (float *)
                   context->calloc( pdata->lamp.tilt.num_pairs, sizeof( float ) )) == NULL )
            {
                status = FALSE;
            }
        }
    }

    if ( status == TRUE )
    {
        if ( (pdata->lamp.tilt.mult_factors = (float *)
               context->calloc( pdata->lamp.tilt.num_pairs, sizeof( float ) )) == NULL )
        {
            status = FALSE;
        }
    }

    if ( status == TRUE )
    {
        /* Read in the angle values                                         */
        if ( ReadArray( context, pdata->lamp.tilt.angles,
                        pdata->lamp.tilt.num_pairs ) == FALSE )
            status = FALSE;
    }

    if ( status == TRUE )
    {
        /* Read in the multiplying factor values                            */
        if ( ReadArray( context, pdata->lamp.tilt.mult_factors,
                        pdata->lamp.tilt.num_pairs ) == FALSE )
            status = FALSE;
    }

    return status;
}

static IE_Bool ReadList( IE_Context const * context, char *format, ... )
{
    char c;               /* Scratch variable                             */
    char type;            /* Format specifier                             */
    char *pbuf;           /* Input buffer pointer                         */
    char *pfmt = format;  /* Format string pointer                        */
    int itemp;            /* Temporary integer variable                   */
    float ftemp;          /* Temporary floating point variable            */
    va_list pvla;         /* Variable list argument pointer               */

    va_start( pvla, format );       /* Set up for optional arguments        */

    /* Read in the first line                                             */
    if ( (pbuf = ReadLine( context, IE_ValueBuf, ARRAY_SIZE( IE_ValueBuf ) )) == NULL )
        return FALSE;

    for ( ; ; )   /* Skip over leading delimiters                         */
    {
        c = *pbuf;

        if ( c == '\0' )      /* End of current line?                         */
            return FALSE;
        else if ( isspace( c ) != 0 )
            pbuf++;
        else
            break;
    }

    for ( ; ; )
    {
        if ( *pfmt != '%' )           /* Check format specifier delimiter     */
            return FALSE;

        /* Get and validate format specifier                                */
        switch ( type = *(pfmt + 1) )
        {
        case 'd':
        case 'D':
            sscanf( pbuf, "%d", &itemp );     /* Get integer value            */

            *(va_arg( pvla, int * )) = itemp;

            for ( ; ; )     /* Advance buffer pointer past the substring    */
            {
                c = *pbuf;
                if ( (isdigit( c ) != 0) || c == '-' )
                    pbuf++;
                else
                    break;
            }
            break;
        case 'f':
        case 'F':
            sscanf( pbuf, "%f", &ftemp );     /* Get float value              */

            *(va_arg( pvla, float * )) = ftemp;

            for ( ; ; )     /* Advance buffer pointer past the substring    */
            {
                c = *pbuf;
                if ( (isdigit( c ) != 0) || c == '.' || c == '-' )
                    pbuf++;
                else
                    break;
            }
            break;
        default:
            return FALSE;
        }

        /* Point to next format specifier                                   */

        pfmt++;             /* Skip over format specifier delimiter         */

        if ( *pfmt == '\0' )  /* End of format string ?                       */
            return FALSE;

        pfmt++;             /* Skip over format specifier parameter         */

        if ( *pfmt == '\0' )  /* End of format string ?                       */
            break;

        for ( ; ; )         /* Skip over delimiters                         */
        {
            c = *pbuf;
            if ( c == '\0' )    /* End of current line?                         */
            {
                /* Get next line                                                */
                if ( (pbuf = ReadLine( context, IE_ValueBuf, ARRAY_SIZE( IE_ValueBuf ) )) == NULL )
                    return FALSE;
            } else if ( (isspace( c ) != 0) || c == ',' )
                pbuf++;
            else
                break;
        }
    }

    return TRUE;
}

static IE_Bool ReadArray( IE_Context const * context, float *array, int size )
{
    int i = 0;            /* Loop index                                   */
    char c;               /* Scratch variable                             */
    char *pbuf;           /* Input buffer pointer                         */
    float ftemp;          /* Temporary floating point variable            */

                          /* Read in the first line                                             */
    if ( (pbuf = ReadLine( context, IE_ValueBuf, ARRAY_SIZE( IE_ValueBuf ) )) == NULL )
        return FALSE;

    for ( ; ; )   /* Skip over leading delimiters                         */
    {
        c = *pbuf;

        if ( c == '\0' )      /* End of current line?                         */
            return FALSE;
        else if ( (isspace( c ) != 0) )
            pbuf++;
        else
            break;
    }

    for ( ; ; )   /* Parse the array elements                             */
    {
        /* Convert the current substring to its floating point value        */
        (void)sscanf( pbuf, "%f", &ftemp );

        array[i++] = ftemp;

        if ( i == size )      /* All substrings converted ?                   */
            break;

        for ( ; ; )         /* Advance buffer pointer past the substring    */
        {
            c = *pbuf;
            if ( (isdigit( c ) != 0) || c == '.' || c == '-' )
                pbuf++;
            else
                break;
        }

        for ( ; ; )         /* Skip over delimiters                         */
        {
            if ( c == '\0' )    /* End of current line?                         */
            {
                /* Get next line                                                */
                if ( (pbuf = ReadLine( context, IE_ValueBuf, ARRAY_SIZE( IE_ValueBuf ) )) == NULL )
                    return FALSE;
            } else if ( (isspace( c ) != 0) || c == ',' )
                pbuf++;
            else
                break;
            c = *pbuf;        /* Get next delimiter                           */
        }
    }

    return TRUE;
}

static char *ReadLine( IE_Context const * context, char *pbuf, int size )
{
    char *pnl;

    if ( context->fgets( pbuf, size, context->userData ) ) {
        // Strip off trailing newline (if any)
        if ( (pnl = strchr( pbuf, '\n' )) != NULL ) {
            *pnl = '\0';
        }

        return pbuf;
    }
    /* Report error                                                     */
    //if ( ferror( pfile ) != 0 )
    //    fputs( "ERROR: could not read file %s\n", stderr );
    //else
    //    fputs( "ERROR: unexpected end of file %s\n", stderr );

    return NULL;
}
