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
#include "Containers/PagedVector.h"
#include "Thread.h"
#include "HashFunc.h"

HK_NAMESPACE_BEGIN

class StringID
{
public:
                    StringID() = default;
    explicit        StringID(StringView str);

    bool            IsEmpty() const;

    void            Clear();

    void            FromString(StringView str);

    StringView      GetStringView() const;

    const char*     GetRawString() const;

    bool            operator==(StringID const& rhs) const;

    bool            operator<(StringID const& rhs) const;

    bool            operator>(StringID const& rhs) const;

    uint32_t        Hash() const;

private:
    using ID = uint16_t;

    class Pool
    {
    public:
        static Pool&    Instance();

                        Pool();
        ID              Insert(StringView str);
        StringView      GetString(ID id);
        const char*     GetRawString(ID id);

    private:
        TStringHashMap<ID>                  m_Storage;
        TPagedVector<StringView, 1024, 64>  m_Strings;
        Mutex                               m_Mutex;
    };

    ID              m_Id = 0;
};

HK_FORCEINLINE StringID::StringID(StringView str) :
    m_Id(Pool::Instance().Insert(str))
{}

HK_FORCEINLINE bool StringID::IsEmpty() const
{
    return m_Id == 0;
}

HK_FORCEINLINE void StringID::Clear()
{
    m_Id = 0;
}

HK_FORCEINLINE void StringID::FromString(StringView str)
{
    m_Id = Pool::Instance().Insert(str);
}

HK_FORCEINLINE StringView StringID::GetStringView() const
{
    return Pool::Instance().GetString(m_Id);
}

HK_FORCEINLINE const char* StringID::GetRawString() const
{
    return Pool::Instance().GetRawString(m_Id);
}

HK_FORCEINLINE bool StringID::operator==(StringID const& rhs) const
{
    return m_Id == rhs.m_Id;
}

HK_FORCEINLINE bool StringID::operator<(StringID const& rhs) const
{
    return m_Id < rhs.m_Id;
}

HK_FORCEINLINE bool StringID::operator>(StringID const& rhs) const
{
    return m_Id > rhs.m_Id;
}

HK_FORCEINLINE uint32_t StringID::Hash() const
{
    return HashTraits::Hash(m_Id);
}

HK_FORCEINLINE StringView StringID::Pool::GetString(ID id)
{
    return m_Strings.Get(id);
}

HK_FORCEINLINE const char* StringID::Pool::GetRawString(ID id)
{
    return m_Strings.Get(id).ToPtr();
}

HK_NAMESPACE_END

template <> struct fmt::formatter<Hk::StringID>
{
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }

    template <typename FormatContext> auto format(Hk::StringID const& v, FormatContext& ctx) -> decltype(ctx.out())
    {
        Hk::StringView view = v.GetStringView();
        return fmt::detail::copy_str<char, const char*>(view.Begin(), view.End(), ctx.out());
    }
};
