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

void ImportImage(ImageStorage& storage, StringView outputFile)
{
    LOG("Importing texture {}...\n", outputFile);

    String fileName{HK_FORMAT("{}.asset", outputFile)};

    File file = File::sOpenWrite(fileName);
    if (!file)
    {
        LOG("Failed to open \"{}\"\n", fileName);
        return;
    }

    AssetUtils::CreateTexture(file, storage);
}

int RunApplication()
{
    Core::SetEnableConsoleOutput(true);

    const char* help = R"(
    -h                      -- Help
    -s <filename>           -- Source filename
    -no_alpha               -- Don't aware about alpha channel or ignore it
    -alpha_premult          -- Set this flag if your texture has premultiplied alpha
    -format                 -- Output texture format (See Image.cpp, TexFormat)

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

        -filter_3d <filter>     -- Mipmap resampling filter for 3D textures (Not yet implemented. Reserved for future.):
                                        average
                                        min
                                        max
    )";

    auto& args = CoreApplication::sArgs();
    int i;

    ImageStorage source;
    String outputFile;

    IMAGE_STORAGE_FLAGS flags = IMAGE_STORAGE_FLAGS_DEFAULT;
    TEXTURE_FORMAT format = TEXTURE_FORMAT_UNDEFINED;
    ImageMipmapConfig mipmapConfig;
    bool generateMipmaps = false;

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
    i = args.Find("-filter_3d");
    if (i != -1 && i + 1 < args.Count())
    {
        mipmapConfig.Filter3D = GetResampleFilter3D(args.At(i + 1));
        generateMipmaps = true;
    }

    i = args.Find("-s");
    if (i != -1 && i + 1 < args.Count())
    {
        const char* filename = args.At(i + 1);

        LOG("Loading {}...\n", filename);

        source = CreateImage(filename, generateMipmaps ? &mipmapConfig : nullptr, flags, format);
        if (!source)
        {
            LOG("Failed to load {}\n", filename);
            return 0;
        }

        outputFile = PathUtils::sGetFilenameNoExt(filename);

        ImportImage(source, outputFile);
    }
    else
    {
        LOG("Source file is not specified. Use -s <filename>\n");
        return 0;
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
