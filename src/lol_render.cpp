#include "lol_render.h"
#include "imath_ext.h"
#include <OpenEXR/ImathVec.h>
#include <OpenEXR/ImathGL.h>
#include <GL/gl.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>

using namespace std;

void updateBone(SklHeader *skl, const AnmHeader *anm, int frame)
{
	if(!skl || !anm || frame < 0 || frame >= anm -> frameCount)
	{
		cerr << "Unable to update skeleton." << endl;
		return;
	}
	int i;
	//cout<<skl -> numBones<<"___"<<anm->boneCount<<endl;
	for(i = 0; i < skl -> numBones; i++)
	{
		int j;
		bool hasBone = false;
		for(j = 0; j < anm -> boneCount; j++)
			if(strcmp(skl -> bones[i].name, anm -> anmBones[j].boneName) == 0)
			{
				hasBone = true;
				break;
			}
		if(!hasBone)
			continue;
		SklBone &bone = skl -> bones[i];
		const AnmData &a = anm -> anmBones[j].anms[frame];
		if(bone.parent == -1)
		{
			bone.localMatrix = g_quat_to_matrix(a.rotation, a.translation);
			bone.globalMatrix = bone.localMatrix;
		}
		else
		{
			const SklBone &pBone = skl -> bones[bone.parent];
			bone.localMatrix = g_quat_to_matrix(a.rotation, a.translation);
			bone.globalMatrix = pBone.globalMatrix * bone.localMatrix;
		}
	}
}

void drawSkn(const LolSknHeader *sknHeader, GLuint texid, const SklHeader *skl, const AnmHeader *anm, int frame)
{
	if(!sknHeader || !skl)
	{
		cerr << "Unable to draw animation model." << endl;
		return;
	}
	LolVertex *verteces = (LolVertex *)malloc(sizeof(LolVertex) * sknHeader -> numOfVertices);
	memcpy(verteces, sknHeader -> verteces, sizeof(LolVertex) * sknHeader -> numOfVertices);
	if(texid != 0)
	{
		glBindTexture(GL_TEXTURE_2D,texid);
	}
	//updateBone(skl, anm, frame);
	int i;
	for(i = 0; i < sknHeader -> numOfVertices; i++)
	{
		LolVertex &vertex = verteces[i];
		const Vec3<float> v(vertex.position[0], vertex.position[1], vertex.position[2]);
		const Vec3<float> n(vertex.normals[0], vertex.normals[1], vertex.normals[2]);
		Vec3<float> nv(0.0);
		Vec3<float> nn(0.0);
		unsigned int j;
		for(j = 0; j < sizeof(vertex.boneldx) / sizeof(int8); j++)
		{
			if(vertex.weights[j] == 0.0) continue;
			unsigned int idx = (unsigned int)vertex.boneldx[j];
			if(idx < skl -> numBoneIndices)
				idx = skl -> boneIndices[idx];
			/*
			if(idx >= skl -> numBones)
				continue;
			*/

			Matrix44<float> mat = g_transpose_matrix(skl -> bones[idx].offsetMatrix);
			Matrix44<float> mat2 = g_transpose_matrix(skl -> bones[idx].globalMatrix);
			nv += (v * (mat * mat2) * vertex.weights[j]);
			nn += (n * (mat * mat2) * vertex.weights[j]);
		}
		vertex.position[0] = nv.x;
		vertex.position[1] = nv.y;
		vertex.position[2] = nv.z;
		vertex.normals[0] = nn.x;
		vertex.normals[1] = nn.y;
		vertex.normals[2] = nn.z;
	}
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);		// 1.
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glNormalPointer(GL_FLOAT,sizeof(LolVertex),&(verteces[0].normals[0]));
	glVertexPointer(3,GL_FLOAT,sizeof(LolVertex),verteces);	// 2.
	glTexCoordPointer(2,GL_FLOAT,sizeof(LolVertex),&(verteces[0].texcoords[0]));

	glDrawElements(GL_TRIANGLES,sknHeader->numOfIndices,GL_UNSIGNED_SHORT,sknHeader->indices);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	if(texid != 0)
	{
		glBindTexture(GL_TEXTURE_2D,0);
	}
	free(verteces);
}

void drawSkn(const LolSknHeader *sknHeader, GLuint texid)
{
	if(!sknHeader)
	{
		cerr << "Unable to draw model." << endl;
		return;
	}
	if(texid != 0)
	{
		glBindTexture(GL_TEXTURE_2D,texid);
	}
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);		// 1.
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glNormalPointer(GL_FLOAT,sizeof(LolVertex),&(sknHeader->verteces[0].normals[0]));
	glVertexPointer(3,GL_FLOAT,sizeof(LolVertex),sknHeader->verteces);	// 2.
	glTexCoordPointer(2,GL_FLOAT,sizeof(LolVertex),&(sknHeader->verteces[0].texcoords[0]));
	glDrawElements(GL_TRIANGLES,sknHeader->numOfIndices,GL_UNSIGNED_SHORT,sknHeader->indices);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	if(texid != 0)
	{
		glBindTexture(GL_TEXTURE_2D,0);
	}
}

void drawBone(const SklHeader *skl)
{
	if(!skl)
	{
		cerr << "Unable to draw skeleton." << endl;
		return;
	}
	//updateBone(skl, anm, frame);
	glColor4f(0.0f, 0.0f, 0.0f, 0.9);
	glPointSize(5.0);
	const SklBone *bones = skl -> bones;
	Vec3<float> vertexs[skl -> numBones];
	for(int i = 0; i < skl -> numBones; i++)
	{
		glPushMatrix();
		//glMultTransposeMatrixf((GLfloat *)(skl -> bones[i].globalMatrix.x));
		glBegin(GL_POINTS);

		Vec3<float> v(1.0f, 1.0f, 1.0f);
		Matrix44<float> tm = g_transpose_matrix(skl -> bones[i].globalMatrix);
		v = v * tm;
		glVertex3f(v.x, v.y, v.z);

		glEnd();
		vertexs[i] = v;
		glPopMatrix();
	}

	glLineWidth(2.0);
	glColor4f(1.0, 0.0, 0.0, 0.9);
	for(int i = 0; i < skl -> numBones; i++)
	{
		int *indexs = 0;
		int count = 0;
		for(int j = 0; j < skl -> numBones; j++)
		{
			if(bones[j].parent == i)
				count++;
		}
		if(count == 0)
			continue;
		indexs = (int *)malloc(sizeof(int) * count);
		memset(indexs, 0, sizeof(int) * count);
		int k = 0;
		for(int j = 0; j < skl -> numBones; j++)
		{
			if(bones[j].parent == i)
			{
				indexs[k] = j;
				k++;
			}
		}
		Vec3<float> v = vertexs[i];
		for(int m = 0; m < count; m++)
		{
			Vec3<float> v2 = vertexs[indexs[m]];
			glPushMatrix();
			glBegin(GL_LINES);

				glVertex3f(v.x, v.y, v.z);
				glVertex3f(v2.x, v2.y, v2.z);

			glEnd();
			glPopMatrix();
		}
		free(indexs);
	}
}

void drawSkn(const LolSknHeader *sknHeader, const GLuint *texid, const SklHeader *skl, const AnmHeader *anm, int frame)
{
	if(!sknHeader || !skl)
	{
		cerr << "Unable to draw animation model." << endl;
		return;
	}
	LolVertex *verteces = (LolVertex *)malloc(sizeof(LolVertex) * sknHeader -> numOfVertices);
	memcpy(verteces, sknHeader -> verteces, sizeof(LolVertex) * sknHeader -> numOfVertices);

	//updateBone(skl, anm, frame);
	int i;
	for(i = 0; i < sknHeader -> numOfVertices; i++)
	{
		LolVertex &vertex = verteces[i];
		const Vec3<float> v(vertex.position[0], vertex.position[1], vertex.position[2]);
		const Vec3<float> n(vertex.normals[0], vertex.normals[1], vertex.normals[2]);
		Vec3<float> nv(0.0);
		Vec3<float> nn(0.0);
		unsigned int j;
		for(j = 0; j < sizeof(vertex.boneldx) / sizeof(int8); j++)
		{
			if(vertex.weights[j] == 0.0) continue;
			unsigned int idx = (unsigned int)vertex.boneldx[j];
			if(idx < skl -> numBoneIndices)
				idx = skl -> boneIndices[idx];
			/*
			if(idx >= skl -> numBones)
				continue;
			*/

			Matrix44<float> mat = g_transpose_matrix(skl -> bones[idx].offsetMatrix);
			Matrix44<float> mat2 = g_transpose_matrix(skl -> bones[idx].globalMatrix);
			nv += (v * (mat * mat2) * vertex.weights[j]);
			nn += (n * (mat * mat2) * vertex.weights[j]);
		}
		vertex.position[0] = nv.x;
		vertex.position[1] = nv.y;
		vertex.position[2] = nv.z;
		vertex.normals[0] = nn.x;
		vertex.normals[1] = nn.y;
		vertex.normals[2] = nn.z;
	}

	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);		// 1.
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glNormalPointer(GL_FLOAT,sizeof(LolVertex),&(verteces[0].normals[0]));
	glVertexPointer(3,GL_FLOAT,sizeof(LolVertex),verteces);	// 2.
	glTexCoordPointer(2,GL_FLOAT,sizeof(LolVertex),&(verteces[0].texcoords[0]));

	for(i = 0; i < sknHeader -> numMaterials; i++)
	{
		if(texid && texid[i] != 0)
			glBindTexture(GL_TEXTURE_2D,texid[i]);
		glDrawElements(GL_TRIANGLES, sknHeader -> materials[i].numIndices, GL_UNSIGNED_SHORT, sknHeader -> indices + sknHeader -> materials[i].startIndex);
		if(texid && texid[i] != 0)
			glBindTexture(GL_TEXTURE_2D,0);
	}

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	free(verteces);
}

void drawSkn(const LolSknHeader *sknHeader, const GLuint *texid)
{
	if(!sknHeader)
	{
		cerr << "Unable to draw model." << endl;
		return;
	}

	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);		// 1.
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glNormalPointer(GL_FLOAT,sizeof(LolVertex),&(sknHeader->verteces[0].normals[0]));
	glVertexPointer(3,GL_FLOAT,sizeof(LolVertex),sknHeader->verteces);	// 2.
	glTexCoordPointer(2,GL_FLOAT,sizeof(LolVertex),&(sknHeader->verteces[0].texcoords[0]));
	int i;
	for(i = 0; i < sknHeader -> numMaterials; i++)
	{
		if(texid && texid[i] != 0)
			glBindTexture(GL_TEXTURE_2D,texid[i]);
		glDrawElements(GL_TRIANGLES, sknHeader -> materials[i].numIndices, GL_UNSIGNED_SHORT, sknHeader -> indices + sknHeader -> materials[i].startIndex);
		if(texid && texid[i] != 0)
			glBindTexture(GL_TEXTURE_2D,0);
	}


	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}
