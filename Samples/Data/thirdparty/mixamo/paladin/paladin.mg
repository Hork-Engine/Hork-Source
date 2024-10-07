version "1"
MaterialType "PBR"
AllowScreenSpaceReflections "true"
$Color "color"
$Metallic "=0"
$Roughness "roughness"
$Normal "normalmap"
textures
[
	{
		id "tex_base_color"
		Filter "Trilinear"
	}

	{
		id "tex_normalmap"
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
		type "NormalLoad"
		id "normalmap"
		Pack "RGBx"
		$TexCoord "tc"
		$Texture "tex_normalmap"
	}
	{
		type "Add"
		id "roughness"
		$A "base_color.r"
		$B "=0.4"
	}
	{
		type "Add"
		id "color"
		$A "base_color"
		$B "=(0.02 0.02 0.02)"
	}
]
