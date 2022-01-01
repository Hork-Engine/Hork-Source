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

#include <Geometry/Public/Lexer.h>
#include <Platform/Public/Logger.h>

ALexer::ALexer() :
    BufferName("<memory buffer>"), Ptr(AString::NullCString()), CurrentLine(0), bPrevToken(false), PrintFlags(PRINT_ALL)
{
}

ALexer::~ALexer()
{
}

void ALexer::Initialize(const char* InBuffer, const char* InBufferName, int InPrint)
{
    CurrentLine = 0;
    Ptr         = InBuffer ? InBuffer : AString::NullCString();
    bPrevToken  = false;
    BufferName  = InBufferName ? InBufferName : "<memory buffer>";
    PrintFlags  = InPrint;
}

void ALexer::MakeString(AString& Str, int InMessage, const char* InText)
{

    Str.Clear();

    if (PrintFlags & PRINT_BUFFER_NAME)
    {
        Str += BufferName;
        Str += " ";
    }

    if (PrintFlags & PRINT_BUFFER_LINE)
    {
        Str += TSprintfBuffer<32>().Sprintf("ln=%d ", CurrentLine);
    }

    if (PrintFlags)
    {
        Str += ": ";
    }

    switch (InMessage)
    {
        case MSG_ERROR:
            Str += "error: ";
            break;
        case MSG_WARNING:
            Str += "warning: ";
            break;
    }

    Str += InText;
}

void ALexer::ErrorPrint(int _Err)
{
    AString str;
    MakeString(str, MSG_ERROR, GetError(_Err));
    str += "\n";
    GLogger.Print(str.CStr());
}

void ALexer::ErrorPrintf(const char* _Format, ...)
{
    char    text[4096];
    va_list VaList;
    va_start(VaList, _Format);
    Core::VSprintf(text, sizeof(text), _Format, VaList);
    va_end(VaList);

    AString str;
    MakeString(str, MSG_ERROR, text);
    GLogger.Print(str.CStr());
}

void ALexer::WarnPrintf(const char* _Format, ...)
{
    char    text[4096];
    va_list VaList;
    va_start(VaList, _Format);
    Core::VSprintf(text, sizeof(text), _Format, VaList);
    va_end(VaList);

    AString str;
    MakeString(str, MSG_WARNING, text);
    GLogger.Print(str.CStr());
}

void ALexer::AddOperator(const char* _String)
{
    SOperator Op;

    Core::Strcpy(Op.Str, sizeof(Op.Str), _String);
    Op.Len = static_cast<int>(strlen(Op.Str));

    Operators.Append(Op);
}

int ALexer::CheckOperator(const char* _Ptr) const
{
    if (!Operators.IsEmpty())
    {
        for (SOperator const& op : Operators)
        {
            if (!Core::StrcmpN(_Ptr, op.Str, op.Len))
            {
                return op.Len;
            }
        }
        return 0;
    }

    // Default operators
    if (_Ptr[0] == '{' || _Ptr[0] == '}' || _Ptr[0] == '[' || _Ptr[0] == ']' || _Ptr[0] == '(' || _Ptr[0] == ')' || _Ptr[0] == ',' || _Ptr[0] == '.' || _Ptr[0] == ';' || _Ptr[0] == '!' || _Ptr[0] == '\\' || _Ptr[0] == '#')
    {
        return 1;
    }

    if (_Ptr[0] == '+' || _Ptr[0] == '-' || _Ptr[0] == '*' || _Ptr[0] == '/' || _Ptr[0] == '|' || _Ptr[0] == '&' || _Ptr[0] == '^' || _Ptr[0] == '=' || _Ptr[0] == '>' || _Ptr[0] == '<')
    {
        return (_Ptr[1] == '=') ? 2 : 1;
    }

    return 0;
}

void ALexer::PrevToken()
{
    bPrevToken = true;
}

int ALexer::TokenBegin(bool _CrossLine)
{
    if (CurrentLine == 0)
    {
        // init first line
        CurrentLine = 1;
    }

    // skip space
    while (*Ptr <= 32 && *Ptr >= 0)
    {
        if (!*Ptr)
        {
            return ERROR_EOF;
        }

        if (*Ptr++ == '\n')
        {
            CurrentLine++;
            if (!_CrossLine)
            {
                return ERROR_EOL;
            }
        }
    }

    if (Ptr[0] == '/' && Ptr[1] == '/')
    { // comment field
        if (!_CrossLine)
        {
            return ERROR_EOL;
        }

        while (*Ptr++ != '\n')
        {
            if (!*Ptr)
            {
                return ERROR_EOF;
            }
        }
        CurrentLine++;
        return TokenBegin(_CrossLine);
    }

    // skip /* */ comments
    if (Ptr[0] == '/' && Ptr[1] == '*')
    {
        while (1)
        {
            Ptr++;
            if (Ptr[0] == '\n')
            {
                CurrentLine++;
            }

            if (Ptr[-1] == '*' && Ptr[0] == '/')
            {
                break;
            }

            if (Ptr[1] == 0)
            {
                return ERROR_UNEXPECTED_EOF_IN_COMMENT;
            }
        }

        Ptr++;
        return TokenBegin(_CrossLine);
    }
    return ERROR_NO;
}

int ALexer::NextToken(bool _CrossLine)
{
    if (bPrevToken)
    {
        bPrevToken = false;
        ErrorCode  = ERROR_NO;
        return ErrorCode;
    }

    ErrorCode = TokenBegin(_CrossLine);

    if (ErrorCode != ERROR_NO)
    {
        return ErrorCode;
    }

    // copy token
    char* token_p = CurToken;

    if (*Ptr == '"')
    {
        Ptr++;
        while (1)
        {
            if (*Ptr == '"')
            {
                if (*(Ptr - 1) == '\\')
                {
                    token_p--;
                    *token_p++ = *Ptr++;
                    continue;
                }
                else
                {
                    break;
                }
            }

            if (!*Ptr)
            {
                ErrorCode = ERROR_EOF_INSIDE_QUOTE;
                return ErrorCode;
            }

            if (*Ptr == '\n')
            {
                ErrorCode = ERROR_NEWLINE_INSIDE_QUOTE;
                return ErrorCode;
            }

            *token_p++ = *Ptr++;
            if (token_p == &CurToken[MAX_TOKEN_LENGTH])
            {
                ErrorCode = ERROR_TOO_MANY_CHARS_IN_STRING;
                return ErrorCode;
            }
        }
        Ptr++;

        TokenType = TOKEN_TYPE_STRING;
    }
    else if (*Ptr == '\'')
    { // parse character
        *token_p++ = *Ptr++;
        if (Ptr[0] == '\\')
        {
            if (Ptr[1] == '\\')
            {
                *token_p++ = '\\';
            }
            else if (Ptr[1] == '\'')
            {
                *token_p++ = '\'';
            }
            else if (Ptr[1] == '\0')
            {
                *token_p++ = '\0';
            }
            else
            {
                *token_p++ = '\0'; // FIXME: return error?
            }

            Ptr += 2;
        }
        else
        {
            *token_p++ = *Ptr++;
        }
        if (*Ptr != '\'')
        {
            ErrorCode = ERROR_NEWLINE_IN_CONSTANT;
            return ErrorCode;
        }
        *token_p++ = *Ptr++;

        TokenType = TOKEN_TYPE_INTEGER;
    }
    else if (Ptr[0] == '0' && Ptr[1] == 'x')
    { // parse hex
        *token_p++ = *Ptr++;
        *token_p++ = *Ptr++;

        while (*Ptr && ((*Ptr >= '0' && *Ptr <= '9') || (*Ptr >= 'a' && *Ptr <= 'f') || (*Ptr >= 'A' && *Ptr <= 'F')))
        {
            *token_p++ = *Ptr;
            if (token_p == &CurToken[MAX_TOKEN_LENGTH])
            {
                ErrorCode = ERROR_TOKEN_IS_TOO_LONG;
                return ErrorCode;
            }
            Ptr++;
        }

        TokenType = TOKEN_TYPE_INTEGER; // FIXME: integer always?
    }
    else if ((*Ptr >= '0' && *Ptr <= '9') // parse num
             || (*Ptr == '-' && Ptr[1] >= '0' && Ptr[1] <= '9'))
    {
        bool point = false;

        while (1)
        {
            *token_p++ = *Ptr;
            if (token_p == &CurToken[MAX_TOKEN_LENGTH])
            {
                ErrorCode = ERROR_TOKEN_IS_TOO_LONG;
                return ErrorCode;
            }

            Ptr++;

            if (*Ptr == '.')
            {
                if (point)
                {
                    break;
                }
                point = true;
                continue;
            }

            if (*Ptr < '0' || *Ptr > '9')
            {
                break;
            }
        }

        if (point)
        {
            TokenType = TOKEN_TYPE_REAL; // float point
        }
        else
        {
            TokenType = TOKEN_TYPE_INTEGER; // fixed point
        }
    }
    else
    {
        int length = CheckOperator(Ptr);
        if (length > 0)
        {
            if (length >= MAX_TOKEN_LENGTH)
            {
                ErrorCode = ERROR_TOKEN_IS_TOO_LONG;
                return ErrorCode;
            }

            while (length-- > 0)
            {
                *token_p++ = *Ptr++;
            }
        }
        else
            do {
                *token_p++ = *Ptr;
                if (token_p == &CurToken[MAX_TOKEN_LENGTH])
                {
                    ErrorCode = ERROR_TOKEN_IS_TOO_LONG;
                    return ErrorCode;
                }

                Ptr++;

                if (CheckOperator(Ptr) > 0 || (Ptr[0] == '/' && Ptr[1] == '/') || (Ptr[0] == '/' && Ptr[1] == '*'))
                {
                    break;
                }
            } while (*Ptr > 32 || *Ptr < 0);

        TokenType = TOKEN_TYPE_IDENTIFIER;
    }

    *token_p = 0;

    ErrorCode = ERROR_NO;
    return ErrorCode;
}

int ALexer::Expect(const char* _Str, int _TokenType, bool _MatchCase)
{
    if (_TokenType != TokenType && _TokenType != TOKEN_TYPE_ANY)
    {
        switch (_TokenType)
        {
            case TOKEN_TYPE_IDENTIFIER:
                ErrorCode = ERROR_EXPECTED_IDENTIFIER;
                break;
            case TOKEN_TYPE_STRING:
                ErrorCode = ERROR_EXPECTED_STRING;
                break;
            case TOKEN_TYPE_INTEGER:
                ErrorCode = ERROR_EXPECTED_INTEGER;
                break;
            case TOKEN_TYPE_REAL:
                ErrorCode = ERROR_EXPECTED_REAL;
                break;
            default:
                ErrorCode = ERROR_UNEXPECTED_TOKEN_FOUND;
                break;
        }
        return ErrorCode;
    }

    bool compare = _MatchCase ? !Core::Strcmp(_Str, CurToken) : !Core::Stricmp(_Str, CurToken);

    ErrorCode = compare ? ERROR_NO : ERROR_UNEXPECTED_TOKEN_FOUND;

    return ErrorCode;
}

int ALexer::SkipBlock()
{
    int numBrackets = 1;
    while (numBrackets != 0)
    {
        int err = NextToken();
        if (err != ERROR_NO)
        {
            ErrorPrint(err);
            return err;
        }

        if (GetTokenType() == TOKEN_TYPE_IDENTIFIER)
        {
            if (Token()[0] == '{')
            {
                numBrackets++;
            }
            else if (Token()[0] == '}')
            {
                numBrackets--;
            }
        }
    }

    return ERROR_NO;
}

void ALexer::SkipRestOfLine()
{
    while (*Ptr)
    {
        if (*Ptr++ == '\n')
        {
            CurrentLine++;
            break;
        }
    }
}

int ALexer::GetRestOfLine(char* _Dest, int _DestSz, bool _FixPos)
{
    const char* p;
    char*       d;

    p = Ptr;
    d = _Dest;

    while (*p && d - _Dest != _DestSz - 1)
    {
        if (*p == '\n' || *p == '\r')
        {
            if (!_FixPos)
            {
                CurrentLine++;
            }
            p++;
            break;
        }
        *d++ = *p++;
    }

    *d = 0;

    if (!_FixPos)
    {
        Ptr = p;
    }

    return (*Ptr ? ERROR_NO : ERROR_EOF);
}

static const char* __ErrorStr[ALexer::ERROR_MAX] = {
    "no error",
    "unexpected EOF found",
    "unexpected end of file found in comment",
    "unexpected newline found",
    "unexpected token found",
    "EOF inside quote",
    "newline inside quote",
    "newline in constant",
    "token is too long",
    "too many chars in string",
    "expected identifier",
    "expected string",
    "expected integer",
    "expected real"};

const char* ALexer::GetError(int _Error) const
{
    if (_Error < 0 || _Error >= ERROR_MAX)
    {
        return "unknown error";
    }
    return __ErrorStr[_Error];
}

const char* ALexer::GetError() const
{
    if (ErrorCode >= ERROR_MAX)
    {
        return "no error";
    }
    return __ErrorStr[ErrorCode];
}

const char* ALexer::GetIdentifier(bool _CrossLine)
{
    int err = NextToken(_CrossLine);
    if (err == ERROR_EOF)
    {
        return AString::NullCString();
    }
    if (err == ERROR_EOL)
    {
        return AString::NullCString();
    }
    if (err != ERROR_NO)
    {
        ErrorPrint(err);
        return AString::NullCString();
    }

    if (GetTokenType() != TOKEN_TYPE_IDENTIFIER)
    {
        ErrorPrintf("expected identifier, found '%s'\n", Token());
        return AString::NullCString();
    }
    return Token();
}

const char* ALexer::GetInteger(bool _CrossLine)
{
    int err = NextToken(_CrossLine);
    if (err == ERROR_EOF)
    {
        return AString::NullCString();
    }
    if (err == ERROR_EOL)
    {
        return AString::NullCString();
    }
    if (err != ERROR_NO)
    {
        ErrorPrint(err);
        return AString::NullCString();
    }

    if (GetTokenType() != TOKEN_TYPE_INTEGER)
    {
        ErrorPrintf("expected integer, found '%s'\n", Token());
        return AString::NullCString();
    }
    return Token();
}

const char* ALexer::ExpectIdentifier(bool _CrossLine)
{
    int err = NextToken(_CrossLine);
    if (err == ERROR_EOF)
    {
        ErrorPrint(ERROR_UNEXPECTED_EOF_FOUND);
    }
    else if (err == ERROR_EOL)
    {
        ErrorPrint(ERROR_UNEXPECTED_NEWLINE_FOUND);
    }
    else if (err != ERROR_NO)
    {
        ErrorPrint(err);
    }
    if (err != ERROR_NO)
    {
        return AString::NullCString();
    }
    if (GetTokenType() != TOKEN_TYPE_IDENTIFIER)
    {
        ErrorPrintf("expected token, found '%s'\n", Token());
        return AString::NullCString();
    }
    return Token();
}

const char* ALexer::GetString(bool _CrossLine)
{
    int err = NextToken(_CrossLine);
    if (err == ERROR_EOF)
    {
        return AString::NullCString();
    }
    if (err == ERROR_EOL)
    {
        return AString::NullCString();
    }
    if (err != ERROR_NO)
    {
        ErrorPrint(err);
        return AString::NullCString();
    }

    if (GetTokenType() != TOKEN_TYPE_STRING)
    {
        ErrorPrintf("expected string, found '%s'\n", Token());
        return AString::NullCString();
    }

    return Token();
}

const char* ALexer::ExpectString(bool _CrossLine)
{
    int err = NextToken(_CrossLine);
    if (err == ERROR_EOF)
    {
        ErrorPrint(ERROR_UNEXPECTED_EOF_FOUND);
    }
    else if (err == ERROR_EOL)
    {
        ErrorPrint(ERROR_UNEXPECTED_NEWLINE_FOUND);
    }
    else if (err != ERROR_NO)
    {
        ErrorPrint(err);
    }
    if (err != ERROR_NO)
    {
        return AString::NullCString();
    }
    if (GetTokenType() != TOKEN_TYPE_STRING)
    {
        ErrorPrintf("expected string, found '%s'\n", Token());
        return AString::NullCString();
    }
    return Token();
}

int32_t ALexer::ExpectInteger(bool _CrossLine)
{
    int err = NextToken(_CrossLine);
    if (err == ERROR_EOF)
    {
        ErrorPrint(ERROR_UNEXPECTED_EOF_FOUND);
    }
    else if (err == ERROR_EOL)
    {
        ErrorPrint(ERROR_UNEXPECTED_NEWLINE_FOUND);
    }
    else if (err != ERROR_NO)
    {
        ErrorPrint(err);
    }
    if (err != ERROR_NO)
    {
        return 0;
    }
    if (GetTokenType() == TOKEN_TYPE_INTEGER)
    {
        return Math::ToInt<int32_t>(Token());
    }
    if (GetTokenType() == TOKEN_TYPE_REAL)
    {
        WarnPrintf("conversion from 'real' to 'integer'\n");
        return (int32_t)Math::ToFloat(Token());
    }

    ErrorPrintf("expected integer, found '%s'\n", Token());
    return 0;
}

bool ALexer::ExpectBoolean(bool _CrossLine)
{
    int err = NextToken(_CrossLine);
    if (err == ERROR_EOF)
    {
        ErrorPrint(ERROR_UNEXPECTED_EOF_FOUND);
    }
    else if (err == ERROR_EOL)
    {
        ErrorPrint(ERROR_UNEXPECTED_NEWLINE_FOUND);
    }
    else if (err != ERROR_NO)
    {
        ErrorPrint(err);
    }
    if (err != ERROR_NO)
    {
        return false;
    }
    if (GetTokenType() == TOKEN_TYPE_INTEGER)
    {
        return Math::ToInt<int32_t>(Token()) != 0;
    }
    if (GetTokenType() == TOKEN_TYPE_IDENTIFIER)
    {
        if (!Core::Stricmp(Token(), "true"))
        {
            return true;
        }
        if (!Core::Stricmp(Token(), "false"))
        {
            return false;
        }
    }

    if (GetTokenType() == TOKEN_TYPE_REAL)
    {
        WarnPrintf("conversion from 'real' to 'boolean'\n");
        return (int32_t)Math::ToFloat(Token()) != 0;
    }

    ErrorPrintf("expected boolean, found '%s'\n", Token());
    return false;
}

float ALexer::ExpectFloat(bool _CrossLine)
{
    int err = NextToken(_CrossLine);
    if (err == ERROR_EOF)
    {
        ErrorPrint(ERROR_UNEXPECTED_EOF_FOUND);
    }
    else if (err == ERROR_EOL)
    {
        ErrorPrint(ERROR_UNEXPECTED_NEWLINE_FOUND);
    }
    else if (err != ERROR_NO)
    {
        ErrorPrint(err);
    }
    if (err != ERROR_NO)
    {
        return 0;
    }
    if (GetTokenType() != TOKEN_TYPE_REAL && GetTokenType() != TOKEN_TYPE_INTEGER)
    {
        ErrorPrintf("expected real, found '%s'\n", Token());
        return 0;
    }
    return Math::ToFloat(Token());
}

double ALexer::ExpectDouble(bool _CrossLine)
{
    int err = NextToken(_CrossLine);
    if (err == ERROR_EOF)
    {
        ErrorPrint(ERROR_UNEXPECTED_EOF_FOUND);
    }
    else if (err == ERROR_EOL)
    {
        ErrorPrint(ERROR_UNEXPECTED_NEWLINE_FOUND);
    }
    else if (err != ERROR_NO)
    {
        ErrorPrint(err);
    }
    if (err != ERROR_NO)
    {
        return 0;
    }
    if (GetTokenType() != TOKEN_TYPE_REAL && GetTokenType() != TOKEN_TYPE_INTEGER)
    {
        ErrorPrintf("expected real, found '%s'\n", Token());
        return 0;
    }
    return Math::ToDouble(Token());
}

bool ALexer::ExpectQuaternion(Quat& _DestQuat, bool _CrossLine)
{
    return ExpectVector(_DestQuat.ToPtr(), _DestQuat.NumComponents(), _CrossLine);
}

bool ALexer::ExpectVector(Float2& _DestVector, bool _CrossLine)
{
    return ExpectVector(_DestVector.ToPtr(), _DestVector.NumComponents(), _CrossLine);
}

bool ALexer::ExpectVector(Float3& _DestVector, bool _CrossLine)
{
    return ExpectVector(_DestVector.ToPtr(), _DestVector.NumComponents(), _CrossLine);
}

bool ALexer::ExpectVector(Float4& _DestVector, bool _CrossLine)
{
    return ExpectVector(_DestVector.ToPtr(), _DestVector.NumComponents(), _CrossLine);
}

bool ALexer::ExpectVector(float* _DestVector, int _NumComponents, bool _CrossLine)
{
    for (int i = 0; i < _NumComponents; i++)
    {
        int err = NextToken(_CrossLine);
        if (err == ERROR_EOF)
        {
            ErrorPrint(ERROR_UNEXPECTED_EOF_FOUND);
        }
        else if (err == ERROR_EOL)
        {
            ErrorPrint(ERROR_UNEXPECTED_NEWLINE_FOUND);
        }
        else if (err != ERROR_NO)
        {
            ErrorPrint(err);
        }
        if (err != ERROR_NO)
        {
            return false;
        }

        // first pass
        if (i == 0 && GetTokenType() == TOKEN_TYPE_IDENTIFIER)
        {
            if (Token()[0] == '(')
            {
                if (!ExpectVector(_DestVector, _NumComponents, _CrossLine))
                    return false;

                const char* t = ExpectIdentifier(_CrossLine);
                if (t[0] != ')')
                {
                    ErrorPrintf("expected ')', found '%s'\n", t);
                    return false;
                }

                return true;
            }
        }

        if (GetTokenType() != TOKEN_TYPE_REAL && GetTokenType() != TOKEN_TYPE_INTEGER)
        {
            ErrorPrintf("expected vector's real, found '%s'\n", Token());
            return false;
        }

        _DestVector[i] = Math::ToFloat(Token());
    }
    return true;
}

bool ALexer::ExpectDVector(double* _DestVector, int _NumComponents, bool _CrossLine)
{
    for (int i = 0; i < _NumComponents; i++)
    {
        int err = NextToken(_CrossLine);
        if (err == ERROR_EOF)
        {
            ErrorPrint(ERROR_UNEXPECTED_EOF_FOUND);
        }
        else if (err == ERROR_EOL)
        {
            ErrorPrint(ERROR_UNEXPECTED_NEWLINE_FOUND);
        }
        else if (err != ERROR_NO)
        {
            ErrorPrint(err);
        }
        if (err != ERROR_NO)
        {
            return false;
        }

        // first pass
        if (i == 0 && GetTokenType() == TOKEN_TYPE_IDENTIFIER)
        {
            if (Token()[0] == '(')
            {
                if (!ExpectDVector(_DestVector, _NumComponents, _CrossLine))
                    return false;

                const char* t = ExpectIdentifier(_CrossLine);
                if (t[0] != ')')
                {
                    ErrorPrintf("expected ')', found '%s'\n", t);
                    return false;
                }

                return true;
            }
        }

        if (GetTokenType() != TOKEN_TYPE_REAL && GetTokenType() != TOKEN_TYPE_INTEGER)
        {
            ErrorPrintf("expected vector's real, found '%s'\n", Token());
            return false;
        }

        _DestVector[i] = Math::ToDouble(Token());
    }
    return true;
}

bool ALexer::ExpectIVector(int* _DestVector, int _NumComponents, bool _CrossLine)
{
    for (int i = 0; i < _NumComponents; i++)
    {
        int err = NextToken(_CrossLine);
        if (err == ERROR_EOF)
        {
            ErrorPrint(ERROR_UNEXPECTED_EOF_FOUND);
        }
        else if (err == ERROR_EOL)
        {
            ErrorPrint(ERROR_UNEXPECTED_NEWLINE_FOUND);
        }
        else if (err != ERROR_NO)
        {
            ErrorPrint(err);
        }
        if (err != ERROR_NO)
        {
            return false;
        }

        // first pass
        if (i == 0 && GetTokenType() == TOKEN_TYPE_IDENTIFIER)
        {
            if (Token()[0] == '(')
            {
                if (!ExpectIVector(_DestVector, _NumComponents, _CrossLine))
                    return false;

                const char* t = ExpectIdentifier(_CrossLine);
                if (t[0] != ')')
                {
                    ErrorPrintf("expected ')', found '%s'\n", t);
                    return false;
                }

                return true;
            }
        }

        if (GetTokenType() != TOKEN_TYPE_REAL && GetTokenType() != TOKEN_TYPE_INTEGER)
        {
            ErrorPrintf("expected vector's integer, found '%s'\n", Token());
            return false;
        }

        _DestVector[i] = Math::ToInt<int64_t>(Token());
    }
    return true;
}

bool ALexer::ExpectAngles(Angl& _DestAngles, bool _CrossLine)
{
    return ExpectVector(_DestAngles.ToFloat3(), _CrossLine);
}

bool ALexer::GoToNearest(const char* _Identifier)
{
    const char* str;
    int         err;

    do {
        str = GetIdentifier();
        err = GetErrorCode();

        if (err == ERROR_EOF)
        {
            // Unexpected EOF
            ErrorPrint(ERROR_UNEXPECTED_EOF_FOUND);
            return false;
        }

        if (err != ERROR_NO)
        {
            // Something go wrong
            ErrorPrint(err);
            return false;
        }
    } while (Core::Stricmp(str, _Identifier));

    // Token found
    return true;
}
