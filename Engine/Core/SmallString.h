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

#include "String.h"

HK_NAMESPACE_BEGIN

// SmallString is immutable string with small string optimization.
// The structure weighs only 16 bytes. For strings no longer than 14 characters, no memory allocation is required.
// Can be used also for storing large immutable strings.

// BIG ENDIAN
class SmallStringBE final
{
    union
    {
        struct
        {
            uint32_t Size;
            uint32_t Capacity; // Not used now
            char* Str;
        } m_Data;

        struct
        {
            uint8_t Size;
            char Str[sizeof(m_Data) - 1];
        } m_SSO;
    };

    static constexpr uint32_t SSO_CAPACITY = sizeof(m_Data) - 2;

public:
    using SizeType = uint32_t;

    SmallStringBE()
    {
        m_SSO.Size = 1 << 7; // SSO bit
        m_SSO.Str[0] = 0;
    }

    explicit SmallStringBE(StringView Rhs)
    {
        Construct(Rhs.ToPtr(), Rhs.Size());
    }

    //    SmallStringBE(SmallStringBE const& Rhs)
    //    {
    //        Construct(Rhs.GetRawString(), Rhs.Size());
    //    }

    //    SmallStringBE& operator=(SmallStringBE const& Rhs)
    //    {
    //        Free();
    //        Construct(Rhs.GetRawString(), Rhs.Size());
    //        return *this;
    //    }

    SmallStringBE(SmallStringBE const& Rhs) = delete;
    SmallStringBE& operator=(SmallStringBE const& Rhs) = delete;

    void CopyFrom(SmallStringBE const& Rhs)
    {
        Free();
        Construct(Rhs.GetRawString(), Rhs.Size());
    }

    SmallStringBE(SmallStringBE&& Rhs) noexcept
    {
        Core::Memcpy(&m_Data, &Rhs.m_Data, sizeof(m_Data));

        Rhs.m_SSO.Size = 1 << 7;
        Rhs.m_SSO.Str[0] = 0;
    }
    SmallStringBE& operator=(SmallStringBE&& Rhs) noexcept
    {
        Free();

        Core::Memcpy(&m_Data, &Rhs.m_Data, sizeof(m_Data));

        Rhs.m_SSO.Size = 1 << 7;
        Rhs.m_SSO.Str[0] = 0;
        return *this;
    }

    ~SmallStringBE()
    {
        Free();
    }

    void Clear()
    {
        Free();
        m_SSO.Size = 1 << 7;
        m_SSO.Str[0] = 0;
    }

    SizeType Size() const
    {
        if (m_SSO.Size & (1 << 7))
        {
            return m_SSO.Size & ~(1 << 7);
        }
        return m_Data.Size;
    }

    const char* GetRawString() const
    {
        if (m_SSO.Size & (1 << 7))
        {
            return m_SSO.Str;
        }
        return m_Data.Str;
    }

    operator StringView() const
    {
        if (m_SSO.Size & (1 << 7))
        {
            return StringView(m_SSO.Str, m_SSO.Size & ~(1 << 7) /*, true*/);
        }
        return StringView(m_Data.Str, m_Data.Size /*, true*/);
    }

private:
    void Construct(const char* pRawString, SizeType Size)
    {
        if (Size <= SSO_CAPACITY)
        {
            m_SSO.Size = Size;
            Core::Memcpy(m_SSO.Str, pRawString, m_SSO.Size);
            m_SSO.Str[m_SSO.Size] = 0;
            m_SSO.Size |= 1 << 7;
        }
        else
        {
            m_Data.Size = Size;

            m_Data.Str = (char*)Core::GetHeapAllocator<HEAP_STRING>().Alloc(m_Data.Size + 1, 0);
            Core::Memcpy(m_Data.Str, pRawString, m_Data.Size);
            m_Data.Str[m_Data.Size] = 0;
        }
    }

    void Free()
    {
        if (m_SSO.Size & (1 << 7))
            return;
        Core::GetHeapAllocator<HEAP_STRING>().Free(m_Data.Str);
    }
};

// LITTLE ENDIAN
class SmallStringLE final
{
    union
    {
        struct
        {
            uint32_t Size;
            uint32_t Capacity; // Not used now
            char* Str;
        } m_Data;

        struct
        {
            uint8_t Size;
            char Str[sizeof(m_Data) - 1];
        } m_SSO;
    };

    static constexpr uint32_t SSO_CAPACITY = sizeof(m_Data) - 2;

public:
    using SizeType = uint32_t;

    SmallStringLE()
    {
        m_SSO.Size = 1; // SSO bit
        m_SSO.Str[0] = 0;
    }

    explicit SmallStringLE(StringView Rhs)
    {
        Construct(Rhs.ToPtr(), Rhs.Size());
    }

    //    SmallStringLE(SmallStringLE const& Rhs)
    //    {
    //        Construct(Rhs.GetRawString(), Rhs.Size());
    //    }

    //    SmallStringLE& operator=(SmallStringLE const& Rhs)
    //    {
    //        Free();
    //        Construct(Rhs.GetRawString(), Rhs.Size());
    //        return *this;
    //    }

    SmallStringLE(SmallStringLE const& Rhs) = delete;
    SmallStringLE& operator=(SmallStringLE const& Rhs) = delete;

    void CopyFrom(SmallStringLE const& Rhs)
    {
        Free();
        Construct(Rhs.GetRawString(), Rhs.Size());
    }

    SmallStringLE(SmallStringLE&& Rhs) noexcept
    {
        Core::Memcpy(&m_Data, &Rhs.m_Data, sizeof(m_Data));

        Rhs.m_SSO.Size = 1;
        Rhs.m_SSO.Str[0] = 0;
    }
    SmallStringLE& operator=(SmallStringLE&& Rhs) noexcept
    {
        Free();

        Core::Memcpy(&m_Data, &Rhs.m_Data, sizeof(m_Data));

        Rhs.m_SSO.Size = 1;
        Rhs.m_SSO.Str[0] = 0;
        return *this;
    }

    ~SmallStringLE()
    {
        Free();
    }

    void Clear()
    {
        Free();
        m_SSO.Size = 1;
        m_SSO.Str[0] = 0;
    }

    SizeType Size() const
    {
        if (m_SSO.Size & 1)
        {
            return m_SSO.Size >> 1;
        }
        return m_Data.Size >> 1;
    }

    const char* GetRawString() const
    {
        if (m_SSO.Size & 1)
        {
            return m_SSO.Str;
        }
        return m_Data.Str;
    }

    operator StringView() const
    {
        if (m_SSO.Size & 1)
        {
            return StringView(m_SSO.Str, m_SSO.Size >> 1, true);
        }
        return StringView(m_Data.Str, m_Data.Size >> 1, true);
    }

private:
    void Construct(const char* pRawString, SizeType Size)
    {
        if (Size <= SSO_CAPACITY)
        {
            m_SSO.Size = Size;
            Core::Memcpy(m_SSO.Str, pRawString, m_SSO.Size);
            m_SSO.Str[m_SSO.Size] = 0;
            m_SSO.Size = (m_SSO.Size << 1) | 1;
        }
        else
        {
            m_Data.Size = Size;
            m_Data.Str = (char*)Core::GetHeapAllocator<HEAP_STRING>().Alloc(m_Data.Size + 1, 0);
            Core::Memcpy(m_Data.Str, pRawString, m_Data.Size);
            m_Data.Str[m_Data.Size] = 0;
            m_Data.Size <<= 1;
        }
    }

    void Free()
    {
        if (m_SSO.Size & 1)
            return;
        Core::GetHeapAllocator<HEAP_STRING>().Free(m_Data.Str);
    }
};

#ifdef HK_LITTLE_ENDIAN
using SmallString = SmallStringLE;
#else
using SmallString = SmallStringBE;
#endif

HK_NAMESPACE_END

template <> struct fmt::formatter<Hk::SmallString>
{
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }

    template <typename FormatContext> auto format(Hk::SmallString const& v, FormatContext& ctx) -> decltype(ctx.out())
    {
        Hk::StringView view = v.GetStringView();
        return fmt::detail::copy_str<char, const char*>(view.Begin(), view.End(), ctx.out());
    }
};
