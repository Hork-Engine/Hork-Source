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

#include <Image/SvgDocument.h>

#include <lunasvg/lunasvg.h>
#include <lunasvg/layoutcontext.h>
#include <lunasvg/parser.h>

HK_NAMESPACE_BEGIN

SvgDocument::~SvgDocument()
{
    delete m_Root;
}

void SvgDocument::Reset()
{
    delete m_Root;
    m_Root = nullptr;
}

uint32_t SvgDocument::GetWidth() const
{
    return m_Root ? m_Root->width : 0;
}

uint32_t SvgDocument::GetHeight() const
{
    return m_Root ? m_Root->height : 0;
}

void SvgDocument::RenderToImage(void* pData, uint32_t Width, uint32_t Height, size_t SizeInBytes) const
{
    using namespace lunasvg;

    if (!m_Root || m_Root->width <= 0.0 || m_Root->height <= 0.0)
    {
        return;
    }

    if (SizeInBytes < Width * Height * 4)
    {
        return;
    }

    Matrix matrix(Width / m_Root->width, 0, 0, Height / m_Root->height, 0, 0);

    RenderState state(nullptr, RenderMode::Display);
    state.canvas    = Canvas::create((unsigned char*)pData, Width, Height, Width * 4);
    state.transform = Transform(matrix);
    m_Root->render(state);
}

SvgDocument CreateSVG(IBinaryStreamReadInterface& Stream)
{
    using namespace lunasvg;

    if (!Stream.IsValid())
        return {};

    HeapBlob blob = Stream.AsBlob();

    ParseDocument parser;
    if (!parser.parse(blob.ToRawString(), blob.Size()))
    {
        LOG("CreateSVG: Failed to parse data {}\n", Stream.GetName());
        return {};
    }

    auto root = parser.layout();
    if (!root || root->children.empty())
    {
        LOG("CreateSVG: Empty SVG {}\n", Stream.GetName());
        return {};
    }

    SvgDocument document;
    document.m_Root = root.release();
    return document;
}

HK_NAMESPACE_END
