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

#include <Core/Document.h>
#include <Core/Parse.h>
#include <Platform/Logger.h>

HK_NAMESPACE_BEGIN

static const char* TokenType[] = {
    "Unknown token",
    "EOF",
    "Bracket",
    "Member",
    "String"};

const char* DocumentToken::NamedType() const
{
    return TokenType[Type];
}

DocumentTokenizer::DocumentTokenizer()
{
    m_Cur = m_Start = (char*)"";
    m_LineNumber    = 1;
    m_bInSitu       = true;
}

DocumentTokenizer::~DocumentTokenizer()
{
    if (!m_bInSitu)
    {
        Platform::GetHeapAllocator<HEAP_STRING>().Free(m_Start);
    }
}

void DocumentTokenizer::Reset(const char* pDocumentData, bool InSitu)
{
    if (!m_bInSitu)
    {
        Platform::GetHeapAllocator<HEAP_STRING>().Free(m_Start);
        m_Cur = m_Start = (char*)"";
        m_LineNumber    = 1;
        m_bInSitu       = true;
    }

    m_bInSitu = InSitu;
    if (m_bInSitu)
    {
        m_Start = const_cast<char*>(pDocumentData);
    }
    else
    {
        int n = StringLength(pDocumentData) + 1;
        m_Start = (char*)Platform::GetHeapAllocator<HEAP_STRING>().Alloc(n);
        Platform::Memcpy(m_Start, pDocumentData, n);
    }
    m_Cur        = m_Start;
    m_LineNumber = 1;

    // Go to first token
    NextToken();
}

void DocumentTokenizer::SkipWhitespaces()
{
start:
    while (*m_Cur == ' ' || *m_Cur == '\t' || *m_Cur == '\n' || *m_Cur == '\r')
    {
        if (*m_Cur == '\n')
        {
            m_LineNumber++;
        }
        m_Cur++;
    }
    if (*m_Cur == '/')
    {
        if (*(m_Cur + 1) == '/')
        {
            m_Cur += 2;
            // go to next line
            while (*m_Cur && *m_Cur != '\n')
                m_Cur++;
            goto start;
        }
        if (*(m_Cur + 1) == '*')
        {
            m_Cur += 2;
            while (*m_Cur)
            {
                if (*m_Cur == '\n')
                {
                    m_LineNumber++;
                }
                else if (*m_Cur == '*' && *(m_Cur + 1) == '/')
                {
                    m_Cur += 2;
                    goto start;
                }
                m_Cur++;
            }
            LOG("Warning: unclosed comment /* */\n");
            return;
        }
    }
}

void DocumentTokenizer::NextToken()
{
    // Skip white spaces, tabs and comments
    SkipWhitespaces();

    // Check string
    if (*m_Cur == '\"')
    {
        m_Cur++;
        m_CurToken.Begin = m_Cur;
        for (;;)
        {
            if (*m_Cur == '\"' && *(m_Cur - 1) != '\\')
            {
                break;
            }

            if (*m_Cur == 0)
            {
                // unexpected eof
                m_CurToken.Begin = m_CurToken.End = "";
                m_CurToken.Type                   = DOCUMENT_TOKEN_UNKNOWN;
                return;
            }
            if (*m_Cur == '\n')
            {
                // unexpected eol
                m_CurToken.Begin = m_CurToken.End = "";
                m_CurToken.Type                   = DOCUMENT_TOKEN_UNKNOWN;
                return;
            }
            m_Cur++;
        }
        m_CurToken.End = m_Cur++;
        m_CurToken.Type = DOCUMENT_TOKEN_STRING;
        return;
    }

    // Check brackets
    if (*m_Cur == '{' || *m_Cur == '}' || *m_Cur == '[' || *m_Cur == ']')
    {
        m_CurToken.Begin = m_Cur;
        m_CurToken.End   = ++m_Cur;
        m_CurToken.Type  = DOCUMENT_TOKEN_BRACKET;
        return;
    }

    // Check member
    m_CurToken.Begin = m_Cur;
    while ((*m_Cur >= 'a' && *m_Cur <= 'z') || (*m_Cur >= 'A' && *m_Cur <= 'Z') || (*m_Cur >= '0' && *m_Cur <= '9') || *m_Cur == '_' || *m_Cur == '.' || *m_Cur == '$')
    {
        m_Cur++;
    }
    m_CurToken.End = m_Cur;
    if (m_CurToken.Begin == m_CurToken.End)
    {
        if (*m_Cur)
        {
            LOG("undefined symbols in token\n");
            m_CurToken.Type = DOCUMENT_TOKEN_UNKNOWN;
        }
        else
        {
            m_CurToken.Type = DOCUMENT_TOKEN_EOF;
        }
        return;
    }
    else
    {
        m_CurToken.Type = DOCUMENT_TOKEN_MEMBER;
    }
}


struct ADocumentSerializer
{
    int DocumentStack = 0;

    String InsertSpaces();

    String SerializeValue(DocumentValue const* Value);
    String SerializeMember(DocumentMember const* Member);
    String SerializeObject(DocumentMember const* Members, bool& bSingleMember);

    String SerializeValueCompact(DocumentValue const* Value);
    String SerializeMemberCompact(DocumentMember const* Member);
    String SerializeObjectCompact(DocumentMember const* Members);
};

String ADocumentSerializer::InsertSpaces()
{
    String s;
    s.Resize(DocumentStack);
    return s;
}

String ADocumentSerializer::SerializeValue(DocumentValue const* Value)
{
    if (Value->IsString())
    {
        return HK_FORMAT("\"{}\"", Value->GetStringView());
    }

    String s("{");

    DocumentStack++;
    bool bSingleMember;
    s += SerializeObject(Value->GetListOfMembers(), bSingleMember);
    DocumentStack--;

    if (!bSingleMember)
    {
        s += InsertSpaces();
    }
    s += "}";

    return s;
}

String ADocumentSerializer::SerializeMember(DocumentMember const* Member)
{
    String s;

    s += Member->GetName();
    s += " ";

    DocumentValue const* value = Member->GetArrayValues();
    if (value)
    {
        bool bSingleValue = true;

        while (value)
        {
            DocumentValue const* next = value->GetNext();

            if (bSingleValue && next != nullptr)
            {
                bSingleValue = false;
                s += /*"\n" + InserSpaces() + */ "[\n";
                DocumentStack++;
            }

            if (!bSingleValue)
            {
                s += InsertSpaces();
            }
            s += SerializeValue(value);
            if (!bSingleValue)
            {
                s += "\n";
            }

            value = next;
        }

        if (!bSingleValue)
        {
            DocumentStack--;
            s += InsertSpaces() + "]";
        }
    }
    else
    {
        s += "[]\n";
    }

    return s;
}

String ADocumentSerializer::SerializeObject(DocumentMember const* Members, bool& bSingleMember)
{
    String s;
    bSingleMember = true;
    for (DocumentMember const* f = Members; f;)
    {
        DocumentMember const* next = f->GetNext();

        if (bSingleMember && next != nullptr)
        {
            bSingleMember = false;
            s += "\n";
        }

        if (!bSingleMember)
        {
            s += InsertSpaces();
        }
        s += SerializeMember(f);

        if (!bSingleMember)
        {
            s += "\n";
        }

        f = next;
    }
    return s;
}

String ADocumentSerializer::SerializeValueCompact(DocumentValue const* Value)
{
    if (Value->IsString())
    {
        return HK_FORMAT("\"{}\"", Value->GetStringView());
    }
    return HK_FORMAT("{{{}}}", SerializeObjectCompact(Value->GetListOfMembers()));
}

String ADocumentSerializer::SerializeMemberCompact(DocumentMember const* Member)
{
    String s;

    s += Member->GetName();

    DocumentValue const* value = Member->GetArrayValues();
    if (value)
    {
        bool singleValue = true;
        while (value)
        {
            DocumentValue const* next = value->GetNext();
            if (singleValue && next != nullptr)
            {
                singleValue = false;
                s += "[";
            }
            s += SerializeValueCompact(value);
            value = next;
        }
        if (!singleValue)
        {
            s += "]";
        }
    }
    else
    {
        s += "[]";
    }
    return s;
}

String ADocumentSerializer::SerializeObjectCompact(DocumentMember const* Members)
{
    String s;
    for (DocumentMember const* member = Members; member; member = member->GetNext())
    {
        s += SerializeMemberCompact(member);
    }
    return s;
}

DocumentValue::DocumentValue(TYPE Type) :
    m_Type(Type)
{
    m_StrBegin = m_StrEnd = "";
}

DocumentValue::~DocumentValue()
{
    for (DocumentMember* member = m_MembersHead; member;)
    {
        DocumentMember* next = member->GetNext();
        member->RemoveRef();
        member = next;
    }
}

DocumentMember* DocumentValue::FindMember(StringView Name)
{
    for (DocumentMember* member = m_MembersHead; member; member = member->GetNext())
    {
        if (Name.Icompare(member->GetName()))
        {
            return member;
        }
    }
    return nullptr;
}

DocumentMember const* DocumentValue::FindMember(StringView Name) const
{
    return const_cast<DocumentValue*>(this)->FindMember(Name);
}

bool DocumentValue::GetBool(StringView Name, bool Default) const
{
    DocumentMember const* member = FindMember(Name);
    if (!member)
        return Default;
    return Core::ParseBool(member->GetStringView());
}

uint8_t DocumentValue::GetUInt8(StringView Name, uint8_t Default) const
{
    DocumentMember const* member = FindMember(Name);
    if (!member)
        return Default;
    return Core::ParseUInt8(member->GetStringView());
}

uint16_t DocumentValue::GetUInt16(StringView Name, uint16_t Default) const
{
    DocumentMember const* member = FindMember(Name);
    if (!member)
        return Default;
    return Core::ParseUInt16(member->GetStringView());
}

uint32_t DocumentValue::GetUInt32(StringView Name, uint32_t Default) const
{
    DocumentMember const* member = FindMember(Name);
    if (!member)
        return Default;
    return Core::ParseUInt32(member->GetStringView());
}

uint64_t DocumentValue::GetUInt64(StringView Name, uint64_t Default) const
{
    DocumentMember const* member = FindMember(Name);
    if (!member)
        return Default;
    return Core::ParseUInt64(member->GetStringView());
}

int8_t DocumentValue::GetInt8(StringView Name, int8_t Default) const
{
    DocumentMember const* member = FindMember(Name);
    if (!member)
        return Default;
    return Core::ParseInt8(member->GetStringView());
}

int16_t DocumentValue::GetInt16(StringView Name, int16_t Default) const
{
    DocumentMember const* member = FindMember(Name);
    if (!member)
        return Default;
    return Core::ParseInt16(member->GetStringView());
}

int32_t DocumentValue::GetInt32(StringView Name, int32_t Default) const
{
    DocumentMember const* member = FindMember(Name);
    if (!member)
        return Default;
    return Core::ParseInt32(member->GetStringView());
}

int64_t DocumentValue::GetInt64(StringView Name, int64_t Default) const
{
    DocumentMember const* member = FindMember(Name);
    if (!member)
        return Default;
    return Core::ParseInt64(member->GetStringView());
}

float DocumentValue::GetFloat(StringView Name, float Default) const
{
    DocumentMember const* member = FindMember(Name);
    if (!member)
        return Default;
    return Core::ParseFloat(member->GetStringView());
}

double DocumentValue::GetDouble(StringView Name, double Default) const
{
    DocumentMember const* member = FindMember(Name);
    if (!member)
        return Default;
    return Core::ParseDouble(member->GetStringView());
}

StringView DocumentValue::GetString(StringView Name, StringView Default) const
{
    DocumentMember const* member = FindMember(Name);
    if (!member)
        return Default;
    return member->GetStringView();
}

void DocumentValue::Clear()
{
    for (DocumentMember* member = m_MembersHead; member;)
    {
        DocumentMember* next = member->m_Next;
        member->RemoveRef();
        member = next;
    }

    m_MembersHead = nullptr;
    m_MembersTail = nullptr;
}

DocumentMember* DocumentValue::AddString(GlobalStringView Name, StringView Str)
{
    if (!IsObject())
    {
        LOG("DocumentValue::AddString: called on non-object type\n");
        return nullptr;
    }

    // Create member
    TRef<DocumentMember> member = MakeRef<DocumentMember>();
    member->SetName(Name);

    // Create string
    TRef<DocumentValue> value = MakeRef<DocumentValue>(TYPE_STRING);

    // Allocate string
    value->SetString(Str);

    member->AddValue(value);

    AddMember(member);

    return member;
}

DocumentMember* DocumentValue::AddString(StringView Name, StringView Str)
{
    if (!IsObject())
    {
        LOG("DocumentValue::AddString: called on non-object type\n");
        return nullptr;
    }

    // Create member
    TRef<DocumentMember> member = MakeRef<DocumentMember>();
    member->SetName(Name);

    // Create string
    TRef<DocumentValue> value = MakeRef<DocumentValue>(TYPE_STRING);

    // Allocate string
    value->SetString(Str);

    member->AddValue(value);

    AddMember(member);

    return member;
}

DocumentMember* DocumentValue::AddObject(GlobalStringView Name, DocumentValue* Object)
{
    if (!IsObject())
    {
        LOG("DocumentValue::AddObject: called on non-object type\n");
        return nullptr;
    }

    // Create member
    TRef<DocumentMember> member = MakeRef<DocumentMember>();
    member->SetName(Name);

    member->AddValue(Object);

    AddMember(member);

    return member;
}

DocumentMember* DocumentValue::AddArray(GlobalStringView ArrayName)
{
    if (!IsObject())
    {
        LOG("DocumentValue::AddArray: called on non-object type\n");
        return nullptr;
    }

    TRef<DocumentMember> array = MakeRef<DocumentMember>();
    array->SetName(ArrayName);

    AddMember(array);

    return array;
}

void DocumentValue::AddMember(DocumentMember* Member)
{
    HK_ASSERT(m_Type == DocumentValue::TYPE_OBJECT);

    Member->AddRef();

    Member->m_Prev = m_MembersTail;
    if (Member->m_Prev != nullptr)
    {
        Member->m_Prev->m_Next = Member;
    }
    else
    {
        m_MembersHead = Member;
    }
    m_MembersTail = Member;
}

void DocumentValue::Print() const
{
    LOG("Type: {}\n", IsString() ? "STRING" : "OBJECT");

    if (IsString())
    {
        LOG("{}\n", GetStringView());
        return;
    }

    for (DocumentMember const* member = GetListOfMembers(); member; member = member->GetNext())
    {
        member->Print();
    }
}

String DocumentValue::SerializeToString(DocumentSerializeInfo const& SerializeInfo) const
{
    ADocumentSerializer serializer;

    if (SerializeInfo.bCompactStringConversion)
    {
        return serializer.SerializeObjectCompact(GetListOfMembers());
    }

    bool bSingleMember;
    return serializer.SerializeObject(GetListOfMembers(), bSingleMember);
}

DocumentMember::DocumentMember()
{
    m_NameBegin = m_NameEnd = "";
}

void DocumentMember::SetName(GlobalStringView Name)
{
    m_NameBegin = Name.CStr();
    m_NameEnd = Name.CStr() + StringLength(Name);
}

void DocumentMember::SetName(StringView Name)
{
    if (m_NameData)
        Platform::GetHeapAllocator<HEAP_STRING>().Free(m_NameData);
    m_NameData = (char*)Platform::GetHeapAllocator<HEAP_STRING>().Alloc(Name.Size() + 1);
    Platform::Memcpy(m_NameData, Name.Begin(), Name.Size());
    m_NameData[Name.Size()] = 0;
    m_NameBegin = m_NameData;
    m_NameEnd = m_NameData + Name.Size();
}

DocumentMember::~DocumentMember()
{
    for (DocumentValue* value = m_Values; value;)
    {
        DocumentValue* next = value->m_Next;
        value->RemoveRef();
        value = next;
    }

    if (m_NameData)
    {
        Platform::GetHeapAllocator<HEAP_STRING>().Free(m_NameData);
    }
}

void DocumentMember::AddValue(DocumentValue* pValue)
{
    HK_ASSERT(pValue->m_Next == nullptr && pValue->m_Prev == nullptr);
    pValue->AddRef();
    pValue->m_Prev = m_ValuesEnd;
    if (pValue->m_Prev != nullptr)
    {
        pValue->m_Prev->m_Next = pValue;
    }
    else
    {
        m_Values = pValue;
    }
    m_ValuesEnd = pValue;
}

void DocumentMember::Print() const
{
    LOG("Member: {}\n", GetName());

    for (DocumentValue const* value = GetArrayValues(); value; value = value->GetNext())
    {
        value->Print();
    }
}

Document::Document() :
    DocumentValue(TYPE_OBJECT)
{}

void Document::ParseArray(DocumentValue** ppArrayHead, DocumentValue** ppArrayTail)
{
    bool bHasErrors = false;

    *ppArrayHead = nullptr;
    *ppArrayTail = nullptr;

    while (1)
    {
        DocumentToken token = m_Tokenizer.GetToken();

        if (token.Type == DOCUMENT_TOKEN_BRACKET)
        {
            if (*token.Begin == ']')
            {
                m_Tokenizer.NextToken();
                if (*ppArrayHead == nullptr)
                {
                    // empty array
                }
                break;
            }
            if (*token.Begin != '{')
            {
                LOG("unexpected bracket {}\n", *token.Begin);
                bHasErrors = true;
                break;
            }

            m_Tokenizer.NextToken();

            // array element is object
            TRef<DocumentValue> object = ParseObject();
            if (object == nullptr)
            {
                bHasErrors = true;
                break;
            }

            // add object to array
            object->AddRef();
            object->m_Prev = *ppArrayTail;
            if (object->m_Prev != nullptr)
            {
                object->m_Prev->m_Next = object;
            }
            else
            {
                *ppArrayHead = object;
            }
            *ppArrayTail = object;

            continue;
        }

        if (token.Type == DOCUMENT_TOKEN_STRING)
        {
            // array element is string

            TRef<DocumentValue> value = MakeRef<DocumentValue>(TYPE_STRING);
            value->SetStringInsitu({token.Begin, token.End});

            // Add string to array
            value->AddRef();
            value->m_Prev = *ppArrayTail;
            if (value->m_Prev != nullptr)
            {
                value->m_Prev->m_Next = value;
            }
            else
            {
                *ppArrayHead = value;
            }
            *ppArrayTail = value;

            m_Tokenizer.NextToken();

            continue;
        }

        LOG("unexpected '{}' {}\n", token.NamedType(), StringView(token.Begin, token.End));
        bHasErrors = true;
        break;
    }

    if (bHasErrors)
    {
        for (DocumentValue* value = *ppArrayHead; value;)
        {
            DocumentValue* next = value->m_Next;
            value->RemoveRef();
            value = next;
        }

        *ppArrayHead = *ppArrayTail = nullptr;
    }
}

static bool Expect(DOCUMENT_TOKEN_TYPE Type, DocumentToken const& Token)
{
    if (Token.Type != Type)
    {
        LOG("unexpected {} found, expected {}\n", Token.NamedType(), TokenType[Type]);
        return false;
    }
    return true;
}

TRef<DocumentValue> Document::ParseObject()
{
    TRef<DocumentValue> value = MakeRef<DocumentValue>(TYPE_OBJECT);

    while (1)
    {
        DocumentToken token = m_Tokenizer.GetToken();

        if (token.Type == DOCUMENT_TOKEN_BRACKET)
        {
            if (*token.Begin == '}')
            {
                m_Tokenizer.NextToken();
                return value;
            }

            LOG("unexpected bracket {}\n", *token.Begin);
            break;
        }

        if (!Expect(DOCUMENT_TOKEN_MEMBER, token))
        {
            //error
            break;
        }

        m_Tokenizer.NextToken();

        TRef<DocumentMember> member = ParseMember(token);
        if (member == nullptr)
        {
            break;
        }

        value->AddMember(member);
    }

    return {};
}

TRef<DocumentMember> Document::ParseMember(DocumentToken const& MemberToken)
{
    DocumentToken token = m_Tokenizer.GetToken();

    if (token.Type == DOCUMENT_TOKEN_BRACKET)
    {
        if (*token.Begin == '[')
        {
            // value is array
            m_Tokenizer.NextToken();

            DocumentValue *arrayHead, *arrayTail;
            ParseArray(&arrayHead, &arrayTail);
            if (arrayHead == nullptr)
            {
                return {};
            }

            TRef<DocumentMember> member = MakeRef<DocumentMember>();
            member->m_NameBegin     = MemberToken.Begin;
            member->m_NameEnd       = MemberToken.End;
            member->m_Values        = arrayHead;
            member->m_ValuesEnd     = arrayTail;
            return member;
        }
        if (*token.Begin == '{')
        {
            // value is object
            m_Tokenizer.NextToken();

            TRef<DocumentValue> object = ParseObject();
            if (object == nullptr)
            {
                return {};
            }

            TRef<DocumentMember> member = MakeRef<DocumentMember>();
            member->m_NameBegin     = MemberToken.Begin;
            member->m_NameEnd       = MemberToken.End;
            member->m_Values = member->m_ValuesEnd = object;

            object->AddRef();

            return member;
        }
        LOG("unexpected bracket {}\n", *token.Begin);
        return {};
    }

    if (token.Type == DOCUMENT_TOKEN_STRING)
    {
        m_Tokenizer.NextToken();

        // value is string
        TRef<DocumentValue> value = MakeRef<DocumentValue>(TYPE_STRING);
        value->SetStringInsitu({token.Begin, token.End});

        TRef<DocumentMember> member = MakeRef<DocumentMember>();
        member->m_NameBegin     = MemberToken.Begin;
        member->m_NameEnd       = MemberToken.End;
        member->m_Values = member->m_ValuesEnd = value;

        value->AddRef();

        return member;
    }

    LOG("expected value, found {}\n", token.NamedType());
    return {};
}

void Document::DeserializeFromString(DocumentDeserializeInfo const& DeserializeInfo)
{
    Clear();

    m_Tokenizer.Reset(DeserializeInfo.pDocumentData, DeserializeInfo.bInsitu);

    while (1)
    {
        DocumentToken token = m_Tokenizer.GetToken();
        if (token.Type == DOCUMENT_TOKEN_EOF || token.Type == DOCUMENT_TOKEN_UNKNOWN)
        {
            break;
        }

        if (!Expect(DOCUMENT_TOKEN_MEMBER, token))
        {
            //error
            Clear();
            break;
        }

        m_Tokenizer.NextToken();

        TRef<DocumentMember> member = ParseMember(token);
        if (member == nullptr)
        {
            // error
            Clear();
            break;
        }

        AddMember(member);
    }
}

HK_NAMESPACE_END
