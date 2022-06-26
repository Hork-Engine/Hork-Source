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
#include <Platform/Logger.h>


static const char* TokenType[] = {
    "Unknown token",
    "EOF",
    "Bracket",
    "Member",
    "String"};

const char* SToken::NamedType() const
{
    return TokenType[Type];
}


ADocumentTokenizer::ADocumentTokenizer()
{
    Cur = Start = (char*)"";
    LineNumber  = 1;
    bInSitu     = true;
}

ADocumentTokenizer::~ADocumentTokenizer()
{
    if (!bInSitu)
    {
        Platform::GetHeapAllocator<HEAP_STRING>().Free(Start);
    }
}

void ADocumentTokenizer::Reset(const char* pDocumentData, bool InSitu)
{
    if (!bInSitu)
    {
        Platform::GetHeapAllocator<HEAP_STRING>().Free(Start);
        Cur = Start = (char*)"";
        LineNumber  = 1;
        bInSitu     = true;
    }

    bInSitu = InSitu;
    if (bInSitu)
    {
        Start = const_cast<char*>(pDocumentData);
    }
    else
    {
        int n = Platform::Strlen(pDocumentData) + 1;
        Start = (char*)Platform::GetHeapAllocator<HEAP_STRING>().Alloc(n);
        Platform::Memcpy(Start, pDocumentData, n);
    }
    Cur        = Start;
    LineNumber = 1;

    // Go to first token
    NextToken();
}

void ADocumentTokenizer::SkipWhitespaces()
{
start:
    while (*Cur == ' ' || *Cur == '\t' || *Cur == '\n' || *Cur == '\r')
    {
        if (*Cur == '\n')
        {
            LineNumber++;
        }
        Cur++;
    }
    if (*Cur == '/')
    {
        if (*(Cur + 1) == '/')
        {
            Cur += 2;
            // go to next line
            while (*Cur && *Cur != '\n')
                Cur++;
            goto start;
        }
        if (*(Cur + 1) == '*')
        {
            Cur += 2;
            while (*Cur)
            {
                if (*Cur == '\n')
                {
                    LineNumber++;
                }
                else if (*Cur == '*' && *(Cur + 1) == '/')
                {
                    Cur += 2;
                    goto start;
                }
                Cur++;
            }
            LOG("Warning: unclosed comment /* */\n");
            return;
        }
    }
}

void ADocumentTokenizer::NextToken()
{
    // Skip white spaces, tabs and comments
    SkipWhitespaces();

    // Check string
    if (*Cur == '\"')
    {
        Cur++;
        CurToken.Begin = Cur;
        for (;;)
        {
            if (*Cur == '\"' && *(Cur - 1) != '\\')
            {
                break;
            }

            if (*Cur == 0)
            {
                // unexpected eof
                CurToken.Begin = CurToken.End = "";
                CurToken.Type                 = TOKEN_TYPE_UNKNOWN;
                return;
            }
            if (*Cur == '\n')
            {
                // unexpected eol
                CurToken.Begin = CurToken.End = "";
                CurToken.Type                 = TOKEN_TYPE_UNKNOWN;
                return;
            }
            Cur++;
        }
        CurToken.End  = Cur++;
        CurToken.Type = TOKEN_TYPE_STRING;
        return;
    }

    // Check brackets
    if (*Cur == '{' || *Cur == '}' || *Cur == '[' || *Cur == ']')
    {
        CurToken.Begin = Cur;
        CurToken.End   = ++Cur;
        CurToken.Type  = TOKEN_TYPE_BRACKET;
        return;
    }

    // Check member
    CurToken.Begin = Cur;
    while ((*Cur >= 'a' && *Cur <= 'z') || (*Cur >= 'A' && *Cur <= 'Z') || (*Cur >= '0' && *Cur <= '9') || *Cur == '_' || *Cur == '.')
    {
        Cur++;
    }
    CurToken.End = Cur;
    if (CurToken.Begin == CurToken.End)
    {
        if (*Cur)
        {
            LOG("undefined symbols in token\n");
            CurToken.Type = TOKEN_TYPE_UNKNOWN;
        }
        else
        {
            CurToken.Type = TOKEN_TYPE_EOF;
        }
        return;
    }
    else
    {
        CurToken.Type = TOKEN_TYPE_MEMBER;
    }
}


struct ADocumentSerializer
{
    int DocumentStack = 0;

    AString InsertSpaces();

    AString SerializeValue(ADocValue const* Value);
    AString SerializeMember(ADocMember const* Member);
    AString SerializeObject(ADocMember const* Members, bool& bSingleMember);

    AString SerializeValueCompact(ADocValue const* Value);
    AString SerializeMemberCompact(ADocMember const* Member);
    AString SerializeObjectCompact(ADocMember const* Members);
};

AString ADocumentSerializer::InsertSpaces()
{
    AString s;
    s.Resize(DocumentStack);
    return s;
}

AString ADocumentSerializer::SerializeValue(ADocValue const* Value)
{
    if (Value->IsString())
    {
        return Core::Format("\"{}\"", Value->GetString());
    }

    AString s("{");

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

AString ADocumentSerializer::SerializeMember(ADocMember const* Member)
{
    AString s;

    s += Member->GetName();
    s += " ";

    bool bSingleValue = true;

    for (ADocValue const* value = Member->GetArrayValues(); value;)
    {
        ADocValue const* next = value->GetNext();

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

    return s;
}

AString ADocumentSerializer::SerializeObject(ADocMember const* Members, bool& bSingleMember)
{
    AString s;
    bSingleMember = true;
    for (ADocMember const* f = Members; f;)
    {
        ADocMember const* next = f->GetNext();

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

AString ADocumentSerializer::SerializeValueCompact(ADocValue const* Value)
{
    if (Value->IsString())
    {
        return Core::Format("\"{}\"", Value->GetString());
    }
    return Core::Format("{{{}}}", SerializeObjectCompact(Value->GetListOfMembers()));
}

AString ADocumentSerializer::SerializeMemberCompact(ADocMember const* Member)
{
    AString s;

    s += Member->GetName();

    bool singleValue = true;
    for (ADocValue const* value = Member->GetArrayValues(); value;)
    {
        ADocValue const* next = value->GetNext();
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
    return s;
}

AString ADocumentSerializer::SerializeObjectCompact(ADocMember const* Members)
{
    AString s;
    for (ADocMember const* member = Members; member; member = member->GetNext())
    {
        s += SerializeMemberCompact(member);
    }
    return s;
}


ADocument::ADocument()
{
}

ADocument::~ADocument()
{
    Clear();
}

void ADocument::Clear()
{
    for (ADocMember* member = MembersHead; member;)
    {
        ADocMember* next = member->Next;
        member->RemoveRef();
        member = next;
    }

    MembersHead = nullptr;
    MembersTail = nullptr;
}

void ADocument::ParseArray(ADocValue** ppArrayHead, ADocValue** ppArrayTail)
{
    bool bHasErrors = false;

    *ppArrayHead = nullptr;
    *ppArrayTail = nullptr;

    while (1)
    {
        SToken token = Tokenizer.GetToken();

        if (token.Type == TOKEN_TYPE_BRACKET)
        {
            if (*token.Begin == ']')
            {
                Tokenizer.NextToken();
                if (*ppArrayHead == nullptr)
                {
                    LOG("empty array\n");
                }
                break;
            }
            if (*token.Begin != '{')
            {
                LOG("unexpected bracket {}\n", *token.Begin);
                bHasErrors = true;
                break;
            }

            Tokenizer.NextToken();

            // array element is object
            TRef<ADocObject> object = ParseObject();
            if (object == nullptr)
            {
                bHasErrors = true;
                break;
            }

            // add object to array
            object->AddRef();
            object->Prev = *ppArrayTail;
            if (object->Prev != nullptr)
            {
                object->Prev->Next = object;
            }
            else
            {
                *ppArrayHead = object;
            }
            *ppArrayTail = object;

            continue;
        }

        if (token.Type == TOKEN_TYPE_STRING)
        {
            // array element is string

            TRef<ADocString> value = MakeRef<ADocString>();
            value->SetStringInsitu(token.Begin, token.End);

            // Add string to array
            value->AddRef();
            value->Prev = *ppArrayTail;
            if (value->Prev != nullptr)
            {
                value->Prev->Next = value;
            }
            else
            {
                *ppArrayHead = value;
            }
            *ppArrayTail = value;

            Tokenizer.NextToken();

            continue;
        }

        LOG("unexpected {}\n", token.NamedType());
        bHasErrors = true;
        break;
    }

    if (bHasErrors)
    {
        for (ADocValue* value = *ppArrayHead; value; value = value->Next)
        {
            value->RemoveRef();
        }

        *ppArrayHead = *ppArrayTail = nullptr;
    }
}

static bool Expect(int Type, SToken const& Token)
{
    if (Token.Type != Type)
    {
        LOG("unexpected {} found, expected {}\n", Token.NamedType(), TokenType[Type]);
        return false;
    }
    return true;
}

TRef<ADocObject> ADocument::ParseObject()
{
    TRef<ADocObject> value = MakeRef<ADocObject>();

    while (1)
    {
        SToken token = Tokenizer.GetToken();

        if (token.Type == TOKEN_TYPE_BRACKET)
        {
            if (*token.Begin == '}')
            {
                if (value->MembersEnd == nullptr)
                {
                    LOG("empty object\n");
                    break;
                }
                Tokenizer.NextToken();
                return value;
            }

            LOG("unexpected bracket {}\n", *token.Begin);
            break;
        }

        if (!Expect(TOKEN_TYPE_MEMBER, token))
        {
            //error
            break;
        }

        Tokenizer.NextToken();

        TRef<ADocMember> member = ParseMember(token);
        if (member == nullptr)
        {
            break;
        }

        value->AddMember(member);
    }

    return TRef<ADocObject>();
}

TRef<ADocMember> ADocument::ParseMember(SToken const& MemberToken)
{
    SToken token = Tokenizer.GetToken();

    if (token.Type == TOKEN_TYPE_BRACKET)
    {
        if (*token.Begin == '[')
        {
            // value is array
            Tokenizer.NextToken();

            ADocValue *arrayHead, *arrayTail;
            ParseArray(&arrayHead, &arrayTail);
            if (arrayHead == nullptr)
            {
                return TRef<ADocMember>();
            }

            TRef<ADocMember> member = MakeRef<ADocMember>();
            member->NameBegin       = MemberToken.Begin;
            member->NameEnd         = MemberToken.End;
            member->Values          = arrayHead;
            member->ValuesEnd       = arrayTail;
            return member;
        }
        if (*token.Begin == '{')
        {
            // value is object
            Tokenizer.NextToken();

            TRef<ADocObject> object = ParseObject();
            if (object == nullptr)
            {
                return TRef<ADocMember>();
            }

            TRef<ADocMember> member = MakeRef<ADocMember>();
            member->NameBegin       = MemberToken.Begin;
            member->NameEnd         = MemberToken.End;
            member->Values = member->ValuesEnd = object;

            object->AddRef();

            return member;
        }
        LOG("unexpected bracket {}\n", *token.Begin);
        return TRef<ADocMember>();
    }

    if (token.Type == TOKEN_TYPE_STRING)
    {
        Tokenizer.NextToken();

        // value is string
        TRef<ADocString> value = MakeRef<ADocString>();
        value->SetStringInsitu(token.Begin, token.End);

        TRef<ADocMember> member = MakeRef<ADocMember>();
        member->NameBegin       = MemberToken.Begin;
        member->NameEnd         = MemberToken.End;
        member->Values = member->ValuesEnd = value;

        value->AddRef();

        return member;
    }

    LOG("expected value, found {}\n", token.NamedType());
    return TRef<ADocMember>();
}

void ADocument::Print() const
{
    LOG("-------------- Document ----------------\n");

    for (ADocMember const* member = MembersHead; member; member = member->Next)
    {
        member->Print();
    }

    LOG("----------------------------------------\n");
}

AString ADocument::SerializeToString(SDocumentSerializeInfo const& SerializeInfo) const
{
    ADocumentSerializer serializer;

    if (SerializeInfo.bCompactStringConversion)
    {
        return serializer.SerializeObjectCompact(MembersHead);
    }

    bool bSingleMember;
    return serializer.SerializeObject(MembersHead, bSingleMember);
}

void ADocument::DeserializeFromString(SDocumentDeserializeInfo const& DeserializeInfo)
{
    Clear();

    Tokenizer.Reset(DeserializeInfo.pDocumentData, DeserializeInfo.bInsitu);

    while (1)
    {
        SToken token = Tokenizer.GetToken();
        if (token.Type == TOKEN_TYPE_EOF || token.Type == TOKEN_TYPE_UNKNOWN)
        {
            break;
        }

        if (!Expect(TOKEN_TYPE_MEMBER, token))
        {
            //error
            Clear();
            break;
        }

        Tokenizer.NextToken();

        TRef<ADocMember> member = ParseMember(token);
        if (member == nullptr)
        {
            // error
            Clear();
            break;
        }

        AddMember(member);
    }
}

ADocMember* ADocument::AddString(const char* MemberName, const char* Str)
{
    // Create member
    TRef<ADocMember> member = MakeRef<ADocMember>();
    member->NameBegin       = MemberName;
    member->NameEnd         = MemberName + Platform::Strlen(MemberName);

    // Create string
    TRef<ADocString> value = MakeRef<ADocString>();

    // Allocate string
    value->SetString(Str);

    member->AddValue(value);

    AddMember(member);

    return member;
}

ADocMember* ADocument::AddObject(const char* MemberName, ADocObject* Object)
{
    // Create member
    TRef<ADocMember> member = MakeRef<ADocMember>();
    member->NameBegin       = MemberName;
    member->NameEnd         = MemberName + Platform::Strlen(MemberName);

    member->AddValue(Object);

    AddMember(member);

    return member;
}

ADocMember* ADocument::AddArray(const char* ArrayName)
{
    TRef<ADocMember> array = MakeRef<ADocMember>();
    array->NameBegin       = ArrayName;
    array->NameEnd         = ArrayName + Platform::Strlen(ArrayName);

    AddMember(array);

    return array;
}

void ADocument::AddMember(ADocMember* Member)
{
    Member->AddRef();
    Member->Prev = MembersTail;
    if (Member->Prev != nullptr)
    {
        Member->Prev->Next = Member;
    }
    else
    {
        MembersHead = Member;
    }
    MembersTail = Member;
}

static bool NameCmp(const char* Begin, const char* End, const char* Str)
{
    const char* p = Begin;
    while (*Str && p < End)
    {
        if (*Str != *p)
        {
            return false;
        }
        Str++;
        p++;
    }
    return (*Str || p != End) ? false : true;
}

ADocMember* ADocument::FindMember(const char* Name)
{
    for (ADocMember* member = MembersHead; member; member = member->Next)
    {
        if (NameCmp(member->NameBegin, member->NameEnd, Name))
        {
            return member;
        }
    }
    return nullptr;
}

ADocMember const* ADocument::FindMember(const char* Name) const
{
    return const_cast<ADocument*>(this)->FindMember(Name);
}


ADocValue::~ADocValue()
{
    for (ADocMember* member = Members; member;)
    {
        ADocMember* next = member->Next;
        member->RemoveRef();
        member = next;
    }

    if (pTokenMemory)
    {
        Platform::GetHeapAllocator<HEAP_STRING>().Free(pTokenMemory);
    }
}

ADocMember* ADocValue::FindMember(const char* Name)
{
    for (ADocMember* member = GetListOfMembers(); member; member = member->GetNext())
    {
        if (NameCmp(member->NameBegin, member->NameEnd, Name))
        {
            return member;
        }
    }
    return nullptr;
}

ADocMember const* ADocValue::FindMember(const char* Name) const
{
    return const_cast<ADocValue*>(this)->FindMember(Name);
}

ADocMember* ADocValue::AddString(const char* MemberName, const char* Str)
{
    if (!IsObject())
    {
        LOG("ADocValue::AddString: called on non-object type\n");
        return nullptr;
    }

    // Create member
    TRef<ADocMember> member = MakeRef<ADocMember>();
    member->NameBegin       = MemberName;
    member->NameEnd         = MemberName + Platform::Strlen(MemberName);

    // Create string
    TRef<ADocString> value = MakeRef<ADocString>();

    // Allocate string
    value->SetString(Str);

    member->AddValue(value);

    AddMember(member);

    return member;
}

ADocMember* ADocValue::AddObject(const char* MemberName, ADocObject* Object)
{
    if (!IsObject())
    {
        LOG("ADocValue::AddObject: called on non-object type\n");
        return nullptr;
    }

    // Create member
    TRef<ADocMember> member = MakeRef<ADocMember>();
    member->NameBegin       = MemberName;
    member->NameEnd         = MemberName + Platform::Strlen(MemberName);

    member->AddValue(Object);

    AddMember(member);

    return member;
}

ADocMember* ADocValue::AddArray(const char* ArrayName)
{
    if (!IsObject())
    {
        LOG("ADocValue::AddArray: called on non-object type\n");
        return nullptr;
    }

    TRef<ADocMember> array = MakeRef<ADocMember>();
    array->NameBegin       = ArrayName;
    array->NameEnd         = ArrayName + Platform::Strlen(ArrayName);

    AddMember(array);

    return array;
}

void ADocValue::AddMember(ADocMember* Member)
{
    HK_ASSERT(Type == ADocValue::TYPE_OBJECT);

    Member->AddRef();

    Member->Prev = MembersEnd;
    if (Member->Prev != nullptr)
    {
        Member->Prev->Next = Member;
    }
    else
    {
        Members = Member;
    }
    MembersEnd = Member;
}

void ADocValue::Print() const
{
    LOG("Type: {}\n", IsString() ? "STRING" : "OBJECT");

    if (IsString())
    {
        LOG("{}\n", GetString());
        return;
    }

    for (ADocMember const* member = GetListOfMembers(); member; member = member->GetNext())
    {
        member->Print();
    }
}

AString ADocValue::SerializeToString(SDocumentSerializeInfo const& SerializeInfo) const
{
    ADocumentSerializer serializer;

    if (SerializeInfo.bCompactStringConversion)
    {
        return serializer.SerializeObjectCompact(GetListOfMembers());
    }

    bool bSingleMember;
    return serializer.SerializeObject(GetListOfMembers(), bSingleMember);
}


ADocMember::~ADocMember()
{
    for (ADocValue* value = Values; value;)
    {
        ADocValue* next = value->Next;
        value->RemoveRef();
        value = next;
    }
}

void ADocMember::AddValue(ADocValue* pValue)
{
    HK_ASSERT(pValue->Next == nullptr && pValue->Prev == nullptr);
    pValue->AddRef();
    pValue->Prev = ValuesEnd;
    if (pValue->Prev != nullptr)
    {
        pValue->Prev->Next = pValue;
    }
    else
    {
        Values = pValue;
    }
    ValuesEnd = pValue;
}

void ADocMember::Print() const
{
    LOG("Member: {}\n", GetName());

    for (ADocValue const* value = GetArrayValues(); value; value = value->GetNext())
    {
        value->Print();
    }
}
