/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include "Lexer.h"
#include <Hork/Core/Logger.h>
#include <Hork/Core/Parse.h>

HK_NAMESPACE_BEGIN

namespace
{
    static const char* ErrorStr[ToUnderlying(Lexer::ErrorCode::MAX)] = {
        "no error",
        "unexpected EOF found",
        "unexpected end of file found in comment",
        "unexpected newline found",
        "unexpected token found",
        "EOF inside quote",
        "newline inside quote",
        "newline in constant",
        "token is too long",
        "string is too long",
        "expected identifier",
        "expected string",
        "expected integer",
        "expected real"
    };
}

void Lexer::SetSource(const char* buffer)
{
    m_LineNum = 1;
    m_Ptr = buffer ? buffer : "";
    m_IsPrevToken = false;
}

void Lexer::SetPrintFlags(PrintFlags printFlags)
{
    m_PrintFlags = printFlags;
}

void Lexer::SetName(StringView name)
{
    m_Name = name;
}

String Lexer::MsgPrefix(MessageType type) const
{
    String prefix;

    if (!!(m_PrintFlags & PrintFlags::PrintName) && !m_Name.IsEmpty())
    {
        prefix += m_Name;
        prefix += " ";
    }

    if (!!(m_PrintFlags & PrintFlags::PrintLine))
        prefix += TSprintfBuffer<32>().Sprintf("ln=%d ", m_LineNum);

    if (ToUnderlying(m_PrintFlags) && !prefix.IsEmpty())
        prefix += ": ";

    switch (type)
    {
        case MessageType::Error:
            prefix += "error:";
            break;
        case MessageType::Warning:
            prefix += "warning:";
            break;
    }

    return prefix;
}

void Lexer::ErrorPrint(ErrorCode errcode)
{
    LOG("{} {}\n", MsgError(), GetError(errcode));
}

void Lexer::AddOperator(StringView name)
{
    m_Operators.EmplaceBack(name);
}

int Lexer::ParseOperator(const char* str) const
{
    if (!m_Operators.IsEmpty())
    {
        for (auto& op : m_Operators)
            if (!Core::StrcmpN(str, op.CStr(), op.Size()))
                return op.Size();
        return 0;
    }

    // Default operators
    if (str[0] == '{' || str[0] == '}' || str[0] == '[' || str[0] == ']' || str[0] == '(' || str[0] == ')' || str[0] == ',' || str[0] == '.' || str[0] == ';' || str[0] == '!' || str[0] == '\\' || str[0] == '#')
        return 1;

    if (str[0] == '+' || str[0] == '-' || str[0] == '*' || str[0] == '/' || str[0] == '|' || str[0] == '&' || str[0] == '^' || str[0] == '=' || str[0] == '>' || str[0] == '<')
        return (str[1] == '=') ? 2 : 1;

    return 0;
}

void Lexer::PrevToken()
{
    m_IsPrevToken = true;
}

Lexer::ErrorCode Lexer::TokenBegin(CrossLine crossLine)
{
    // skip space
    while (*m_Ptr <= 32 && *m_Ptr >= 0)
    {
        if (!*m_Ptr)
            return ErrorCode::EndOfFile;

        if (*m_Ptr++ == '\n')
        {
            m_LineNum++;
            if (crossLine == CrossLine::No)
                return ErrorCode::EndOfLine;
        }
    }

    if (m_Ptr[0] == '/' && m_Ptr[1] == '/')
    { // comment field
        if (crossLine == CrossLine::No)
            return ErrorCode::EndOfLine;

        while (*m_Ptr++ != '\n')
            if (!*m_Ptr)
                return ErrorCode::EndOfFile;
        m_LineNum++;
        return TokenBegin(crossLine);
    }

    // skip /* */ comments
    if (m_Ptr[0] == '/' && m_Ptr[1] == '*')
    {
        while (1)
        {
            m_Ptr++;
            if (m_Ptr[0] == '\n')
                m_LineNum++;
            if (m_Ptr[-1] == '*' && m_Ptr[0] == '/')
                break;
            if (m_Ptr[1] == 0)
                return ErrorCode::UnexpectedEOFInComment;
        }

        m_Ptr++;
        return TokenBegin(crossLine);
    }
    return ErrorCode::No;
}

Lexer::ErrorCode Lexer::NextToken(CrossLine crossLine)
{
    if (m_IsPrevToken)
    {
        m_IsPrevToken = false;
        m_ErrorCode  = ErrorCode::No;
        return m_ErrorCode;
    }

    m_ErrorCode = TokenBegin(crossLine);

    if (m_ErrorCode != ErrorCode::No)
        return m_ErrorCode;

    // copy token
    char* token_p = m_Token;

    if (*m_Ptr == '"')
    {
        m_Ptr++;
        while (1)
        {
            if (*m_Ptr == '"')
            {
                if (*(m_Ptr - 1) == '\\')
                {
                    token_p--;
                    *token_p++ = *m_Ptr++;
                    continue;
                }
                else
                    break;
            }

            if (!*m_Ptr)
            {
                m_ErrorCode = ErrorCode::EndOfFileInsideQuote;
                return m_ErrorCode;
            }

            if (*m_Ptr == '\n')
            {
                m_ErrorCode = ErrorCode::NewLineInsideQuote;
                return m_ErrorCode;
            }

            *token_p++ = *m_Ptr++;
            if (token_p == &m_Token[MAX_TOKEN_LENGTH])
            {
                m_ErrorCode = ErrorCode::StringIsTooLong;
                return m_ErrorCode;
            }
        }
        m_Ptr++;

        m_TokenType = TokenType::STRING;
    }
    else if (*m_Ptr == '\'')
    { // parse character
        *token_p++ = *m_Ptr++;
        if (m_Ptr[0] == '\\')
        {
            if (m_Ptr[1] == '\\')
                *token_p++ = '\\';
            else if (m_Ptr[1] == '\'')
                *token_p++ = '\'';
            else if (m_Ptr[1] == '\0')
                *token_p++ = '\0';
            else
                *token_p++ = '\0'; // FIXME: return error?
            m_Ptr += 2;
        }
        else
            *token_p++ = *m_Ptr++;
        if (*m_Ptr != '\'')
        {
            m_ErrorCode = ErrorCode::NewLineInConstant;
            return m_ErrorCode;
        }
        *token_p++ = *m_Ptr++;

        m_TokenType = TokenType::INTEGER;
    }
    else if (m_Ptr[0] == '0' && m_Ptr[1] == 'x')
    { // parse hex
        *token_p++ = *m_Ptr++;
        *token_p++ = *m_Ptr++;

        while (*m_Ptr && ((*m_Ptr >= '0' && *m_Ptr <= '9') || (*m_Ptr >= 'a' && *m_Ptr <= 'f') || (*m_Ptr >= 'A' && *m_Ptr <= 'F')))
        {
            *token_p++ = *m_Ptr;
            if (token_p == &m_Token[MAX_TOKEN_LENGTH])
            {
                m_ErrorCode = ErrorCode::TokenIsTooLong;
                return m_ErrorCode;
            }
            m_Ptr++;
        }

        m_TokenType = TokenType::INTEGER; // FIXME: integer always?
    }
    else if ((*m_Ptr >= '0' && *m_Ptr <= '9') // parse num
             || (*m_Ptr == '-' && m_Ptr[1] >= '0' && m_Ptr[1] <= '9'))
    {
        bool point = false;

        while (1)
        {
            *token_p++ = *m_Ptr;
            if (token_p == &m_Token[MAX_TOKEN_LENGTH])
            {
                m_ErrorCode = ErrorCode::TokenIsTooLong;
                return m_ErrorCode;
            }

            m_Ptr++;

            if (*m_Ptr == '.')
            {
                if (point)
                    break;
                point = true;
                continue;
            }

            if (*m_Ptr < '0' || *m_Ptr > '9')
                break;
        }

        m_TokenType = point ? TokenType::REAL : TokenType::INTEGER;
    }
    else
    {
        int length = ParseOperator(m_Ptr);
        if (length > 0)
        {
            if (length >= MAX_TOKEN_LENGTH)
            {
                m_ErrorCode = ErrorCode::TokenIsTooLong;
                return m_ErrorCode;
            }

            while (length-- > 0)
                *token_p++ = *m_Ptr++;
        }
        else
        {
            do {
                *token_p++ = *m_Ptr;
                if (token_p == &m_Token[MAX_TOKEN_LENGTH])
                {
                    m_ErrorCode = ErrorCode::TokenIsTooLong;
                    return m_ErrorCode;
                }

                m_Ptr++;

                if (ParseOperator(m_Ptr) > 0 || (m_Ptr[0] == '/' && m_Ptr[1] == '/') || (m_Ptr[0] == '/' && m_Ptr[1] == '*'))
                    break;
            } while (*m_Ptr > 32 || *m_Ptr < 0);
        }
        m_TokenType = TokenType::IDENTIFIER;
    }

    *token_p = 0;

    m_ErrorCode = ErrorCode::No;
    return m_ErrorCode;
}

Lexer::ErrorCode Lexer::Expect(const char* name, TokenType tokenType, bool matchCase)
{
    if (tokenType != m_TokenType && tokenType != TokenType::ANY)
    {
        switch (tokenType)
        {
            case TokenType::IDENTIFIER:
                m_ErrorCode = ErrorCode::ExpectedIdentifier;
                break;
            case TokenType::STRING:
                m_ErrorCode = ErrorCode::ExpectedString;
                break;
            case TokenType::INTEGER:
                m_ErrorCode = ErrorCode::ExpectedInteger;
                break;
            case TokenType::REAL:
                m_ErrorCode = ErrorCode::ExpectedReal;
                break;
            default:
                m_ErrorCode = ErrorCode::UnexpectedToken;
                break;
        }
        return m_ErrorCode;
    }

    bool compare = matchCase ? !Core::Strcmp(name, m_Token) : !Core::Stricmp(name, m_Token);

    m_ErrorCode = compare ? ErrorCode::No : ErrorCode::UnexpectedToken;

    return m_ErrorCode;
}

Lexer::ErrorCode Lexer::SkipBlock()
{
    int numBrackets = 1;
    while (numBrackets != 0)
    {
        ErrorCode err = NextToken();
        if (err != ErrorCode::No)
        {
            ErrorPrint(err);
            return err;
        }

        if (GetTokenType() == TokenType::IDENTIFIER)
        {
            if (Token()[0] == '{')
                numBrackets++;
            else if (Token()[0] == '}')
                numBrackets--;
        }
    }

    return ErrorCode::No;
}

void Lexer::SkipRestOfLine()
{
    while (*m_Ptr)
    {
        if (*m_Ptr++ == '\n')
        {
            m_LineNum++;
            break;
        }
    }
}

Lexer::ErrorCode Lexer::GetRestOfLine(char* buffer, int bufferSz, bool fixPos)
{
    const char* p;
    char*       d;

    p = m_Ptr;
    d = buffer;

    while (*p && d - buffer != bufferSz - 1)
    {
        if (*p == '\n' || *p == '\r')
        {
            if (!fixPos)
                m_LineNum++;
            p++;
            break;
        }
        *d++ = *p++;
    }

    *d = 0;

    if (!fixPos)
        m_Ptr = p;

    return (*m_Ptr ? ErrorCode::No : ErrorCode::EndOfFile);
}

const char* Lexer::GetError(ErrorCode errcode) const
{
    if ((int)errcode < 0 || errcode >= ErrorCode::MAX)
        return "unknown error";
    return ErrorStr[ToUnderlying(errcode)];
}

const char* Lexer::GetError() const
{
    if ((int)m_ErrorCode < 0 || m_ErrorCode >= ErrorCode::MAX)
        return "no error";
    return ErrorStr[ToUnderlying(m_ErrorCode)];
}

const char* Lexer::GetIdentifier(CrossLine crossLine)
{
    ErrorCode err = NextToken(crossLine);
    if (err == ErrorCode::EndOfFile)
        return "";
    if (err == ErrorCode::EndOfLine)
        return "";
    if (err != ErrorCode::No)
    {
        ErrorPrint(err);
        return "";
    }

    if (GetTokenType() != TokenType::IDENTIFIER)
    {
        LOG("{} expected identifier, found '{}'\n", MsgError(), Token());
        return "";
    }
    return Token();
}

const char* Lexer::GetInteger(CrossLine crossLine)
{
    ErrorCode err = NextToken(crossLine);
    if (err == ErrorCode::EndOfFile)
        return "";
    if (err == ErrorCode::EndOfLine)
        return "";
    if (err != ErrorCode::No)
    {
        ErrorPrint(err);
        return "";
    }

    if (GetTokenType() != TokenType::INTEGER)
    {
        LOG("{} expected integer, found '{}'\n", MsgError(), Token());
        return "";
    }
    return Token();
}

const char* Lexer::ExpectIdentifier(CrossLine crossLine)
{
    ErrorCode err = NextToken(crossLine);
    if (err == ErrorCode::EndOfFile)
        ErrorPrint(ErrorCode::UnexpectedEOF);
    else if (err == ErrorCode::EndOfLine)
        ErrorPrint(ErrorCode::UnexpectedNewLine);
    else if (err != ErrorCode::No)
        ErrorPrint(err);
    if (err != ErrorCode::No)
        return "";
    if (GetTokenType() != TokenType::IDENTIFIER)
    {
        LOG("{} expected token, found '{}'\n", MsgError(), Token());
        return "";
    }
    return Token();
}

const char* Lexer::GetString(CrossLine crossLine)
{
    ErrorCode err = NextToken(crossLine);
    if (err == ErrorCode::EndOfFile)
        return "";
    if (err == ErrorCode::EndOfLine)
        return "";
    if (err != ErrorCode::No)
    {
        ErrorPrint(err);
        return "";
    }

    if (GetTokenType() != TokenType::STRING)
    {
        LOG("{} expected string, found '{}'\n", MsgError(), Token());
        return "";
    }

    return Token();
}

const char* Lexer::ExpectString(CrossLine crossLine)
{
    ErrorCode err = NextToken(crossLine);
    if (err == ErrorCode::EndOfFile)
        ErrorPrint(ErrorCode::UnexpectedEOF);
    else if (err == ErrorCode::EndOfLine)
        ErrorPrint(ErrorCode::UnexpectedNewLine);
    else if (err != ErrorCode::No)
        ErrorPrint(err);
    if (err != ErrorCode::No)
        return "";
    if (GetTokenType() != TokenType::STRING)
    {
        LOG("{} expected string, found '{}'\n", MsgError(), Token());
        return "";
    }
    return Token();
}

int32_t Lexer::ExpectInteger(CrossLine crossLine)
{
    ErrorCode err = NextToken(crossLine);
    if (err == ErrorCode::EndOfFile)
        ErrorPrint(ErrorCode::UnexpectedEOF);
    else if (err == ErrorCode::EndOfLine)
        ErrorPrint(ErrorCode::UnexpectedNewLine);
    else if (err != ErrorCode::No)
        ErrorPrint(err);
    if (err != ErrorCode::No)
        return 0;

    if (GetTokenType() == TokenType::INTEGER)
        return Core::ParseInt32(Token());

    if (GetTokenType() == TokenType::REAL)
    {
        LOG("{} conversion from 'real' to 'integer'\n", MsgWarning());
        return (int32_t)Core::ParseFloat(Token());
    }

    LOG("{} expected integer, found '{}'\n", MsgError(), Token());
    return 0;
}

bool Lexer::ExpectBoolean(CrossLine crossLine)
{
    ErrorCode err = NextToken(crossLine);
    if (err == ErrorCode::EndOfFile)
        ErrorPrint(ErrorCode::UnexpectedEOF);
    else if (err == ErrorCode::EndOfLine)
        ErrorPrint(ErrorCode::UnexpectedNewLine);
    else if (err != ErrorCode::No)
        ErrorPrint(err);
    if (err != ErrorCode::No)
        return false;

    if (GetTokenType() == TokenType::INTEGER)
        return Core::ParseInt32(Token()) != 0;

    if (GetTokenType() == TokenType::IDENTIFIER)
    {
        if (!Core::Stricmp(Token(), "true"))
            return true;
        if (!Core::Stricmp(Token(), "false"))
            return false;
    }

    if (GetTokenType() == TokenType::REAL)
    {
        LOG("{} conversion from 'real' to 'boolean'\n", MsgWarning());
        return (int32_t)Core::ParseFloat(Token()) != 0;
    }

    LOG("{} expected boolean, found '{}'\n", MsgError(), Token());
    return false;
}

float Lexer::ExpectFloat(CrossLine crossLine)
{
    ErrorCode err = NextToken(crossLine);
    if (err == ErrorCode::EndOfFile)
        ErrorPrint(ErrorCode::UnexpectedEOF);
    else if (err == ErrorCode::EndOfLine)
        ErrorPrint(ErrorCode::UnexpectedNewLine);
    else if (err != ErrorCode::No)
        ErrorPrint(err);
    if (err != ErrorCode::No)
        return 0;
    if (GetTokenType() != TokenType::REAL && GetTokenType() != TokenType::INTEGER)
    {
        LOG("{} expected real, found '{}'\n", MsgError(), Token());
        return 0;
    }
    return Core::ParseFloat(Token());
}

double Lexer::ExpectDouble(CrossLine crossLine)
{
    ErrorCode err = NextToken(crossLine);
    if (err == ErrorCode::EndOfFile)
        ErrorPrint(ErrorCode::UnexpectedEOF);
    else if (err == ErrorCode::EndOfLine)
        ErrorPrint(ErrorCode::UnexpectedNewLine);
    else if (err != ErrorCode::No)
        ErrorPrint(err);
    if (err != ErrorCode::No)
        return 0;
    if (GetTokenType() != TokenType::REAL && GetTokenType() != TokenType::INTEGER)
    {
        LOG("{} expected real, found '{}'\n", MsgError(), Token());
        return 0;
    }
    return Core::ParseDouble(Token());
}

bool Lexer::ExpectQuaternion(Quat& q, CrossLine crossLine)
{
    return ExpectVector(q.ToPtr(), q.sNumComponents(), crossLine);
}

bool Lexer::ExpectVector(Float2& v, CrossLine crossLine)
{
    return ExpectVector(v.ToPtr(), v.sNumComponents(), crossLine);
}

bool Lexer::ExpectVector(Float3& v, CrossLine crossLine)
{
    return ExpectVector(v.ToPtr(), v.sNumComponents(), crossLine);
}

bool Lexer::ExpectVector(Float4& v, CrossLine crossLine)
{
    return ExpectVector(v.ToPtr(), v.sNumComponents(), crossLine);
}

bool Lexer::ExpectVector(float* v, int numComponents, CrossLine crossLine)
{
    for (int i = 0; i < numComponents; i++)
    {
        ErrorCode err = NextToken(crossLine);
        if (err == ErrorCode::EndOfFile)
            ErrorPrint(ErrorCode::UnexpectedEOF);
        else if (err == ErrorCode::EndOfLine)
            ErrorPrint(ErrorCode::UnexpectedNewLine);
        else if (err != ErrorCode::No)
            ErrorPrint(err);
        if (err != ErrorCode::No)
            return false;

        // first pass
        if (i == 0 && GetTokenType() == TokenType::IDENTIFIER)
        {
            if (Token()[0] == '(')
            {
                if (!ExpectVector(v, numComponents, crossLine))
                    return false;

                const char* t = ExpectIdentifier(crossLine);
                if (t[0] != ')')
                {
                    LOG("{} expected ')', found '{}'\n", MsgError(), t);
                    return false;
                }

                return true;
            }
        }

        if (GetTokenType() != TokenType::REAL && GetTokenType() != TokenType::INTEGER)
        {
            LOG("{} expected vector's real, found '{}'\n", MsgError(), Token());
            return false;
        }

        v[i] = Core::ParseFloat(Token());
    }
    return true;
}

bool Lexer::ExpectDVector(double* v, int numComponents, CrossLine crossLine)
{
    for (int i = 0; i < numComponents; i++)
    {
        ErrorCode err = NextToken(crossLine);
        if (err == ErrorCode::EndOfFile)
            ErrorPrint(ErrorCode::UnexpectedEOF);
        else if (err == ErrorCode::EndOfLine)
            ErrorPrint(ErrorCode::UnexpectedNewLine);
        else if (err != ErrorCode::No)
            ErrorPrint(err);
        if (err != ErrorCode::No)
            return false;

        // first pass
        if (i == 0 && GetTokenType() == TokenType::IDENTIFIER)
        {
            if (Token()[0] == '(')
            {
                if (!ExpectDVector(v, numComponents, crossLine))
                    return false;

                const char* t = ExpectIdentifier(crossLine);
                if (t[0] != ')')
                {
                    LOG("{} expected ')', found '{}'\n", MsgError(), t);
                    return false;
                }

                return true;
            }
        }

        if (GetTokenType() != TokenType::REAL && GetTokenType() != TokenType::INTEGER)
        {
            LOG("{} expected vector's real, found '{}'\n", MsgError(), Token());
            return false;
        }

        v[i] = Core::ParseDouble(Token());
    }
    return true;
}

bool Lexer::ExpectIVector(int* v, int numComponents, CrossLine crossLine)
{
    for (int i = 0; i < numComponents; i++)
    {
        ErrorCode err = NextToken(crossLine);
        if (err == ErrorCode::EndOfFile)
            ErrorPrint(ErrorCode::UnexpectedEOF);
        else if (err == ErrorCode::EndOfLine)
            ErrorPrint(ErrorCode::UnexpectedNewLine);
        else if (err != ErrorCode::No)
            ErrorPrint(err);
        if (err != ErrorCode::No)
            return false;

        // first pass
        if (i == 0 && GetTokenType() == TokenType::IDENTIFIER)
        {
            if (Token()[0] == '(')
            {
                if (!ExpectIVector(v, numComponents, crossLine))
                    return false;

                const char* t = ExpectIdentifier(crossLine);
                if (t[0] != ')')
                {
                    LOG("{} expected ')', found '{}'\n", MsgError(), t);
                    return false;
                }

                return true;
            }
        }

        if (GetTokenType() != TokenType::REAL && GetTokenType() != TokenType::INTEGER)
        {
            LOG("{} expected vector's integer, found '{}'\n", MsgError(), Token());
            return false;
        }

        v[i] = Core::ParseInt64(Token());
    }
    return true;
}

bool Lexer::ExpectAngles(Angl& angles, CrossLine crossLine)
{
    return ExpectVector(angles.ToFloat3(), crossLine);
}

bool Lexer::GoToNearest(const char* identifier)
{
    const char* str;
    ErrorCode   err;

    do {
        str = GetIdentifier();
        err = GetErrorCode();

        if (err == ErrorCode::EndOfFile)
        {
            // Unexpected EOF
            ErrorPrint(ErrorCode::UnexpectedEOF);
            return false;
        }

        if (err != ErrorCode::No)
        {
            // Something go wrong
            ErrorPrint(err);
            return false;
        }
    } while (Core::Stricmp(str, identifier));

    // Token found
    return true;
}

HK_NAMESPACE_END
