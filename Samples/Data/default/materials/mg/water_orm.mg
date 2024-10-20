version "1"
MaterialType "PBR"
AllowScreenSpaceReflections "true"
IsTranslucent "true"
Blending "Alpha"
IsTwoSided "true"
AllowScreenAmbientOcclusion "false"
$Color "albedo"
$Emissive "emissive"
$Normal "normal"
$AmbientOcclusion "ormx.Occlusion"
$Roughness "ormx.Roughness"
$Metallic "ormx.Metallic"
$Opacity "ormx.Roughness"
textures
[
	{
		id "tex_albedo"
		Filter "Trilinear"
	}
	
	{
		id "tex_emissive"
		Filter "Trilinear"
	}
	
	{
		id "tex_ormx"
		Filter "Trilinear"
	}
	
	{
		id "tex_normal"
		Filter "Trilinear"
	}
]
nodes
[
	{
		type "InTexCoord"
		id "tc"
	}
	{
		type "InTimer"
		id "timer"
	}
	{
		type "Mul"
		id "scaled_time"
		$A "timer.GameplayTimeSeconds"
		$B "=3"
	}
	{
		type "DecomposeVector"
		id "texcoord_xy_decomposed"
		$Vector "tc"
	}
	{
		type "MakeVector"
		id "texcoord_yx"
		$X "texcoord_xy_decomposed.Y"
		$Y "texcoord_xy_decomposed.X"
	}
	{
		type "MAD"
		id "sin_arg"
		$A "texcoord_yx"
		$B "=8"
		$C "scaled_time"
	}
	{
		type "Sinus"
		id "sinus"
		$Value "sin_arg"
	}
	{
		type "MAD"
		id "sample_tc"
		$A "sinus"
		$B "=0.03125" // 1/32
		$C "tc"
	}
	{
		type "TextureLoad"
		id "albedo"
		$TexCoord "sample_tc"
		$Texture "tex_albedo"
	}
	{
		type "TextureLoad"
		id "emissive"
		$TexCoord "sample_tc"
		$Texture "tex_emissive"
	}
	{
		type "NormalLoad"
		id "normal"
		Pack "RGBx"
		$TexCoord "sample_tc"
		$Texture "tex_normal"
	}
	{
		type "ORMXLoad"
		id "ormx"
		$TexCoord "sample_tc"
		$Texture "tex_ormx"
	}
]
