#include "lol_reader.h"
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <assert.h>
#include <iostream>

using namespace std;

static void gLoadSkn_v4(ifstream &in, LolSknHeader *header)
{
	in.read((char*)&header -> numMaterials, 4);
	header -> materials = (LolMaterial *)malloc(sizeof(LolMaterial) * header -> numMaterials);
	memset(header -> materials, 0, sizeof(LolMaterial) * header -> numMaterials);
	int i;
	for(i = 0; i < header -> numMaterials; i++)
	{
		in.read(header -> materials[i].nameMat, 64);
		in.read((char*)&header -> materials[i].startVertex, 4);
		in.read((char*)&header -> materials[i].numVertices, 4);
		in.read((char*)&header -> materials[i].startIndex, 4);
		in.read((char*)&header -> materials[i].numIndices, 4);
	}
	in.seekg(4, ios::cur);
	in.read((char*)&header -> numOfIndices, 4);
	in.read((char*)&header -> numOfVertices, 4);
	in.seekg(2 * 24, ios::cur);
	header -> indices = (int16 *)malloc(sizeof(int16) * header -> numOfIndices);
	in.read((char *)header -> indices, sizeof(int16) * header -> numOfIndices);
	header -> verteces = (LolVertex *)malloc(sizeof(LolVertex) * header -> numOfVertices);
	in.read((char *)header -> verteces, sizeof(LolVertex) * header -> numOfVertices);
}

static void gLoadSkn_v1(ifstream &in, LolSknHeader *header)
{
	in.read((char*)&header -> numMaterials, 4);
	header -> materials = (LolMaterial *)malloc(sizeof(LolMaterial) * header -> numMaterials);
	memset(header -> materials, 0, sizeof(LolMaterial) * header -> numMaterials);
	int i;
	for(i = 0; i < header -> numMaterials; i++)
	{
		in.read(header -> materials[i].nameMat, 64);
		in.read((char*)&header -> materials[i].startVertex, 4);
		in.read((char*)&header -> materials[i].numVertices, 4);
		in.read((char*)&header -> materials[i].startIndex, 4);
		in.read((char*)&header -> materials[i].numIndices, 4);
	}
	in.read((char*)&header -> numOfIndices, 4);
	in.read((char*)&header -> numOfVertices, 4);
	header -> indices = (int16 *)malloc(sizeof(int16) * header -> numOfIndices);
	in.read((char *)header -> indices, sizeof(int16) * header -> numOfIndices);
	header -> verteces = (LolVertex *)malloc(sizeof(LolVertex) * header -> numOfVertices);
	in.read((char *)header -> verteces, sizeof(LolVertex) * header -> numOfVertices);
}

static void gLoadSkn_v0(ifstream &in, LolSknHeader *header)
{
	header -> numMaterials = 0;
	header -> materials = NULL;
	in.read((char*)&header -> numOfIndices, 4);
	in.read((char*)&header -> numOfVertices, 4);
	header -> indices = (int16 *)malloc(sizeof(int16) * header -> numOfIndices);
	in.read((char *)header -> indices, sizeof(int16) * header -> numOfIndices);
	header -> verteces = (LolVertex *)malloc(sizeof(LolVertex) * header -> numOfVertices);
	in.read((char *)header -> verteces, sizeof(LolVertex) * header -> numOfVertices);
}

LolSknHeader * loadSkn(const char *file)
{
	if(!file)
		return NULL;
	ifstream in(file, ios::in | ios::binary);
	if(!in.is_open())
	{
		cerr << "Unable to load skn file." << endl;
		return NULL;
	}

	LolSknHeader *header = (LolSknHeader*) malloc(sizeof(LolSknHeader));
	memset(header, 0, sizeof(LolSknHeader));
	in.read((char*)&header -> magic, 4);
	in.read((char*)&header -> version, 2);
	in.read((char*)&header -> numObjects, 2);
	assert(header -> numObjects == 1 && "matHeader == else not implimented!");
	printf("skn version -> %d\n", header -> version);
	if(header -> version == 1 || header -> version == 2 || header -> version == 3)
		gLoadSkn_v1(in, header);
	else if(header -> version == 0)
		gLoadSkn_v0(in, header);
	else if(header -> version == 4)
		gLoadSkn_v4(in, header);
	else
	{
		fprintf(stderr, "Unsupported skn version -> %d\n", header -> version);
		free(header);
		header = NULL;
	}
	/*
	cout<<"§§§"<<header->numMaterials<<endl;
		 for(int i = 0; i < header -> numOfVertices; i++)
		 {
		 if(header -> verteces[i].weights[0] + 
		 header -> verteces[i].weights[1] + 
		 header -> verteces[i].weights[2] + 
		 header -> verteces[i].weights[3] < 1)
		 {
			 cout<<i<<"____";

		 cout<<header -> verteces[i].weights[0]<<"_"<<
		 header -> verteces[i].weights[1]<<"_"<< 
		 header -> verteces[i].weights[2]<<"_"<<
		 header -> verteces[i].weights[3];
			 cout<<"______"<<"_"<<(int)header -> verteces[i].boneldx[0]<<" ";
		 cout<<"_"<<(int)header -> verteces[i].boneldx[1]<<" ";
		 cout<<"_"<<(int)header -> verteces[i].boneldx[2]<<" ";
		 cout<<"_"<<(int)header -> verteces[i].boneldx[3]<<" "<<endl;
		 }
	//cout<<endl;
	}
		 */
	int k;
	for (k = 0; k < header -> numOfVertices; k++)
	{
		LolVertex &i = header -> verteces[k];
		const float totalWeight = i.weights[0] + i.weights[1] + i.weights[2] + i.weights[3];
		const float weightError = 1.0f - totalWeight;

		if (weightError != 0.0f)
		{
			for (int j = 0; j < 4; ++j)
			{
				if (i.weights[j] > 0.0f)
				{
					i.weights[j] += (i.weights[j] / totalWeight) * weightError;
				}
			}	
		}
	}
	in.close();
	return header;
}

void freeSkn(LolSknHeader *headerPtr)
{
	if(headerPtr == 0)
		return;
	if(headerPtr -> materials)
		free(headerPtr -> materials);
	if(headerPtr -> indices)
		free(headerPtr -> indices);
	if(headerPtr -> verteces)
		free(headerPtr -> verteces);
	free(headerPtr);
}

