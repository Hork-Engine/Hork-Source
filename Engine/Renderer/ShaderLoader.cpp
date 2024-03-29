/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include <Engine/Core/ConsoleVar.h>
#include <Engine/Runtime/EmbeddedResources.h>

HK_NAMESPACE_BEGIN

ConsoleVar r_EmbeddedShaders("r_EmbeddedShaders"s, "0"s);

using namespace RenderCore;

//#define CSTYLE_LINE_DIRECTIVE  // NOTE: Supported by NVidia, not supported by AMD.

// Modified version of stb_include.h v0.02 originally written by Sean Barrett and Michal Klos

struct IncludeInfo
{
    int           Offset;
    int           End;
    const char*   FileName;
    int           Length;
    int           NextLineAfter;
    IncludeInfo* pNext;
};

static void AddInclude(IncludeInfo*& list, IncludeInfo*& prev, int offset, int end, const char* filename, int len, int next_line)
{
    IncludeInfo* z  = (IncludeInfo*)Platform::GetHeapAllocator<HEAP_MISC>().Alloc(sizeof(IncludeInfo));
    z->Offset        = offset;
    z->End           = end;
    z->FileName      = filename;
    z->Length        = len;
    z->NextLineAfter = next_line;
    z->pNext         = nullptr;
    if (prev)
    {
        prev->pNext = z;
        prev        = z;
    }
    else
    {
        list = prev = z;
    }
}

static void FreeIncludes(IncludeInfo* list)
{
    IncludeInfo* next;
    for (IncludeInfo* includeInfo = list; includeInfo; includeInfo = next)
    {
        next = includeInfo->pNext;
        Platform::GetHeapAllocator<HEAP_MISC>().Free(includeInfo);
    }
}

static HK_FORCEINLINE int IsSpace(int ch)
{
    return (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');
}

// find location of all #include
static IncludeInfo* FindIncludes(StringView text)
{
    int           line_count = 1;
    const char *  s          = text.Begin(), *start;
    const char*   e          = text.End();
    IncludeInfo *list = nullptr, *prev = nullptr;
    while (s < e)
    {
        // parse is always at start of line when we reach here
        start = s;
        while ((*s == ' ' || *s == '\t') && s < e)
        {
            ++s;
        }
        if (s < e && *s == '#')
        {
            ++s;
            while ((*s == ' ' || *s == '\t') && s < e)
                ++s;
            if (e - s > 7 && !Platform::StrcmpN(s, "include", 7) && IsSpace(s[7]))
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
                        AddInclude(list, prev, start - text.Begin(), s - text.Begin(), filename, len, line_count + 1);
                    }
                }
            }
        }
        while (*s != '\r' && *s != '\n' && s < e)
            ++s;
        if ((*s == '\r' || *s == '\n') && s < e)
        {
            s = s + (s[0] + s[1] == '\r' + '\n' ? 2 : 1);
        }
        ++line_count;
    }
    return list;
}

static void CleanComments(char* s)
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
                    {
                        *s++ = ' ';
                    }
                    else
                    {
                        s++;
                    }
                }
                // end of file inside comment
                return;
            }
        }
        s++;
    }
}

String ShaderLoader::LoadShader(StringView FileName, TArrayView<MaterialSource> Predefined)
{
    m_Predefined = Predefined;

    String result;
#ifdef CSTYLE_LINE_DIRECTIVE
    result += "#line 1 \"";
    result += FileName;
    result += "\"\n";
#else
    result += "#line 1\n";
#endif

    if (!LoadShaderWithInclude(FileName, result))
    {
        CriticalError("LoadShader: failed to open {}\n", FileName);
    }

    return result;
}

String ShaderLoader::LoadShaderFromString(StringView FileName, StringView Source, TArrayView<MaterialSource> Predefined)
{
    m_Predefined = Predefined;

    String result;
#ifdef CSTYLE_LINE_DIRECTIVE
    result += "#line 1 \"";
    result += FileName;
    result += "\"\n";
#else
    result += "#line 1\n";
#endif

    String source = Source;

    CleanComments(source.ToPtr());

    if (!LoadShaderFromString(FileName, source, result))
    {
        CriticalError("LoadShader: failed to open {}\n", FileName);
    }

    return result;
}

bool ShaderLoader::LoadFile(StringView FileName, String& Source)
{
    File f;
    if (r_EmbeddedShaders)
    {
        f = File::OpenRead("Shaders/" + FileName, Runtime::GetEmbeddedResources());
    }
    else
    {
        // Load shaders from sources
        String fn = PathUtils::GetFilePath(__FILE__);
        fn += "/../Embedded/Shaders/";
        fn += FileName;
        PathUtils::FixPathInplace(fn);

        f = File::OpenRead(fn);
    }
    if (!f)
        return false;
    Source = f.AsString();
    return true;
}

bool ShaderLoader::LoadShaderFromString(StringView FileName, StringView Source, String& Out)
{
    char          temp[4096];
    IncludeInfo* includeList  = FindIncludes(Source);
    size_t        sourceOffset = 0;

    for (IncludeInfo* includeInfo = includeList; includeInfo; includeInfo = includeInfo->pNext)
    {
        Out += StringView(&Source[sourceOffset], includeInfo->Offset - sourceOffset);

        if (!m_Predefined.IsEmpty() && includeInfo->FileName[0] == '$')
        {
            // predefined source
#ifdef CSTYLE_LINE_DIRECTIVE
            Out += "#line 1 \"";
            Out += StringView(includeInfo->FileName, includeInfo->Length);
            Out += "\"\n";
#else
            Out += "#line 1\n";
#endif
            StringView includeFn = StringView(includeInfo->FileName, includeInfo->Length);

            MaterialSource const* src = nullptr;
            for (MaterialSource const& s : m_Predefined)
            {
                if (!s.SourceName.Icmp(includeFn))
                {
                    src = &s;
                    break;
                }
            }

            if (!src || !LoadShaderFromString(FileName, src->Code, Out))
            {
                FreeIncludes(includeList);
                return false;
            }
        }
        else
        {
#ifdef CSTYLE_LINE_DIRECTIVE
            Out += "#line 1 \"";
            Out += StringView(includeInfo->FileName, includeInfo->Length);
            Out += "\"\n";
#else
            Out += "#line 1\n";
#endif
            temp[0] = 0;
            Platform::StrcatN(temp, sizeof(temp), includeInfo->FileName, includeInfo->Length);
            if (!LoadShaderWithInclude(temp, Out))
            {
                FreeIncludes(includeList);
                return false;
            }
        }

#ifdef CSTYLE_LINE_DIRECTIVE
        Platform::Sprintf(temp, sizeof(temp), "\n#line %d \"%s\"", includeInfo->NextLineAfter, FileName.ToString().CStr());
#else
        Platform::Sprintf(temp, sizeof(temp), "\n#line %d", includeInfo->NextLineAfter);
#endif
        Out += temp;

        sourceOffset = includeInfo->End;
    }

    if (!Source.IsEmpty())
        Out += StringView(&Source[sourceOffset], Source.Length() - sourceOffset);
    FreeIncludes(includeList);

    return true;
}

bool ShaderLoader::LoadShaderWithInclude(StringView FileName, String& Out)
{
    String source;

    if (!LoadFile(FileName, source))
    {
        LOG("Couldn't load {}\n", FileName);
        return false;
    }

    CleanComments(source.ToPtr());

    return LoadShaderFromString(FileName, source, Out);
}

HK_NAMESPACE_END
