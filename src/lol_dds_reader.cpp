#include "SOIL.h"
#include "lol_reader.h"
#include <stdlib.h>
#include <stdio.h>

texinfo * loadDds(const char *dds)
{
	if(!dds)
		return NULL;
	texinfo *g_tex = (texinfo *)malloc(sizeof(texinfo));
	g_tex -> data = SOIL_load_image((char *)dds, &g_tex -> w, &g_tex -> h, &g_tex -> channel, SOIL_LOAD_AUTO);
	if(g_tex -> data == NULL)
	{
		fprintf(stderr, "Unable load dds file.\n");
		free(g_tex);
		return NULL;
	}

	GLenum format = 0;
	switch(g_tex -> channel)
	{
		case SOIL_LOAD_L:
			format = GL_LUMINANCE;
			break;
		case SOIL_LOAD_LA:
			format = GL_LUMINANCE_ALPHA;
			break;
		case SOIL_LOAD_RGB:
			format = GL_RGB;
			break;
		case SOIL_LOAD_RGBA:
			format = GL_RGBA;
			break;
		default:
			break;
	}
	if(format != 0)
	{
		// SOIL_load_OGL_texture()
		glGenTextures(1, &(g_tex -> texid));
		glBindTexture(GL_TEXTURE_2D, g_tex -> texid);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);	
		glTexImage2D(GL_TEXTURE_2D, 0, format, g_tex -> w, g_tex -> h, 0, format, GL_UNSIGNED_BYTE, g_tex -> data);

		glBindTexture(GL_TEXTURE_2D, 0);

		free(g_tex -> data);
		g_tex -> data = NULL;

		printf("dds %d: width -> %d, height -> %d, channel -> %d(%s)\n", g_tex -> texid, g_tex -> w, g_tex -> h, g_tex -> channel, 
				(g_tex -> channel == 1 ? "luminance" : (g_tex -> channel == 2 ? "luminance-alpha" : (g_tex -> channel == 3 ? "RGB" : "RGBA"))));
		return g_tex;
	}
	else
	{
		fprintf(stderr, "Unsupport format of this dds file.\n");
		free(g_tex -> data);
		free(g_tex);
		return NULL;
	}
}
