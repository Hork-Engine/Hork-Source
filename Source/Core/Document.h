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

#include <Platform/Memory/PoolAllocator.h>
#include "String.h"
#include "Ref.h"

enum ETokenType
{
    TOKEN_TYPE_UNKNOWN,
    TOKEN_TYPE_EOF,
    TOKEN_TYPE_BRACKET,
    TOKEN_TYPE_MEMBER,
    TOKEN_TYPE_STRING
};

struct SToken
{
    const char* Begin = "";
    const char* End   = "";
    int         Type  = TOKEN_TYPE_UNKNOWN;

    const char* NamedType() const;
};

class ADocumentTokenizer
{
    AN_FORBID_COPY(ADocumentTokenizer)

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

//template< typename T >
//struct SDocumentAllocator
//{
//    static TPoolAllocator< T > & GetMemoryPool()
//    {
//        static TPoolAllocator< T > MemoryPool;
//        return MemoryPool;
//    }
//
//    static void FreeMemoryPool()
//    {
//        GetMemoryPool().Free();
//    }
//
//    void * Allocate( std::size_t _SizeInBytes )
//    {
//        AN_ASSERT( _SizeInBytes == sizeof( T ) );
//        return GetMemoryPool().Allocate();
//    }
//
//    void Deallocate( void * _Bytes )
//    {
//        GetMemoryPool().Deallocate( _Bytes );
//    }
//};

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

class ADocValue : public ARefCounted //TRefCounted< SDocumentAllocator< ADocValue > >
{
public:
    enum
    {
        TYPE_STRING,
        TYPE_OBJECT
    };

    ADocValue(int InType)
    {
        Members    = nullptr;
        MembersEnd = nullptr;
        Next       = nullptr;
        Prev       = nullptr;
        Type       = InType;
        StrBegin = StrEnd = "";
        pTokenMemory      = nullptr;
    }

    ~ADocValue();

    AString SerializeToString(SDocumentSerializeInfo const& SerializeInfo) const;

    /** Find child member */
    ADocMember*       FindMember(const char* Name);
    ADocMember const* FindMember(const char* Name) const;

    /** Add string member */
    ADocMember* AddString(const char* MemberName, const char* Str);

    ADocMember* AddString(const char* MemberName, AString const& Str)
    {
        return AddString(MemberName, Str.CStr());
    }

    /** Add object member */
    ADocMember* AddObject(const char* MemberName, class ADocObject* Object);

    /** Add array member */
    ADocMember* AddArray(const char* ArrayName);

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

    /** Get members inside the object */
    ADocMember* GetListOfMembers()
    {
        return Members;
    }

    /** Get members inside the object */
    ADocMember const* GetListOfMembers() const
    {
        return Members;
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
    void SetString(const char* Str)
    {
        if (pTokenMemory)
        {
            GZoneMemory.Free(pTokenMemory);
        }

        int len = Platform::Strlen(Str);

        pTokenMemory = GZoneMemory.Alloc(len + 1);
        Platform::Memcpy(pTokenMemory, Str, len + 1);

        StrBegin = (const char*)pTokenMemory;
        StrEnd   = (const char*)pTokenMemory + len;
    }

    void SetStringInsitu(const char* Begin, const char* End)
    {
        StrBegin = Begin;
        StrEnd   = End;
    }

    /** Get string value */
    AString GetString() const
    {
        return AString(StrBegin, StrEnd);
    }

    void Print() const;

private:
    void AddMember(ADocMember* Member);

protected:
    int Type;

private:
    const char* StrBegin; // string data for TYPE_STRING
    const char* StrEnd;   // string data for TYPE_STRING

    void* pTokenMemory;

    ADocMember* Members;    // for TYPE_OBJECT list of members
    ADocMember* MembersEnd; // for TYPE_OBJECT list of members

    ADocValue* Next;
    ADocValue* Prev;

    friend class ADocument;
    friend class ADocMember;
};

class ADocString : public ADocValue
{
public:
    ADocString() :
        ADocValue(TYPE_STRING)
    {
    }
};

class ADocObject : public ADocValue
{
public:
    ADocObject() :
        ADocValue(TYPE_OBJECT)
    {
    }
};

class ADocMember : public ARefCounted //TRefCounted< SDocumentAllocator< ADocMember > >
{
public:
    ADocMember()
    {
        NameBegin = "";
        NameEnd   = "";
        Values    = nullptr;
        ValuesEnd = nullptr;
        Next      = nullptr;
        Prev      = nullptr;
    }

    ~ADocMember();

    /** Add value to array */
    void AddValue(ADocValue* _Value);

    /** Get member name */
    AString GetName() const
    {
        return AString(NameBegin, NameEnd);
    }

    /** Get member value as string if possible */
    AString GetString() const
    {
        if (IsString())
        {
            return AString(Values->StrBegin, Values->StrEnd);
        }

        return AString::NullString();
    }

    /** Is string member */
    bool IsString() const
    {
        return HasSingleValue() && Values->IsString();
    }

    /** Is object member */
    bool IsObject() const
    {
        return HasSingleValue() && Values->IsObject();
    }

    /** Is array member */
    bool HasSingleValue() const
    {
        if (!Values || Values->Next)
        {
            return false;
        }
        return true;
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

    ADocValue* Values;
    ADocValue* ValuesEnd;

    ADocMember* Next;
    ADocMember* Prev;

    friend class ADocument;
    friend class ADocValue;
};

class ADocument : public ARefCounted
{
public:
    ADocument();
    ~ADocument();

    void Clear();

    AString SerializeToString(SDocumentSerializeInfo const& SerializeInfo) const;

    void DeserializeFromString(SDocumentDeserializeInfo const& DeserializeInfo);

    /** Find global member */
    ADocMember*       FindMember(const char* Name);
    ADocMember const* FindMember(const char* Name) const;

    /** Add global string member */
    ADocMember* AddString(const char* MemberName, const char* Str);

    ADocMember* AddString(const char* MemberName, AString const& Str)
    {
        return AddString(MemberName, Str.CStr());
    }

    /** Add global object member */
    ADocMember* AddObject(const char* MemberName, ADocObject* Object);

    /** Add global array member */
    ADocMember* AddArray(const char* ArrayName);

    void Print() const;

private:
    TRef<ADocObject> ParseObject();
    TRef<ADocMember> ParseMember(SToken const& MemberToken);
    void             ParseArray(ADocValue** ppArrayHead, ADocValue** ppArrayTail);

    /** Add global member */
    void AddMember(ADocMember* Member);

    ADocumentTokenizer Tokenizer;

    /** Global members */
    ADocMember* MembersHead = nullptr;
    ADocMember* MembersTail = nullptr;

    friend class ADocMember;
    friend class ADocValue;
};
