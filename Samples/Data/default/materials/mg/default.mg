version "1"
MaterialType "PBR"
AllowScreenSpaceReflections "false"
$Color "base_color"
$Metallic "metallic"
$Roughness "roughness"
textures
[
	{
		id "tex_base_color"
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
		id "base_color"
		$TexCoord "tc"
		$Texture "tex_base_color"
	}
	{
		type "UniformAddress"
		id "metallic"
		Address "0"
		UniformType "float"
	}
	{
		type "UniformAddress"
		id "roughness"
		Address "1"
		UniformType "float"
	}
]
