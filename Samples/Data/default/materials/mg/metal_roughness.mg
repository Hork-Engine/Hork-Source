version "1"
MaterialType "PBR"
AllowScreenSpaceReflections "false"
$Color "base_color"
$Metallic "metal_roughness.r"
$Roughness "metal_roughness.g"
textures
[
	{
		id "tex_base_color"
		Filter "Trilinear"
	}
	
	{
		id "tex_metal_roughness"
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
		type "TextureLoad"
		id "metal_roughness"
		$TexCoord "tc"
		$Texture "tex_metal_roughness"
	}
]
