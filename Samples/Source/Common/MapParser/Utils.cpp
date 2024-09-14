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

#include "MapGeometry.h"

#include <Hork/Runtime/World/World.h>
#include <Hork/Runtime/World/Modules/Render/Components/MeshComponent.h>
#include <Hork/Runtime/World/Modules/Physics/Components/StaticBodyComponent.h>
#include <Hork/Runtime/GameApplication/GameApplication.h>

HK_NAMESPACE_BEGIN

void CreateSceneFromMap(World* world, StringView mapFilename, StringView defaultMaterial)
{
    auto& resourceMngr = GameApplication::sGetResourceManager();
    auto& materialMngr = GameApplication::sGetMaterialManager();

    if (auto file = resourceMngr.OpenFile(mapFilename))
    {
        MapParser parser;
        parser.Parse(file.AsString().CStr());

        MapGeometry geometry;
        geometry.Build(parser);

        auto& surfaces = geometry.GetSurfaces();
        auto& vertices = geometry.GetVertices();
        auto& indices = geometry.GetIndices();
        auto& clipVertices = geometry.GetClipVertices();
        //auto& clipIndices = geometry.GetClipIndices();
        auto& clipHull = geometry.GetClipHulls();
        auto& entities = geometry.GetEntities();

        for (int i = 0; i < entities.Size(); ++i)
        {
            auto& entity = entities[i];

            GameObjectDesc desc;
            GameObject* object;
            world->CreateObject(desc, object);

            for (int surfaceNum = 0; surfaceNum < entity.SurfaceCount; ++surfaceNum)
            {
                int surfaceIndex = entity.FirstSurface + surfaceNum;
                auto& surface = surfaces[surfaceIndex];
                auto surfaceHandle = GameApplication::sGetResourceManager().CreateResource<MeshResource>("surface_" + Core::ToString(surfaceIndex));

                MeshResource* resource = GameApplication::sGetResourceManager().TryGet(surfaceHandle);
                HK_ASSERT(resource);

                BvAxisAlignedBox bounds;
                bounds.Clear();
                for (int v = 0; v < surface.VertexCount; ++v)
                    bounds.AddPoint(vertices[surface.FirstVert + v].Position);

                MeshAllocateDesc alloc;
                alloc.SurfaceCount = 1;
                alloc.VertexCount = surface.VertexCount;
                alloc.IndexCount = surface.IndexCount;

                resource->Allocate(alloc);
                resource->WriteVertexData(&vertices[surface.FirstVert], surface.VertexCount, 0);
                resource->WriteIndexData(&indices[surface.FirstIndex], surface.IndexCount, 0);
                resource->SetBoundingBox(bounds);

                MeshSurface& meshSurface = resource->LockSurface(0);
                meshSurface.BoundingBox = bounds;

                StaticMeshComponent* mesh;
                object->CreateComponent(mesh);
                mesh->SetMesh(surfaceHandle);
                mesh->SetMaterial(materialMngr.TryGet(defaultMaterial));
                mesh->SetLocalBoundingBox(bounds);
            }

            //#define SINGLE_OBJECT

#ifdef SINGLE_OBJECT
            if (entity.ClipHullCount > 0)
            {
                StaticBodyComponent* body;
                object->CreateComponent(body);
            }
#endif

            for (int hullNum = 0; hullNum < entity.ClipHullCount; ++hullNum)
            {
                auto& chull = clipHull[entity.FirstClipHull + hullNum];

#ifdef SINGLE_OBJECT
                MeshCollider* collider;
                object->CreateComponent(collider);
#else
                GameObjectDesc collisionObjectDesc;
                collisionObjectDesc.Parent = object->GetHandle();
                GameObject* collisionObject;
                world->CreateObject(desc, collisionObject);
                StaticBodyComponent* body;
                collisionObject->CreateComponent(body);
                MeshCollider* collider;
                collisionObject->CreateComponent(collider);
#endif
                collider->Data = MakeRef<MeshCollisionData>();
                collider->Data->CreateConvexHull(ArrayView(&clipVertices[chull.FirstVert], chull.VertexCount));

                //collider->Data->CreateTriangleSoup(ArrayView(&clipVertices[chull.FirstVert], chull.VertexCount),
                //                                   ArrayView(&clipIndices[chull.FirstIndex], chull.IndexCount));
            }
        }
    }
}

HK_NAMESPACE_END
