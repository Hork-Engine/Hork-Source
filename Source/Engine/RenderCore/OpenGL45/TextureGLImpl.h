/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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

#include <RenderCore/Texture.h>

namespace RenderCore {

class ADeviceGLImpl;

class ATextureGLImpl final : public ITexture
{
public:
    ATextureGLImpl( ADeviceGLImpl * _Device, STextureCreateInfo const & _CreateInfo );
    ATextureGLImpl( ADeviceGLImpl * _Device, STextureViewCreateInfo const & _CreateInfo );
    ~ATextureGLImpl();

    void GenerateLods() override;

    void GetLodInfo( uint16_t _Lod, STextureLodInfo * _Info ) const;

    void Read( uint16_t _Lod,
               DATA_FORMAT _Format,
               size_t _SizeInBytes,
               unsigned int _Alignment,
               void * _SysMem ) override;

    void ReadRect( STextureRect const & _Rectangle,
                   DATA_FORMAT _Format,
                   size_t _SizeInBytes,
                   unsigned int _Alignment,
                   void * _SysMem ) override;

    bool Write( uint16_t _Lod,
                DATA_FORMAT _Format,
                size_t _SizeInBytes,
                unsigned int _Alignment,
                const void * _SysMem ) override;

    bool WriteRect( STextureRect const & _Rectangle,
                    DATA_FORMAT _Format,
                    size_t _SizeInBytes,
                    unsigned int _Alignment,
                    const void * _SysMem ) override;

    void Invalidate( uint16_t _Lod ) override;

    void InvalidateRect( uint32_t _NumRectangles, STextureRect const * _Rectangles ) override;

private:
    ADeviceGLImpl * pDevice;
    TRef< ITexture > pOriginalTex;
};

}
