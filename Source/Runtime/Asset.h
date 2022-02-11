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

#pragma once

constexpr uint32_t FMT_FILE_TYPE_MESH                = 1;
constexpr uint32_t FMT_FILE_TYPE_SKELETON            = 2;
constexpr uint32_t FMT_FILE_TYPE_ANIMATION           = 3;
constexpr uint32_t FMT_FILE_TYPE_MATERIAL_INSTANCE   = 4;
constexpr uint32_t FMT_FILE_TYPE_MATERIAL            = 5;
constexpr uint32_t FMT_FILE_TYPE_TEXTURE             = 6;
constexpr uint32_t FMT_FILE_TYPE_PHOTOMETRIC_PROFILE = 7;
constexpr uint32_t FMT_FILE_TYPE_ENVMAP              = 8;

constexpr uint32_t FMT_VERSION_MESH                = 1;
constexpr uint32_t FMT_VERSION_SKELETON            = 1;
constexpr uint32_t FMT_VERSION_ANIMATION           = 1;
constexpr uint32_t FMT_VERSION_MATERIAL_INSTANCE   = 1;
constexpr uint32_t FMT_VERSION_MATERIAL            = 1;
constexpr uint32_t FMT_VERSION_TEXTURE             = 1;
constexpr uint32_t FMT_VERSION_PHOTOMETRIC_PROFILE = 1;
constexpr uint32_t FMT_VERSION_ENVMAP              = 1;
