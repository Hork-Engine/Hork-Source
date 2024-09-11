version "1"
MaterialType "Unlit"
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
		type "InScreenUV"
		id "screen_uv"
	}
	{
		type "Mul"
		id "mirrored_uv"
		$A "screen_uv"
		$B "=(-1, 1)"
	}
	{
		type "TextureLoad"
		id "base_color"
		$TexCoord "mirrored_uv"
		$Texture "tex_base_color"
	}
]
