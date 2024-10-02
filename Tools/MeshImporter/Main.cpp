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

bool ImportMesh(RawMesh const& rawMesh, StringView outputFile)
{
    String fileName = PathUtils::sGetFilenameNoExt(outputFile) + ".mesh";

    LOG("Importing mesh {}...\n", fileName);

    MeshResourceBuilder builder;
    auto meshResource = builder.Build(rawMesh);
    if (!meshResource)
    {
        LOG("Failed to build mesh\n");
        return false;
    }


    File file = File::sOpenWrite(fileName);
    if (!file)
    {
        LOG("Failed to open \"{}\"\n", fileName);
        return false;
    }

    meshResource->Write(file);
    return true;
}

bool ImportAnimation(RawMesh const& rawMesh, uint32_t animationIndex, StringView outputFile)
{
    AnimationResourceBuilder builder;

    if (animationIndex >= rawMesh.Animations.Size())
    {
        LOG("Invalid animation index {}\n", animationIndex);
        return false;
    }

    LOG("Importing animation {}...\n", animationIndex);

    UniqueRef<AnimationResource> animationResource = builder.Build(*rawMesh.Animations[animationIndex].RawPtr(), rawMesh.Skeleton);
    if (!animationResource)
    {
        LOG("Failed to build animation {}\n", animationIndex);
        return false;
    }

    String fileName;
    
    if (animationIndex > 0)
        fileName = HK_FORMAT("{}_{}.anim", PathUtils::sGetFilenameNoExt(outputFile), animationIndex);
    else
        fileName = PathUtils::sGetFilenameNoExt(outputFile) + ".anim";
    
    File file = File::sOpenWrite(fileName);
    if (!file)
    {
        LOG("Failed to open \"{}\"\n", fileName);
        return false;
    }

    animationResource->Write(file);
    return true;
}

void CreateDefaultMeshes(StringView path)
{
    {
        RawMesh mesh;
        mesh.CreateBox(Float3(1), 1.0f);

        MeshResourceBuilder builder;
        auto resource = builder.Build(mesh);
        HK_ASSERT(resource);

        auto file = File::sOpenWrite(path / "box.mesh");
        HK_ASSERT(file);
        resource->Write(file);
    }

    {
        RawMesh mesh;
        mesh.CreateSphere(0.5f, 1.0f);

        MeshResourceBuilder builder;
        auto resource = builder.Build(mesh);
        HK_ASSERT(resource);
        resource->GenerateBVH();

        auto file = File::sOpenWrite(path / "sphere.mesh");
        HK_ASSERT(file);
        resource->Write(file);
    }

    {
        RawMesh mesh;
        mesh.CreateCylinder(0.5f, 1.0f, 1.0f);

        MeshResourceBuilder builder;
        auto resource = builder.Build(mesh);
        HK_ASSERT(resource);
        resource->GenerateBVH();

        auto file = File::sOpenWrite(path / "cylinder.mesh");
        HK_ASSERT(file);
        resource->Write(file);
    }

    {
        RawMesh mesh;
        mesh.CreateCone(0.5f, 1.0f, 1.0f);

        MeshResourceBuilder builder;
        auto resource = builder.Build(mesh);
        HK_ASSERT(resource);
        resource->GenerateBVH();

        auto file = File::sOpenWrite(path / "cone.mesh");
        HK_ASSERT(file);
        resource->Write(file);
    }

    {
        RawMesh mesh;
        mesh.CreateCapsule(0.5f, 1.0f, 1.0f);

        MeshResourceBuilder builder;
        auto resource = builder.Build(mesh);
        HK_ASSERT(resource);
        resource->GenerateBVH();

        auto file = File::sOpenWrite(path / "capsule.mesh");
        HK_ASSERT(file);
        resource->Write(file);
    }

    {
        RawMesh mesh;
        mesh.CreatePlaneXZ(256, 256, Float2(256));

        MeshResourceBuilder builder;
        auto resource = builder.Build(mesh);
        HK_ASSERT(resource);

        auto file = File::sOpenWrite(path / "plane_xz.mesh");
        HK_ASSERT(file);
        resource->Write(file);
    }

    {
        RawMesh mesh;
        mesh.CreatePlaneXY(256, 256, Float2(256));

        MeshResourceBuilder builder;
        auto resource = builder.Build(mesh);
        HK_ASSERT(resource);

        auto file = File::sOpenWrite(path / "plane_xy.mesh");
        HK_ASSERT(file);
        resource->Write(file);
    }

    {
        RawMesh mesh;
        mesh.CreatePlaneXZ(1, 1, Float2(1));

        MeshResourceBuilder builder;
        auto resource = builder.Build(mesh);
        HK_ASSERT(resource);

        auto file = File::sOpenWrite(path / "quad_xz.mesh");
        HK_ASSERT(file);
        resource->Write(file);
    }

    {
        RawMesh mesh;
        mesh.CreatePlaneXY(1, 1, Float2(1));

        MeshResourceBuilder builder;
        auto resource = builder.Build(mesh);
        HK_ASSERT(resource);

        auto file = File::sOpenWrite(path / "quad_xy.mesh");
        HK_ASSERT(file);
        resource->Write(file);
    }

    {
        RawMesh mesh;
        mesh.CreateSkybox(Float3(1), 1);

        MeshResourceBuilder builder;
        auto resource = builder.Build(mesh);
        HK_ASSERT(resource);
        resource->GenerateBVH();

        auto file = File::sOpenWrite(path / "skybox.mesh");
        HK_ASSERT(file);
        resource->Write(file);
    }

    {
        RawMesh mesh;
        mesh.CreateSkydome(0.5f, 1, 32, 32, false);

        MeshResourceBuilder builder;
        auto resource = builder.Build(mesh);
        HK_ASSERT(resource);
        resource->GenerateBVH();

        auto file = File::sOpenWrite(path / "skydome.mesh");
        HK_ASSERT(file);
        resource->Write(file);
    }

    {
        RawMesh mesh;
        mesh.CreateSkydome(0.5f, 1, 32, 32, true);

        MeshResourceBuilder builder;
        auto resource = builder.Build(mesh);
        HK_ASSERT(resource);
        resource->GenerateBVH();

        auto file = File::sOpenWrite(path / "skydome_hemisphere.mesh");
        HK_ASSERT(file);
        resource->Write(file);
    }
}

int RunApplication()
{
    Core::SetEnableConsoleOutput(true);

    const char* help = R"(    
    -s <filename>             -- Source filename
    -o <filename>             -- Output filename
    -m                        -- Tag to import mesh
    -a <index/all>            -- Tag to import animation(s)
    -d <path>                 -- Tag for creating default meshes such as box, cylinder, sphere, etc
    )";

    auto& args = CoreApplication::sArgs();
    int i;
    const char* inputFile = nullptr;
    const char* outputFile = nullptr;

    RawMesh mesh;

    i = args.Find("-h");
    if (i != -1)
    {
        LOG(help);
        return 0;
    }

    i = args.Find("-d");
    if (i != -1 && i + 1 < args.Count())
    {
        CreateDefaultMeshes(args.At(i + 1));
        return 0;
    }

    i = args.Find("-s");
    if (i == -1 || i + 1 >= args.Count())
    {
        LOG("Source file is not specified. Use -s <filename>\n");
        return -1;
    }

    inputFile = args.At(i + 1);

    i = args.Find("-o");
    if (i == -1 || i + 1 >= args.Count())
    {
        LOG("Output file is not specified. Use -o <filename>\n");
        return -1;
    }

    outputFile = args.At(i + 1);

    LOG("Loading {}...\n", inputFile);
    if (!mesh.Load(inputFile))
    {
        LOG("Failed to load {}\n", inputFile);
        return -1;
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
                    if (!ImportAnimation(mesh, animationIndex, outputFile))
                        return -1;
                }
            }
            else
            {
                uint32_t animationIndex = Core::ParseUInt32(value);
                if (!ImportAnimation(mesh, animationIndex, outputFile))
                    return -1;
            }
        }
        else
        {
            LOG("Expected -a <index/all>\n");
            return -1;
        }
    }

    i = args.Find("-m");
    if (i != -1)
    {
        if (!ImportMesh(mesh, outputFile))
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
