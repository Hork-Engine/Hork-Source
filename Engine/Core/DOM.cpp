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

#include "DOM.h"
#include "Logger.h"

HK_NAMESPACE_BEGIN

namespace DOM
{

Object::Object(StringView str) :
    m_String(str)
{}

Object::~Object()
{
    for (auto* m : m_Members)
        delete m;
    for (auto* object : m_Array)
        delete object;
}

Object Object::Copy() const
{
    return Object(*this);
}

void Object::CopyFrom(Object const& source)
{
    Clear();

    m_Members.Reserve(source.m_Members.Size());
    m_Array.Reserve(source.m_Array.Size());

    for (auto sourceMember : source.m_Members)
    {
        auto member = new Member(sourceMember->GetName());
        member->GetObject().CopyFrom(sourceMember->GetObject());
        m_Members.Add(member);
    }

    for (auto sourceObject : source.m_Array)
        m_Array.Add(new Object(*sourceObject));

    m_String.CopyFrom(source.m_String);
}

void Object::Clear()
{
    ClearStructure();
    ClearArray();
    ClearString();
}

bool Object::IsStructure() const
{
    return !m_Members.IsEmpty();
}

bool Object::IsArray() const
{
    return !m_Array.IsEmpty();
}

bool Object::IsString() const
{
    return !IsStructure() && !IsArray();
}

bool Object::HasMember(StringID memberName) const
{
    return !!Find(memberName);
}

Member* Object::Find(StringID memberName) const
{
    for (auto* m : m_Members)
        if (m->GetName() == memberName)
            return m;
    return {};
}

Object& Object::operator[](StringView memberName)
{
    return Insert(StringID(memberName));
}

Object& Object::Insert(StringID memberName)
{
    ClearString();
    ClearArray();

    auto* member = Find(memberName);

    if (!member)
    {
        member = new Member(memberName);
        m_Members.Add(member);
    }

    return member->GetObject();
}

Object& Object::Insert(StringID memberName, Object&& object)
{
    ClearString();
    ClearArray();

    auto* member = Find(memberName);

    if (!member)
    {
        member = new Member(memberName, std::forward<Object>(object));
        m_Members.Add(member);
    }

    return member->GetObject();
}

void Object::Remove(StringID memberName)
{
    for (auto it = m_Members.begin(); it != m_Members.end(); it++)
    {
        Member* m = *it;
        if (m->GetName() == memberName)
        {
            delete m;
            m_Members.Erase(it);
            break;
        }
    }
}

Object& Object::operator=(StringView str)
{
    ClearStructure();
    ClearArray();
    m_String = SmallString(str);
    return *this;
}

void Object::ClearStructure()
{
    for (auto* m : m_Members)
        delete m;
    m_Members.Clear();
}

void Object::ClearArray()
{
    for (auto* object : m_Array)
        delete object;
    m_Array.Clear();
}

void Object::ClearString()
{
    m_String.Clear();
}

Member::Member(StringID name) :
    m_Name(name)
{}

Member::Member(StringID name, Object&& object) :
    m_Name(name), m_Object(std::forward<Object>(object))
{}

void Writer::Write(StringView text)
{
    LOG("{}", text);
}

void Writer::OnBeginStructure(StringID name, Object const& dobject)
{
    Indent();
    Write(name.GetStringView());
    Write(" {\n");
}

void Writer::OnBeginStructure(Object const& dobject, int index)
{
    Indent();
    Write("{\n");
}

void Writer::OnEndStructure()
{
    Indent();
    Write("}\n");
}

void Writer::OnBeginArray(StringID name, Object const& dobject)
{
    Indent();
    Write(name.GetStringView());
    Write(" [\n");
}

void Writer::OnBeginArray(Object const& dobject, int index)
{
    Indent();
    Write("[\n");
}

void Writer::OnEndArray()
{
    Indent();
    Write("]\n");
}

void Writer::OnVisitString(StringID name, Object const& dobject)
{
    Indent();
    Write(name.GetStringView());
    Write(HK_FORMAT(" \"{}\"\n", dobject.AsString()));
}

void Writer::OnVisitString(Object const& dobject, int index)
{
    Indent();
    Write(HK_FORMAT("\"{}\"\n", dobject.AsString()));
}

void Writer::Indent()
{
    char buffer[32];
    int buffer_size = 0;

    for (int n = Stack(); n-- > 0;)
    {
        buffer[buffer_size++] = ' ';
        if (buffer_size == 32)
        {
            Write(StringView(buffer, buffer_size));
            buffer_size = 0;
        }
    }

    if (buffer_size > 0)
        Write(StringView(buffer, buffer_size));
}

void WriterCompact::Write(StringView text)
{
    LOG("{}", text);
}

void WriterCompact::OnBeginStructure(StringID name, Object const& dobject)
{
    Write(name.GetStringView());
    Write("{");
}

void WriterCompact::OnBeginStructure(Object const& dobject, int index)
{
    Write("{");
}

void WriterCompact::OnEndStructure()
{
    Write("}");
}

void WriterCompact::OnBeginArray(StringID name, Object const& dobject)
{
    Write(name.GetStringView());
    Write("[");
}

void WriterCompact::OnBeginArray(Object const& dobject, int index)
{
    Write("[");
}

void WriterCompact::OnEndArray()
{
    Write("]");
}

void WriterCompact::OnVisitString(StringID name, Object const& dobject)
{
    Write(name.GetStringView());
    Write(HK_FORMAT("\"{}\"", dobject.AsString()));
}

void WriterCompact::OnVisitString(Object const& dobject, int index)
{
    Write(HK_FORMAT("\"{}\"", dobject.AsString()));
}

Object Serialize(TR::TypeRegistry const& inTypeRegistry, void const* inObjectPtr, TR::TypeInfo const* inTypeInfo)
{
    if (!inTypeInfo)
    {
        return {};
    }

    auto& structure = inTypeInfo->Struct;

    // Array
    if (inTypeInfo->ArrayElementTypeId)
    {
        auto array_size = inTypeInfo->Array.GetArraySize(inObjectPtr);

        auto array_element_type = inTypeRegistry.FindType(inTypeInfo->ArrayElementTypeId);
        if (!array_element_type || array_size == 0)
            return {};

        Object dobject;
        dobject.PreallocateArray(array_size);
        for (size_t n = 0; n < array_size; n++)
        {
            void const* array_element = inTypeInfo->Array.GetArrayAt(n, const_cast<void*>(inObjectPtr));

            dobject.Add(Serialize(inTypeRegistry, array_element, array_element_type));
        }
        return dobject;
    }

    // Structure
    if (structure)
    {
        Object dobject;
        for (auto& member : structure->GetMembers())
        {
            auto member_type = inTypeRegistry.FindType(member->GetTypeId());
            if (!member_type)
                continue;

            void const* member_ptr = member->DereferencePtr(const_cast<void*>(inObjectPtr));

            dobject.Insert(member->GetName(), Serialize(inTypeRegistry, member_ptr, member_type));
        }

        return dobject;
    }

    // Trivial
    return Object(inTypeInfo->Value.ToString(inObjectPtr));
}

void Deserialize(Object const& dobject, TR::TypeRegistry const& inTypeRegistry, void* inObjectPtr, TR::TypeInfo const* inTypeInfo)
{
    if (!inTypeInfo)
    {
        return;
    }

    auto& structure = inTypeInfo->Struct;

    // Array
    if (inTypeInfo->ArrayElementTypeId)
    {
        auto array_size = dobject.GetArraySize();
        bool resizable = inTypeInfo->Array.TryResize(array_size, inObjectPtr);
        if (!resizable)
            array_size = inTypeInfo->Array.GetArraySize(inObjectPtr);

        auto array_element_type = inTypeRegistry.FindType(inTypeInfo->ArrayElementTypeId);
        if (!array_element_type)
            return;

        size_t read_array_size = std::min(array_size, dobject.GetArraySize());
        for (size_t n = 0; n < read_array_size; n++)
        {
            void* array_element = inTypeInfo->Array.GetArrayAt(n, inObjectPtr);
            Deserialize(dobject.At(n), inTypeRegistry, array_element, array_element_type);
        }
        return;
    }

    // Structure
    if (structure)
    {
        for (auto& member : structure->GetMembers())
        {
            auto dmember = dobject.Find(member->GetName());
            if (!dmember)
                continue;

            auto member_type = inTypeRegistry.FindType(member->GetTypeId());
            if (!member_type)
                continue;

            void* member_ptr = member->DereferencePtr(inObjectPtr);

            Deserialize(dmember->GetObject(), inTypeRegistry, member_ptr, member_type);
        }
        return;
    }

    // Trivial
    if (dobject.IsString())
        inTypeInfo->Value.FromString(inObjectPtr, dobject.AsString());
}

Parser::Tokenizer::Tokenizer()
{
    m_Cur = "";
    //m_LineNumber = 1;
}

void Parser::Tokenizer::Reset(const char* pDocumentData)
{
    m_Cur = pDocumentData;
    //m_LineNumber = 1;

    // Go to first token
    NextToken();
}

void Parser::Tokenizer::SkipWhitespaces()
{
start:
    while (*m_Cur == ' ' || *m_Cur == '\t' || *m_Cur == '\n' || *m_Cur == '\r')
    {
        if (*m_Cur == '\n')
        {
            //m_LineNumber++;
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
                    //m_LineNumber++;
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

void Parser::Tokenizer::NextToken()
{
    // Skip white spaces, tabs and comments
    SkipWhitespaces();

    // Check string
    if (*m_Cur == '\"')
    {
        m_Cur++;
        m_Token.Begin = m_Cur;
        for (;;)
        {
            if (*m_Cur == '\"' && *(m_Cur - 1) != '\\')
            {
                break;
            }

            if (*m_Cur == 0)
            {
                // unexpected eof
                m_Token.Begin = m_Token.End = "";
                m_Token.Type = TOKEN_UNKNOWN;
                return;
            }
            if (*m_Cur == '\n')
            {
                // unexpected eol
                m_Token.Begin = m_Token.End = "";
                m_Token.Type = TOKEN_UNKNOWN;
                return;
            }
            m_Cur++;
        }
        m_Token.End = m_Cur++;
        m_Token.Type = TOKEN_STRING;
        return;
    }

    // Check brackets
    if (*m_Cur == '{' || *m_Cur == '}' || *m_Cur == '[' || *m_Cur == ']')
    {
        m_Token.Begin = m_Cur;
        m_Token.End = ++m_Cur;
        m_Token.Type = TOKEN_BRACKET;
        return;
    }

    // Check member
    m_Token.Begin = m_Cur;
    while ((*m_Cur >= 'a' && *m_Cur <= 'z') || (*m_Cur >= 'A' && *m_Cur <= 'Z') || (*m_Cur >= '0' && *m_Cur <= '9') || *m_Cur == '_' || *m_Cur == '.' || *m_Cur == '$')
    {
        m_Cur++;
    }
    m_Token.End = m_Cur;
    if (m_Token.Begin == m_Token.End)
    {
        if (*m_Cur)
        {
            LOG("undefined symbols in token\n");
            m_Token.Type = TOKEN_UNKNOWN;
        }
        else
        {
            m_Token.Type = TOKEN_EOF;
        }
        return;
    }
    else
    {
        m_Token.Type = TOKEN_MEMBER;
    }
}

Object Parser::Parse(const char* inStr)
{
    m_Tokenizer.Reset(inStr);

    Token token = m_Tokenizer.GetToken();

    switch (token.Type)
    {
        case TOKEN_BRACKET:
            if (*token.Begin == '{')
            {
                m_Tokenizer.NextToken();
                return ParseStructure(true);
            }
            if (*token.Begin == '[')
            {
                m_Tokenizer.NextToken();
                return ParseArray();
            }
            LOG("unexpected token {}\n", *token.Begin);
            return {};
        case TOKEN_STRING:
            return Object(StringView(token.Begin, token.End));
        case TOKEN_MEMBER:
            return ParseStructure(false);
        case TOKEN_EOF:
            return {};
        default:
            LOG("unexpected token {}\n", StringView(token.Begin, token.End));
            break;
    }

    return {};
}

Object Parser::Parse(String const& inStr)
{
    return Parse(inStr.CStr());
}

Object Parser::ParseStructure(bool bExpectClosedBracket)
{
    Object dobject;
    while (1)
    {
        Token token = m_Tokenizer.GetToken();

        if (token.Type == TOKEN_BRACKET)
        {
            if (bExpectClosedBracket && *token.Begin == '}')
            {
                m_Tokenizer.NextToken();
                break;
            }

            LOG("unexpected token {}\n", *token.Begin);
            break;
        }

        if (token.Type == TOKEN_EOF)
        {
            if (!bExpectClosedBracket)
                break;

            LOG("unexpected EOF\n");
            break;
        }

        if (token.Type != TOKEN_MEMBER)
        {
            LOG("unexpected token {}\n", StringView(token.Begin, token.End));
            break;
        }

        StringView memberName(token.Begin, token.End);

        m_Tokenizer.NextToken();

        token = m_Tokenizer.GetToken();

        if (*token.Begin == '{')
        {
            m_Tokenizer.NextToken();

            Object child = ParseStructure(true);
            dobject.Insert(StringID(memberName), std::move(child));
        }
        else if (*token.Begin == '[')
        {
            m_Tokenizer.NextToken();

            Object child = ParseArray();
            dobject.Insert(StringID(memberName), std::move(child));
        }
        else if (token.Type == TOKEN_STRING)
        {
            dobject.Insert(StringID(memberName), Object(StringView(token.Begin, token.End)));
            m_Tokenizer.NextToken();
        }
        else if (token.Type == TOKEN_EOF)
        {
            LOG("unexpected EOF\n");
            break;
        }
        else
        {
            LOG("unexpected token {}\n", StringView(token.Begin, token.End));
            break;
        }
    }
    return dobject;
}

Object Parser::ParseArray()
{
    Object dobject;
    while (1)
    {
        Token token = m_Tokenizer.GetToken();

        if (*token.Begin == ']')
        {
            m_Tokenizer.NextToken();
            break;
        }

        if (*token.Begin == '{')
        {
            m_Tokenizer.NextToken();

            Object child = ParseStructure(true);
            dobject.Add(std::move(child));
        }
        else if (*token.Begin == '[')
        {
            m_Tokenizer.NextToken();

            Object child = ParseArray();
            dobject.Add(std::move(child));
        }
        else if (token.Type == TOKEN_STRING)
        {
            dobject.Add(StringView(token.Begin, token.End));
            m_Tokenizer.NextToken();
        }
        else if (token.Type == TOKEN_EOF)
        {
            LOG("unexpected EOF\n");
            break;
        }
        else
        {
            LOG("unexpected token {}\n", StringView(token.Begin, token.End));
            break;
        }
    }

    return dobject;
}

} // namespace DOM

HK_NAMESPACE_END
