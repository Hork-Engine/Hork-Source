version "1"
MaterialType "PBR"
AllowScreenSpaceReflections "false"
$Color "=(0 0 0)"
$Metallic "=0"
$Roughness "=1"
$Emissive "emission_color"
textures
[
	{
		id "tex_emission"
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
		id "emission_color"
		$TexCoord "tc"
		$Texture "tex_emission"
	}
]
