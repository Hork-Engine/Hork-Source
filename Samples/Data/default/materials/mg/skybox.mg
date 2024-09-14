version "1"
MaterialType "Unlit"
DepthHack "Skybox"
$Color "COLOR"
textures
[
	{
		id "CUBEMAP_TEXTURE"
		TextureType "Cube"
		Filter "Linear"
		AddressU "Clamp"
		AddressV "Clamp"
		AddressW "Clamp"
	}
]
nodes
[
	{
		type "InPosition"
		id "POSITION"
	}
	{
		type "TextureLoad"
		id "COLOR"
		$TexCoord "POSITION"
		$Texture "CUBEMAP_TEXTURE"
	}
]
