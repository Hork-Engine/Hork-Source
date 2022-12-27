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

HK_NAMESPACE_BEGIN

enum DOCUMENT_TOKEN_TYPE : uint8_t
{
    DOCUMENT_TOKEN_UNKNOWN,
    DOCUMENT_TOKEN_EOF,
    DOCUMENT_TOKEN_BRACKET,
    DOCUMENT_TOKEN_MEMBER,
    DOCUMENT_TOKEN_STRING
};

struct DocumentToken
{
    const char*         Begin = "";
    const char*         End   = "";
    DOCUMENT_TOKEN_TYPE Type  = DOCUMENT_TOKEN_UNKNOWN;

    const char* NamedType() const;
};

class DocumentTokenizer
{
    HK_FORBID_COPY(DocumentTokenizer)

public:
    DocumentTokenizer();
    ~DocumentTokenizer();

    void Reset(const char* pDocumentData, bool InSitu);

    const char* GetBuffer() const { return m_Start; }

    bool InSitu() const { return m_bInSitu; }

    void NextToken();

    DocumentToken const& GetToken() const { return m_CurToken; }

private:
    void SkipWhitespaces();

    char*       m_Start;
    const char* m_Cur;
    int         m_LineNumber;
    bool        m_bInSitu;
    DocumentToken      m_CurToken;
};

struct DocumentSerializeInfo
{
    bool bCompactStringConversion;
};

struct DocumentDeserializeInfo
{
    const char* pDocumentData;
    bool        bInsitu;
};

class DocumentMember;

class DocumentValue : public RefCounted
{
public:
    enum TYPE
    {
        TYPE_STRING,
        TYPE_OBJECT
    };

    DocumentValue(TYPE Type);
    ~DocumentValue();

    String SerializeToString(DocumentSerializeInfo const& SerializeInfo) const;

    /** Get next value inside array */
    DocumentValue* GetNext()
    {
        return m_Next;
    }

    /** Get next value inside array */
    DocumentValue const* GetNext() const
    {
        return m_Next;
    }

    /** Get prev value inside array */
    DocumentValue* GetPrev()
    {
        return m_Prev;
    }

    /** Get prev value inside array */
    DocumentValue const* GetPrev() const
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
    void SetString(StringView Str)
    {
        m_Value = Str;

        m_StrBegin = m_Value.Begin();
        m_StrEnd   = m_Value.End();
    }

    void SetStringInsitu(StringView Str)
    {
        m_StrBegin = Str.Begin();
        m_StrEnd   = Str.End();
    }

    /** Get string value */
    StringView GetStringView() const
    {
        return StringView(m_StrBegin, m_StrEnd);
    }

    DocumentMember* FindMember(StringView Name);

    DocumentMember const* FindMember(StringView Name) const;

    bool GetBool(StringView Name, bool Default = false) const;

    uint8_t GetUInt8(StringView Name, uint8_t Default = 0) const;

    uint16_t GetUInt16(StringView Name, uint16_t Default = 0) const;

    uint32_t GetUInt32(StringView Name, uint32_t Default = 0) const;

    uint64_t GetUInt64(StringView Name, uint64_t Default = 0) const;

    int8_t GetInt8(StringView Name, int8_t Default = 0) const;

    int16_t GetInt16(StringView Name, int16_t Default = 0) const;

    int32_t GetInt32(StringView Name, int32_t Default = 0) const;

    int64_t GetInt64(StringView Name, int64_t Default = 0) const;

    float GetFloat(StringView Name, float Default = 0) const;

    double GetDouble(StringView Name, double Default = 0) const;

    StringView GetString(StringView Name, StringView Default = "") const;

    void Clear();

    /** Add string */
    DocumentMember* AddString(GlobalStringView Name, StringView Str);

    /** Add object */
    DocumentMember* AddObject(GlobalStringView Name, DocumentValue* Object);

    /** Add array */
    DocumentMember* AddArray(GlobalStringView Name);

    /** Get members inside the object */
    DocumentMember* GetListOfMembers()
    {
        return m_MembersHead;
    }

    /** Get members inside the object */
    DocumentMember const* GetListOfMembers() const
    {
        return m_MembersHead;
    }

    void Print() const;

private:
    void AddMember(DocumentMember* Member);

    TYPE m_Type;

    const char* m_StrBegin; // string data for TYPE_STRING
    const char* m_StrEnd;   // string data for TYPE_STRING

    String m_Value;

    DocumentMember* m_MembersHead{};
    DocumentMember* m_MembersTail{};
    DocumentValue* m_Next{};
    DocumentValue* m_Prev{};

    friend class Document;
    friend class DocumentMember;
};

class DocumentMember : public RefCounted
{
public:
    DocumentMember();
    ~DocumentMember();

    /** Add value to array */
    void AddValue(DocumentValue* Value);

    /** Get member name */
    StringView GetName() const
    {
        return StringView(m_NameBegin, m_NameEnd);
    }

    /** Get member value as string if possible */
    StringView GetStringView() const
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
    DocumentMember* GetNext()
    {
        return m_Next;
    }

    /** Get next member inside object */
    DocumentMember const* GetNext() const
    {
        return m_Next;
    }

    /** Get prev member inside object */
    DocumentMember* GetPrev()
    {
        return m_Prev;
    }

    /** Get prev member inside object */
    DocumentMember const* GetPrev() const
    {
        return m_Prev;
    }

    /** Get list of values */
    DocumentValue* GetArrayValues()
    {
        return m_Values;
    }

    /** Get list of values */
    DocumentValue const* GetArrayValues() const
    {
        return m_Values;
    }

    void Print() const;

private:
    const char* m_NameBegin;
    const char* m_NameEnd;

    DocumentValue* m_Values{};
    DocumentValue* m_ValuesEnd{};

    DocumentMember* m_Next{};
    DocumentMember* m_Prev{};

    friend class Document;
    friend class DocumentValue;
};

class Document : public DocumentValue
{
public:
    Document();

    void DeserializeFromString(DocumentDeserializeInfo const& DeserializeInfo);

private:
    TRef<DocumentValue>  ParseObject();
    TRef<DocumentMember> ParseMember(DocumentToken const& MemberToken);
    void                 ParseArray(DocumentValue** ppArrayHead, DocumentValue** ppArrayTail);

    DocumentTokenizer m_Tokenizer;
};

HK_NAMESPACE_END
