#include "lol_reader.h"
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <assert.h>
#include <iostream>

using namespace std;

static void gLoadSkl_v1953262451(ifstream &in, SklHeader *sklHeader)
{
	in.read((char *)(&sklHeader -> version), 4);
	printf("skl version -> %d\n", sklHeader -> version);
	in.seekg(4, ios_base::cur);
	in.read((char *)(&sklHeader -> numBones), 4);

	sklHeader -> bones = (SklBone *)malloc(sklHeader -> numBones * sizeof(SklBone));
	memset(sklHeader -> bones, 0, sizeof(SklBone) * sklHeader -> numBones);

	sklHeader -> boneHashes = (BoneHash *)malloc(sizeof(BoneHash) * sklHeader -> numBones);
	memset(sklHeader -> boneHashes, 0, sizeof(BoneHash) * sklHeader -> numBones);
	for (int i = 0; i < sklHeader -> numBones; i++)
	{
		in.read((char *)(&sklHeader -> bones[i].name), 32);
		sklHeader -> bones[i].hash = StringToHash(sklHeader -> bones[i].name);
		//boneHashes[bones.at(i).hash] = bones.at(i).name;
		sklHeader -> boneHashes[i].key = sklHeader -> bones[i].hash;
		strcpy(sklHeader -> boneHashes[i].value, sklHeader -> bones[i].name);
		in.read((char *)(&sklHeader -> bones[i].parent), 4);
		in.read((char *)(&sklHeader -> bones[i].scale.x), 4);
		sklHeader -> bones[i].scale.x *= 10.0f;
		sklHeader -> bones[i].scale.y = sklHeader -> bones[i].scale.z = sklHeader -> bones[i].scale.x;
		in.read((char *)(&sklHeader -> bones[i].globalMatrix.x), 48);

		sklHeader -> bones[i].globalMatrix[3][0] = sklHeader -> bones[i].globalMatrix[3][1] = sklHeader -> bones[i].globalMatrix[3][2] = 0.0f;
		sklHeader -> bones[i].globalMatrix[3][3] = 1.0f;
	}

	for (int n = 0; n < sklHeader -> numBones; n++)
	{
		SklBone &i = sklHeader -> bones[n]; 
		Matrix44<float> relativeMatrix, inverseMatrix;

		if (i.parent != -1)
		{
			inverseMatrix = sklHeader -> bones[i.parent].globalMatrix.gjInverse();
			i.localMatrix = inverseMatrix * i.globalMatrix;
		}
		else
		{
			i.localMatrix = i.globalMatrix;
		}
		i.offsetMatrix = i.globalMatrix.gjInverse();
	}

	if (sklHeader -> sknVersion == 0 || sklHeader -> sknVersion == 1)
	{
		sklHeader -> numBoneIndices = sklHeader -> numBones;
		sklHeader -> boneIndices = (int *)malloc(sklHeader -> numBoneIndices * sizeof(int));
		memset(sklHeader -> boneIndices, 0, sklHeader -> numBoneIndices * sizeof(int));

		for (int i = 0; i < sklHeader -> numBoneIndices; i++)
		{
			sklHeader -> boneIndices[i] = i;
		}
	}
	else if (sklHeader -> sknVersion == 2)
	{
		in.read((char *)(&sklHeader -> numBoneIndices), 4);

		sklHeader -> boneIndices = (int *)malloc(sklHeader -> numBoneIndices * sizeof(int));
		memset(sklHeader -> boneIndices, 0, sklHeader -> numBoneIndices * sizeof(int));

		for (int i = 0; i < sklHeader -> numBoneIndices; i++)
		{
			in.read((char*)(&sklHeader -> boneIndices[i]), 4);
		}
	}
}

static void gLoadSkl_v587026371(ifstream &in, SklHeader *sklHeader)
{
	in.read((char *)(&sklHeader -> version), 4);
	printf("skl version -> %d\n", sklHeader -> version);
	in.seekg(2, ios_base::cur);
	in.read((char *)(&sklHeader -> numBones), 2);
	sklHeader -> numBones = short(sklHeader -> numBones);
	sklHeader -> bones = (SklBone *)malloc(sklHeader -> numBones * sizeof(SklBone));
	memset(sklHeader -> bones, 0, sizeof(SklBone) * sklHeader -> numBones);

	in.read((char *)(&sklHeader -> numBoneIndices), 4);
	sklHeader -> boneIndices = (int *)malloc(sklHeader -> numBoneIndices * sizeof(int));
	memset(sklHeader -> boneIndices, 0, sizeof(int) * sklHeader -> numBoneIndices);

	short dataOffset;
	in.read((char *)(&dataOffset), 2);
	in.seekg(2, ios_base::cur);

	int boneIndicesOffset, boneNamesOffset;
	in.seekg(4, ios_base::cur);
	in.read((char *)(&boneIndicesOffset), 4);
	in.seekg(8, ios_base::cur);
	in.read((char *)(&boneNamesOffset), 4);

	in.seekg(dataOffset, ios_base::beg);

	sklHeader -> boneHashes = (BoneHash *)malloc(sizeof(BoneHash) * sklHeader -> numBones);
	memset(sklHeader -> boneHashes, 0, sizeof(BoneHash) * sklHeader -> numBones);
	for (int i = 0; i < sklHeader -> numBones; i++)
	{
		in.seekg(2, ios_base::cur);
		short boneId;
		in.read((char *)(&boneId), 2);

		if (boneId != i)
		{
			cerr << "Read error, unexpected identification value." << endl;
		}

		in.read((char *)(&sklHeader -> bones[i].parent), 2);
		sklHeader -> bones[i].parent = (short)sklHeader -> bones[i].parent;

		in.seekg(2, ios_base::cur);

		//in.seekg(4, ios_base::cur); // hash
		in.read((char *)(&sklHeader -> bones[i].hash), 4);
		sklHeader -> boneHashes[i].key = sklHeader -> bones[i].hash;

		in.seekg(4, ios_base::cur);

		float tx, ty, tz;
		in.read((char *)(&tx), 4);
		in.read((char *)(&ty), 4);
		in.read((char *)(&tz), 4);

		in.read((char *)(&sklHeader -> bones[i].scale), 12);

		Quat<float> q;

		in.read((char *)(&q.v.x), 4);
		in.read((char *)(&q.v.y), 4);
		in.read((char *)(&q.v.z), 4);
		in.read((char *)(&q.r), 4);

		sklHeader -> bones[i].localMatrix = q.toMatrix44();
		sklHeader -> bones[i].localMatrix.gjInvert();

		sklHeader -> bones[i].localMatrix[0][3] = tx;
		sklHeader -> bones[i].localMatrix[1][3] = ty;
		sklHeader -> bones[i].localMatrix[2][3] = tz;

		sklHeader -> bones[i].localMatrix[3][0] = sklHeader -> bones[i].localMatrix[3][1] = sklHeader -> bones[i].localMatrix[3][2] = 0.0f;
		sklHeader -> bones[i].localMatrix[3][3] = 1.0f;

		in.seekg(44, ios_base::cur);
	}

	for (int n = 0; n < sklHeader -> numBones; n++)
	{
		SklBone &i = sklHeader -> bones[n];
		if (i.parent != -1)
		{
			i.globalMatrix = sklHeader -> bones[i.parent].globalMatrix * i.localMatrix;
		}
		else
		{
			i.globalMatrix = i.localMatrix;
		}
		i.offsetMatrix = i.globalMatrix.gjInverse();
	}

	in.seekg(boneIndicesOffset, ios_base::beg);

	for (int i = 0; i < sklHeader -> numBoneIndices; ++i)
	{
		in.read((char *)(&sklHeader -> boneIndices[i]), 2);
		sklHeader -> boneIndices[i] = short(sklHeader -> boneIndices[i]);
	}

	in.seekg(boneNamesOffset, ios_base::beg);

	for (int i = 0; i < sklHeader -> numBones; i++)
	{
		unsigned char curChar;
		streamoff filePos;

		do
		{
			filePos = in.tellg();
			in.read((char *)(&curChar), 1);
		} while (filePos % 4 != 0);

		int curPos = 0;

		while (curChar)
		{
			sklHeader -> bones[i].name[curPos] = curChar;
			++curPos;

			if (curPos >= 31)
			{
				break;
			}

			in.read((char *)(&curChar), 1);
		}

		sklHeader -> bones[i].name[curPos] = '\0';
		int j;
		for(j = 0; j < sklHeader -> numBones; j++)
			if(sklHeader -> boneHashes[j].key == sklHeader -> bones[i].hash)
			{
				strcpy(sklHeader -> boneHashes[i].value, sklHeader -> bones[i].name);
				break;
			}
	}
}

SklHeader * gLoadSkl(const char *path, int version)
{
	if(!path)
		return NULL;
	ifstream in(path, ios::in | ios::binary);
	if (!in.is_open())
	{
		cerr << "Unable to open skeleton file." << endl;
		return NULL;
	}
	//map<unsigned int, char*> boneHashes;
	SklHeader *sklHeader = (SklHeader *)malloc(sizeof(SklHeader));
	// TODO
	sklHeader -> sknVersion = version;

	in.seekg(4, ios_base::cur);
	in.read((char *)(&sklHeader -> fileVersion), 4);

	//printf("skl version -> %d\n", sklHeader -> fileVersion);

	if (sklHeader -> fileVersion == 1953262451)
		gLoadSkl_v1953262451(in, sklHeader);
	else if (sklHeader -> fileVersion == 587026371)
		gLoadSkl_v587026371(in, sklHeader);
	else
	{
		fprintf(stderr, "Unsupported skl version -> %d\n", sklHeader -> fileVersion);
		free(sklHeader);
		return NULL;
	}

	in.close();

	return sklHeader;
}

void gFreeSkl(SklHeader *skl)
{
	if(!skl)
		return;
	if(skl -> bones)
		free(skl -> bones);
	if(skl -> boneIndices)
		free(skl -> boneIndices);
	if(skl -> boneHashes)
		free(skl -> boneHashes);
	free(skl);
}

