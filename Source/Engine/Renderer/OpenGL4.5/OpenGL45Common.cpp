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

#include "OpenGL45Common.h"
#include <Core/Public/Color.h>
#include <Core/Public/Image.h>
#include <Core/Public/CriticalError.h>
#include <Core/Public/Logger.h>

namespace OpenGL45 {

GHI::Device          GDevice;
GHI::State           GState;
GHI::CommandBuffer   Cmd;
SRenderFrame *       GFrameData;
SRenderView *        GRenderView;

void SaveSnapshot( GHI::Texture & _Texture ) {

    const int w = _Texture.GetWidth();
    const int h = _Texture.GetHeight();
    const int numchannels = 3;
    const int size = w * h * numchannels;

    int hunkMark = GHunkMemory.SetHunkMark();

    byte * data = (byte *)GHunkMemory.Alloc( size );

#if 0
    _Texture.Read( 0, GHI::PIXEL_FORMAT_BYTE_RGB, size, 1, data );
#else
    float * fdata = (float *)GHunkMemory.Alloc( size*sizeof(float) );
    _Texture.Read( 0, GHI::PIXEL_FORMAT_FLOAT_RGB, size*sizeof(float), 1, fdata );
    // to sRGB
    for ( int i = 0 ; i < size ; i++ ) {
        data[i] = LinearToSRGB( fdata[i] )*255.0f;
    }
#endif

    FlipImageY( data, w, h, numchannels, w * numchannels );
    
    static int n = 0;
    AFileStream f;
    if ( f.OpenWrite( Core::Fmt( "snapshots/%d.png", n++ ) ) ) {
         WritePNG( f, w, h, numchannels, data, w*numchannels );
    }

    GHunkMemory.ClearToMark( hunkMark );
}

struct SIncludeCtx {
    /** Callback for file loading */
    bool ( *LoadFile )( const char * FileName, AString & Source );

    /** Root path for includes */
    const char * PathToIncludes;

    /** Predefined shaders */
    SMaterialShader const * Predefined;
};

// Modified version of stb_include.h v0.02 originally written by Sean Barrett and Michal Klos

struct SIncludeInfo
{
    int offset;
    int end;
    const char *filename;
    int len;
    int next_line_after;
    SIncludeInfo * next;
};

static void AddInclude( SIncludeInfo *& list, SIncludeInfo *& prev, int offset, int end, const char *filename, int len, int next_line ) {
    SIncludeInfo *z = (SIncludeInfo *)GZoneMemory.Alloc( sizeof( SIncludeInfo ) );
    z->offset = offset;
    z->end = end;
    z->filename = filename;
    z->len = len;
    z->next_line_after = next_line;
    z->next = NULL;
    if ( prev ) {
        prev->next = z;
        prev = z;
    } else {
        list = prev = z;
    }
}

static void FreeIncludes( SIncludeInfo * list ) {
    SIncludeInfo * next;
    for ( SIncludeInfo * includeInfo = list ; includeInfo ; includeInfo = next ) {
        next = includeInfo->next;
        GZoneMemory.Free( includeInfo );
    }
}

static AN_FORCEINLINE int IsSpace( int ch ) {
    return (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');
}

// find location of all #include
static SIncludeInfo * FindIncludes( const char * text ) {
    int line_count = 1;
    const char *s = text, *start;
    SIncludeInfo *list = NULL, *prev = NULL;
    while ( *s ) {
        // parse is always at start of line when we reach here
        start = s;
        while ( *s == ' ' || *s == '\t' ) {
            ++s;
        }
        if ( *s == '#' ) {
            ++s;
            while ( *s == ' ' || *s == '\t' )
                ++s;
            if ( !Core::StrcmpN( s, "include", 7 ) && IsSpace( s[7] ) ) {
                s += 7;
                while ( *s == ' ' || *s == '\t' )
                    ++s;
                if ( *s == '"' ) {
                    const char *t = ++s;
                    while ( *t != '"' && *t != '\n' && *t != '\r' && *t != 0 )
                        ++t;
                    if ( *t == '"' ) {
                        int len = t - s;
                        const char * filename = s;
                        s = t;
                        while ( *s != '\r' && *s != '\n' && *s != 0 )
                            ++s;
                        // s points to the newline, so s-start is everything except the newline
                        AddInclude( list, prev, start-text, s-text, filename, len, line_count+1 );
                    }
                }
            }
        }
        while ( *s != '\r' && *s != '\n' && *s != 0 )
            ++s;
        if ( *s == '\r' || *s == '\n' ) {
            s = s + (s[0] + s[1] == '\r' + '\n' ? 2 : 1);
        }
        ++line_count;
    }
    return list;
}

static void CleanComments( char * s ) {
start:
    while ( *s ) {
        if ( *s == '/' ) {
            if ( *(s+1) == '/' ) {
                *s++ = ' ';
                *s++ = ' ';
                while ( *s && *s != '\n' )
                    *s++ = ' ';
                continue;
            }
            if ( *(s+1) == '*' ) {
                *s++ = ' ';
                *s++ = ' ';
                while ( *s ) {
                    if ( *s == '*' && *(s+1) == '/' ) {
                        *s++ = ' ';
                        *s++ = ' ';
                        goto start;
                    }
                    if ( *s != '\n' ) {
                        *s++ = ' ';
                    } else {
                        s++;
                    }
                }
                // end of file inside comment
                return;
            }
        }
        s++;
    }
}

static bool LoadShaderWithInclude( SIncludeCtx * Ctx, const char * FileName, AString & Out );

static bool LoadShaderFromString( SIncludeCtx * Ctx, const char * FileName, AString const & Source, AString & Out ) {
    char temp[4096];
    SIncludeInfo * includeList = FindIncludes( Source.CStr() );
    size_t sourceOffset = 0;

    for ( SIncludeInfo * includeInfo = includeList ; includeInfo ; includeInfo = includeInfo->next ) {
        Out.ConcatN( &Source[sourceOffset], includeInfo->offset - sourceOffset );

        if ( Ctx->Predefined && includeInfo->filename[0] == '$' ) {
            // predefined source
            Out.Concat( "#line 1 \"" );
            Out.ConcatN( includeInfo->filename, includeInfo->len );
            Out.Concat( "\"\n" );

            SMaterialShader const * s;
            for ( s = Ctx->Predefined ; s ; s = s->Next ) {
                if ( !Core::StricmpN( s->SourceName, includeInfo->filename, includeInfo->len ) ) {
                    break;
                }
            }

            if ( !s || !LoadShaderFromString( Ctx, FileName, s->Code, Out ) ) {
                FreeIncludes( includeList );
                return false;
            }
        } else {
            Out.Concat( "#line 1 \"" );
            Out.Concat( Ctx->PathToIncludes );
            Out.ConcatN( includeInfo->filename, includeInfo->len );
            Out.Concat( "\"\n" );

            Core::Strcpy( temp, sizeof( temp ), Ctx->PathToIncludes );
            Core::StrcatN( temp, sizeof( temp ), includeInfo->filename, includeInfo->len );
            if ( !LoadShaderWithInclude( Ctx, temp, Out ) ) {
                FreeIncludes( includeList );
                return false;
            }
        }        

        Core::Sprintf( temp, sizeof( temp ), "\n#line %d \"%s\"", includeInfo->next_line_after, FileName ? FileName : "source-file" );
        Out.Concat( temp );

        sourceOffset = includeInfo->end;
    }

    Out.ConcatN( &Source[sourceOffset], Source.Length() - sourceOffset + 1 );
    FreeIncludes( includeList );

    return true;
}

static bool LoadShaderWithInclude( SIncludeCtx * Ctx, const char * FileName, AString & Out ) {
    AString source;

    if ( !Ctx->LoadFile( FileName, source ) ) {
        GLogger.Printf( "Couldn't load %s\n", FileName );
        return false;
    }

    CleanComments( source.ToPtr() );

    return LoadShaderFromString( Ctx, FileName, source, Out );
}

static bool LoadInclude( const char * FileName, AString & Source ) {
    AFileStream f;
    if ( !f.OpenRead( FileName ) ) {
        return false;
    }
    Source.FromFile( f );
    return true;
}

AString LoadShader( const char * FileName, SMaterialShader const * Predefined ) {
    AString path = __FILE__;
    path.StripFilename();
    path.FixPath();
    path += "/Shaders/";

    SIncludeCtx ctx;
    ctx.LoadFile = LoadInclude;
    ctx.PathToIncludes = path.CStr();
    ctx.Predefined = Predefined;

    AString result;
    result.Concat( Core::Fmt( "#line 1 \"%s\"\n", FileName ) );

    if ( !LoadShaderWithInclude( &ctx, (path + FileName).CStr(), result ) ) {
        CriticalError( "LoadShader: failed to open %s\n", FileName );
    }

    return result;
}

AString LoadShaderFromString( const char * FileName, const char * Source, SMaterialShader const * Predefined ) {
    AString path = __FILE__;
    path.StripFilename();
    path.FixPath();
    path += "/Shaders/";

    SIncludeCtx ctx;
    ctx.LoadFile = LoadInclude;
    ctx.PathToIncludes = path.CStr();
    ctx.Predefined = Predefined;

    AString result;
    result.Concat( Core::Fmt( "#line 1 \"%s\"\n", FileName ) );

    AString source = Source;

    CleanComments( source.ToPtr() );

    if ( !LoadShaderFromString( &ctx, (path + FileName).CStr(), source, result ) ) {
        CriticalError( "LoadShader: failed to open %s\n", FileName );
    }

    return result;
}

}
