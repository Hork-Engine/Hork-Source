/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

Note that this is a slightly modified version of the original file.

*/


/*
*************************************************************************
*
*  IESNA.C - IESNA LM-63 Photometric Data Module
*
*  Version:    1.00D
*
*  History:    95/08/15 - Created.
*              95/08/29 - Version 1.00A release.
*              95/09/01 - Fixed memory deallocation error in IE_Flush.
*              95/09/04 - Version 1.00B release.
*              96/01/28 - Added IE_CalcCU_Array, IE_CalcCoeff,
*                         IE_CalcCU, and IE_CalcData functions.
*                       - Added zonal multiplier equation constants.
*                       - Added IE_Cosine array.
*                       - Added CIE luminaire classification type
*                         strings.
*                       - Fixed error messages in IE_GetLine.
*              96/01/30 - Version 1.00C release.
*              98/03/07 - Fixed file close problem in IE_ReadFile.
*              98/03/09 - Version 1.00D release.
*
*  Compilers:  Any ANSI C-compliant compiler
*
*  Author:     Ian Ashdown, P. Eng.
*              byHeart Consultants Limited
*              620 Ballantree Road
*              West Vancouver, B.C.
*              Canada V7S 1W3
*              Tel. (604) 922-6148
*              Fax. (604) 987-7621
*
*  Copyright 1995-1998 byHeart Consultants Limited
*
*  Permission: The following source code is copyrighted. However, it may
*              be freely copied, redistributed, and modified for personal
*              use or for royalty-free inclusion in commercial programs.
*
*************************************************************************
*/

#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "iesna.h"

static char IE_TextBuf[IE_MaxLabel + 1];        /* Input text buffer    */
static char IE_ValueBuf[IE_MaxLine + 1];        /* Input value buffer   */

                                                /* Zonal multiplier equation constants                                  */
static const double IE_A[] =
{ 0.000, 0.041, 0.070, 0.100, 0.136, 0.190, 0.315, 0.640, 2.100 };
static const double IE_B[] =
{ 0.00, 0.98, 1.05, 1.12, 1.16, 1.25, 1.25, 1.25, 0.80 };

/* CIE luminaire classification types                                   */
const char *IE_CIE_Type[5] =
{
    "I - Direct",
    "II - Semi-Direct",
    "III - General Diffuse",
    "IV - Semi-Indirect",
    "V - Indirect"
};

/* Cosine lookup table (five degree increments from 0 to 180 degrees)   */
static const double IE_Cosine[] =
{
    1.000000,  0.996195,  0.984808,  0.965926,  0.939693,  0.906308,
    0.866025,  0.819152,  0.766044,  0.707107,  0.642788,  0.573576,
    0.500000,  0.422618,  0.342020,  0.258819,  0.173648,  0.087156,
    0.000000, -0.087156, -0.173648, -0.258819, -0.342020, -0.422618,
    -0.500000, -0.573576, -0.642788, -0.707107, -0.766044, -0.819152,
    -0.866025, -0.906308, -0.939693, -0.965926, -0.984808, -0.996195,
    -1.000000
};

static IE_Bool IE_CalcCoeff( IE_CALC *, double, double, double, double );
static IE_Bool IE_GetArray( FILE *, float *, int );
static IE_Bool IE_GetList( FILE *, char *, ... );
static IE_Bool IE_ReadTilt( IE_DATA *, FILE * );
static char *IE_CopyString( const char * );
static char *IE_GetLine( char *, int, FILE * );
static double IE_CalcCU( IE_CALC *, double, double, double, double );
static void IE_AllocErr();
static void IE_CalcCU_Array( IE_CALC * );

/*
*************************************************************************
*
*  IE_ReadFile - Read IESNA-Format Photometric Data File
*
*  Purpose:    To read an IESNA-format photometric data file.
*
*  Setup:      IE_Bool IE_ReadFile
*              (
*                char *fname,
*                IE_DATA *pdata
*              )
*
*  Where:      fname is the IESNA-format data file name.
*              pdata is a pointer to the photometric data structure the
*                file is to be read into.
*
*  Return:     TRUE if the file was successfully read; otherwise FALSE.
*
*  Note:       The file is parsed in accordance with the following
*              Illuminating Engineering Society of North America
*              publications:
*
*                IES Computer Committee: "IES Recommended Standard File
*                Format for Electronic Transfer of Photometric Data and
*                Related Information," IES Publication LM-63-1995 (to
*                be published).
*
*                IES Computer Committee: "IES Recommended Standard File
*                Format for Electronic Transfer of Photometric Data and
*                Related Information," IES Publication LM-63-1991.
*
*                IES Computer Committee: "IES Recommended Standard File
*                Format for Electronic Transfer of Photometric Data,"
*                IES Publication LM-63-1986.
*
*              The latest edition of this publication (currently
*              LM-63-1991) may be purchased from:
*
*                Illuminating Engineering Society of North America
*                120 Wall Street, 17th Floor
*                New York, NY 10005
*
*                Tel. (212) 248-5000
*
*************************************************************************
*/

IE_Bool IE_ReadFile( const char *fname, IE_DATA *pdata )
{
    char *tilt_str = NULL;               /* TILT line parameter pointer          */
    int i;                        /* Loop index                           */
    IE_LABEL *pnew;        /* Current label line list element ptr  */
    IE_LABEL *pold = NULL;        /* Previous label line list element ptr */
    IE_Bool status = TRUE;           /* Status flag                          */
    FILE *piesf = NULL;                  /* IESNA data file pointer              */
    FILE *ptilt = NULL;           /* TILT data file pointer               */

                                  /* Save file name                                                     */
    if ( (pdata->file.name = IE_CopyString( fname )) == NULL )
    {
        IE_AllocErr();      /* Report memory allocation error               */
        status = FALSE;
    }

    if ( status == TRUE )
    {
        /* Initialize the photometric data structure                        */
        pdata->plline = NULL;
        pdata->lamp.tilt_fname = NULL;
        pdata->lamp.tilt.angles = NULL;
        pdata->lamp.tilt.mult_factors = NULL;
        pdata->photo.vert_angles = NULL;
        pdata->photo.horz_angles = NULL;
        pdata->photo.pcandela = NULL;

        /* Open the IESNA data file                                         */
        if ( (piesf = fopen( fname, "r" )) == NULL )
        {
            fprintf( stderr, "ERROR: could not open file %s\n", fname );
            status = FALSE;
        }
    }

    if ( status == TRUE )
    {
        /* Read the first line                                              */
        if ( IE_GetLine( IE_TextBuf, IE_MaxLabel + 1, piesf ) == NULL )
            status = FALSE;
    }

    if ( status == TRUE )
    {
        /* Determine file format                                            */
        if ( strcmp( IE_TextBuf, "IESNA:LM-63-1995" ) == 0 )
        {
            /* File is LM-63-1995 format                                      */
            pdata->file.format = IESNA_95;
        } else if ( strcmp( IE_TextBuf, "IESNA91" ) == 0 )
        {
            /* File is LM-63-1991 format                                      */
            pdata->file.format = IESNA_91;
        } else
        {
            /* File is presumably LM-63-1986 format                           */
            pdata->file.format = IESNA_86;

            rewind( piesf );    /* First line is a label line or "TILT="        */
        }

        for ( ; ; )         /* Read label lines                             */
        {
            if ( IE_GetLine( IE_TextBuf, IE_MaxLabel + 1, piesf ) == NULL )
            {
                status = FALSE;
                break;
            }

            /* Check for "TILT" keyword indicating end of label lines         */
            if ( strncmp( IE_TextBuf, "TILT=", 5 ) == 0 )
                break;

            /* Instantiate a new label line linked list element               */
            if ( (pnew = (IE_LABEL *)malloc( sizeof( IE_LABEL ) ))
                 == NULL )
            {
                IE_AllocErr();  /* Report memory allocation error               */
                status = FALSE;
                break;
            }

            /* Copy buffer to label line                                      */
            if ( (pnew->line = IE_CopyString( IE_TextBuf )) == NULL )
            {
                IE_AllocErr();  /* Report memory allocation error               */
                status = FALSE;
                break;
            }

            /* Add label line element to linked list                          */
            if ( pdata->plline == NULL )        /* Empty list?                  */
                pdata->plline = pnew;
            else                              /* Add to end of list           */
                pold->pnext = pnew;

            pold = pnew;              /* Update previous list element pointer */
            pnew->pnext = NULL;       /* Terminate list                       */
        }
    }

    if ( status == TRUE )   /* Check for errors                             */
    {
        tilt_str = IE_TextBuf + 5;  /* Point to TILT line parameter         */

                                    /* Save the TILT data file name                                     */
        if ( (pdata->lamp.tilt_fname = IE_CopyString( tilt_str )) == NULL )
        {
            IE_AllocErr();    /* Report memory allocation error               */
            status = FALSE;
        }
    }

    if ( status == TRUE )
    {
        /* Check for TILT data                                              */
        if ( strcmp( tilt_str, "NONE" ) != 0 )
        {
            if ( strcmp( tilt_str, "INCLUDE" ) != 0 )
            {
                /* Open the TILT data file                                      */
                if ( (ptilt = fopen( tilt_str, "r" )) == NULL )
                    status = FALSE;
                else
                {
                    /* Read the TILT data from the TILT data file                 */
                    status = IE_ReadTilt( pdata, ptilt );

                    fclose( ptilt );        /* Close the TILT data file             */
                }
            } else
            {
                /* Read the TILT data from the IESNA data file                  */
                status = IE_ReadTilt( pdata, piesf );
            }
        }
    }

    if ( status == TRUE )
    {
        /* Read in next two lines                                           */
        status = IE_GetList( piesf, "%d%f%f%d%d%d%d%f%f%f",
                             &(pdata->lamp.num_lamps), &(pdata->lamp.lumens_lamp),
                             &(pdata->lamp.multiplier), &(pdata->photo.num_vert_angles),
                             &(pdata->photo.num_horz_angles), &(pdata->photo.gonio_type),
                             &(pdata->units), &(pdata->dim.width), &(pdata->dim.length),
                             &(pdata->dim.height) );
    }

    if ( status == TRUE )
    {
        status = IE_GetList( piesf, "%f%f%f", &(pdata->elec.ball_factor),
                             &(pdata->elec.blp_factor), &(pdata->elec.input_watts) );
    }

    if ( status == TRUE )
    {
        /* Allocate space for the vertical angles array                     */
        if ( (pdata->photo.vert_angles = (float *)
               calloc( pdata->photo.num_vert_angles, sizeof( float ) )) == NULL )
        {
            IE_AllocErr();    /* Report memory allocation error               */
            status = FALSE;
        }
    }

    if ( status == TRUE )
    {
        /* Allocate space for the horizontal angles array                   */
        if ( (pdata->photo.horz_angles = (float *)
               calloc( pdata->photo.num_horz_angles, sizeof( float ) )) == NULL )
        {
            IE_AllocErr();    /* Report memory allocation error               */
            status = FALSE;
        }
    }

    if ( status == TRUE )
    {
        // HERE !!!!
        /* Read in vertical angles array                                    */
        status = IE_GetArray( piesf, pdata->photo.vert_angles,
                              pdata->photo.num_vert_angles );
    }

    if ( status == TRUE )
    {
        /* Read in horizontal angles array                                  */
        status = IE_GetArray( piesf, pdata->photo.horz_angles,
                              pdata->photo.num_horz_angles );
    }

    if ( status == TRUE )
    {
        /* Allocate space for the candela values array pointers             */
        if ( (pdata->photo.pcandela = (float **)
               calloc( pdata->photo.num_horz_angles, sizeof( float * ) )) == NULL )
        {
            IE_AllocErr();    /* Report memory allocation error               */
            status = FALSE;
        }
    }

    if ( status == TRUE )
    {
        /* Read in candela values arrays                                    */
        for ( i = 0; i < pdata->photo.num_horz_angles; i++ )
        {
            /* Allocate space for the candela values array                    */
            if ( (pdata->photo.pcandela[i] = (float *)
                   calloc( pdata->photo.num_vert_angles, sizeof( float ) )) == NULL )
            {
                IE_AllocErr();  /* Report memory allocation error               */
                status = FALSE;
                break;
            }

            /* Read in candela values                                         */
            if ( IE_GetArray( piesf, pdata->photo.pcandela[i],
                              pdata->photo.num_vert_angles ) == FALSE )
            {
                status = FALSE;
                break;
            }
        }
    }

    if ( piesf != NULL )
        fclose( piesf );      /* Close the IESNA data file                    */

    if ( status == FALSE )  /* Check for errors                             */
        IE_Flush( pdata );

    return status;
}

/*
*************************************************************************
*
*  IE_Flush- Release Photometric Data Structure
*
*  Purpose:    To release memory allocated to members of a photometric
*              data structure.
*
*  Setup:      void IE_Flush
*              (
*                IE_DATA *pdata
*              )
*
*  Where:      pdata is a pointer to a photometric data structure.
*
*************************************************************************
*/

void IE_Flush( IE_DATA *pdata )
{
    int i;                        /* Loop index                           */
    float **ppcandela;            /* Candela values array pointer pointer */
    IE_LABEL *pcurr;       /* Current linked list element pointer  */
    IE_LABEL *pnext;       /* Next linked list element pointer     */

                           /* Free file name                                                     */
    if ( pdata->file.name != NULL )
        free( pdata->file.name );

    /* Free label line linked list                                        */
    if ( (pcurr = pdata->plline) != NULL )
    {
        pdata->plline = NULL;
        while ( pcurr != NULL )
        {
            free( pcurr->line );        /* Free the label line memory           */
            pnext = pcurr->pnext;     /* Get next element pointer             */
            free( pcurr );              /* Free the list element memory         */
            pcurr = pnext;            /* Make next element current    */
        }
    }

    /* Free TILT data file name (if allocated)                            */
    if ( pdata->lamp.tilt_fname != NULL )
    {
        free( pdata->lamp.tilt_fname );
        pdata->lamp.tilt_fname = NULL;
    }

    /* Free angle and multiplying factor arrays (if allocated)            */
    if ( pdata->lamp.tilt.angles != NULL )
    {
        free( pdata->lamp.tilt.angles );
        pdata->lamp.tilt.angles = NULL;
    }

    if ( pdata->lamp.tilt.mult_factors != NULL )
    {
        free( pdata->lamp.tilt.mult_factors );
        pdata->lamp.tilt.mult_factors = NULL;
    }

    /* Free vertical and horizontal angles arrays (if allocated)          */
    if ( pdata->photo.vert_angles != NULL )
    {
        free( pdata->photo.vert_angles );
        pdata->photo.vert_angles = NULL;
    }

    if ( pdata->photo.horz_angles != NULL )
    {
        free( pdata->photo.horz_angles );
        pdata->photo.horz_angles = NULL;
    }

    /* Free candela arrays (if allocated)                                 */
    if ( (ppcandela = pdata->photo.pcandela) != NULL )
    {
        for ( i = 0; i < pdata->photo.num_horz_angles; i++ )
            free( ppcandela[i] );

        /* Free candela array pointer array                                 */
        free( pdata->photo.pcandela );
        pdata->photo.pcandela = NULL;
    }
}

/*
*************************************************************************
*
*  IE_CalcData - Calculate Photometric Data
*
*  Purpose:    To calculate the luminaire photometric data from the IES
*              Standard File data.
*
*  Setup:      IE_Bool IE_CalcData
*              (
*                IE_DATA *pdata,
*                IE_CALC *pcalc
*              )
*
*  Where:      pdata is a pointer to an IE_DATA data structure.
*              pcalc is a pointer to an IE_CALC data structure.
*
*  Return:     TRUE if successful; otherwise FALSE (insufficient
*              photometric data).
*
*  Note:       The following calculations are in accordance with:
*
*                "IES Recommended Procedure for Calculating Coefficients
*                of Utilization, Wall and Ceiling Cavity Exitance", IES
*                Publication LM-57
*
*              The candela and lamp lumen values are multiplied by the
*              value of "multiplier".
*
*              This function requires photometric measurements at
*              vertical angle increments of 5.0 degrees (e.g., 0.0
*              degrees, 5.0 degrees, 10.0 degrees, ... ).
*
*************************************************************************
*/

IE_Bool IE_CalcData( IE_DATA *pdata, IE_CALC *pcalc )
{
    float **pcandela;                     /* Candela array pointer        */
    int i;                                /* Scratch counter              */
    int j;                                /* Scratch counter              */
    long emit_lm;                         /* Emitted lumens               */
    IE_Bool vva_flag;                        /* Valid vertical angles flag   */
    static IE_Bool horz_flag[IE_HORZ];       /* Valid horz angle flags       */
    static IE_Bool vert_flag[IE_VERT_CAND];  /* Valid vert angle flags       */
    static long avg_candela[IE_VERT_CAND];   /* Avg candela value array   */

                                             /* Clear the candela values array                                     */
    for ( i = 0; i < IE_HORZ; i++ )
        for ( j = 0; j < IE_VERT_CAND; j++ )
            pcalc->candela[i][j] = 0L;

    /* Initialize the horizontal and vertical angles array                */
    for ( i = 0; i < IE_HORZ; i++ )
        pcalc->h_angle[i] = IE_INDEX_NONE;

    for ( i = 0; i < IE_VERT_CAND; i++ )
        pcalc->v_angle[i] = IE_INDEX_NONE;

    /* Initialize the horizontal and vertical angle flags array           */
    for ( i = 0; i < IE_HORZ; i++ )
        horz_flag[i] = FALSE;

    for ( i = 0; i < IE_VERT_CAND; i++ )
        vert_flag[i] = FALSE;

    /* Search for valid horizontal angles                                 */
    pcalc->horz_num = 0;

    for ( i = 0; i < IE_HORZ; i++ )
        for ( j = 0; j < pdata->photo.num_horz_angles; j++ )
            if ( fabs( ((double)i * IE_H_ANGLE) - (double)
                       pdata->photo.horz_angles[j] ) < 1.0 )
            {
                pcalc->horz_num++;
                pcalc->h_angle[i] = j;
                horz_flag[i] = TRUE;
            }

    /* Search for valid vertical angles                                   */
    for ( i = 0; i < IE_VERT_CAND; i++ )
        for ( j = 0; j < pdata->photo.num_vert_angles; j++ )
            if ( fabs( ((double)i * IE_V_ANGLE) - (double)
                       pdata->photo.vert_angles[j] ) < 1.0 )
            {
                pcalc->v_angle[i] = j;
                vert_flag[i] = TRUE;
            }

    /* Determine whether vertical angles exist at 5 degree increments     */
    /* over range of 0 to 90 degrees                                      */
    vva_flag = TRUE;
    for ( i = 0; i <= IE_VERT_90; i++ )
    {
        if ( vert_flag[i] == FALSE )
        {
            vva_flag = FALSE;
            break;
        }
    }

    if ( vva_flag == FALSE )
    {
        /* Determine whether vertical angles exist at 5 degree increments   */
        /* over range of 90 to 180 degrees                                  */
        vva_flag = TRUE;
        for ( i = IE_VERT_90; i <= IE_VERT_180; i++ )
            if ( vert_flag[i] == FALSE )
                return FALSE;               /* Insufficient photometric data    */
    }

    /* Add the candela values for valid angles                            */
    pcandela = pdata->photo.pcandela;

    for ( i = 0; i < IE_HORZ; i++ )
        if ( horz_flag[i] == TRUE )
            for ( j = 0; j < IE_VERT_CAND; j++ )
                if ( vert_flag[j] == TRUE )
                    pcalc->candela[i][j] += (long)((double)
                                                    pcandela[pcalc->h_angle[i]][pcalc->v_angle[j]] *
                                                    pdata->lamp.multiplier);

    /* Calculate the average candela values                               */
    for ( i = 0; i < IE_VERT_CAND; i++ )
        if ( pcalc->v_angle[i] != IE_INDEX_NONE )
        {
            avg_candela[i] = 0L;

            for ( j = 0; j < IE_HORZ; j++ )
                if ( pcalc->h_angle[j] != IE_INDEX_NONE )
                    avg_candela[i] += pcalc->candela[j][i];

            avg_candela[i] /= (long)pcalc->horz_num;
        }

    /* Calculate the total lamp lumens                                    */
    pcalc->total_lm = ((double)pdata->lamp.num_lamps * (double)
                        pdata->lamp.lumens_lamp);

    emit_lm = 0L;         /* Initialize emitted lumens                    */

                          /* Calculate the zonal flux values                                    */
    for ( i = 0; i < IE_VERT_FLUX; i++ )
    {
        j = 2 * i + 1;

        if ( pcalc->v_angle[j] != IE_INDEX_NONE )
        {
            pcalc->flux[i] = (long)(2.0 * PI * avg_candela[j] *
                (IE_Cosine[j - 1] - IE_Cosine[j + 1]));

            emit_lm += pcalc->flux[i];       /* Update emitted lumens        */
        } else
            pcalc->flux[i] = 0L;
    }

    /* Calculate the luminaire efficiency                                 */
    pcalc->efficiency = (double)(emit_lm * 100L) / pcalc->total_lm;

    /* Calculate the zonal lumen summary                                  */

    /* 0-30 degree zone                                                   */
    pcalc->zonal_lm[0] = pcalc->flux[0] + pcalc->flux[1] + pcalc->flux[2];
    pcalc->lamp_pct[0] = (int)((pcalc->zonal_lm[0] * 100L) /
                                pcalc->total_lm);
    pcalc->fixt_pct[0] = (int)((pcalc->zonal_lm[0] * 100L) / emit_lm);

    /* 0-40 degree zone                                                   */
    pcalc->zonal_lm[1] = pcalc->zonal_lm[0] + pcalc->flux[3];
    pcalc->lamp_pct[1] = (int)((pcalc->zonal_lm[1] * 100L) /
                                pcalc->total_lm);
    pcalc->fixt_pct[1] = (int)((pcalc->zonal_lm[1] * 100L) / emit_lm);

    /* 0-60 degree zone                                                   */
    pcalc->zonal_lm[2] = pcalc->zonal_lm[1] + pcalc->flux[4] +
        pcalc->flux[5];
    pcalc->lamp_pct[2] = (int)((pcalc->zonal_lm[2] * 100L) /
                                pcalc->total_lm);
    pcalc->fixt_pct[2] = (int)((pcalc->zonal_lm[2] * 100L) / emit_lm);

    /* 0-90 degree zone                                                   */
    pcalc->zonal_lm[3] = pcalc->zonal_lm[2] + pcalc->flux[6] +
        pcalc->flux[7] + pcalc->flux[8];
    pcalc->lamp_pct[3] = (int)((pcalc->zonal_lm[3] * 100L) /
                                pcalc->total_lm);
    pcalc->fixt_pct[3] = (int)((pcalc->zonal_lm[3] * 100L) / emit_lm);

    /* 90-120 degree zone                                                 */
    pcalc->zonal_lm[4] = pcalc->flux[9] + pcalc->flux[10] +
        pcalc->flux[11];
    pcalc->lamp_pct[4] = (int)((pcalc->zonal_lm[4] * 100L) /
                                pcalc->total_lm);
    pcalc->fixt_pct[4] = (int)((pcalc->zonal_lm[4] * 100L) / emit_lm);

    /* 90-130 degree zone                                                 */
    pcalc->zonal_lm[5] = pcalc->zonal_lm[4] + pcalc->flux[12];
    pcalc->lamp_pct[5] = (int)((pcalc->zonal_lm[5] * 100L) /
                                pcalc->total_lm);
    pcalc->fixt_pct[5] = (int)((pcalc->zonal_lm[5] * 100L) / emit_lm);

    /* 90-150 degree zone                                                 */
    pcalc->zonal_lm[6] = pcalc->zonal_lm[5] + pcalc->flux[13] +
        pcalc->flux[14];
    pcalc->lamp_pct[6] = (int)((pcalc->zonal_lm[6] * 100L) /
                                pcalc->total_lm);
    pcalc->fixt_pct[6] = (int)((pcalc->zonal_lm[6] * 100L) / emit_lm);

    /* 90-180 degree zone                                                 */
    pcalc->zonal_lm[7] = pcalc->zonal_lm[6] + pcalc->flux[15] +
        pcalc->flux[16] + pcalc->flux[17];
    pcalc->lamp_pct[7] = (int)((pcalc->zonal_lm[7] * 100L) /
                                pcalc->total_lm);
    pcalc->fixt_pct[7] = (int)((pcalc->zonal_lm[7] * 100L) / emit_lm);

    /* 0 - 180 degree zone                                                */
    pcalc->zonal_lm[8] = pcalc->zonal_lm[3] + pcalc->zonal_lm[7];
    pcalc->lamp_pct[8] = (int)((pcalc->zonal_lm[8] * 100L) /
                                pcalc->total_lm);
    pcalc->fixt_pct[8] = (int)((pcalc->zonal_lm[8] * 100L) / emit_lm);

    /* Determine the CIE luminaire type                                   */
    if ( pcalc->fixt_pct[7] < 10 )
        pcalc->cie_type = IE_CIE_1;         /* Direct                       */
    else if ( pcalc->fixt_pct[7] < 40 )
        pcalc->cie_type = IE_CIE_2;         /* Semi-direct                  */
    else if ( pcalc->fixt_pct[7] < 60 )
        pcalc->cie_type = IE_CIE_3;         /* General diffuse              */
    else if ( pcalc->fixt_pct[7] < 90 )
        pcalc->cie_type = IE_CIE_4;         /* Semi-indirect                */
    else
        pcalc->cie_type = IE_CIE_5;         /* Indirect                     */

    IE_CalcCU_Array( pcalc );   /* Calculate coefficients of utilization    */

    return (TRUE);
}

/*
*************************************************************************
*
*  IE_CalcCU_Array - Calculate Coefficients of Utilization Array
*
*  Purpose:    To calculate the coefficients of utilization for the
*              selected product.
*
*  Setup:      static void IE_CalcCU_Array
*              (
*                IE_CALC *pcalc
*              )
*
*  Where:      pcalc is a pointer to an IE_CALC data structure.
*
*************************************************************************
*/

static void IE_CalcCU_Array( IE_CALC *pcalc )
{
    int i;                        /* Scratch counter                      */

    for ( i = 0; i < IE_CU_ROWS; i++ )
    {
        pcalc->IE_CU_Array[i][0] = (int)(IE_CalcCU( pcalc, (double)i, 0.70,
                                                    0.80, 0.20 ) * 100.0);
        pcalc->IE_CU_Array[i][1] = (int)(IE_CalcCU( pcalc, (double)i, 0.50,
                                                    0.80, 0.20 ) * 100.0);
        pcalc->IE_CU_Array[i][2] = (int)(IE_CalcCU( pcalc, (double)i, 0.30,
                                                    0.80, 0.20 ) * 100.0);
        pcalc->IE_CU_Array[i][3] = (int)(IE_CalcCU( pcalc, (double)i, 0.10,
                                                    0.80, 0.20 ) * 100.0);
        pcalc->IE_CU_Array[i][4] = (int)(IE_CalcCU( pcalc, (double)i, 0.70,
                                                    0.70, 0.20 ) * 100.0);
        pcalc->IE_CU_Array[i][5] = (int)(IE_CalcCU( pcalc, (double)i, 0.50,
                                                    0.70, 0.20 ) * 100.0);
        pcalc->IE_CU_Array[i][6] = (int)(IE_CalcCU( pcalc, (double)i, 0.30,
                                                    0.70, 0.20 ) * 100.0);
        pcalc->IE_CU_Array[i][7] = (int)(IE_CalcCU( pcalc, (double)i, 0.10,
                                                    0.70, 0.20 ) * 100.0);
        pcalc->IE_CU_Array[i][8] = (int)(IE_CalcCU( pcalc, (double)i, 0.50,
                                                    0.50, 0.20 ) * 100.0);
        pcalc->IE_CU_Array[i][9] = (int)(IE_CalcCU( pcalc, (double)i, 0.30,
                                                    0.50, 0.20 ) * 100.0);
        pcalc->IE_CU_Array[i][10] = (int)(IE_CalcCU( pcalc, (double)i, 0.10,
                                                     0.50, 0.20 ) * 100.0);
        pcalc->IE_CU_Array[i][11] = (int)(IE_CalcCU( pcalc, (double)i, 0.50,
                                                     0.30, 0.20 ) * 100.0);
        pcalc->IE_CU_Array[i][12] = (int)(IE_CalcCU( pcalc, (double)i, 0.30,
                                                     0.30, 0.20 ) * 100.0);
        pcalc->IE_CU_Array[i][13] = (int)(IE_CalcCU( pcalc, (double)i, 0.10,
                                                     0.30, 0.20 ) * 100.0);
        pcalc->IE_CU_Array[i][14] = (int)(IE_CalcCU( pcalc, (double)i, 0.50,
                                                     0.10, 0.20 ) * 100.0);
        pcalc->IE_CU_Array[i][15] = (int)(IE_CalcCU( pcalc, (double)i, 0.30,
                                                     0.10, 0.20 ) * 100.0);
        pcalc->IE_CU_Array[i][16] = (int)(IE_CalcCU( pcalc, (double)i, 0.10,
                                                     0.10, 0.20 ) * 100.0);
        pcalc->IE_CU_Array[i][17] = (int)(IE_CalcCU( pcalc, (double)i, 0.0,
                                                     0.0, 0.20 ) * 100.0);
    }
}

/*
*************************************************************************
*
*  IE_ReadTilt - Read TILT Data From File
*
*  Purpose:    To read TILT data from an IESNA-format data file into a
*              photometric data structure.
*
*  Setup:      static IE_Bool IE_ReadTilt
*              (
*                IE_DATA *pdata,
*                FILE *pfile
*              )
*
*  Where:      pdata is a pointer to a photometric data structure.
*              pfile is the IESNA-format data file pointer.
*
*  Return:     TRUE if no errors occurred; otherwise FALSE.
*
*  Note:       The file can be either part of a full IESNA-format data
*              file or a separate TILT data file that was specified in
*              the parent IESNA-format file on the "TILT=" line.
*
*************************************************************************
*/

static IE_Bool IE_ReadTilt( IE_DATA *pdata, FILE *pfile )
{
    IE_Bool status = TRUE;   /* Status flag                                  */

                             /* Read the lamp-to-luminaire geometry line                           */
    if ( IE_GetLine( IE_ValueBuf, IE_MaxLine + 1, pfile )  == NULL )
        status = FALSE;

    if ( status == TRUE )
    {
        /* Get the lamp-to-luminaire geometry value                         */
        pdata->lamp.tilt.orientation = atoi( IE_ValueBuf );

        /* Read the number of angle-multiplying factor pairs line           */
        if ( IE_GetLine( IE_ValueBuf, IE_MaxLine + 1, pfile )  == NULL )
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
                   calloc( pdata->lamp.tilt.num_pairs, sizeof( float ) )) == NULL )
            {
                IE_AllocErr();  /* Report memory allocation error               */
                status = FALSE;
            }
        }
    }

    if ( status == TRUE )
    {
        if ( (pdata->lamp.tilt.mult_factors = (float *)
               calloc( pdata->lamp.tilt.num_pairs, sizeof( float ) )) == NULL )
        {
            IE_AllocErr();    /* Report memory allocation error               */
            status = FALSE;
        }
    }

    if ( status == TRUE )
    {
        /* Read in the angle values                                         */
        if ( IE_GetArray( pfile, pdata->lamp.tilt.angles,
                          pdata->lamp.tilt.num_pairs ) == FALSE )
            status = FALSE;
    }

    if ( status == TRUE )
    {
        /* Read in the multiplying factor values                            */
        if ( IE_GetArray( pfile, pdata->lamp.tilt.mult_factors,
                          pdata->lamp.tilt.num_pairs ) == FALSE )
            status = FALSE;
    }

    return status;
}

/*
*************************************************************************
*
*  IE_CalcCoeff - Calculate Zonal Cavity Luminaire Coefficients
*
*  Purpose:    To calculate the zonal cavity coefficients of a luminaire.
*
*  Setup:      IE_Bool IE_CalcCoeff
*              (
*                IE_CALC *pcalc,
*                double g,
*                double p1,
*                double p2,
*                double p3
*              )
*
*  Where:      pcalc is a pointer to an IE_CALC data structure.
*              g is the room cavity ratio.
*              p1 is the wall cavity reflectance (0.001 to 0.999).
*              p2 is the effective ceiling cavity reflectance (0.000 to
*                0.999).
*              p3 is the effective floor cavity reflectance (0.000 to
*                0.999).
*
*  Return:     TRUE if successful; otherwise FALSE (input parameters out
*              of range).
*
*  Result:     The structure pointed to by "pcalc" is modified.
*
*  Note:       The following calculations are in accordance with:
*
*                "IES Recommended Procedure for Calculating Coefficients
*                of Utilization, Wall and Ceiling Cavity Exitance", IES
*                Publication LM-57.
*
*************************************************************************
*/

IE_Bool IE_CalcCoeff( IE_CALC *pcalc, double g, double p1, double p2,
                      double p3 )
{
    double C0;            /* Intermediate calculation parameter           */
    double C1;            /* Intermediate calculation parameter           */
    double C2;            /* Intermediate calculation parameter           */
    double C3;            /* Intermediate calculation parameter           */
    double f23;           /* Ceiling-floor form factor                    */
    double phi_d = 0.0;   /* Total downward luminaire flux                */
    double phi_u = 0.0;   /* Total upward luminaire flux                  */
    double Dg = 0.0;      /* Luminaire direct ratio                       */
    int n;                /* Scratch counter                              */

                          /* Check for conditions which could cause a divide-by-zero error      */
    if ( p1 > 0.999 || p2 > 0.999 || p3 > 0.999 )
    {
        pcalc->cu = 0.0;
        pcalc->ccec = 0.0;
        pcalc->wec = 0.0;
        pcalc->wdrc = 0.0;
        return (FALSE);
    }

    /* Calculate the flux functions                                       */
    for ( n = 0; n < IE_VERT_FLUX / 2; n++ )
        phi_d += (double)pcalc->flux[n];

    for ( n = IE_VERT_FLUX / 2; n < IE_VERT_FLUX; n++ )
        phi_u += (double)pcalc->flux[n];
    phi_d /= (double)pcalc->total_lm;
    phi_u /= (double)pcalc->total_lm;

    if ( g < 0.001 )
    {
        /* Calculate the coefficient of utilization                         */
        pcalc->cu = (phi_d + p2 * phi_u) / (1.0 - p2 * p3);

        /* Calculate the ceiling cavity luminous exitance coefficient       */
        pcalc->ccec = p2 * (phi_u + p3 * phi_d) / (1.0 - p2 * p3);

        pcalc->wec = 0.0;
        pcalc->wdrc = 0.0;
    } else
    {
        /* Calculate the luminaire direct ratio                             */
        for ( n = 0; n < IE_VERT_FLUX / 2; n++ )
            Dg += exp( -IE_A[n] * pow( g, IE_B[n] ) ) * (double)pcalc->flux[n];

        if ( phi_d > 0.001 )
            Dg /= (phi_d * (double)pcalc->total_lm);

        /* Calculate the form factor approximation                          */
        f23 = 0.026 + 0.503 * exp( -0.270 * g ) + 0.470 * exp( -0.119 * g );

        /* Calculate the intermediate calculation parameters                */
        C1 = (1.0 - p1) * (1.0 - f23 * f23) * g / (2.5 * p1 * (1.0 - f23 *
                                                                f23) + g * f23 * (1.0 - p1));
        C2 = (1.0 - p2) * (1.0 + f23) / (1.0 + p2 * f23);
        C3 = (1.0 - p3) * (1.0 + f23) / (1.0 + p3 * f23);
        C0 = C1 + C2 + C3;

        /* Calculate the coefficient of utilization                         */
        pcalc->cu = 2.5 * p1 * C1 * C3 * (1.0 - Dg) * phi_d / (g * (1.0 -
                                                                     p1) * (1.0 - p3) * C0) + p2 * C2 * C3 * phi_u / ((1.0 - p2) *
                                                                     (1.0 - p3) * C0) + (1.0 - p3 * C3 * (C1 + C2) / ((1.0 - p3) *
                                                                                                                       C0)) * Dg * phi_d / (1.0 - p3);

        /* Calculate the ceiling cavity luminous exitance coefficient       */
        pcalc->ccec = 2.5 * p1 * p2 * C1 * C2 * (1.0 - Dg) * phi_d / (g *
            (1.0 - p1) * (1.0 - p2) * C0) + (p2 * phi_u / (1.0 - p2)) * (1.0 -
                                                                          p2 * C2 * (C1 + C3) / ((1.0 - p2) * C0)) + p2 * p3 * C2 * C3 *
            Dg * phi_d / ((1.0 - p2) * (1.0 - p3) * C0);

        /* Calculate the wall luminous exitance coefficient                 */
        pcalc->wec = 2.5 / g * (p1 * (1.0 - Dg) * phi_d / (1.0 - p1) * (1.0 -
                                                                         2.5 * p1 * C1 * (C2 + C3) / (g * (1.0 - p1) * C0)) + p1 * p2 * C1
                                 * C2 * phi_u / ((1.0 - p1) * (1.0 - p2) * C0) + p1 * p3 * C1 * C3
                                 * Dg * phi_d / ((1.0 - p1) * (1.0 - p3) * C0));

        /* Calculate the wall direct radiation coefficient                  */
        pcalc->wdrc = 2.5 * phi_d * (1.0 - Dg) / g;
    }

    return (TRUE);
}

/*
*************************************************************************
*
*  IE_CalcCU - Calculate Luminaire Coefficient of Utilization
*
*  Purpose:    To calculate the zonal cavity coefficient of utilization
*              for a luminaire.
*
*  Setup:      double IE_CalcCU
*              (
*                IE_CALC *pcalc,
*                double g,
*                double p1,
*                double p2,
*                double p3
*              )
*
*  Where:      pcalc is a pointer to an IE_CALC data structure.
*              g is the room cavity ratio.
*              p1 is the wall cavity reflectance (0.001 to 0.999).
*              p2 is the effective ceiling cavity reflectance (0.000 to
*                0.999).
*              p3 is the effective floor cavity reflectance (0.000 to
*                0.999).
*
*  Return:     The calculated coefficient of utilization if the input
*              parameters are within range; otherwise 0.0.
*
*  Note:       The following calculations are in accordance with:
*
*                "IES Recommended Procedure for Calculating Coefficients
*                of Utilization, Wall and Ceiling Cavity Exitance", IES
*                Publication LM-57.
*
*************************************************************************
*/

double IE_CalcCU( IE_CALC *pcalc, double g, double p1, double p2,
                  double p3 )
{
    double C0;            /* Intermediate calculation parameter           */
    double C1;            /* Intermediate calculation parameter           */
    double C2;            /* Intermediate calculation parameter           */
    double C3;            /* Intermediate calculation parameter           */
    double f23;           /* Ceiling-floor form factor                    */
    double phi_d = 0.0;   /* Total downward luminaire flux                */
    double phi_u = 0.0;   /* Total upward luminaire flux                  */
    double Dg = 0.0;      /* Luminaire direct ratio                       */
    double cu;            /* Calculated coefficient of utilization        */
    int n;                /* Scratch counter                              */

                          /* Check for conditions which could cause a divide-by-zero error      */
    if ( p1 > 0.999 || p2 > 0.999 || p3 > 0.999 )
        return (0.0);

    /* Calculate the flux functions                                       */
    for ( n = 0; n < IE_VERT_FLUX / 2; n++ )
        phi_d += (double)pcalc->flux[n];

    for ( n = IE_VERT_FLUX / 2; n < IE_VERT_FLUX; n++ )
        phi_u += (double)pcalc->flux[n];

    phi_d /= (double)pcalc->total_lm;
    phi_u /= (double)pcalc->total_lm;

    if ( g < 0.001 )
    {
        /* Calculate the coefficient of utilization                         */
        cu = (phi_d + p2 * phi_u) / (1.0 - p2 * p3);
    } else
    {
        /* Calculate the luminaire direct ratio                             */
        for ( n = 0; n < IE_VERT_FLUX / 2; n++ )
            Dg += exp( -IE_A[n] * pow( g, IE_B[n] ) ) * (double)pcalc->flux[n];

        if ( phi_d > 0.001 )
            Dg /= (phi_d * (double)pcalc->total_lm);

        /* Calculate the form factor approximation                          */
        f23 = 0.026 + 0.503 * exp( -0.270 * g ) + 0.470 * exp( -0.119 * g );

        /* Calculate the intermediate calculation parameters                */
        C1 = (1.0 - p1) * (1.0 - f23 * f23) * g / (2.5 * p1 * (1.0 - f23 *
                                                                f23) + g * f23 * (1.0 - p1));
        C2 = (1.0 - p2) * (1.0 + f23) / (1.0 + p2 * f23);
        C3 = (1.0 - p3) * (1.0 + f23) / (1.0 + p3 * f23);
        C0 = C1 + C2 + C3;

        /* Calculate the coefficient of utilization                         */
        cu = 2.5 * p1 * C1 * C3 * (1.0 - Dg) * phi_d / (g * (1.0 -
                                                              p1) * (1.0 - p3) * C0) + p2 * C2 * C3 * phi_u / ((1.0 - p2) *
                                                              (1.0 - p3) * C0) + (1.0 - p3 * C3 * (C1 + C2) / ((1.0 - p3) *
                                                                                                                C0)) * Dg * phi_d / (1.0 - p3);
    }

    return (cu);
}

/*
*************************************************************************
*
*  IE_GetList - Get List of Values From IESNA-Format Data File
*
*  Purpose:    To read in one or more lines from an IESNA-format data
*              file and convert their substrings to a list of floating
*              point and/or integer values.
*
*  Setup:      static IE_Bool IE_GetList
*              (
*                FILE *pfile,
*                char *format,
*                ...
*              )
*
*  Where:      pfile is a pointer to an IESNA-format data file.
*              format is a format string containing the following
*                specifiers:
*
*                  %d - read in integer value
*                  %f - read in floating point value
*
*                A pointer to a variable of the appropriate type
*                must follow the format parameter in the same order
*                (similar to "scanf").
*
*  Return:     TRUE if no errors occurred; otherwise FALSE.
*
*************************************************************************
*/

static IE_Bool IE_GetList( FILE *pfile, char *format, ... )
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
    if ( (pbuf = IE_GetLine( IE_ValueBuf, IE_MaxLine + 1, pfile )) == NULL )
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
                if ( (pbuf = IE_GetLine( IE_ValueBuf, IE_MaxLine + 1, pfile )) ==
                     NULL )
                    return FALSE;
            } else if ( (isspace( c ) != 0) || c == ',' )
                pbuf++;
            else
                break;
        }
    }

    return TRUE;
}

/*
*************************************************************************
*
*  IE_GetArray - Get Array Elements From IESNA-Format Data File
*
*  Purpose:    To read in one or more lines from an IESNA-format data
*              file and convert their substrings to an array of floating
*              point numbers.
*
*  Setup:      static IE_Bool IE_GetArray
*              (
*                FILE *pfile,
*                float *array,
*                int size
*              )
*
*  Where:      pfile is a pointer to an IESNA-format data file.
*              array is a pointer to an array of floats.
*              size is the number of elements in the array.
*
*  Return:     TRUE if no errors occurred; otherwise FALSE.
*
*************************************************************************
*/

static IE_Bool IE_GetArray
(
    FILE *pfile,
    float *array,
    int size
)
{
    int i = 0;            /* Loop index                                   */
    char c;               /* Scratch variable                             */
    char *pbuf;           /* Input buffer pointer                         */
    float ftemp;          /* Temporary floating point variable            */

                          /* Read in the first line                                             */
    if ( (pbuf = IE_GetLine( IE_ValueBuf, IE_MaxLine + 1, pfile )) == NULL )
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
                if ( (pbuf = IE_GetLine( IE_ValueBuf, IE_MaxLine + 1, pfile )) ==
                     NULL )
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

/*
*************************************************************************
*
*  IE_GetLine - Get Line From File
*
*  Purpose:    To read in a line from a file and remove any trailing
*              newline character.
*
*  Setup:      static char *IE_GetLine
*              (
*                char *pbuf,
*                int size,
*                FILE *pfile
*              )
*
*  Where:      pbuf is a pointer to where the line is to be returned.
*              size is the maximum number of characters to be returned
*                in the buffer, including the '\0' terminator.
*              pfile is the file pointer of the file to be read from.
*
*  Return:     A pointer to pbuf if successful; otherwise NULL.
*
*  Result:     Up to size - 1 characters are read into the buffer,
*              which is then null-terminated.  Any trailing '\n'
*              character is stripped off.
*
*  Note:       IES LM-63 specifies that lines are terminated with a
*              <CR><LF> pair. The "fgets" function as implemented for
*              MS-DOS C compilers typically converts these characters
*              into a single <CR> (i.e., newline) character. Thus,
*              this function should work for both MS-DOS and UNIX
*              environments.
*
*************************************************************************
*/

static char *IE_GetLine( char *pbuf, int size, FILE *pfile )
{
    char *pnl;    /* Newline character pointer                            */

                  /* Read in the line                                                   */
    if ( fgets( pbuf, size, pfile ) != NULL )
    {
        /* Strip off trailing newline (if any)                              */
        if ( (pnl = strchr( pbuf, '\n' )) != NULL )
            *pnl = '\0';

        return pbuf;
    } else
    {
        /* Report error                                                     */
        if ( ferror( pfile ) != 0 )
            fputs( "ERROR: could not read file %s\n", stderr );
        else
            fputs( "ERROR: unexpected end of file %s\n", stderr );

        return NULL;
    }
}

/*
*************************************************************************
*
*  IE_CopyString - Copy String
*
*  Purpose:    To copy a string.
*
*  Setup:      static char *IE_CopyString
*              (
*                char *pstr
*              )
*
*  Where:      pstr is a pointer to the string that is to be copied.
*
*  Return:     A pointer to the copied string if successful; otherwise
*              NULL.
*
*************************************************************************
*/

static char *IE_CopyString( const char *pstr )
{
    char *pdup;   /* Duplicated string pointer                            */

    if ( (pdup = (char *)malloc( strlen( pstr ) + 1 )) != NULL )
        strcpy( pdup, pstr );

    return pdup;
}

/*
*************************************************************************
*
*  IE_AllocErr - Report Memory Allocation Error
*
*  Purpose:    To report an out-of-memory error.
*
*  Setup:      static void IE_AllocErr()
*
*************************************************************************
*/

static void IE_AllocErr()
{
    fputs( "ERROR: out of memory\n", stderr );
}
