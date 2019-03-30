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

#include <Engine/World/Public/BaseObject.h>
#include <Engine/Core/Public/Guid.h>

#include "Material.h"

class FAssemblyBlock;

enum EAssemblyType {
    AT_Unknown = 0,
    AT_Float1,
    AT_Float2,
    AT_Float3,
    AT_Float4
};

enum EMaterialStage {
    VERTEX_STAGE,
    FRAGMENT_STAGE,

    MAX_MATERIAL_STAGES
};

enum EMaterialStageBit {
    UNKNOWN_STAGE       = 0,
    VERTEX_STAGE_BIT    = 1 << VERTEX_STAGE,
    FRAGMENT_STAGE_BIT  = 1 << FRAGMENT_STAGE,

    ANY_STAGE_BIT       = ~0
};

enum EMaterialPass {
    MATERIAL_COLOR_PASS,
    MATERIAL_DEPTH_PASS,
    MATERIAL_WIREFRAME_PASS
};

class FAssemblyBlockOutput;

class FMaterialBuildContext {
public:
    mutable FString SourceCode;

    void Reset( EMaterialType _Type, EMaterialPass _Pass ) { ++BuildSerial; MaterialType = _Type; MaterialPass = _Pass; }
    int GetBuildSerial() const { return BuildSerial; }

    FString GenerateVariableName() const;
    void GenerateSourceCode( FAssemblyBlockOutput * _Slot, FString const & _Expression, bool _AddBrackets );

    void SetStage( EMaterialStage _Stage );
    EMaterialStage GetStage() const { return Stage; }
    int GetStageMask() const { return 1 << Stage; }

    EMaterialType GetMaterialType() const { return MaterialType; }
    EMaterialPass GetMaterialPass() const { return MaterialPass; }

private:
    mutable int VariableName = 0;
    static int BuildSerial;
    EMaterialStage Stage;
    EMaterialType MaterialType;
    EMaterialPass MaterialPass;
};

class FAssemblyBlockOutput : public FBaseObject {
    AN_CLASS( FAssemblyBlockOutput, FBaseObject )

public:
    FString Expression;
    EAssemblyType Type;
    int Usages[ MAX_MATERIAL_STAGES ];

protected:
    FAssemblyBlockOutput() {}
    ~FAssemblyBlockOutput() {}
};

class FAssemblyBlockInput : public FBaseObject {
    AN_CLASS( FAssemblyBlockInput, FBaseObject )

public:
    void Connect( FAssemblyBlock * _Block, const char * _Slot );

    void Disconnect();

    FAssemblyBlockOutput * GetConnection();

    FAssemblyBlock * ConnectedBlock() { return Block; }

    int Serialize( FDocument & _Doc ) override;

protected:
    FString Slot;
    TRefHolder< FAssemblyBlock > Block;

    FAssemblyBlockInput();
};

class FAssemblyNextStageVariable : public FAssemblyBlockOutput {
    AN_CLASS( FAssemblyNextStageVariable, FAssemblyBlockOutput )

public:
    void Connect( FAssemblyBlock * _Block, const char * _Slot );

    void Disconnect();

    FAssemblyBlockOutput * GetConnection();

    FAssemblyBlock * ConnectedBlock() { return Block; }

    int Serialize( FDocument & _Doc ) override;

protected:
    FString Slot;
    TRefHolder< FAssemblyBlock > Block;

    FAssemblyNextStageVariable() {}
    ~FAssemblyNextStageVariable() {}
};

class FAssemblyBlock : public FBaseObject {
    AN_CLASS( FAssemblyBlock, FBaseObject )

public:
    Float2 Location;        // Block xy location for editing

    FGUID const & GetGUID() const { return GUID; }

    FAssemblyBlockOutput * FindOutput( const char * _Name );

    bool Build( FMaterialBuildContext & _Context );

    void ResetConnections( FMaterialBuildContext const & _Context );
    void TouchConnections( FMaterialBuildContext const & _Context );

    int Serialize( FDocument & _Doc ) override;

protected:
    int Stages;

    FAssemblyBlock();
    ~FAssemblyBlock();

    FAssemblyBlockInput * NewInput( const char * _Name );

    FAssemblyBlockOutput * NewOutput( const char * _Name, EAssemblyType _Type = AT_Unknown );

    virtual void Compute( FMaterialBuildContext & _Context ) {}

private:
    FGUID GUID;
    TPodArray< FAssemblyBlockInput *, 4 > Inputs;
    TPodArray< FAssemblyBlockOutput *, 1 > Outputs;
    int Serial;
    bool bTouched;
};

class FMaterialStageBlock : public FAssemblyBlock {
    AN_CLASS( FMaterialStageBlock, FAssemblyBlock )

    friend class FAssemblyBlock;

public:
    FAssemblyNextStageVariable * AddNextStageVariable( const char * _Name, EAssemblyType _Type );

    FAssemblyNextStageVariable * FindNextStageVariable( const char * _Name );

    int NumNextStageVariables() const { return NextStageVariables.Length(); }

    FString NSV_OutputSection();

    FString NSV_InputSection();

    int Serialize( FDocument & _Doc ) override;

protected:
    TPodArray< FAssemblyNextStageVariable *, 4 > NextStageVariables;
    FString NsvPrefix;

    FMaterialStageBlock() {}
    ~FMaterialStageBlock();

    void Compute( FMaterialBuildContext & _Context ) override;
};

class FMaterialVertexStage : public FMaterialStageBlock {
    AN_CLASS( FMaterialVertexStage, FMaterialStageBlock )

public:
    FAssemblyBlockInput * Position;

protected:
    FMaterialVertexStage();
    ~FMaterialVertexStage();

    void Compute( FMaterialBuildContext & _Context ) override;
};

class FMaterialFragmentStage : public FMaterialStageBlock {
    AN_CLASS( FMaterialFragmentStage, FMaterialStageBlock )

public:
    FAssemblyBlockInput * Color;

protected:
    FMaterialFragmentStage();
    ~FMaterialFragmentStage();

    void Compute( FMaterialBuildContext & _Context ) override;
};

class FMaterialProjectionBlock : public FAssemblyBlock {
    AN_CLASS( FMaterialProjectionBlock, FAssemblyBlock )

public:
    FAssemblyBlockInput * Vector;
    FAssemblyBlockOutput * Result;

protected:
    FMaterialProjectionBlock();

    void Compute( FMaterialBuildContext & _Context ) override;
};

class FMaterialLengthBlock : public FAssemblyBlock {
    AN_CLASS( FMaterialLengthBlock, FAssemblyBlock )

public:
    FAssemblyBlockInput * Value;
    FAssemblyBlockOutput * Result;

protected:
    FMaterialLengthBlock();

    void Compute( FMaterialBuildContext & _Context ) override;
};

class FMaterialNormalizeBlock : public FAssemblyBlock {
    AN_CLASS( FMaterialNormalizeBlock, FAssemblyBlock )

public:
    FAssemblyBlockInput * Value;
    FAssemblyBlockOutput * Result;

protected:
    FMaterialNormalizeBlock();

    void Compute( FMaterialBuildContext & _Context ) override;
};

class FMaterialDecomposeVectorBlock : public FAssemblyBlock {
    AN_CLASS( FMaterialDecomposeVectorBlock, FAssemblyBlock )

public:
    FAssemblyBlockInput * Vector;
    FAssemblyBlockOutput * X;
    FAssemblyBlockOutput * Y;
    FAssemblyBlockOutput * Z;
    FAssemblyBlockOutput * W;

protected:
    FMaterialDecomposeVectorBlock();

    void Compute( FMaterialBuildContext & _Context ) override;
};

class FMaterialMakeVectorBlock : public FAssemblyBlock {
    AN_CLASS( FMaterialMakeVectorBlock, FAssemblyBlock )

public:
    FAssemblyBlockInput * X;
    FAssemblyBlockInput * Y;
    FAssemblyBlockInput * Z;
    FAssemblyBlockInput * W;
    FAssemblyBlockOutput * Result;

protected:
    FMaterialMakeVectorBlock();

    void Compute( FMaterialBuildContext & _Context ) override;
};

class FMaterialNegateBlock : public FAssemblyBlock {
    AN_CLASS( FMaterialNegateBlock, FAssemblyBlock )

public:
    FAssemblyBlockInput * Value;
    FAssemblyBlockOutput * Result;

protected:
    FMaterialNegateBlock();

    void Compute( FMaterialBuildContext & _Context ) override;
};

class FMaterialFractBlock : public FAssemblyBlock {
    AN_CLASS( FMaterialFractBlock, FAssemblyBlock )

public:
    FAssemblyBlockInput * Value;
    FAssemblyBlockOutput * Result;

protected:
    FMaterialFractBlock();

    void Compute( FMaterialBuildContext & _Context ) override;
};

class FMaterialSinusBlock : public FAssemblyBlock {
    AN_CLASS( FMaterialSinusBlock, FAssemblyBlock )

public:
    FAssemblyBlockInput * Value;
    FAssemblyBlockOutput * Result;

protected:
    FMaterialSinusBlock();

    void Compute( FMaterialBuildContext & _Context ) override;
};

class FMaterialCosinusBlock : public FAssemblyBlock {
    AN_CLASS( FMaterialCosinusBlock, FAssemblyBlock )

public:
    FAssemblyBlockInput * Value;
    FAssemblyBlockOutput * Result;

protected:
    FMaterialCosinusBlock();

    void Compute( FMaterialBuildContext & _Context ) override;
};

class FMaterialArithmeticBlock : public FAssemblyBlock {
    AN_CLASS( FMaterialArithmeticBlock, FAssemblyBlock )

public:
    FAssemblyBlockInput * ValueA;
    FAssemblyBlockInput * ValueB;
    FAssemblyBlockOutput * Result;

protected:
    FMaterialArithmeticBlock();

    enum EArithmeticOp {
        Add,
        Sub,
        Mul,
        Div
    };

    EArithmeticOp ArithmeticOp;

    void Compute( FMaterialBuildContext & _Context ) override;
};

class FMaterialMulBlock : public FMaterialArithmeticBlock {
    AN_CLASS( FMaterialMulBlock, FMaterialArithmeticBlock )

protected:
    FMaterialMulBlock() {
        Name = "Mul A * B";
        ArithmeticOp = Mul;
    }
};

class FMaterialDivBlock : public FMaterialArithmeticBlock {
    AN_CLASS( FMaterialDivBlock, FMaterialArithmeticBlock )

protected:
    FMaterialDivBlock() {
        Name = "Div A / B";
        ArithmeticOp = Div;
    }
};

class FMaterialAddBlock : public FMaterialArithmeticBlock {
    AN_CLASS( FMaterialAddBlock, FMaterialArithmeticBlock )

protected:
    FMaterialAddBlock() {
        Name = "Add A + B";
        ArithmeticOp = Add;
    }
};

class FMaterialSubBlock : public FMaterialArithmeticBlock {
    AN_CLASS( FMaterialSubBlock, FMaterialArithmeticBlock )

protected:
    FMaterialSubBlock() {
        Name = "Sub A - B";
        ArithmeticOp = Sub;
    }
};

class FMaterialMADBlock : public FAssemblyBlock {
    AN_CLASS( FMaterialMADBlock, FAssemblyBlock )

public:
    FAssemblyBlockInput * ValueA;
    FAssemblyBlockInput * ValueB;
    FAssemblyBlockInput * ValueC;
    FAssemblyBlockOutput * Result;

protected:
    FMaterialMADBlock();

    void Compute( FMaterialBuildContext & _Context ) override;
};

class FMaterialStepBlock : public FAssemblyBlock {
    AN_CLASS( FMaterialStepBlock, FAssemblyBlock )

public:
    FAssemblyBlockInput * ValueA;
    FAssemblyBlockInput * ValueB;
    FAssemblyBlockOutput * Result;

protected:
    FMaterialStepBlock();

    void Compute( FMaterialBuildContext & _Context ) override;
};

class FMaterialPowBlock : public FAssemblyBlock {
    AN_CLASS( FMaterialPowBlock, FAssemblyBlock )

public:
    FAssemblyBlockInput * ValueA;
    FAssemblyBlockInput * ValueB;
    FAssemblyBlockOutput * Result;

protected:
    FMaterialPowBlock();

    void Compute( FMaterialBuildContext & _Context ) override;
};

class FMaterialLerpBlock : public FAssemblyBlock {
    AN_CLASS( FMaterialLerpBlock, FAssemblyBlock )

public:
    FAssemblyBlockInput * ValueA;
    FAssemblyBlockInput * ValueB;
    FAssemblyBlockInput * ValueC;
    FAssemblyBlockOutput * Result;

protected:
    FMaterialLerpBlock();

    void Compute( FMaterialBuildContext & _Context ) override;
};

class FMaterialFloatBlock : public FAssemblyBlock {
    AN_CLASS( FMaterialFloatBlock, FAssemblyBlock )

public:
    FAssemblyBlockOutput * OutValue;
    float Value;

protected:
    FMaterialFloatBlock();

    void Compute( FMaterialBuildContext & _Context ) override;
};

class FMaterialFloat2Block : public FAssemblyBlock {
    AN_CLASS( FMaterialFloat2Block, FAssemblyBlock )

public:
    FAssemblyBlockOutput * OutValue;
    Float2 Value;

protected:
    FMaterialFloat2Block();

    void Compute( FMaterialBuildContext & _Context ) override;
};

class FMaterialFloat3Block : public FAssemblyBlock {
    AN_CLASS( FMaterialFloat3Block, FAssemblyBlock )

public:
    FAssemblyBlockOutput * OutValue;
    Float3 Value;

protected:
    FMaterialFloat3Block();

    void Compute( FMaterialBuildContext & _Context ) override;
};

class FMaterialFloat4Block : public FAssemblyBlock {
    AN_CLASS( FMaterialFloat4Block, FAssemblyBlock )

public:
    FAssemblyBlockOutput * OutValue;
    Float4 Value;

protected:
    FMaterialFloat4Block();

    void Compute( FMaterialBuildContext & _Context ) override;
};

class FMaterialTextureSlotBlock : public FAssemblyBlock {
    AN_CLASS( FMaterialTextureSlotBlock, FAssemblyBlock )

    friend class FMaterialBuilder;

public:
    FAssemblyBlockOutput * Value;

    ETextureType TextureType;
    ETextureFilter Filter;
    ESamplerAddress AddressU;
    ESamplerAddress AddressV;
    ESamplerAddress AddressW;
    float MipLODBias;
    float Anisotropy;
    float MinLod;
    float MaxLod;

    int GetSlotIndex() const { return SlotIndex; }

protected:
    FMaterialTextureSlotBlock();

    void Compute( FMaterialBuildContext & _Context ) override;

private:
    int SlotIndex;
};

class FMaterialSamplerBlock : public FAssemblyBlock {
    AN_CLASS( FMaterialSamplerBlock, FAssemblyBlock )

public:
    enum EColorSpace {
        COLORSPACE_RGBA,
        COLORSPACE_SRGB_ALPHA,
        COLORSPACE_YCOCG
    };

    FAssemblyBlockInput * TextureSlot;
    FAssemblyBlockInput * TexCoord;
    FAssemblyBlockOutput * R;
    FAssemblyBlockOutput * G;
    FAssemblyBlockOutput * B;
    FAssemblyBlockOutput * A;
    FAssemblyBlockOutput * RGBA;

    bool bSwappedToBGR;
    EColorSpace ColorSpace;

protected:
    FMaterialSamplerBlock();

    void Compute( FMaterialBuildContext & _Context ) override;
};

class FMaterialNormalSamplerBlock : public FAssemblyBlock {
    AN_CLASS( FMaterialNormalSamplerBlock, FAssemblyBlock )

public:
    enum ENormalCompression {
        NM_XYZ              = 0,
        NM_XY               = 1,
        NM_SPHEREMAP        = 2,
        NM_STEREOGRAPHIC    = 3,
        NM_PARABOLOID       = 4,
        NM_QUARTIC          = 5,
        NM_FLOAT            = 6,
        NM_DXT5             = 7
    };

    FAssemblyBlockInput * TextureSlot;
    FAssemblyBlockInput * TexCoord;
    FAssemblyBlockOutput * X;
    FAssemblyBlockOutput * Y;
    FAssemblyBlockOutput * Z;
    FAssemblyBlockOutput * XYZ;

    ENormalCompression Compression;

protected:
    FMaterialNormalSamplerBlock();

    void Compute( FMaterialBuildContext & _Context ) override;
};

class FMaterialInFragmentCoordBlock : public FAssemblyBlock {
    AN_CLASS( FMaterialInFragmentCoordBlock, FAssemblyBlock )

public:

protected:
    FMaterialInFragmentCoordBlock();

    void Compute( FMaterialBuildContext & _Context ) override;
};

class FMaterialInPositionBlock : public FAssemblyBlock {
    AN_CLASS( FMaterialInPositionBlock, FAssemblyBlock )

public:
    FAssemblyBlockOutput * Value;

protected:
    FMaterialInPositionBlock();

    void Compute( FMaterialBuildContext & _Context ) override;
};

class FMaterialInColorBlock : public FAssemblyBlock {
    AN_CLASS( FMaterialInColorBlock, FAssemblyBlock )

public:
    FAssemblyBlockOutput * Value;

protected:
    FMaterialInColorBlock();

    void Compute( FMaterialBuildContext & _Context ) override;
};

class FMaterialInTexCoordBlock : public FAssemblyBlock {
    AN_CLASS( FMaterialInTexCoordBlock, FAssemblyBlock )

public:

protected:
    FMaterialInTexCoordBlock();

    void Compute( FMaterialBuildContext & _Context ) override;
};

#if 0
class FMaterialInLightmapTexCoordBlock : public FAssemblyBlock {
    AN_CLASS( FMaterialInLightmapTexCoordBlock, FAssemblyBlock )

public:

protected:
    FMaterialInLightmapTexCoordBlock();

    void Compute( FMaterialBuildContext & _Context ) override;
};
#endif

class FMaterialInTimerBlock : public FAssemblyBlock {
    AN_CLASS( FMaterialInTimerBlock, FAssemblyBlock )

public:

protected:
    FMaterialInTimerBlock();

    void Compute( FMaterialBuildContext & _Context ) override;
};

class FMaterialInViewPositionBlock : public FAssemblyBlock {
    AN_CLASS( FMaterialInViewPositionBlock, FAssemblyBlock )

public:

protected:
    FMaterialInViewPositionBlock();

    void Compute( FMaterialBuildContext & _Context ) override;
};

class FMaterialCondLessBlock : public FAssemblyBlock {
    AN_CLASS( FMaterialCondLessBlock, FAssemblyBlock )

public:
    FAssemblyBlockInput * ValueA;
    FAssemblyBlockInput * ValueB;
    FAssemblyBlockInput * True;
    FAssemblyBlockInput * False;
    FAssemblyBlockOutput * Result;

protected:
    FMaterialCondLessBlock();

    void Compute( FMaterialBuildContext & _Context ) override;
};

// TODO: add greater, lequal, gequal, equal, not equal

class FMaterialBuilder : public FBaseObject {
    AN_CLASS( FMaterialBuilder, FBaseObject )

public:
    TRefHolder< FMaterialStageBlock > VertexStage;
    TRefHolder< FMaterialStageBlock > FragmentStage;
    EMaterialType MaterialType;

    void RegisterTextureSlot( FMaterialTextureSlotBlock * _Slot );

    FMaterial * Build();

protected:
    TPodArray< FMaterialTextureSlotBlock * > TextureSlots;

    FMaterialBuilder();
    ~FMaterialBuilder();

    FString SamplersString() const;
};

class FMaterialProject : public FBaseObject {
    AN_CLASS( FMaterialProject, FBaseObject )

public:
    template< typename T >
    T * NewBlock() {
        FAssemblyBlock * block = NewObject< T >();
        Blocks.Append( block );
        block->AddRef();
        return static_cast< T * >( block );
    }

    int Serialize( FDocument & _Doc ) override;

protected:
    TPodArray< FAssemblyBlock * > Blocks;

    FMaterialProject();

    ~FMaterialProject();
};
