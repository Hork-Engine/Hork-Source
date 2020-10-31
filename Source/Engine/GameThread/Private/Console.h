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

#pragma once

#include <Core/Public/Utf8.h>
#include <World/Public/CommandContext.h>

struct SKeyEvent;
struct SCharEvent;
struct SMouseWheelEvent;
class ACanvas;

class AConsole
{
    AN_FORBID_COPY( AConsole )

public:
    AConsole() {}

    /** Clear console text */
    void Clear();

    /** Is console active */
    bool IsActive() const;

    /** Set console to fullscreen mode */
    void SetFullscreen( bool _Fullscreen );

    /** Set console width */
    void Resize( int _VidWidth );

    /** Print Utf8 text */
    void Print( const char * _Text );

    /** Print WideChar text */
    void WidePrint( SWideChar const * _Text );

    /** Process key event */
    void KeyEvent( SKeyEvent const & _Event, ACommandContext & _CommandCtx, ARuntimeCommandProcessor & _CommandProcessor );

    /** Process char event */
    void CharEvent( SCharEvent const & _Event );

    /** Process mouse wheel event */
    void MouseWheelEvent( SMouseWheelEvent const & _Event );

    /** Draw console to canvas */
    void Draw( ACanvas * _Canvas, float _TimeStep );

    /** Write command line history */
    void WriteStoryLines();

    /** Read command line history */
    void ReadStoryLines();

private:
    void _Resize( int _VidWidth );
    void CopyStoryLine( SWideChar const * _StoryLine );
    void AddStoryLine( SWideChar * _Text, int _Length );
    void InsertUTF8Text( const char * _Utf8 );
    void InsertClipboardText();
    void CompleteString( ACommandContext & _CommandCtx, const char * _Str );
    void DrawCmdLine( ACanvas * _Canvas, int x, int y );

    static const int CON_IMAGE_SIZE = 1024*1024;
    static const int MAX_CMD_LINE_CHARS = 256;
    static const int MAX_STORY_LINES = 32;

    SWideChar ImageData[2][CON_IMAGE_SIZE];
    SWideChar * pImage = ImageData[0];
    SWideChar CmdLine[MAX_CMD_LINE_CHARS];
    SWideChar StoryLines[MAX_STORY_LINES][MAX_CMD_LINE_CHARS];
    AThreadSync ConSync;
    int MaxLineChars = 0;
    int PrintLine = 0;
    int CurWidth = 0;
    int MaxLines = 0;
    int NumLines = 0;
    int Scroll = 0;
    float ConHeight = 0;
    int CmdLineLength = 0;
    int CmdLinePos = 0;
    int NumStoryLines = 0;
    int CurStoryLine = 0;
    bool bDown = false;
    bool bFullscreen = false;
    bool bInitialized = false;
};
