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

#pragma once

#include "Containers/Hash.h"
#include "Containers/Vector.h"
#include "Thread.h"

HK_NAMESPACE_BEGIN

class StringId
{
public:
    StringId() = default;

    explicit StringId(StringView str) :
        m_Id(Pool::Instance().Insert(str))
    {}

    void Clear()
    {
        m_Id = 0;
    }

    bool IsEmpty() const
    {
        return m_Id == 0;
    }

    StringView GetStringView() const
    {
        return Pool::Instance().GetString(m_Id);
    }

    const char* GetRawString() const
    {
        return Pool::Instance().GetRawString(m_Id);
    }

    bool operator==(StringId const& rhs) const
    {
        return m_Id == rhs.m_Id;
    }

    bool operator<(StringId const& rhs) const
    {
        return m_Id < rhs.m_Id;
    }

    bool operator>(StringId const& rhs) const
    {
        return m_Id > rhs.m_Id;
    }

private:
    using ID = uint16_t;

    class Pool
    {
    public:
        static Pool& Instance();

        Pool();
        ID Insert(StringView str);
        StringView GetString(ID id);
        const char* GetRawString(ID id);

    private:
        TStringHashMap<ID> m_Storage;
        TVector<StringView> m_Strings;
        Mutex m_Mutex;
    };

    ID m_Id = 0;
};

HK_NAMESPACE_END

template <> struct fmt::formatter<Hk::StringId>
{
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }

    template <typename FormatContext> auto format(Hk::StringId const& v, FormatContext& ctx) -> decltype(ctx.out())
    {
        Hk::StringView view = v.Get();
        return fmt::detail::copy_str<char, const char*>(view.Begin(), view.End(), ctx.out());
    }
};
