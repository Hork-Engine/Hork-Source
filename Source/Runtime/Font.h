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

#include "Texture.h"

class AFontStash : public ARefCounted
{
public:
    AFontStash();
    ~AFontStash();

    struct FONScontext* GetImpl()
    {
        return m_pImpl;
    }

    bool ReallocTexture();
    void UpdateTexture();

    RenderCore::ITexture* GetTexture()
    {
        return m_FontImages[m_FontImageIdx];
    }

    void Cleanup();

private:
    enum
    {
        MAX_FONT_IMAGES = 4,
        MAX_FONTIMAGE_SIZE = 2048,
        INITIAL_FONTIMAGE_SIZE = 512
    };
    struct FONScontext*        m_pImpl{};
    TRef<RenderCore::ITexture> m_FontImages[MAX_FONT_IMAGES];
    int                        m_FontImageIdx{};
};

struct TextBounds
{
    float MinX;
    float MinY;
    float MaxX;
    float MaxY;

    float Width() const
    {
        return MaxX - MinX;
    }

    float Height() const
    {
        return MaxY - MinY;
    }
};

struct TextLineBounds
{
    float MinY;
    float MaxY;
};

struct TextMetrics
{
    float Ascender;
    float Descender;
    float LineHeight;
};

class AFont : public AResource
{
    HK_CLASS(AFont, AResource)

public:
    AFont();
    ~AFont();

    int GetId() const
    {
        return m_FontId;
    }

    float CalcTextBounds(float texSize, float blur, int align, float spacing, float x, float y, AStringView text, TextBounds& bounds);
    void CalcLineBounds(float textSize, int align, float y, TextLineBounds& bounds);
    void CalcVertMetrics(float textSize, TextMetrics& metrics);

    float GetCharAdvance(WideChar ch, float textSize, float blur, float devicePixelRatio = 1) const;

    bool AddFallbackFont(AFont* fallbackFont);
    void ResetFallbackFonts();

protected:
    /** Load resource from file */
    bool LoadResource(IBinaryStreamReadInterface& _Stream) override;

    /** Create internal resource */
    void LoadInternalResource(AStringView Path) override;

    const char* GetDefaultResourcePath() const override { return "/Default/Fonts/Default"; }

private:
    int                  m_FontId{};
    mutable TRef<AFontStash> m_FontStash;
    HeapBlob             m_Blob;
    TVector<TRef<AFont>> m_Fallbacks;
};
