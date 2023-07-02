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

#include "BaseTypes.h"

//#define FMT_HEADER_ONLY
//#include <fmt/format.h>
#include <fmt/core.h>

#define _HK_FORMAT_GET_ARG1(_1)                                                                     v._1
#define _HK_FORMAT_GET_ARG2(_1, _2)                                                                 v._1, v._2
#define _HK_FORMAT_GET_ARG3(_1, _2, _3)                                                             v._1, v._2, v._3
#define _HK_FORMAT_GET_ARG4(_1, _2, _3, _4)                                                         v._1, v._2, v._3, v._4
#define _HK_FORMAT_GET_ARG5(_1, _2, _3, _4, _5)                                                     v._1, v._2, v._3, v._4, v._5
#define _HK_FORMAT_GET_ARG6(_1, _2, _3, _4, _5, _6)                                                 v._1, v._2, v._3, v._4, v._5, v._6
#define _HK_FORMAT_GET_ARG7(_1, _2, _3, _4, _5, _6, _7)                                             v._1, v._2, v._3, v._4, v._5, v._6, v._7
#define _HK_FORMAT_GET_ARG8(_1, _2, _3, _4, _5, _6, _7, _8)                                         v._1, v._2, v._3, v._4, v._5, v._6, v._7, v._8
#define _HK_FORMAT_GET_ARG9(_1, _2, _3, _4, _5, _6, _7, _8, _9)                                     v._1, v._2, v._3, v._4, v._5, v._6, v._7, v._8, v._9
#define _HK_FORMAT_GET_ARG10(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10)                               v._1, v._2, v._3, v._4, v._5, v._6, v._7, v._8, v._9, v._10
#define _HK_FORMAT_GET_ARG11(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11)                          v._1, v._2, v._3, v._4, v._5, v._6, v._7, v._8, v._9, v._10, v._11
#define _HK_FORMAT_GET_ARG12(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12)                     v._1, v._2, v._3, v._4, v._5, v._6, v._7, v._8, v._9, v._10, v._11, v._12
#define _HK_FORMAT_GET_ARG13(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13)                v._1, v._2, v._3, v._4, v._5, v._6, v._7, v._8, v._9, v._10, v._11, v._12, v._13
#define _HK_FORMAT_GET_ARG14(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14)           v._1, v._2, v._3, v._4, v._5, v._6, v._7, v._8, v._9, v._10, v._11, v._12, v._13, v._14
#define _HK_FORMAT_GET_ARG15(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15)      v._1, v._2, v._3, v._4, v._5, v._6, v._7, v._8, v._9, v._10, v._11, v._12, v._13, v._14, v._15
#define _HK_FORMAT_GET_ARG16(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16) v._1, v._2, v._3, v._4, v._5, v._6, v._7, v._8, v._9, v._10, v._11, v._12, v._13, v._14, v._15, v._16

#define _HK_FORMAT_GET_ARG_(X, ...) X(__VA_ARGS__)
#define _HK_FORMAT_ARGS(...)        _HK_FORMAT_GET_ARG_(HK_CONCAT(_HK_FORMAT_GET_ARG, HK_NARG(__VA_ARGS__)), __VA_ARGS__)

#define HK_FORMAT_DEF(ClassName, FormatString, ...)                                                                                                                                               \
    template <> struct fmt::formatter<ClassName>                                                                                                                                                  \
    {                                                                                                                                                                                             \
        constexpr auto                         parse(format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }                                                                  \
        template <typename FormatContext> auto format(ClassName const& v, FormatContext& ctx) -> decltype(ctx.out()) { return format_to(ctx.out(), FormatString, _HK_FORMAT_ARGS(__VA_ARGS__)); } \
    };

#define HK_FORMAT_DEF_(ClassName, FormatString, ...)                                                                                                                             \
    template <> struct fmt::formatter<ClassName>                                                                                                                                 \
    {                                                                                                                                                                            \
        constexpr auto                         parse(format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }                                                 \
        template <typename FormatContext> auto format(ClassName const& v, FormatContext& ctx) -> decltype(ctx.out()) { return format_to(ctx.out(), FormatString, __VA_ARGS__); } \
    };

#define HK_FORMAT_DEF_TO_STRING(ClassName)                                                                           \
    template <> struct fmt::formatter<ClassName>                                                                     \
    {                                                                                                                \
        constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }             \
                                                                                                                     \
        template <typename FormatContext> auto format(ClassName const& v, FormatContext& ctx) -> decltype(ctx.out()) \
        {                                                                                                            \
            Hk::String str = v.ToString();                                                                           \
            return fmt::detail::copy_str<char, const char*>(str.Begin(), str.End(), ctx.out());                      \
        }                                                                                                            \
    };

#define HK_FMT(X) FMT_STRING(X)
