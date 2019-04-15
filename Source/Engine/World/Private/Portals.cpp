/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

//#include <Engine/World/Public/Portals.h>





//void FWorld::BuildSpatialTree() {
//    TPodArray< FLevelPortal * > Portals;

//    // Free current tree
//    ViewArea = -1;

//    GetComponents< FLevelPortal >( Portals );

//    Float3 HalfExtents;
//    for ( FSpatialAreaComponent * Area = SpatialAreas ; Area : Area = Area->Next ) {
//        // Update area bounds
//        HalfExtents = Area->Extents * 0.5f;
//        for ( int i = 0 ; i < 3 ; i++ ) {
//            Area->Bounds.Mins[i] = Area->Position[i] - HalfExtents[i];
//            Area->Bounds.Maxs[i] = Area->Position[i] + HalfExtents[i];
//        }

//        // Clear area portals
//        Area->PortalList = NULL;
//    }

//    AreaPortals.Resize( Portals.Length() * 2 );
//    AreaPortalId = 0;

//    for ( FLevelPortal * Portal = Portals ; Portal ; Portal = Portal->Next ) {
//        if ( !Portal->Area1 && !Portal->Area1.Expired()
//             || !Portal->Area2 && !Portal->Area2.Expired() ) {
//            Out() << "WARNING: Portal with no area";
//            continue;
//        }

//        AddPortalToAreas( Portal, Portal->Area1, Portal->Area2 );
//    }

//    // Relocate renderables and lights
////    TPodArray< FLightComponent * > Lights;
////    GetComponents< FLightComponent >( Lights, true );
////    for ( FLightComponent * Light : Lights ) {
////        Light->InArea.Clear();
////        UpdateLight( Light );
////    }

////    TPodArray< FEnvCaptureComponent * > EnvCaptures;
////    GetComponents< FEnvCaptureComponent >( EnvCaptures, true );
////    for ( FEnvCaptureComponent * EnvCapture : EnvCaptures ) {
////        EnvCapture->InArea.Clear();
////        UpdateEnvCapture( EnvCapture );
////    }

//    for ( FMeshComponent * mesh = MeshList ; mesh ; mesh = mesh->NextWorldMesh() ) {
//        mesh->InArea.Clear();
//        UpdateRenderable( mesh );
//    }
//}
