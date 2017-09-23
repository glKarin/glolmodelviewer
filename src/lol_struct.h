#ifndef _LOL_STRUCT_H
#define _LOL_STRUCT_H

#include "gstd.h"
#include <GL/gl.h>

namespace lol
{
	//00 type


	// skl

	// Bone ID's are implicit to their order in the file starting at 0. 
	// So the first bone has an ID of 0, second bone an ID of 1, etc. 
	struct LolSklBone
	{
		int8 name[32];			// name of bone
		int32 parent;			// parent bone id. -1 implies no parent
		float32 scale;			// scale of bone ? of armature?
		float32 matrix[12];		// 3*4 affine bone matrix in row major format(first four values
		// belong to first row ,2nd four to 2nd row ....
	};

	// ---header
	// ---bone array
	struct LolSklHeader
	{
		int8 version[8];	// file format version ==== extra 1 for '\0'
		int32 numObjects;	// num of objects
		int32 skeletonHash;	// unique id
		int32 numElements; // num of bones
		LolSklBone* bonesArray;//
	};


	// not finished - version 4
	struct LolAnmHeader
	{
		int8 name[8];
		int32 version;
		int32 sizeTillEnd;
		int32 magic3;
		int32 magic4;
		int32 magic5;
		int32 numofbones;
		int32 numofframes;
		float32 playbackFPS;
		int32 offset[9];
		int32 datasize[3];
		
	};

}
// 00 struct
struct LolVertex
{
	float32 position[3];	// xyz position of vertex
	int8 boneldx[4];		// Array of 4 bone number ID's
	float32 weights[4];		// Corresponding bone weights
	float32 normals[3];		// vertex normals
	float32 texcoords[2];	// vertex UV coordinates
};

struct LolMaterial
{
	// if matHeader == 1 && numMaerials
	int8 nameMat[64];		// name of material
	int32 startVertex;		// First vertex belonging to this material
	int32 numVertices;		// Number of vertices in this material
	int32 startIndex;		// First index belonging to this material
	int32 numIndices;		// Number of indicies belonging to this material
	//if matHeader == 0 the above block will be absent from the file. 

};

struct LolSknHeader
{
	int32 magic;
	int16 version;
	int16 numObjects;
	//---------if matHeader = 1----------------
	int32 numMaterials;	//if matHeader = 1
	//-------------------------

	//-----block
	LolMaterial *materials;
	//----count block -------------------------
	// the values obtained here should match the sums of the numIndices and numVertices for the material blocks if present 
	int32 numOfIndices;
	int32 numOfVertices;

	int16 *indices;		// Array of vertex indices
	LolVertex*verteces;	// Array of vertex values
};

struct texinfo
{
	unsigned char* data;
	int w;
	int h;
	int channel;
	GLuint texid;
};

#include <OpenEXR/ImathMatrix.h>
#include <OpenEXR/ImathVec.h>
#include <OpenEXR/ImathQuat.h>

using Imath::Vec3;
using Imath::Matrix44;
using Imath::Quat;

using namespace lol;

struct SklBone
{
	char name[32];
	unsigned int hash;
	int parent;
	Vec3<float> scale;
	Matrix44<float> localMatrix;
	Matrix44<float> globalMatrix;
	Matrix44<float> offsetMatrix;
};

struct BoneHash
{
	unsigned int key;
	char value[32];
};

struct SklHeader
{
	short sknVersion;
	int fileVersion;
	int version;
	int numBones;
	SklBone *bones;
	int numBoneIndices;
	int *boneIndices;
	BoneHash *boneHashes;
};

struct AnmData
{
	float32 rotation[4]; // x y z w
	float32 translation[3];
	float32 scale[3];
};

struct AnmBone
{
	char boneName[32];
	int32 boneType; //2 root 0 normal
	AnmData *anms;
};

struct AnmHeader
{
	char name[8];
	int32 version;
	int32 unknow;
	int32 boneCount;
	int32 frameCount;
	float32 fps;
	AnmBone *anmBones;
};

#endif
