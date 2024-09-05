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

#include "Utils.h"
#include "ResourceManager.h"
#include "Resource_Mesh.h"
#include <Hork/GameApplication/GameApplication.h>
#include <Hork/MaterialGraph/MaterialGraph.h>

HK_NAMESPACE_BEGIN

void CreateDefaultResources()
{
    {
        RawMesh mesh;
        mesh.CreateBox(Float3(1), 1.0f);

        MeshResourceBuilder builder;
        auto resource = builder.Build(mesh);
        HK_ASSERT(resource);

        auto file = File::sOpenWrite("Data/default/box.mesh");
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

        auto file = File::sOpenWrite("Data/default/sphere.mesh");
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

        auto file = File::sOpenWrite("Data/default/cylinder.mesh");
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

        auto file = File::sOpenWrite("Data/default/cone.mesh");
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

        auto file = File::sOpenWrite("Data/default/capsule.mesh");
        HK_ASSERT(file);
        resource->Write(file);
    }

    {
        RawMesh mesh;
        mesh.CreatePlaneXZ(256, 256, Float2(256));

        MeshResourceBuilder builder;
        auto resource = builder.Build(mesh);
        HK_ASSERT(resource);

        auto file = File::sOpenWrite("Data/default/plane_xz.mesh");
        HK_ASSERT(file);
        resource->Write(file);
    }

    {
        RawMesh mesh;
        mesh.CreatePlaneXY(256, 256, Float2(256));

        MeshResourceBuilder builder;
        auto resource = builder.Build(mesh);
        HK_ASSERT(resource);

        auto file = File::sOpenWrite("Data/default/plane_xy.mesh");
        HK_ASSERT(file);
        resource->Write(file);
    }

    {
        RawMesh mesh;
        mesh.CreatePlaneXZ(1, 1, Float2(1));

        MeshResourceBuilder builder;
        auto resource = builder.Build(mesh);
        HK_ASSERT(resource);

        auto file = File::sOpenWrite("Data/default/quad_xz.mesh");
        HK_ASSERT(file);
        resource->Write(file);
    }

    {
        RawMesh mesh;
        mesh.CreatePlaneXY(1, 1, Float2(1));

        MeshResourceBuilder builder;
        auto resource = builder.Build(mesh);
        HK_ASSERT(resource);

        auto file = File::sOpenWrite("Data/default/quad_xy.mesh");
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

        auto file = File::sOpenWrite("Data/default/skybox.mesh");
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

        auto file = File::sOpenWrite("Data/default/skydome.mesh");
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

        auto file = File::sOpenWrite("Data/default/skydome_hemisphere.mesh");
        HK_ASSERT(file);
        resource->Write(file);
    }

    CompileMaterial("/Root/materials/default.mg", "Data/default/materials/default.mat");
    CompileMaterial("/Root/materials/unlit.mg", "Data/default/materials/unlit.mat");

    {
        auto graph = MakeRef<MaterialGraph>();

        auto& inPosition = graph->Add2<MGInPosition>();

        MGTextureSlot* cubemapTexture = graph->GetTexture(0);
        cubemapTexture->TextureType = TEXTURE_CUBE;
        cubemapTexture->Filter = TEXTURE_FILTER_LINEAR;
        cubemapTexture->AddressU = TEXTURE_ADDRESS_CLAMP;
        cubemapTexture->AddressV = TEXTURE_ADDRESS_CLAMP;
        cubemapTexture->AddressW = TEXTURE_ADDRESS_CLAMP;

        auto& cubemapSampler = graph->Add2<MGTextureLoad>();
        cubemapSampler.BindInput("TexCoord", inPosition);
        cubemapSampler.BindInput("Texture", cubemapTexture);

        graph->BindInput("Color", cubemapSampler);

        graph->MaterialType = MATERIAL_TYPE_UNLIT;
        graph->DepthHack = MATERIAL_DEPTH_HACK_SKYBOX;

        MaterialResourceBuilder builder;
        if (auto material = builder.Build(*graph))
        {
            if (auto file = File::sOpenWrite("Data/default/materials/skybox.mat"))
                material->Write(file);
        }
    }
}

bool CompileMaterial(StringView input, StringView output)
{
    auto& resourceMngr = GameApplication::sGetResourceManager();
    if (auto file = resourceMngr.OpenFile(input))
    {
        auto graph = MaterialGraph::sLoad(file);

        MaterialResourceBuilder builder;
        if (auto material = builder.Build(*graph))
        {
            if (auto outfile = File::sOpenWrite(output))
            {
                material->Write(outfile);
                return true;
            }
        }
    }
    return false;
}

HK_NAMESPACE_END
