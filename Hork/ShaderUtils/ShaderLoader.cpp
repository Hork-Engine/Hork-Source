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

#include "ShaderLoader.h"

#include <Hork/Core/ConsoleVar.h>
#include <Hork/GameApplication/GameApplication.h>

HK_NAMESPACE_BEGIN

ConsoleVar r_EmbeddedShaders("r_EmbeddedShaders"_s, "1"_s);

#define CSTYLE_LINE_DIRECTIVE

namespace
{
    // Based on stb_include.h
    struct IncludeInfo
    {
        int           Offset;
        int           End;
        const char*   FileName;
        int           Length;
        int           NextLineAfter;
        IncludeInfo*  pNext;
    };

    void AddInclude(IncludeInfo*& list, IncludeInfo*& prev, int offset, int end, const char* filename, int len, int nextLine)
    {
        IncludeInfo* info  = (IncludeInfo*)Core::GetHeapAllocator<HEAP_MISC>().Alloc(sizeof(IncludeInfo));
        info->Offset        = offset;
        info->End           = end;
        info->FileName      = filename;
        info->Length        = len;
        info->NextLineAfter = nextLine;
        info->pNext         = nullptr;
        if (prev)
        {
            prev->pNext = info;
            prev        = info;
        }
        else
        {
            list = prev = info;
        }
    }

    void FreeIncludes(IncludeInfo* list)
    {
        IncludeInfo* next;
        for (IncludeInfo* includeInfo = list; includeInfo; includeInfo = next)
        {
            next = includeInfo->pNext;
            Core::GetHeapAllocator<HEAP_MISC>().Free(includeInfo);
        }
    }

    HK_FORCEINLINE int IsSpace(int ch)
    {
        return (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');
    }

    // find location of all #include
    IncludeInfo* FindIncludes(StringView text)
    {
        int lineCount = 1;
        const char* s = text.Begin(), *start;
        const char* e = text.End();
        IncludeInfo* list = nullptr;
        IncludeInfo* prev = nullptr;
        while (s < e)
        {
            // parse is always at start of line when we reach here
            start = s;
            while ((*s == ' ' || *s == '\t') && s < e)
                ++s;
            if (s < e && *s == '#')
            {
                ++s;
                while ((*s == ' ' || *s == '\t') && s < e)
                    ++s;
                if (e - s > 7 && !Core::StrcmpN(s, "include", 7) && IsSpace(s[7]))
                {
                    s += 7;
                    while ((*s == ' ' || *s == '\t') && s < e)
                        ++s;
                    if (*s == '"' && s < e)
                    {
                        const char* t = ++s;
                        while (*t != '"' && *t != '\n' && *t != '\r' && t < e)
                            ++t;
                        if (*t == '"' && t < e)
                        {
                            int         len      = t - s;
                            const char* filename = s;
                            s                    = t;
                            while (*s != '\r' && *s != '\n' && s < e)
                                ++s;
                            // s points to the newline, so s-start is everything except the newline
                            AddInclude(list, prev, start - text.Begin(), s - text.Begin(), filename, len, lineCount + 1);
                        }
                    }
                }
            }
            while (*s != '\r' && *s != '\n' && s < e)
                ++s;
            if ((*s == '\r' || *s == '\n') && s < e)
                s = s + (s[0] + s[1] == '\r' + '\n' ? 2 : 1);
            ++lineCount;
        }
        return list;
    }

    void CleanComments(char* s)
    {
    start:
        while (*s)
        {
            if (*s == '/')
            {
                if (*(s + 1) == '/')
                {
                    *s++ = ' ';
                    *s++ = ' ';
                    while (*s && *s != '\n')
                        *s++ = ' ';
                    continue;
                }
                if (*(s + 1) == '*')
                {
                    *s++ = ' ';
                    *s++ = ' ';
                    while (*s)
                    {
                        if (*s == '*' && *(s + 1) == '/')
                        {
                            *s++ = ' ';
                            *s++ = ' ';
                            goto start;
                        }
                        if (*s != '\n')
                            *s++ = ' ';
                        else
                            s++;
                    }
                    // end of file inside comment
                    return;
                }
            }
            s++;
        }
    }
}

class ShaderLoader
{
public:
    String  LoadShader(StringView fileName, ArrayView<CodeBlock> codeBlocks = {});
    String  LoadShaderFromString(StringView fileName, StringView source, ArrayView<CodeBlock> codeBlocks = {});

protected:
    bool    LoadFile(StringView fileName, String& source);
    bool    LoadShaderFromString(StringView fileName, StringView source, String& out);
    bool    LoadShaderWithInclude(StringView fileName, String& out);

    ArrayView<CodeBlock> m_CodeBlocks;
};

String ShaderLoader::LoadShader(StringView fileName, ArrayView<CodeBlock> codeBlocks)
{
    m_CodeBlocks = codeBlocks;

#ifdef CSTYLE_LINE_DIRECTIVE
    String result(HK_FORMAT("#line 1 \"{}\"\n", fileName));
#else
    String result("#line 1\n");
#endif

    if (!LoadShaderWithInclude(fileName, result))
        CoreApplication::sTerminateWithError("LoadShader: failed to open {}\n", fileName);

    return result;
}

String ShaderLoader::LoadShaderFromString(StringView fileName, StringView source, ArrayView<CodeBlock> codeBlocks)
{
    m_CodeBlocks = codeBlocks;

#ifdef CSTYLE_LINE_DIRECTIVE
    String result(HK_FORMAT("#line 1 \"{}\"\n", fileName));
#else
    String result("#line 1\n");
#endif

    String _source(source);

    CleanComments(_source.ToPtr());

    if (!LoadShaderFromString(fileName, _source, result))
        CoreApplication::sTerminateWithError("LoadShader: failed to open {}\n", fileName);

    return result;
}

bool ShaderLoader::LoadFile(StringView fileName, String& source)
{
    File f;
    if (r_EmbeddedShaders)
    {
        f = File::sOpenRead("Shaders/" + fileName, GameApplication::sGetEmbeddedArchive());
    }
    else
    {
        // Load shaders from sources
        String fn(PathUtils::sGetFilePath(__FILE__));
        fn += "/../Embedded/Shaders/";
        fn += fileName;
        PathUtils::sFixPathInplace(fn);

        f = File::sOpenRead(fn);
    }
    if (!f)
        return false;
    source = f.AsString();
    return true;
}

bool ShaderLoader::LoadShaderFromString(StringView fileName, StringView source, String& out)
{
    IncludeInfo* includeList  = FindIncludes(source);
    size_t       sourceOffset = 0;

    for (IncludeInfo* includeInfo = includeList; includeInfo; includeInfo = includeInfo->pNext)
    {
        out += StringView(&source[sourceOffset], includeInfo->Offset - sourceOffset);

        if (!m_CodeBlocks.IsEmpty() && includeInfo->FileName[0] == '$')
        {
#ifdef CSTYLE_LINE_DIRECTIVE
            String result(HK_FORMAT("#line 1 \"{}\"\n", StringView(includeInfo->FileName, includeInfo->Length)));
#else
            String result("#line 1\n");
#endif
            StringView includeFn = StringView(includeInfo->FileName, includeInfo->Length);

            CodeBlock const* src = nullptr;
            for (CodeBlock const& s : m_CodeBlocks)
            {
                if (!s.Name.Icmp(includeFn))
                {
                    src = &s;
                    break;
                }
            }

            if (!src || !LoadShaderFromString(fileName, src->Code, out))
            {
                FreeIncludes(includeList);
                return false;
            }
        }
        else
        {
#ifdef CSTYLE_LINE_DIRECTIVE
            String result(HK_FORMAT("#line 1 \"{}\"\n", StringView(includeInfo->FileName, includeInfo->Length)));
#else
            String result("#line 1\n");
#endif
            if (!LoadShaderWithInclude(StringView(includeInfo->FileName, includeInfo->Length), out))
            {
                FreeIncludes(includeList);
                return false;
            }
        }

#ifdef CSTYLE_LINE_DIRECTIVE
        out += HK_FORMAT("\n#line {} \"{}\"", includeInfo->NextLineAfter, fileName);
#else
        out += HK_FORMAT("\n#line {}", includeInfo->NextLineAfter);
#endif

        sourceOffset = includeInfo->End;
    }

    if (!source.IsEmpty())
        out += StringView(&source[sourceOffset], source.Length() - sourceOffset);
    FreeIncludes(includeList);

    return true;
}

bool ShaderLoader::LoadShaderWithInclude(StringView fileName, String& out)
{
    String source;

    if (!LoadFile(fileName, source))
    {
        LOG("Couldn't load {}\n", fileName);
        return false;
    }

    CleanComments(source.ToPtr());

    return LoadShaderFromString(fileName, source, out);
}

String LoadShader(StringView fileName, ArrayView<CodeBlock> codeBlocks)
{
    return ShaderLoader{}.LoadShader(fileName, codeBlocks);
}

HK_NAMESPACE_END
