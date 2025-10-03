/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#include "Format.h"

HK_NAMESPACE_BEGIN

void LOG(const char* message);

template <typename... T>
HK_FORCEINLINE void LOG(fmt::format_string<T...> format, T&&... args)
{
    fmt::memory_buffer buffer;
    fmt::detail::vformat_to(buffer, fmt::string_view(format), fmt::make_format_args(args...));
    buffer.push_back('\0');

    LOG(buffer.data());
}

template <typename... T>
HK_FORCEINLINE void CRITICAL(fmt::format_string<T...> format, T&&... args)
{
    fmt::memory_buffer buffer;
    fmt::detail::vformat_to(buffer, fmt::string_view(format), fmt::make_format_args(args...));
    buffer.push_back('\0');

    LOG(buffer.data());
}

template <typename... T>
HK_FORCEINLINE void ERROR(fmt::format_string<T...> format, T&&... args)
{
    fmt::memory_buffer buffer;
    fmt::detail::vformat_to(buffer, fmt::string_view(format), fmt::make_format_args(args...));
    buffer.push_back('\0');

    LOG(buffer.data());
}

template <typename... T>
HK_FORCEINLINE void WARNING(fmt::format_string<T...> format, T&&... args)
{
    fmt::memory_buffer buffer;
    fmt::detail::vformat_to(buffer, fmt::string_view(format), fmt::make_format_args(args...));
    buffer.push_back('\0');

    LOG(buffer.data());
}

template <typename... T>
HK_FORCEINLINE void DEBUG(fmt::format_string<T...> format, T&&... args)
{
#ifdef HK_DEBUG
    fmt::memory_buffer buffer;
    fmt::detail::vformat_to(buffer, fmt::string_view(format), fmt::make_format_args(args...));
    buffer.push_back('\0');

    LOG(buffer.data());
#endif
}

HK_NAMESPACE_END
