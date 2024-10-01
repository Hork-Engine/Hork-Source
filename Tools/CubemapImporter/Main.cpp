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
    -s <cubemap faces>      -- Source files for each cubemap face
    -o <filename>           -- Output filename
    -format                 -- Output texture format (SRGBA8_UNORM (default), SBGRA8_UNORM, R11G11B10_FLOAT, BC1_UNORM_SRGB, BC6H_UFLOAT)
    -hdri_scale <value>     -- Change the original color using the following formula: result = pow(color * hdri_scale, hdri_pow)
    -hdri_pow <value>       -- Change the original color using the following formula: result = pow(color * hdri_scale, hdri_pow)
    )";

    auto& args = CoreApplication::sArgs();
    int i;

    SkyboxImportSettings importSettings;
    importSettings.Format = SKYBOX_IMPORT_TEXTURE_FORMAT_SRGBA8_UNORM;

    const char* outputFile = nullptr;

    i = args.Find("-h");
    if (i != -1)
    {
        LOG(help);
        return 0;
    }

    i = args.Find("-format");
    if (i != -1 && i + 1 < args.Count())
    {
        TEXTURE_FORMAT format = FindTextureFormat(args.At(i + 1));

        switch (format)
        {
        case SKYBOX_IMPORT_TEXTURE_FORMAT_SRGBA8_UNORM:
        case SKYBOX_IMPORT_TEXTURE_FORMAT_SBGRA8_UNORM:
        case SKYBOX_IMPORT_TEXTURE_FORMAT_R11G11B10_FLOAT:
        case SKYBOX_IMPORT_TEXTURE_FORMAT_BC1_UNORM_SRGB:
        case SKYBOX_IMPORT_TEXTURE_FORMAT_BC6H_UFLOAT:
            importSettings.Format = static_cast<SKYBOX_IMPORT_TEXTURE_FORMAT>(format);
            break;
        default:
            LOG("Unexpected texture format {}\n", args.At(i + 1));
            return -1;
        }
    }

    i = args.Find("-hdri_scale");
    if (i != -1 && i + 1 < args.Count())
        importSettings.HDRIScale = Core::ParseFloat(args.At(i + 1));

    i = args.Find("-hdri_pow");
    if (i != -1 && i + 1 < args.Count())
        importSettings.HDRIPow = Core::ParseFloat(args.At(i + 1));

    i = args.Find("-o");
    if (i == -1 || i + 1 >= args.Count())
    {
        LOG("Output file is not specified. Use -o <filename>\n");
        return -1;
    }
    outputFile = args.At(i + 1);

    i = args.Find("-s");
    if (i == -1 || i + 6 >= args.Count())
    {
        LOG("Source files not specified. Use -s <cubemap faces>\n");
        return -1;
    }
    i++;
    for (int faceNum = 0; faceNum < 6; ++faceNum)
    {
        importSettings.Faces[faceNum] = args.At(i + faceNum);
    }

    ImageStorage skybox = LoadSkyboxImages(importSettings);
    if (!skybox)
    {
        LOG("Failed to load cubemap images\n");
        return -1;
    }

    if (!ImportImage(skybox, outputFile))
        return -1;

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
