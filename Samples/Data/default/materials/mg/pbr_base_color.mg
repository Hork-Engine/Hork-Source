version "1"
MaterialType "PBR"
bAllowScreenSpaceReflections "false"
$Color "base_color"
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
]
