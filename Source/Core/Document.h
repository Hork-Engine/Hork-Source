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

enum DOCUMENT_TOKEN_TYPE : uint8_t
{
    DOCUMENT_TOKEN_UNKNOWN,
    DOCUMENT_TOKEN_EOF,
    DOCUMENT_TOKEN_BRACKET,
    DOCUMENT_TOKEN_MEMBER,
    DOCUMENT_TOKEN_STRING
};

struct SToken
{
    const char*         Begin = "";
    const char*         End   = "";
    DOCUMENT_TOKEN_TYPE Type  = DOCUMENT_TOKEN_UNKNOWN;

    const char* NamedType() const;
};

class ADocumentTokenizer
{
    HK_FORBID_COPY(ADocumentTokenizer)

public:
    ADocumentTokenizer();
    ~ADocumentTokenizer();

    void Reset(const char* pDocumentData, bool InSitu);

    const char* GetBuffer() const { return m_Start; }

    bool InSitu() const { return m_bInSitu; }

    void NextToken();

    SToken const& GetToken() const { return m_CurToken; }

private:
    void SkipWhitespaces();

    char*       m_Start;
    const char* m_Cur;
    int         m_LineNumber;
    bool        m_bInSitu;
    SToken      m_CurToken;
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
    enum TYPE
    {
        TYPE_STRING,
        TYPE_OBJECT
    };

    ADocValue(TYPE Type);
    ~ADocValue();

    AString SerializeToString(SDocumentSerializeInfo const& SerializeInfo) const;

    /** Get next value inside array */
    ADocValue* GetNext()
    {
        return m_Next;
    }

    /** Get next value inside array */
    ADocValue const* GetNext() const
    {
        return m_Next;
    }

    /** Get prev value inside array */
    ADocValue* GetPrev()
    {
        return m_Prev;
    }

    /** Get prev value inside array */
    ADocValue const* GetPrev() const
    {
        return m_Prev;
    }

    /** Is string value */
    bool IsString() const
    {
        return m_Type == TYPE_STRING;
    }

    /** Is object value */
    bool IsObject() const
    {
        return m_Type == TYPE_OBJECT;
    }

    /** Set string value */
    void SetString(AStringView Str)
    {
        m_Value = Str;

        m_StrBegin = m_Value.Begin();
        m_StrEnd   = m_Value.End();
    }

    void SetStringInsitu(AStringView Str)
    {
        m_StrBegin = Str.Begin();
        m_StrEnd   = Str.End();
    }

    /** Get string value */
    AStringView GetStringView() const
    {
        return AStringView(m_StrBegin, m_StrEnd);
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
        return m_MembersHead;
    }

    /** Get members inside the object */
    ADocMember const* GetListOfMembers() const
    {
        return m_MembersHead;
    }

    void Print() const;

private:
    void AddMember(ADocMember* Member);

    TYPE m_Type;

    const char* m_StrBegin; // string data for TYPE_STRING
    const char* m_StrEnd;   // string data for TYPE_STRING

    AString m_Value;

    ADocMember* m_MembersHead{};
    ADocMember* m_MembersTail{};
    ADocValue* m_Next{};
    ADocValue* m_Prev{};

    friend class ADocument;
    friend class ADocMember;
};

class ADocMember : public ARefCounted
{
public:
    ADocMember();
    ~ADocMember();

    /** Add value to array */
    void AddValue(ADocValue* Value);

    /** Get member name */
    AStringView GetName() const
    {
        return AStringView(m_NameBegin, m_NameEnd);
    }

    /** Get member value as string if possible */
    AStringView GetStringView() const
    {
        if (IsString())
        {
            return m_Values->GetStringView();
        }

        return {};
    }

    /** Is string member */
    bool IsString() const
    {
        return !IsArray() && m_Values && m_Values->IsString();
    }

    /** Is object member */
    bool IsObject() const
    {
        return !IsArray() && m_Values && m_Values->IsObject();
    }

    bool IsArray() const
    {
        return (m_Values && m_Values->m_Next);
    }

    /** Get next member inside object */
    ADocMember* GetNext()
    {
        return m_Next;
    }

    /** Get next member inside object */
    ADocMember const* GetNext() const
    {
        return m_Next;
    }

    /** Get prev member inside object */
    ADocMember* GetPrev()
    {
        return m_Prev;
    }

    /** Get prev member inside object */
    ADocMember const* GetPrev() const
    {
        return m_Prev;
    }

    /** Get list of values */
    ADocValue* GetArrayValues()
    {
        return m_Values;
    }

    /** Get list of values */
    ADocValue const* GetArrayValues() const
    {
        return m_Values;
    }

    void Print() const;

private:
    const char* m_NameBegin;
    const char* m_NameEnd;

    ADocValue* m_Values{};
    ADocValue* m_ValuesEnd{};

    ADocMember* m_Next{};
    ADocMember* m_Prev{};

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

    ADocumentTokenizer m_Tokenizer;
};
