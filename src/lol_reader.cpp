#include "lol_reader.h"
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <assert.h>
#include <iostream>

using namespace std;

namespace lol
{

	LolSklHeader * loadSkl(char *file)
	{
		ifstream in(file, ios::in | ios::binary);
		if(!in.is_open())
		{
			cerr << "Unable to load skl file." << endl;
			return NULL;
		}
		LolSklHeader *sklPtr = (LolSklHeader*)malloc(sizeof(LolSklHeader));
		memset(sklPtr, 0, sizeof(LolSklHeader));
		in.read(sklPtr -> version, 8);
		in.read((char*)&sklPtr -> numObjects, 4);
		in.read((char*)&sklPtr -> skeletonHash, 4);
		in.read((char*)&sklPtr -> numElements, 4);
		sklPtr -> bonesArray = (LolSklBone *)malloc(sizeof(LolSklBone) * sklPtr -> numElements);
		in.read((char*)sklPtr -> bonesArray, sizeof(LolSklBone) * sklPtr -> numElements);
		in.close();
		return sklPtr;
	}

	void freeSkl(LolSklHeader *sklHeadePtr)
	{
		if(!sklHeadePtr)
			return;
		if(sklHeadePtr -> bonesArray)
			free(sklHeadePtr -> bonesArray);
		free(sklHeadePtr);
	}


	LolAnmHeader * loadAnm(char *file)
	{
		ifstream in(file,ios::in | ios::binary);
		if(!in.is_open())
			return NULL;
		LolAnmHeader *anmPtr = (LolAnmHeader *)malloc(sizeof(LolAnmHeader));
		memset(anmPtr, 0, sizeof(LolAnmHeader));
		in.read((char *)anmPtr, sizeof(LolAnmHeader) - sizeof(anmPtr -> datasize));

		/*
			 data.data_size4=data.start_offset5-data.start_offset4-- 12720 bytes
			 data.data_size5=data.start_offset6-data.start_offset5-- 29296 bytes
			 data.data_size6=file_size-data.start_offset6-- 36900 bytes
			 */

		anmPtr -> datasize[0] = anmPtr -> offset[4] - anmPtr -> offset[3];
		anmPtr -> datasize[1] = anmPtr -> offset[5] - anmPtr -> offset[4];
		anmPtr -> datasize[2] = anmPtr -> sizeTillEnd - anmPtr -> offset[5];


		//int size = sizeof(LolAnmHeader);
		in.close();
		return anmPtr;
	}
};

