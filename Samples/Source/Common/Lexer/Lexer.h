/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#pragma once

#include <Hork/Math/Angl.h>
#include <Hork/Core/Containers/Vector.h>

HK_NAMESPACE_BEGIN

class Lexer final
{
public:
    enum class ErrorCode
    {
        EndOfFile = -2, // no error, end of file found
        EndOfLine = -1, // no error, end of line found (appear only with CrossLine = false)
        No  = 0,
        UnexpectedEOF,
        UnexpectedEOFInComment,
        UnexpectedNewLine,
        UnexpectedToken,
        EndOfFileInsideQuote,
        NewLineInsideQuote,
        NewLineInConstant,
        TokenIsTooLong,
        StringIsTooLong,
        ExpectedIdentifier,
        ExpectedString,
        ExpectedInteger,
        ExpectedReal,
        MAX
    };

    enum class TokenType
    {
        BAD,
        IDENTIFIER,
        STRING,
        INTEGER,
        REAL,
        ANY
    };

    enum class PrintFlags : uint8_t
    {
        PrintName           = 1,
        PrintLine           = 2,
        PrintAll            = 0xff
    };

    enum class CrossLine
    {
        No,
        Yes
    };

    void                    SetSource(const char* buffer);

    void                    SetPrintFlags(PrintFlags printFlags);
    PrintFlags              GetPrintFlags() const { return m_PrintFlags; }

    void                    SetName(StringView name);
    String const&           GetName() const { return m_Name; }

    void                    AddOperator(StringView name);

    int                     ParseOperator(const char* str) const;

    /// Step back to previous token
    void                    PrevToken();

    /// Step to next token
    ErrorCode               NextToken(CrossLine crossLine = CrossLine::Yes);

    /// Expect token (compare token with string)
    ErrorCode               Expect(const char* name, TokenType tokenType = TokenType::ANY, bool matchCase = false);

    /// Get token type
    TokenType               GetTokenType() const { return m_TokenType; }

    /// Skip all in block { }
    ErrorCode               SkipBlock();

    /// Skip all data until '\n'
    void                    SkipRestOfLine();

    /// Copy rest of the line to dest
    /// If fixpos=true then do not move current position in the text buffer
    ErrorCode               GetRestOfLine(char* buffer, int bufferSz, bool fixPos = false);

    /// Covert error code to error string
    const char*             GetError(ErrorCode errcode) const;

    /// Get current error string
    const char*             GetError() const;

    /// Get error code
    ErrorCode               GetErrorCode() const { return m_ErrorCode; }

    /// Get current parsing line
    int                     GetCurrentLine() const { return m_LineNum; }

    /// Get token string
    const char*             Token() const { return m_Token; }

    const char*             GetIdentifier(CrossLine crossLine = CrossLine::Yes);
    const char*             GetInteger(CrossLine crossLine = CrossLine::Yes);
    const char*             GetString(CrossLine crossLine = CrossLine::Yes);

    const char*             ExpectIdentifier(CrossLine crossLine = CrossLine::Yes);
    const char*             ExpectString(CrossLine crossLine = CrossLine::Yes);
    int32_t                 ExpectInteger(CrossLine crossLine = CrossLine::Yes);
    bool                    ExpectBoolean(CrossLine crossLine = CrossLine::Yes);
    float                   ExpectFloat(CrossLine crossLine = CrossLine::Yes);
    double                  ExpectDouble(CrossLine crossLine = CrossLine::Yes);

    bool                    ExpectQuaternion(Quat& q, CrossLine crossLine = CrossLine::Yes);
    bool                    ExpectVector(Float2& v, CrossLine crossLine = CrossLine::Yes);
    bool                    ExpectVector(Float3& v, CrossLine crossLine = CrossLine::Yes);
    bool                    ExpectVector(Float4& v, CrossLine crossLine = CrossLine::Yes);
    bool                    ExpectVector(float* v, int numComponents, CrossLine crossLine = CrossLine::Yes);
    bool                    ExpectDVector(double* v, int numComponents, CrossLine crossLine = CrossLine::Yes);
    bool                    ExpectIVector(int* v, int numComponents, CrossLine crossLine = CrossLine::Yes);
    bool                    ExpectAngles(Angl& angles, CrossLine crossLine = CrossLine::Yes);

    bool                    GoToNearest(const char* identifier);

    void                    ErrorPrint(ErrorCode errcode);

private:
    ErrorCode               TokenBegin(CrossLine crossLine);

    enum class MessageType
    {
        Error   = 1,
        Warning = 2
    };

    String                  MsgPrefix(MessageType type) const;
    String                  MsgError() const { return MsgPrefix(MessageType::Error); }
    String                  MsgWarning() const { return MsgPrefix(MessageType::Warning); }

    enum { MAX_TOKEN_LENGTH = 1024  };

    String                  m_Name;
    Vector<String>          m_Operators;
    char                    m_Token[MAX_TOKEN_LENGTH] = {};
    const char*             m_Ptr = "";
    int                     m_LineNum = 1;
    bool                    m_IsPrevToken = false;
    ErrorCode               m_ErrorCode = ErrorCode::No;
    TokenType               m_TokenType = TokenType::BAD;
    PrintFlags              m_PrintFlags = PrintFlags::PrintAll;
};

HK_FLAG_ENUM_OPERATORS(Lexer::PrintFlags)

HK_NAMESPACE_END
