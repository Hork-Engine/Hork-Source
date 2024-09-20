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
#include <Hork/Geometry/RawMesh.h>
#include <Hork/Resources/Resource_Mesh.h>
#include <Hork/Resources/Resource_Animation.h>

HK_NAMESPACE_BEGIN

void ImportMesh(RawMesh const& rawMesh, StringView outputFile)
{
    LOG("Importing mesh {}...\n", outputFile);

    MeshResourceBuilder builder;
    auto meshResource = builder.Build(rawMesh);
    if (!meshResource)
    {
        LOG("Failed to build mesh\n");
        return;
    }

    String fileName{HK_FORMAT("{}.asset", outputFile)};

    File file = File::sOpenWrite(fileName);
    if (!file)
    {
        LOG("Failed to open \"{}\"\n", fileName);
        return;
    }

    meshResource->Write(file);
}

void ImportAnimation(RawMesh const& rawMesh, uint32_t animationIndex, StringView outputFile)
{
    AnimationResourceBuilder builder;

    if (animationIndex >= rawMesh.Animations.Size())
    {
        LOG("Invalid animation index {}\n", animationIndex);
        return;
    }

    LOG("Importing animation {} from {}...\n", animationIndex, outputFile);

    UniqueRef<AnimationResource> animationResource = builder.Build(*rawMesh.Animations[animationIndex].RawPtr(), rawMesh.Skeleton);
    if (!animationResource)
    {
        LOG("Failed to build animation {}\n", animationIndex);
        return;
    }

    String fileName{HK_FORMAT("{}_{}.asset", outputFile, animationIndex)};

    File file = File::sOpenWrite(fileName);
    if (!file)
    {
        LOG("Failed to open \"{}\"\n", fileName);
        return;
    }

    animationResource->Write(file);
}

int RunApplication()
{
    Core::SetEnableConsoleOutput(true);

    const char* help = R"(    
    -s <filename>           -- Source filename
    -m                      -- Tag to import mesh
    -a <index/all>          -- Tag to import animation(s)
    )";

    auto& args = CoreApplication::sArgs();
    int i;

    RawMesh mesh;
    String outputFile;

    i = args.Find("-h");
    if (i != -1)
    {
        LOG(help);
        return 0;
    }

    i = args.Find("-s");
    if (i != -1 && i + 1 < args.Count())
    {
        const char* filename = args.At(i + 1);

        LOG("Loading {}...\n", filename);
        if (!mesh.Load(filename))
        {
            LOG("Failed to load {}\n", filename);
            return 0;
        }

        outputFile = PathUtils::sGetFilenameNoExt(filename);
    }
    else
    {
        LOG("Source file is not specified. Use -s <filename>\n");
        return 0;
    }

    i = args.Find("-a");
    if (i != -1)
    {
        if (i + 1 < args.Count())
        {
            const char* value = args.At(i + 1);
            if (!Core::Stricmp(value, "all"))
            {
                for (uint32_t animationIndex = 0; animationIndex < mesh.Animations.Size(); ++animationIndex)
                {
                    ImportAnimation(mesh, animationIndex, outputFile);
                }
            }
            else
            {
                uint32_t animationIndex = Core::ParseUInt32(value);
                ImportAnimation(mesh, animationIndex, outputFile);
            }
        }
        else
        {
            LOG("Expected -a <index/all>\n");
        }
    }

    i = args.Find("-m");
    if (i != -1)
    {
        ImportMesh(mesh, outputFile);
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
