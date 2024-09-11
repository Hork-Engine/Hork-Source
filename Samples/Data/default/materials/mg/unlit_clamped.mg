version "1"
MaterialType "Unlit"
$Color "result_color"
textures
[
	{
		id "tex_base_color"
		Filter "Trilinear"
		AddressU "Clamp"
		AddressV "Clamp"
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
		type "Mul"
		id "result_color"
		$A "base_color"
		$B "=0.4"
	}
]
