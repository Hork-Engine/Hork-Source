version "1"
MaterialType "PBR"
AllowScreenSpaceReflections "true"
$Color "albedo"
$Emissive "emissive"
$Normal "normal"
$AmbientOcclusion "ormx.Occlusion"
$Roughness "ormx.Roughness"
$Metallic "ormx.Metallic"
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
		type "TextureLoad"
		id "albedo"
		$TexCoord "tc"
		$Texture "tex_albedo"
	}
	{
		type "TextureLoad"
		id "emissive"
		$TexCoord "tc"
		$Texture "tex_emissive"
	}
	{
		type "NormalLoad"
		id "normal"
		Pack "RGBx"
		$TexCoord "tc"
		$Texture "tex_normal"
	}
	{
		type "ORMXLoad"
		id "ormx"
		$TexCoord "tc"
		$Texture "tex_ormx"
	}
]
