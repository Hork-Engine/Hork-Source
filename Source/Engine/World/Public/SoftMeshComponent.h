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

#pragma once

#include "MeshComponent.h"

/*

FSoftMeshComponent

Mesh component without skinning

*/
class ANGIE_API FSoftMeshComponent : public FMeshComponent {
    AN_COMPONENT( FSoftMeshComponent, FMeshComponent )

public:

    // Set a wind velocity for interaction with the air
    void SetWindVelocity( Float3 const & _Velocity );

    Float3 const & GetWindVelocity() const;

    // Add force (or gravity) to the entire soft body
    void AddForceSoftBody( Float3 const & _Force );

    // Add force (or gravity) to a vertex of the soft body
    void AddForceToVertex( Float3 const & _Force, int _VertexIndex );

    Float3 GetVertexPosition( int _VertexIndex ) const;

    Float3 GetVertexNormal( int _VertexIndex ) const;

    Float3 GetVertexVelocity( int _VertexIndex ) const;    

protected:
    FSoftMeshComponent();

    void InitializeComponent() override;
    void DeinitializeComponent() override;

    void OnMeshChanged() override;

private:
    Float3 WindVelocity;
    
};
