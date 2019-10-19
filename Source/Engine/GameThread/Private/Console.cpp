/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

#include "Console.h"

#include <Engine/GameThread/Public/GameEngine.h>

#include <Engine/World/Public/Canvas.h>

#include <Engine/Runtime/Public/Runtime.h>
#include <Engine/Runtime/Public/InputDefs.h>
#include <Engine/Runtime/Public/RenderCore.h>

#include <Engine/Core/Public/Utf8.h>
#include <Engine/Core/Public/Color.h>

static const int CON_IMAGE_SIZE = 1024*1024;
static const int MAX_CMD_LINE_CHARS = 256;
static const int MAX_STORY_LINES = 32;

static const int Padding = 8;
static const int CharacterWidth = 8;
static const int CharacterHeight = 16;
static const float DropSpeed = 10;

static FWideChar ImageData[2][CON_IMAGE_SIZE];
static FWideChar * pImage = ImageData[0];
static int MaxLineChars = 0;
static int PrintLine = 0;
static int CurWidth = 0;
static int MaxLines = 0;
static int NumLines = 0;
static bool bInitialized = false;
static FThreadSync ConSync;
static int Scroll = 0;
static bool ConDown = false;
static bool ConFullscreen = false;
static float ConHeight = 0;
static FWideChar CmdLine[MAX_CMD_LINE_CHARS];
static int CmdLineLength = 0;
static int CmdLinePos = 0;
static FWideChar StoryLines[MAX_STORY_LINES][MAX_CMD_LINE_CHARS];
static int NumStoryLines = 0;
static int CurStoryLine = 0;

FConsole & GConsole = FConsole::Inst();

FConsole::FConsole() {
}

void FConsole::Clear() {
    FSyncGuard syncGuard( ConSync );

    memset( pImage, 0, sizeof( *pImage ) * CON_IMAGE_SIZE );

    Scroll = 0;
}

bool FConsole::IsActive() const {
    return ConDown || ConFullscreen;
}

void FConsole::SetFullscreen( bool _Fullscreen ) {
    ConFullscreen = _Fullscreen;
}

static void _Resize( int _VidWidth ) {
    int prevMaxLines = MaxLines;
    int prevMaxLineChars = MaxLineChars;

    MaxLineChars = (_VidWidth - Padding*2) / CharacterWidth;

    if ( MaxLineChars == prevMaxLineChars ) {
        return;
    }

    MaxLines = CON_IMAGE_SIZE / MaxLineChars;

    if ( NumLines > MaxLines ) {
        NumLines = MaxLines;
    }

    FWideChar * pNewImage = ( pImage == ImageData[0] ) ? ImageData[1] : ImageData[0];

    memset( pNewImage, 0, sizeof( *pNewImage ) * CON_IMAGE_SIZE );

    const int width = FMath::Min( prevMaxLineChars, MaxLineChars );
    const int height = FMath::Min( prevMaxLines, MaxLines );

    for ( int i = 0 ; i < height ; i++ ) {
        const int newOffset = ( MaxLines - i - 1 ) * MaxLineChars;
        const int oldOffset = ( ( prevMaxLines + PrintLine - i ) % prevMaxLines ) * prevMaxLineChars;

        memcpy( &pNewImage[ newOffset ], &pImage[ oldOffset ], width * sizeof( *pNewImage ) );
    }

    pImage = pNewImage;

    PrintLine = MaxLines - 1;
    Scroll = 0;
}

void FConsole::Resize( int _VidWidth ) {
    FSyncGuard syncGuard( ConSync );

    _Resize( _VidWidth );
}

void FConsole::Print( const char * _Text ) {
    const char * wordStr;
    int wordLength;
    FWideChar ch;
    int byteLen;

    FSyncGuard syncGuard( ConSync );

    if ( !bInitialized ) {
        _Resize( 640 );
        bInitialized = true;
    }

    const char * s = _Text;

    while ( *s ) {
        byteLen = FCore::WideCharDecodeUTF8( s, ch );
        if ( !byteLen ) {
            break;
        }

        switch ( ch ) {
        case ' ':
            pImage[ PrintLine * MaxLineChars + CurWidth++ ] = ' ';
            if ( CurWidth == MaxLineChars ) {
                CurWidth = 0;
                PrintLine = ( PrintLine + 1 ) % MaxLines;
                NumLines++;
            }
            s += byteLen;
            break;
        case '\t':
            if ( CurWidth + 4 >= MaxLineChars ) {
                CurWidth = 0;
                PrintLine = ( PrintLine + 1 ) % MaxLines;
                NumLines++;
            } else {
                pImage[ PrintLine * MaxLineChars + CurWidth++ ] = ' ';
                pImage[ PrintLine * MaxLineChars + CurWidth++ ] = ' ';
                pImage[ PrintLine * MaxLineChars + CurWidth++ ] = ' ';
                pImage[ PrintLine * MaxLineChars + CurWidth++ ] = ' ';
            }
            s += byteLen;
            break;
        case '\n':
        case '\r':
            pImage[ PrintLine * MaxLineChars + CurWidth++ ] = '\0';
            CurWidth = 0;
            PrintLine = ( PrintLine + 1 ) % MaxLines;
            NumLines++;
            s += byteLen;
            break;
        default:
            wordStr = s;
            wordLength = 0;

            if ( ch > ' ' ) {
                do {
                    s += byteLen;
                    wordLength++;
                    byteLen = FCore::WideCharDecodeUTF8( s, ch );
                } while ( byteLen > 0 && ch > ' ' );
            } else {
                s += byteLen;
            }

            if ( CurWidth + wordLength > MaxLineChars ) {
                CurWidth = 0;
                PrintLine = ( PrintLine + 1 ) % MaxLines;
                NumLines++;
            }

            while ( wordLength-- > 0 ) {
                byteLen = FCore::WideCharDecodeUTF8( wordStr, ch );
                wordStr += byteLen;

                pImage[ PrintLine * MaxLineChars + CurWidth++ ] = ch;
                if ( CurWidth == MaxLineChars ) {
                    CurWidth = 0;
                    PrintLine = ( PrintLine + 1 ) % MaxLines;
                    NumLines++;
                }
            }
            break;
        }
    }

    if ( NumLines > MaxLines ) {
        NumLines = MaxLines;
    }
}

void FConsole::WidePrint( FWideChar const * _Text ) {
    FWideChar const * wordStr;
    int wordLength;

    FSyncGuard syncGuard( ConSync );

    if ( !bInitialized ) {
        _Resize( 640 );
        bInitialized = true;
    }

    while ( *_Text ) {
        switch ( *_Text ) {
        case ' ':
            pImage[ PrintLine * MaxLineChars + CurWidth++ ] = ' ';
            if ( CurWidth == MaxLineChars ) {
                CurWidth = 0;
                PrintLine = ( PrintLine + 1 ) % MaxLines;
                NumLines++;
            }
            _Text++;
            break;
        case '\t':
            if ( CurWidth + 4 >= MaxLineChars ) {
                CurWidth = 0;
                PrintLine = ( PrintLine + 1 ) % MaxLines;
                NumLines++;
            } else {
                pImage[ PrintLine * MaxLineChars + CurWidth++ ] = ' ';
                pImage[ PrintLine * MaxLineChars + CurWidth++ ] = ' ';
                pImage[ PrintLine * MaxLineChars + CurWidth++ ] = ' ';
                pImage[ PrintLine * MaxLineChars + CurWidth++ ] = ' ';
            }
            _Text++;
            break;
        case '\n':
        case '\r':
            pImage[ PrintLine * MaxLineChars + CurWidth++ ] = '\0';
            CurWidth = 0;
            PrintLine = ( PrintLine + 1 ) % MaxLines;
            NumLines++;
            _Text++;
            break;
        default:
            wordStr = _Text;
            wordLength = 0;

            if ( *_Text > ' ' ) {
                do {
                    _Text++;
                    wordLength++;
                } while ( *_Text > ' ' );
            } else {
                _Text++;
            }

            if ( CurWidth + wordLength > MaxLineChars ) {
                CurWidth = 0;
                PrintLine = ( PrintLine + 1 ) % MaxLines;
                NumLines++;
            }

            while ( wordLength-- > 0 ) {
                pImage[ PrintLine * MaxLineChars + CurWidth++ ] = *wordStr++;
                if ( CurWidth == MaxLineChars ) {
                    CurWidth = 0;
                    PrintLine = ( PrintLine + 1 ) % MaxLines;
                    NumLines++;
                }
            }
            break;
        }
    }

    if ( NumLines > MaxLines ) {
        NumLines = MaxLines;
    }
}

static void CopyStoryLine( FWideChar const * _StoryLine ) {
    CmdLineLength = 0;
    while ( *_StoryLine && CmdLineLength < MAX_CMD_LINE_CHARS ) {
        CmdLine[ CmdLineLength++ ] = *_StoryLine++;
    }
    CmdLinePos = CmdLineLength;
}

static void AddStoryLine( FWideChar * _Text, int _Length ) {
    FWideChar * storyLine = StoryLines[NumStoryLines++ & ( MAX_STORY_LINES - 1 )];
    memcpy( storyLine, _Text, sizeof( _Text[0] ) * FMath::Min( _Length, MAX_CMD_LINE_CHARS ) );
    if ( _Length < MAX_CMD_LINE_CHARS ) {
        storyLine[_Length] = 0;
    }
    CurStoryLine = NumStoryLines;
}

static void InsertUTF8Text( const char * _Utf8 ) {
    int len = FCore::UTF8StrLength( _Utf8 );
    if ( CmdLineLength + len >= MAX_CMD_LINE_CHARS ) {
        GLogger.Print( "Text is too long to be copied to command line\n" );
        return;
    }

    if ( len && CmdLinePos != CmdLineLength ) {
        memmove( &CmdLine[CmdLinePos+len], &CmdLine[CmdLinePos], sizeof( CmdLine[0] ) * ( CmdLineLength - CmdLinePos ) );
    }

    CmdLineLength += len;

    FWideChar ch;
    int byteLen;
    while ( len-- > 0 ) {
        byteLen = FCore::WideCharDecodeUTF8( _Utf8, ch );
        if ( !byteLen ) {
            break;
        }
        _Utf8 += byteLen;
        CmdLine[CmdLinePos++] = ch;
    }
}

static void InsertClipboardText() {
    FString const & clipboard = GRuntime.GetClipboard();

    InsertUTF8Text( clipboard.ToConstChar() );
}

static void CompleteString( FCommandContext & _CommandCtx, const char * _Str ) {
    FString completion;
    int count = _CommandCtx.CompleteString( _Str, strlen( _Str ), completion );

    if ( completion.IsEmpty() ) {
        return;
    }

    if ( count > 1 ) {
        _CommandCtx.Print( _Str, CmdLinePos );
    } else {
        completion += " ";
    }

    CmdLinePos = 0;
    CmdLineLength = 0;
    InsertUTF8Text( completion.ToConstChar() );
}

void FConsole::KeyEvent( FKeyEvent const & _Event, FCommandContext & _CommandCtx, FRuntimeCommandProcessor & _CommandProcessor ) {
    if ( _Event.Action == IE_Press ) {
        if ( !ConFullscreen && _Event.Key == KEY_GRAVE_ACCENT ) {
            ConDown = !ConDown;

            if ( !ConDown ) {
                CmdLineLength = CmdLinePos = 0;
                CurStoryLine = NumStoryLines;
            }
        }
    }

    if ( IsActive() && ( _Event.Action == IE_Press || _Event.Action == IE_Repeat ) ) {

        // Scrolling (protected by mutex)
        {
            FSyncGuard syncGuard( ConSync );

            int scrollDelta = 1;
            if ( _Event.ModMask & MOD_MASK_CONTROL ) {
                if ( _Event.Key == KEY_HOME ) {
                    Scroll = NumLines-1;
                } else if ( _Event.Key == KEY_END ) {
                    Scroll = 0;
                }
                scrollDelta = 4;
            }

            switch ( _Event.Key ) {
            case KEY_PAGE_UP:
                Scroll += scrollDelta;
                break;
            case KEY_PAGE_DOWN:
                Scroll -= scrollDelta;
                if ( Scroll < 0 ) {
                    Scroll = 0;
                }
                break;
            //case KEY_ENTER:
            //    Scroll = 0;
            //    break;
            }
        }

        // Command line keys
        switch ( _Event.Key ) {
        case KEY_LEFT:
            if ( _Event.ModMask & MOD_MASK_CONTROL ) {
                while ( CmdLinePos > 0 && CmdLinePos <= CmdLineLength && CmdLine[ CmdLinePos-1 ] == ' ') {
                    CmdLinePos--;
                }
                while ( CmdLinePos > 0 && CmdLinePos <= CmdLineLength && CmdLine[ CmdLinePos-1 ] != ' ') {
                    CmdLinePos--;
                }
            } else {
                CmdLinePos--;
                if ( CmdLinePos < 0 ) {
                    CmdLinePos = 0;
                }
            }
            break;
        case KEY_RIGHT:
            if ( _Event.ModMask & MOD_MASK_CONTROL ) {
                while ( CmdLinePos < CmdLineLength && CmdLine[ CmdLinePos ] != ' ') {
                    CmdLinePos++;
                }
                while ( CmdLinePos < CmdLineLength && CmdLine[ CmdLinePos ] == ' ') {
                    CmdLinePos++;
                }
            } else {
                if ( CmdLinePos < CmdLineLength ) {
                    CmdLinePos++;
                }
            }
            break;
        case KEY_END:
            CmdLinePos = CmdLineLength;
            break;
        case KEY_HOME:
            CmdLinePos = 0;
            break;
        case KEY_BACKSPACE:
            if ( CmdLinePos > 0 ) {
                memmove( CmdLine + CmdLinePos - 1, CmdLine + CmdLinePos, sizeof( CmdLine[0] ) * ( CmdLineLength - CmdLinePos ) );
                CmdLineLength--;
                CmdLinePos--;
            }
            break;
        case KEY_DELETE:
            if ( CmdLinePos < CmdLineLength ) {
                memmove( CmdLine + CmdLinePos, CmdLine + CmdLinePos + 1, sizeof( CmdLine[0] ) * ( CmdLineLength - CmdLinePos - 1 ) );
                CmdLineLength--;
            }
            break;
        case KEY_ENTER: {
            char result[ MAX_CMD_LINE_CHARS * 4 + 1 ];   // In worst case FWideChar transforms to 4 bytes,
                                                         // one additional byte is reserved for trailing '\0'

            FCore::WideStrEncodeUTF8( result, sizeof( result ), CmdLine, CmdLine + CmdLineLength );

            if ( CmdLineLength > 0 ) {
                AddStoryLine( CmdLine, CmdLineLength );
            }

            GLogger.Printf( "%s\n", result );

            _CommandProcessor.Add( result );
            _CommandProcessor.Add( "\n" );

            CmdLineLength = 0;
            CmdLinePos = 0;
            break;
        }
        case KEY_DOWN:
            CmdLineLength = 0;
            CmdLinePos = 0;

            CurStoryLine++;

            if ( CurStoryLine < NumStoryLines ) {
                CopyStoryLine( StoryLines[ CurStoryLine & ( MAX_STORY_LINES - 1 ) ] );
            } else  if ( CurStoryLine > NumStoryLines ) {
                CurStoryLine = NumStoryLines;
            }
            break;
        case KEY_UP: {
            CmdLineLength = 0;
            CmdLinePos = 0;

            CurStoryLine--;

            const int n = NumStoryLines - ( NumStoryLines < MAX_STORY_LINES ? NumStoryLines : MAX_STORY_LINES ) - 1;

            if ( CurStoryLine > n ) {
                CopyStoryLine( StoryLines[ CurStoryLine & ( MAX_STORY_LINES - 1 ) ] );
            }

            if ( CurStoryLine < n ) {
                CurStoryLine = n;
            }
            break;
        }
        case KEY_V:
            if ( _Event.ModMask & MOD_MASK_CONTROL ) {
                InsertClipboardText();
            }
            break;
        case KEY_TAB: {
            char result[ MAX_CMD_LINE_CHARS * 4 + 1 ];   // In worst case FWideChar transforms to 4 bytes,
                                                         // one additional byte is reserved for trailing '\0'

            FCore::WideStrEncodeUTF8( result, sizeof( result ), CmdLine, CmdLine + CmdLinePos );

            CompleteString( _CommandCtx, result );
            break;
        }
        default:
            break;
        }
    }
}

void FConsole::CharEvent( FCharEvent const & _Event ) {
    if ( !IsActive() ) {
        return;
    }

    if ( _Event.UnicodeCharacter == '`' ) {
        return;
    }

    if ( CmdLineLength < MAX_CMD_LINE_CHARS ) {
        if ( CmdLinePos != CmdLineLength ) {
            memmove( &CmdLine[CmdLinePos+1], &CmdLine[CmdLinePos], sizeof( CmdLine[0] ) * ( CmdLineLength - CmdLinePos ) );
        }
        CmdLine[ CmdLinePos ] = _Event.UnicodeCharacter;
        CmdLineLength++;
        CmdLinePos++;
    }
}

void FConsole::MouseWheelEvent( FMouseWheelEvent const & _Event ) {
    if ( !IsActive() ) {
        return;
    }

    FSyncGuard syncGuard( ConSync );

    if ( _Event.WheelY < 0.0 ) {
        Scroll--;
        if ( Scroll < 0 ) {
            Scroll = 0;
        }
    } else if ( _Event.WheelY > 0.0 ) {
        Scroll++;
    }
}

static void DrawCmdLine( FCanvas * _Canvas, int x, int y ) {
    FColor4 const & charColor = FColor4::White();

    FFont * font = _Canvas->GetCurrentFont();

    const float scale = (float)CharacterHeight / font->GetFontSize();

    int cx = x;

    int offset = CmdLinePos + 1 - MaxLineChars;
    if ( offset < 0 ) {
        offset = 0;
    }
    int numDrawChars = CmdLineLength;
    if ( numDrawChars > MaxLineChars ) {
        numDrawChars = MaxLineChars;
    }
    for ( int j = 0 ; j < numDrawChars ; j++ ) {

        int n = j + offset;

        if ( n >= CmdLineLength ) {
            break;
        }

        FWideChar ch = CmdLine[n];

        if ( ch <= ' ' ) {
            cx += CharacterWidth;
            continue;
        }

        _Canvas->DrawWChar( font, ch, cx, y, scale, charColor );

        cx += CharacterWidth;
    }

    if ( ( GRuntime.SysFrameTimeStamp() >> 18 ) & 1 ) {
        cx = x + ( CmdLinePos - offset ) * CharacterWidth;

        _Canvas->DrawWChar( font, '_', cx, y, scale, charColor );
    }
}

void FConsole::Draw( FCanvas * _Canvas, float _TimeStep ) {

    if ( !ConFullscreen ) {
        if ( ConDown ) {
            ConHeight += DropSpeed * _TimeStep;
        } else {
            ConHeight -= DropSpeed * _TimeStep;
        }

        if ( ConHeight <= 0.0f ) {
            ConHeight = 0.0f;
            return;
        }

        if ( ConHeight > 1.0f ) {
            ConHeight = 1.0f;
        }
    } else {
        ConHeight = 2;
    }

    const int fontVStride = CharacterHeight + 4;
    const int cmdLineH = fontVStride;
    const float halfVidHeight = ( _Canvas->Height >> 1 ) * ConHeight;
    const int numVisLines = FMath::Ceil( ( halfVidHeight - cmdLineH ) / fontVStride );

    const FColor4 c1(0,0,0,1.0f);
    const FColor4 c2(0,0,0,0.0f);
    const FColor4 charColor(1,1,1,1);

    if ( ConFullscreen ) {
        _Canvas->DrawRectFilled( Float2( 0, 0 ), Float2( _Canvas->Width, _Canvas->Height ), FColor4::Black() );
    } else {
        _Canvas->DrawRectFilledMultiColor( Float2( 0, 0 ), Float2( _Canvas->Width, halfVidHeight ), c1, c2, c2, c1 );
    }
    _Canvas->DrawLine( Float2( 0, halfVidHeight ), Float2( _Canvas->Width, halfVidHeight ), FColor4::White(), 2.0f );

    int x = Padding;
    int y = halfVidHeight - fontVStride;

    FFont * font = _Canvas->GetCurrentFont();

    const float scale = (float)CharacterHeight / font->GetFontSize();

    ConSync.BeginScope();

    DrawCmdLine( _Canvas, x, y );

    y -= cmdLineH;

    for ( int i = 0 ; i < numVisLines ; i++ ) {
        int n = i + Scroll;
        if ( n >= MaxLines ) {
            break;
        }

        const int offset = ( ( MaxLines + PrintLine - n - 1 ) % MaxLines ) * MaxLineChars;
        FWideChar * line = &pImage[ offset ];

        for ( int j = 0 ; j < MaxLineChars && *line ; j++ ) {
            _Canvas->DrawWChar( font, *line++, x, y, scale, charColor );

            x += CharacterWidth;
        }
        x = Padding;
        y -= fontVStride;
    }

    ConSync.EndScope();
}

void FConsole::WriteStoryLines() {
    if ( !NumStoryLines ) {
        return;
    }

    FFileStream f;
    if ( !f.OpenWrite( "console_story.txt" ) ) {
        GLogger.Printf( "Failed to write console story\n" );
        return;
    }

    char result[ MAX_CMD_LINE_CHARS * 4 + 1 ];   // In worst case FWideChar transforms to 4 bytes,
                                                 // one additional byte is reserved for trailing '\0'

    int numLines = FMath::Min( MAX_STORY_LINES, NumStoryLines );

    for ( int i = 0 ; i < numLines ; i++ ) {
        int n = ( NumStoryLines - numLines + i ) & ( MAX_STORY_LINES - 1 );

        FCore::WideStrEncodeUTF8( result, sizeof( result ), StoryLines[n], StoryLines[n] + MAX_CMD_LINE_CHARS );

        f.Printf( "%s\n", result );
    }
}

void FConsole::ReadStoryLines() {
    FWideChar wideStr[ MAX_CMD_LINE_CHARS ];
    int wideStrLength;
    char buf[ MAX_CMD_LINE_CHARS * 3 + 2 ]; // In worst case FWideChar transforms to 3 bytes,
                                            // two additional bytes are reserved for trailing '\n\0'

    FFileStream f;
    if ( !f.OpenRead( "console_story.txt" ) ) {
        return;
    }

    NumStoryLines = 0;
    while ( NumStoryLines < MAX_STORY_LINES && f.Gets( buf, sizeof( buf ) ) ) {
        wideStrLength = 0;

        const char * s = buf;
        while ( *s && *s != '\n' && wideStrLength < MAX_CMD_LINE_CHARS ) {
            int byteLen = FCore::WideCharDecodeUTF8( s, wideStr[wideStrLength] );
            if ( !byteLen ) {
                break;
            }
            s += byteLen;
            wideStrLength++;
        }

        if ( wideStrLength > 0 ) {
            AddStoryLine( wideStr, wideStrLength );
        }
    }
}
