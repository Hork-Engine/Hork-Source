/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Hork Engine Source Code.

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

#ifndef _IESNA_H
#define _IESNA_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int IE_Bool;               /* Boolean flag                         */

typedef enum
{
    IESNA_86,                 /* LM-63-1986                           */
    IESNA_91,                 /* LM-63-1991                           */
    IESNA_95                  /* LM-63-1995                           */
} IE_Format;

enum IE_LampOrientation                     /* Lamp-to-luminaire geometry           */
{
    LampVert = 1,           /* Lamp vertical base up or down        */
    LampHorz = 2,           /* Lamp horizontal                      */
    LampTilt = 3            /* Lamp tilted                          */
};

typedef struct          /* IESNA Standard File data             */
{
  IE_Format format;

  struct                        /* Lamp data                            */
  {
    int num_lamps;              /* Number of lamps                      */
    float lumens_lamp;          /* Lumens per lamp                      */
    float multiplier;           /* Candela multiplying factor           */

    struct                      /* TILT data structure                  */
    {
      int orientation;

      int num_pairs;            /* # of angle-multiplying factor pairs  */
      float *angles;            /* Angles array pointer                 */
      float *mult_factors;      /* Multiplying factors array pointer    */
    }
    tilt;
  }
  lamp;

  enum                          /* Measurement units                    */
  {
    Feet = 1,                   /* Imperial                             */
    Meters = 2                  /* Standard Internationale              */
  }
  units;

  struct                        /* Luminous cavity dimensions           */
  {
    float width;                /* Opening width                        */
    float length;               /* Opening length                       */
    float height;               /* Cavity height                        */
  }
  dim;

  struct                        /* Electrical data                      */
  {
    float ball_factor;          /* Ballast factor                       */
    float blp_factor;           /* Ballast-lamp photometric factor      */
    float input_watts;          /* Input watts                          */
  }
  elec;

  struct                        /* Photometric data                     */
  {
    enum                        /* Photometric goniometer type          */
    {
      Type_A = 3,               /* Type A                               */
      Type_B = 2,               /* Type B                               */
      Type_C = 1                /* Type C                               */
    }
    gonio_type;

    int num_vert_angles;        /* Number of vertical angles            */
    int num_horz_angles;        /* Number of horizontal angles          */

    float *vert_angles;         /* Vertical angles array                */
    float *horz_angles;         /* Horizontal angles array              */

    float **pcandela;           /* Candela values array pointers array  */
  }
  photo;
} IE_DATA;

typedef struct
{
    void * (*calloc)(size_t, size_t);
    void ( *free )(void *);
    void (*rewind)( void * userData );
    char * (*fgets)( char * pbuf, size_t size, void * userData );
    void * userData;
} IE_Context;

extern IE_Bool IES_Load( IE_Context const * context, IE_DATA * );
extern void IES_Free( IE_Context const * context, IE_DATA * );

#ifdef __cplusplus
} // extern "C"
#endif

#endif
