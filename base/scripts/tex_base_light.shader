// From OA/Q3
textures/base_light/baslt4_1_2k
{
	qer_editorimage textures/base_light/baslt4_1.jpg
	q3map_lightimage textures/base_light/baslt4_1.blend.jpg
	surfaceparm nomarks
	q3map_surfacelight 2000
	
	
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/base_light/baslt4_1.jpg
		blendFunc filter
		rgbGen identity
	}
	{
		map textures/base_light/baslt4_1.blend.jpg
		blendfunc add
	}
}

textures/base_light/baslt4_1_4k
{
	qer_editorimage textures/base_light/baslt4_1.jpg
	q3map_lightimage textures/base_light/baslt4_1.blend.jpg
	surfaceparm nomarks
	q3map_surfacelight 4000
	
	
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/base_light/baslt4_1.jpg
		blendFunc filter
		rgbGen identity
	}
	{
		map textures/base_light/baslt4_1.blend.jpg
		blendfunc add
	}
}

// Shader used for lights with flares in OpenArena's dm6ish.bsp ...
textures/base_light/ceil1_4
{
	qer_editorimage textures/base_light/ceil1_4.tga
	q3map_lightimage textures/base_light/ceil1_4.blend.tga
	surfaceparm metalsteps
	q3map_surfacelight 1000
	q3map_flare flareShader-wide
	{
		map textures/base_light/ceil1_4.tga
		rgbGen identity
	}
	{
		map $lightmap 
		blendfunc filter
		tcGen lightmap 
	}
	{
		map textures/base_light/ceil1_4.blend.tga
		blendfunc add
		rgbGen wave noise 0.8 0.2 0 1
	}
}


