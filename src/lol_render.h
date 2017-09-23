#ifndef _LOL_RENDER_H
#define _LOL_RENDER_H

#include "lol_struct.h"
#include <GL/gl.h>

void drawSkn(const LolSknHeader *sknHeader, GLuint texid);

void drawSkn(const LolSknHeader *sknHeader, GLuint texid, const SklHeader *skl, const AnmHeader *anm, int frame);
void drawBone(const SklHeader *skl);
void updateBone(SklHeader *skl, const AnmHeader *anm, int frame);

void drawSkn(const LolSknHeader *sknHeader, const GLuint *texid);

void drawSkn(const LolSknHeader *sknHeader, const GLuint *texid, const SklHeader *skl, const AnmHeader *anm, int frame);
#endif
