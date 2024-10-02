/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include <Hork/Core/CoreApplication.h>

#include <Hork/Core/IO.h>
#include <Hork/Core/Parse.h>
#include <Hork/Core/Logger.h>
#include <Hork/Core/Platform.h>
#include <Hork/Image/RawImage.h>
#include <Hork/Image/Image.h>
#include <Hork/Resources/Resource_Texture.h>

HK_NAMESPACE_BEGIN

bool ImportImage(ImageStorage& storage, StringView fileName)
{
    LOG("Importing texture {}...\n", fileName);

    File file = File::sOpenWrite(fileName);
    if (!file)
    {
        LOG("Failed to open \"{}\"\n", fileName);
        return false;
    }

    AssetUtils::CreateTexture(file, storage);
    return true;
}

int RunApplication()
{
    Core::SetEnableConsoleOutput(true);

    const char* help = R"(
    -h                      -- Help
    -s <filename>           -- Source filename
    -o <filename>           -- Output filename
    -no_alpha               -- Don't aware about alpha channel or ignore it
    -alpha_premult          -- Set this flag if your texture has premultiplied alpha
    -format                 -- Output texture format (See Image.cpp, TexFormat)

    -resample <width> <height>                  -- Scale input textures
    -resample_edge_mode <mode_h> <mode_v>       -- Use edge mode for resampling (clamp/reflect/wrap/zero)
    -resample_filter <filter_h> <filter_v>      -- Use filter for resampling (box/triangle/cubicspline/catmullrom/mitchell)

    Mipmap generation:
        Don't specify if you don't want to generate mipmaps

        -mip_edge_mode <mode>   -- Mipmap resampling edge mode:
                                        clamp
                                        reflect
                                        wrap
                                        zero

        -mip_filter <filter>    -- Mipmap resampling filter:
                                        box
                                        triangle
                                        cubicspline
                                        catmullrom
                                        mitchell

        -mip_filter_3d <filter>  -- Mipmap resampling filter for 3D textures (Not yet implemented. Reserved for future.):
                                        average
                                        min
                                        max
    )";

    auto& args = CoreApplication::sArgs();
    int i;

    ImageStorage source;
    const char* outputFile = nullptr;

    IMAGE_STORAGE_FLAGS flags = IMAGE_STORAGE_FLAGS_DEFAULT;
    TEXTURE_FORMAT format = TEXTURE_FORMAT_UNDEFINED;
    ImageMipmapConfig mipmapConfig;
    bool generateMipmaps = false;
    RawImageResampleParams resampleParams;
    bool resample = false;

    i = args.Find("-h");
    if (i != -1)
    {
        LOG(help);
        return 0;
    }

    i = args.Find("-no_alpha");
    if (i != -1)
        flags |= IMAGE_STORAGE_NO_ALPHA;
    i = args.Find("-alpha_premult");
    if (i != -1)
        flags |= IMAGE_STORAGE_ALPHA_PREMULTIPLIED;
    i = args.Find("-format");
    if (i != -1 && i + 1 < args.Count())
        format = FindTextureFormat(args.At(i + 1));

    i = args.Find("-mip_edge_mode");
    if (i != -1 && i + 1 < args.Count())
    {
        mipmapConfig.EdgeMode = GetResampleEdgeMode(args.At(i + 1));
        generateMipmaps = true;
    }
    i = args.Find("-mip_filter");
    if (i != -1 && i + 1 < args.Count())
    {
        mipmapConfig.Filter = GetResampleFilter(args.At(i + 1));
        generateMipmaps = true;
    }
    i = args.Find("-mip_filter_3d");
    if (i != -1 && i + 1 < args.Count())
    {
        mipmapConfig.Filter3D = GetResampleFilter3D(args.At(i + 1));
        generateMipmaps = true;
    }

    i = args.Find("-resample");
    if (i != -1 && i + 2 < args.Count())
    {
        resample = true;

        resampleParams.ScaledWidth = Core::ParseUInt32(args.At(i + 1));
        resampleParams.ScaledHeight = Core::ParseUInt32(args.At(i + 2));
        if (resampleParams.ScaledWidth == 0 || resampleParams.ScaledHeight == 0)
        {
            LOG("Invalid resample size {} x {}\n", resampleParams.ScaledWidth, resampleParams.ScaledHeight);
            return -1;
        }
        if (resampleParams.ScaledWidth > 4096 || resampleParams.ScaledHeight > 4096)
        {
            LOG("Resulting texture size is too large {} x {}\n", resampleParams.ScaledWidth, resampleParams.ScaledHeight);
            return -1;
        }

        auto const& texFormat = GetTextureFormatInfo(format);
        const uint32_t blockSize = texFormat.BlockSize;

        resampleParams.ScaledWidth  = generateMipmaps ? Math::Max<uint32_t>(Math::ToClosestPowerOfTwo(resampleParams.ScaledWidth), blockSize) : Align(resampleParams.ScaledWidth, blockSize);
        resampleParams.ScaledHeight = generateMipmaps ? Math::Max<uint32_t>(Math::ToClosestPowerOfTwo(resampleParams.ScaledHeight), blockSize) : Align(resampleParams.ScaledHeight, blockSize);

        resampleParams.Flags = RAW_IMAGE_RESAMPLE_FLAG_DEFAULT;
        if (texFormat.bHasAlpha && !(flags & IMAGE_STORAGE_NO_ALPHA))
        {
            resampleParams.Flags |= RAW_IMAGE_RESAMPLE_HAS_ALPHA;
            if (flags & IMAGE_STORAGE_ALPHA_PREMULTIPLIED)
                resampleParams.Flags |= RAW_IMAGE_RESAMPLE_ALPHA_PREMULTIPLIED;
        }
        if (texFormat.bSRGB)
            resampleParams.Flags |= RAW_IMAGE_RESAMPLE_COLORSPACE_SRGB;
    }

    i = args.Find("-resample_edge_mode");
    if (i != -1 && i + 2 < args.Count())
    {
        resampleParams.HorizontalEdgeMode = GetResampleEdgeMode(args.At(i + 1));
        resampleParams.VerticalEdgeMode = GetResampleEdgeMode(args.At(i + 2));
    }

    i = args.Find("-resample_filter");
    if (i != -1 && i + 2 < args.Count())
    {
        resampleParams.HorizontalFilter = GetResampleFilter(args.At(i + 1));
        resampleParams.VerticalFilter = GetResampleFilter(args.At(i + 2));
    }

    i = args.Find("-o");
    if (i != -1 && i + 1 < args.Count())
    {
        outputFile = args.At(i + 1);
    }
    else
    {
        LOG("Output file is not specified. Use -o <filename>\n");
        return -1;
    }

    i = args.Find("-s");
    if (i != -1 && i + 1 < args.Count())
    {
        String filename = args.At(i + 1);

        LOG("Loading {}...\n", filename);

        if (resample)
        {
            RawImage image = CreateRawImage(filename);
            if (!image)
            {
                LOG("Failed to load {}\n", filename);
                return -1;
            }

            RawImage resampled = ResampleRawImage(image, resampleParams);
            if (!resampled)
            {
                LOG("Failed to resample {}\n", filename);
                return -1;
            }

            filename += ".resample.png";
            if (!WriteImage(filename, resampled))
            {
                LOG("Failed to write resampled image\n");
                return -1;
            }
        }

        source = CreateImage(filename, generateMipmaps ? &mipmapConfig : nullptr, flags, format);
        if (!source)
        {
            LOG("Failed to load {}\n", filename);
            return -1;
        }

        if (!ImportImage(source, outputFile))
            return -1;
    }
    else
    {
        LOG("Source file is not specified. Use -s <filename>\n");
        return -1;
    }

    return 0;
}

HK_NAMESPACE_END

using ApplicationClass = Hk::CoreApplication;

alignas(alignof(ApplicationClass)) static char AppData[sizeof(ApplicationClass)];

int main(int argc, char* argv[])
{
    using namespace Hk;

#if defined(HK_DEBUG) && defined(HK_COMPILER_MSVC)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

#ifdef HK_OS_WIN32
    ArgumentPack args;
#else
    ArgumentPack args(argc, argv);
#endif

    ApplicationClass* app = new (AppData) ApplicationClass(args);
    int exitCode = RunApplication();
    app->~ApplicationClass();
    return exitCode;
}
