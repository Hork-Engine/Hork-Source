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

#pragma once

#include "String.h"
#include "Ref.h"

enum DOCUMENT_TOKEN_TYPE
{
    DOCUMENT_TOKEN_UNKNOWN,
    DOCUMENT_TOKEN_EOF,
    DOCUMENT_TOKEN_BRACKET,
    DOCUMENT_TOKEN_MEMBER,
    DOCUMENT_TOKEN_STRING
};

struct SToken
{
    const char* Begin = "";
    const char* End   = "";
    int         Type  = DOCUMENT_TOKEN_UNKNOWN;

    const char* NamedType() const;
};

class ADocumentTokenizer
{
    HK_FORBID_COPY(ADocumentTokenizer)

public:
    ADocumentTokenizer();
    ~ADocumentTokenizer();

    void Reset(const char* pDocumentData, bool InSitu);

    const char* GetBuffer() const { return Start; }

    bool InSitu() const { return bInSitu; }

    void NextToken();

    SToken const& GetToken() const { return CurToken; }

private:
    void SkipWhitespaces();

    char*       Start;
    const char* Cur;
    int         LineNumber;
    bool        bInSitu;
    SToken      CurToken;
};

struct SDocumentSerializeInfo
{
    bool bCompactStringConversion;
};

struct SDocumentDeserializeInfo
{
    const char* pDocumentData;
    bool        bInsitu;
};

class ADocMember;

class ADocValue : public ARefCounted
{
public:
    enum
    {
        TYPE_STRING,
        TYPE_OBJECT
    };

    ADocValue(int Type);
    ~ADocValue();

    AString SerializeToString(SDocumentSerializeInfo const& SerializeInfo) const;

    /** Get next value inside array */
    ADocValue* GetNext()
    {
        return Next;
    }

    /** Get next value inside array */
    ADocValue const* GetNext() const
    {
        return Next;
    }

    /** Get prev value inside array */
    ADocValue* GetPrev()
    {
        return Prev;
    }

    /** Get prev value inside array */
    ADocValue const* GetPrev() const
    {
        return Prev;
    }

    /** Is string value */
    bool IsString() const
    {
        return Type == TYPE_STRING;
    }

    /** Is object value */
    bool IsObject() const
    {
        return Type == TYPE_OBJECT;
    }

    /** Set string value */
    void SetString(AStringView Str)
    {
        Value = Str;

        StrBegin = Value.Begin();
        StrEnd   = Value.End();
    }

    void SetStringInsitu(AStringView Str)
    {
        StrBegin = Str.Begin();
        StrEnd   = Str.End();
    }

    /** Get string value */
    AStringView GetStringView() const
    {
        return AStringView(StrBegin, StrEnd);
    }

    ADocMember* FindMember(AStringView Name);

    ADocMember const* FindMember(AStringView Name) const;

    bool GetBool(AStringView Name, bool Default = false) const;

    uint8_t GetUInt8(AStringView Name, uint8_t Default = 0) const;

    uint16_t GetUInt16(AStringView Name, uint16_t Default = 0) const;

    uint32_t GetUInt32(AStringView Name, uint32_t Default = 0) const;

    uint64_t GetUInt64(AStringView Name, uint64_t Default = 0) const;

    int8_t GetInt8(AStringView Name, int8_t Default = 0) const;

    int16_t GetInt16(AStringView Name, int16_t Default = 0) const;

    int32_t GetInt32(AStringView Name, int32_t Default = 0) const;

    int64_t GetInt64(AStringView Name, int64_t Default = 0) const;

    float GetFloat(AStringView Name, float Default = 0) const;

    double GetDouble(AStringView Name, double Default = 0) const;

    AStringView GetString(AStringView Name) const;

    void Clear();

    /** Add string */
    ADocMember* AddString(AGlobalStringView Name, AStringView Str);

    /** Add object */
    ADocMember* AddObject(AGlobalStringView Name, ADocValue* Object);

    /** Add array */
    ADocMember* AddArray(AGlobalStringView Name);

    /** Get members inside the object */
    ADocMember* GetListOfMembers()
    {
        return MembersHead;
    }

    /** Get members inside the object */
    ADocMember const* GetListOfMembers() const
    {
        return MembersHead;
    }

    void Print() const;

private:
    void AddMember(ADocMember* Member);

    int Type;

    const char* StrBegin; // string data for TYPE_STRING
    const char* StrEnd;   // string data for TYPE_STRING

    AString Value;

    ADocMember* MembersHead{};
    ADocMember* MembersTail{};
    ADocValue* Next{};
    ADocValue* Prev{};

    friend class ADocument;
    friend class ADocMember;
};

class ADocMember : public ARefCounted
{
public:
    ADocMember();
    ~ADocMember();

    /** Add value to array */
    void AddValue(ADocValue* _Value);

    /** Get member name */
    AStringView GetName() const
    {
        return AStringView(NameBegin, NameEnd);
    }

    /** Get member value as string if possible */
    AStringView GetStringView() const
    {
        if (IsString())
        {
            return AStringView(Values->StrBegin, Values->StrEnd);
        }

        return {};
    }

    /** Is string member */
    bool IsString() const
    {
        return !IsArray() && Values && Values->IsString();
    }

    /** Is object member */
    bool IsObject() const
    {
        return !IsArray() && Values && Values->IsObject();
    }

    bool IsArray() const
    {
        return (Values && Values->Next);
    }

    /** Get next member inside object */
    ADocMember* GetNext()
    {
        return Next;
    }

    /** Get next member inside object */
    ADocMember const* GetNext() const
    {
        return Next;
    }

    /** Get prev member inside object */
    ADocMember* GetPrev()
    {
        return Prev;
    }

    /** Get prev member inside object */
    ADocMember const* GetPrev() const
    {
        return Prev;
    }

    /** Get list of values */
    ADocValue* GetArrayValues()
    {
        return Values;
    }

    /** Get list of values */
    ADocValue const* GetArrayValues() const
    {
        return Values;
    }

    void Print() const;

private:
    const char* NameBegin;
    const char* NameEnd;

    ADocValue* Values{};
    ADocValue* ValuesEnd{};

    ADocMember* Next{};
    ADocMember* Prev{};

    friend class ADocument;
    friend class ADocValue;
};

class ADocument : public ADocValue
{
public:
    ADocument();

    void DeserializeFromString(SDocumentDeserializeInfo const& DeserializeInfo);

private:
    TRef<ADocValue>  ParseObject();
    TRef<ADocMember> ParseMember(SToken const& MemberToken);
    void             ParseArray(ADocValue** ppArrayHead, ADocValue** ppArrayTail);

    ADocumentTokenizer Tokenizer;
};
