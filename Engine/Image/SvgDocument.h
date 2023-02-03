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

#include <Engine/Core/IO.h>

namespace lunasvg
{
class LayoutSymbol;
}

HK_NAMESPACE_BEGIN

class SvgDocument
{
public:
    SvgDocument() = default;
    virtual ~SvgDocument();

    SvgDocument(SvgDocument const& Rhs) = delete;
    SvgDocument& operator=(SvgDocument const& Rhs) = delete;

    SvgDocument(SvgDocument&& Rhs) noexcept :
        m_Root(Rhs.m_Root)
    {
        Rhs.m_Root = nullptr;
    }

    SvgDocument& operator=(SvgDocument&& Rhs) noexcept
    {
        Reset();
        Core::Swap(m_Root, Rhs.m_Root);
        return *this;
    }

    void Reset();

    uint32_t GetWidth() const;
    uint32_t GetHeight() const;

    void RenderToImage(void* pData, uint32_t Width, uint32_t Height, size_t SizeInBytes) const;

    operator bool() const
    {
        return m_Root != nullptr;
    }

private:
    lunasvg::LayoutSymbol* m_Root{};

    friend SvgDocument CreateSVG(IBinaryStreamReadInterface& Stream);
};

HK_NAMESPACE_END
