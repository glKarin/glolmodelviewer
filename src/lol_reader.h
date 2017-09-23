#ifndef _G_LOL_READER_H
#define _G_LOL_READER_H

#include "lol_struct.h"
#include <string.h>

#define READ_UNKNOW_FLOAT(in) {float uk = 0; in.read((char *)&uk, 4); cout << uk <<endl;}
#define READ_UNKNOW_INT(in) {int uk = 0; in.read((char *)&uk, 4); cout << uk <<endl;}
#define READ_UNKNOW_SHORT(in) {short uk = 0; in.read((char *)&uk, 2); cout << uk <<endl;}
#define READ_UNKNOW_CHAR(in) {char uk = 0; in.read((char *)&uk, 1); cout << (int)uk <<endl;}

namespace lol
{
	// interface

	LolSklHeader * loadSkl(char *file);
	void freeSkl(LolSklHeader *skl);

	LolAnmHeader * loadAnm(char *file);

}  // end namespace

LolSknHeader * loadSkn(const char *file);
void freeSkn(LolSknHeader *skn);

texinfo * loadDds(const char *file);

SklHeader * gLoadSkl(const char *file, int version);
AnmHeader * gLoadAnm(const char *file, const SklHeader *skl);

void gFreeSkl(SklHeader *skl);
void gFreeAnm(AnmHeader *anm);

inline unsigned int StringToHash(const char *s)
{
	unsigned int hash = 0;
	unsigned int temp = 0;

	for(unsigned int i = 0; i < strlen(s); i++) // for (auto& c : s)
	{
		hash = (hash << 4) + tolower(s[i]);
		temp = hash & 0xf0000000;

		if (temp != 0)
		{
			hash = hash ^ (temp >> 24);
			hash = hash ^ temp;
		}
	}

	return hash;
}

#endif
