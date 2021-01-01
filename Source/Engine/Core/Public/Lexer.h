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

#pragma once

#include "Angl.h"
#include "PodArray.h"

class ALexer {
    AN_FORBID_COPY( ALexer )

public:
    enum EErrorCode {
        ERROR_EOF = -2, // no error, end of file found
        ERROR_EOL = -1, // no error, end of line found (appear only with CrossLine = false)
        ERROR_NO = 0,
        ERROR_UNEXPECTED_EOF_FOUND,
        ERROR_UNEXPECTED_EOF_IN_COMMENT,
        ERROR_UNEXPECTED_NEWLINE_FOUND,
        ERROR_UNEXPECTED_TOKEN_FOUND,
        ERROR_EOF_INSIDE_QUOTE,
        ERROR_NEWLINE_INSIDE_QUOTE,
        ERROR_NEWLINE_IN_CONSTANT,
        ERROR_TOKEN_IS_TOO_LONG,
        ERROR_TOO_MANY_CHARS_IN_STRING,
        ERROR_EXPECTED_IDENTIFIER,
        ERROR_EXPECTED_STRING,
        ERROR_EXPECTED_INTEGER,
        ERROR_EXPECTED_REAL,
        ERROR_MAX
    };

    enum ETokenType {
        TOKEN_TYPE_BAD = 0x00,
        TOKEN_TYPE_IDENTIFIER = 0x01,
        TOKEN_TYPE_STRING = 0x02,
        TOKEN_TYPE_INTEGER = 0x04,
        TOKEN_TYPE_REAL = 0x08,
        TOKEN_TYPE_RESERVED1 = 0x10,
        TOKEN_TYPE_RESERVED2 = 0x20,
        TOKEN_TYPE_RESERVED3 = 0x40,
        TOKEN_TYPE_RESERVED4 = 0x80,
        TOKEN_TYPE_ANY = 0xFF,
    };

    enum EMessageType {
        MSG_ERROR = 1,
        MSG_WARNING = 2
    };

    enum EPrintFlags {
        PRINT_BUFFER_NAME = 0x01,
        PRINT_BUFFER_LINE = 0x02,
        PRINT_ALL = 0xFF
    };

    enum EConstants {
        MAX_TOKEN_LENGTH = 1024
    };

    ALexer();
    virtual ~ALexer();

    void Initialize( const char * InBuffer, const char * InBufferName = nullptr, int InPrint = PRINT_ALL );

    void AddOperator( const char * _String );

    int CheckOperator( const char * _Ptr ) const;

    /** Step back to previous token */
    void PrevToken();

    /** Step to next token */
    int NextToken( bool _CrossLine = true );

    /** Expect token (compare token with string) */
    int Expect( const char * _Str, int _TokenType = TOKEN_TYPE_ANY, bool _MatchCase = false );

    /** Get token type */
    int GetTokenType() const { return TokenType; }

    /** Skip all in block { } */
    int SkipBlock();

    /** Skip all data until '\n' */
    void SkipRestOfLine();

    /** Copy rest of the line to dest
    If fixpos=true then do not move current position in the text buffer */
    int GetRestOfLine( char * _Dest, int _DestSz, bool _FixPos = false );

    /** Covert error code to error string */
    const char * GetError( int _Error ) const;

    /** Get current error string */
    const char * GetError() const;

    /** Get error code */
    int GetErrorCode() const { return ErrorCode; }

    /** Get current parsing line */
    int GetCurrentLine() const { return CurrentLine; }

    /** Get token string */
    const char * Token() const { return CurToken; }

    const char * GetBufferName() const { return BufferName.CStr(); }

    const char * GetIdentifier( bool _CrossLine = true );
    const char * GetInteger( bool _CrossLine = true );
    const char * GetString( bool _CrossLine = true );

    const char * ExpectIdentifier( bool _CrossLine = true );
    const char * ExpectString( bool _CrossLine = true );
    int32_t ExpectInteger( bool _CrossLine = true );
    bool ExpectBoolean( bool _CrossLine = true );
    float ExpectFloat( bool _CrossLine = true );
    double ExpectDouble( bool _CrossLine = true );

    bool ExpectQuaternion( Quat & _DestQuat, bool _CrossLine = true );
    bool ExpectVector( Float2 & _DestVector, bool _CrossLine = true );
    bool ExpectVector( Float3 & _DestVector, bool _CrossLine = true );
    bool ExpectVector( Float4 & _DestVector, bool _CrossLine = true );
    bool ExpectVector( float * _DestVector, int _NumComponents, bool _CrossLine = true );
    bool ExpectDVector( double * _DestVector, int _NumComponents, bool _CrossLine = true );
    bool ExpectIVector( int * _DestVector, int _NumComponents, bool _CrossLine = true );
    bool ExpectAngles( Angl & _DestAngles, bool _CrossLine = true );

    bool GoToNearest( const char * _Identifier );

    void ErrorPrint( int _Err );
    void ErrorPrintf( const char * _Format, ... );
    void WarnPrintf( const char * _Format, ... );

private:
    int TokenBegin( bool _CrossLine );

    void MakeString( AString & Str, int InMessage, const char * InText );

    enum {
        MAX_OPERATOR_LENGTH = 16
    };

    struct SOperator {
        char Str[ MAX_OPERATOR_LENGTH ];
        int Len;
    };

    AString BufferName;
    TPodArray< SOperator > Operators;
    char CurToken[ MAX_TOKEN_LENGTH ];
    const char * Ptr;
    int CurrentLine;
    bool bPrevToken;
    int ErrorCode;
    int TokenType;
    int PrintFlags;
};
