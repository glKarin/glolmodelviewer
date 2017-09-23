#include "lol_reader.h"
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <assert.h>
#include <iostream>

#include <unordered_set>
#include <bitset>
#include <vector>
#include <list>
#include <map>

#include <OpenEXR/ImathFun.h>

using namespace std;
/*
static void itoa2(int value, char *s)
{
	int a = value % 2;
	value = value >> 1;
	char *p = s;
	if(value != 0)
		itoa2(value, --s);
	*p = (a == 1 ? '1' : '0');
	cout<<*p<<endl;
}
static void toBinaryString(char *dst, uint16 value, size_t len)
{
	char s[len + 1];
	int i;
	for(i = 0; i < len; i++)
		s[i] = '0';
	s[i] = '\0';
	itoa2(value, s + strlen(s) - 1);
	cout<<s<<endl;
	strncpy(dst, s, len);
}
*/

struct FrameIndices
{
	unsigned short translationIndex;
	unsigned short quaternionIndex;
	unsigned short scaleIndex;
};

static Quat<float> uncompressQuaternion(const unsigned short& flag, const unsigned short& sx, const unsigned short& sy, const unsigned short& sz)
{
	float fx = sqrt(2.0f) * ((int)sx - 16384) / 32768.0f;
	float fy = sqrt(2.0f) * ((int)sy - 16384) / 32768.0f;
	float fz = sqrt(2.0f) * ((int)sz - 16384) / 32768.0f;
	float fw = sqrt(1.0f - fx * fx - fy * fy - fz * fz);

	Quat<float> uq;

	switch (flag)
	{
	case 0:
		uq.v.x = fw;
		uq.v.y = fx;
		uq.v.z = fy;
		uq.r = fz;
		break;

	case 1:
		uq.v.x = fx;
		uq.v.y = fw;
		uq.v.z = fy;
		uq.r = fz;
		break;

	case 2:
		uq.v.x = -fx;
		uq.v.y = -fy;
		uq.v.z = -fw;
		uq.r = -fz;
		break;

	case 3:
		uq.v.x = fx;
		uq.v.y = fy;
		uq.v.z = fz;
		uq.r = fw;
		break;
	}

	return uq;
}

static Vec3<float> uncompressVector(const Vec3<float>& min, const Vec3<float>& max, const unsigned short& sx, const unsigned short& sy, const unsigned short& sz)
{
	Vec3<float> uv;

	uv = max - min;

	uv.x *= (sx / 65535.0f);
	uv.y *= (sy / 65535.0f);
	uv.z *= (sz / 65535.0f);

	uv = uv + min;

	return uv;
}

static float uncompressTime(const unsigned short& ct, const float& animationLength)
{
	float ut;

	ut = ct / 65535.0f;
	ut = ut * animationLength;

	return ut;
}


static void gLoadAnm_v3(ifstream &in, AnmHeader *anmHeader)
{
	in.read((char *)(&anmHeader -> unknow), 4);
	in.read((char *)(&anmHeader -> boneCount), 4);
	in.read((char *)(&anmHeader -> frameCount), 4);
	in.read((char *)(&anmHeader -> fps), 4);

	if(anmHeader -> boneCount == 0)
	{
		anmHeader -> anmBones = NULL;
	}

	anmHeader -> anmBones = (AnmBone *)malloc(sizeof(AnmBone) * anmHeader -> boneCount);
	memset(anmHeader -> anmBones, 0, sizeof(AnmBone) * anmHeader -> boneCount);
	int i;
	for(i = 0; i < anmHeader -> boneCount; i++)
	{
		AnmBone &bone = anmHeader -> anmBones[i];
		in.read((char *)&bone.boneName, 32);
		in.read((char *)&bone.boneType, 4);
		if(anmHeader -> frameCount == 0)
			bone.anms = NULL;
		else
		{
			bone.anms = (AnmData *)malloc(sizeof(AnmData) * anmHeader -> frameCount);
			memset(bone.anms, 0, sizeof(AnmData) * anmHeader -> frameCount);
			int j;
			for(j = 0; j < anmHeader -> frameCount; j++)
			{
				AnmData &d = bone.anms[j];
				in.read((char *)(&d.rotation), sizeof(d.rotation));
				in.read((char *)(&d.translation), sizeof(d.translation));
			}
		}
	}
}

static void gLoadAnm_v1(ifstream &in, AnmHeader *anmHeader, const BoneHash *boneHashes, size_t numBones)
{
	int fileSize;
	in.read((char *)(&fileSize), 4);
	fileSize += 12;
	in.seekg(8, ios_base::cur);

	in.read((char *)(&anmHeader -> boneCount), 4);
	int numEntries;
	in.read((char *)(&numEntries), 4);
	in.seekg(4, ios_base::cur);

	float animationLength;
	in.read((char *)(&animationLength), 4);
	in.read((char *)(&anmHeader -> fps), 4);
	anmHeader -> frameCount = (int)(animationLength * anmHeader -> fps);
	float frameDelay = 1.0f / anmHeader -> fps;

	in.seekg(24, ios_base::cur);

	Vec3<float> minTranslation;
	in.read((char *)(&minTranslation[0]), 4);
	in.read((char *)(&minTranslation[1]), 4);
	in.read((char *)(&minTranslation[2]), 4);

	Vec3<float> maxTranslation;
	in.read((char *)(&maxTranslation[0]), 4);
	in.read((char *)(&maxTranslation[1]), 4);
	in.read((char *)(&maxTranslation[2]), 4);

	Vec3<float> minScale;
	in.read((char *)(&minScale[0]), 4);
	in.read((char *)(&minScale[1]), 4);
	in.read((char *)(&minScale[2]), 4);

	Vec3<float> maxScale;
	in.read((char *)(&maxScale[0]), 4);
	in.read((char *)(&maxScale[1]), 4);
	in.read((char *)(&maxScale[2]), 4);

	int entriesOffset, indicesOffset, hashesOffset;
	in.read((char *)(&entriesOffset), 4);
	in.read((char *)(&indicesOffset), 4);
	in.read((char *)(&hashesOffset), 4);

	entriesOffset += 12;
	indicesOffset += 12;
	hashesOffset += 12;

	const int hashBlock = 4;
	vector<unsigned int> hashEntries;

	in.seekg(hashesOffset, ios_base::beg);

	for (int i = 0; i < anmHeader -> boneCount; i++)
	{
		unsigned int hashEntry;
		in.read((char *)(&hashEntry), 4);
		hashEntries.push_back(hashEntry);
	}

	in.seekg(entriesOffset, ios_base::beg);

	const unsigned char quaternionType = 0;
	const unsigned char translationType = 64;
	const unsigned char scaleType = 128;

	vector<vector<pair <unsigned short, bitset<48> > > > compressedQuaternions, compressedTranslations, compressedScales;
	compressedQuaternions.resize(anmHeader -> boneCount);
	compressedTranslations.resize(anmHeader -> boneCount);
	compressedScales.resize(anmHeader -> boneCount);

	unordered_set<unsigned short> frame_set;

	for (int i = 0; i < numEntries; i++)
	{
		unsigned short compressedTime;
		in.read((char *)(&compressedTime), 2);

		frame_set.insert(compressedTime);

		unsigned char hashIndex;
		in.read((char *)(&hashIndex), 1);

		unsigned char dataType;
		in.read((char *)(&dataType), 1);

		bitset<48> compressedData;
		in.read((char *)(&compressedData), 6);

		int boneHash = hashEntries.at(hashIndex);

		if (dataType == quaternionType)
		{
			compressedQuaternions.at(hashIndex).push_back(pair<unsigned short, bitset<48> >(compressedTime, compressedData));
		}

		else if (dataType == translationType)
		{
			compressedTranslations.at(hashIndex).push_back(pair<unsigned short, bitset<48> >(compressedTime, compressedData));
		}

		else if (dataType == scaleType)
		{
			compressedScales.at(hashIndex).push_back(pair<unsigned short, bitset<48> >(compressedTime, compressedData));
		}
	}

	//cout<<"|||"<<frame_set.size()<<endl;
	anmHeader -> frameCount = frame_set.size();

	anmHeader -> anmBones = (AnmBone *)malloc(sizeof(AnmBone) * anmHeader -> boneCount);
	memset(anmHeader -> anmBones, 0, sizeof(AnmBone) * anmHeader -> boneCount);

	for (int i = 0; i < anmHeader -> boneCount; i++)
	{
		unsigned int boneHash = hashEntries.at(i);

		//cout<<i<<"_"<<anmHeader -> frameCount<<"_"<<compressedTranslations.at(i).size()<<"_"<<compressedQuaternions.at(i).size()<<"_"<<compressedScales.at(i).size()<<endl;
		bool hasBone = false;
		unsigned int h;
		for(h = 0; h < numBones; h++)
		{
			if (boneHashes[h].key == boneHash)
			{
				hasBone = true;
				break;
			}
		}

		AnmBone &boneEntry = anmHeader -> anmBones[i];
		boneEntry.anms = (AnmData *)malloc(sizeof(AnmData) * anmHeader -> frameCount);
		memset(boneEntry.anms, 0, sizeof(AnmData) * anmHeader -> frameCount);
		if(hasBone)
			strcpy(boneEntry.boneName, boneHashes[h].value);
		//cout<<boneEntry.boneName<<endl;

		unordered_set<short> translationsTimeSet;
		vector<pair<float, Vec3<float> > > translation;

		for (unsigned int j = 0; j < compressedTranslations.at(i).size(); j++)
		//for (auto &t : compressedTranslations.at(i))
		{
			pair<unsigned short, bitset<48> > &t = compressedTranslations.at(i)[j];
			auto res = translationsTimeSet.insert(t.first);

			if (!res.second)
			{
				continue;
			}

			float uncompressedTime = uncompressTime(t.first, animationLength);

			bitset<48> mask = 0xFFFF;
			unsigned short sx = (unsigned short)((t.second & mask).to_ulong());
			unsigned short sy = (unsigned short)((t.second >> 16 & mask).to_ulong());
			unsigned short sz = (unsigned short)((t.second >> 32 & mask).to_ulong());

			Vec3<float> translationEntry = uncompressVector(minTranslation, maxTranslation, sx, sy, sz);

			translation.push_back(pair<float, Vec3<float> >(uncompressedTime, translationEntry));
		}

		unordered_set<short> rotationsTimeSet;
		vector<pair<float, Quat<float> > > quaternion;

		for (unsigned int j = 0; j < compressedQuaternions.at(i).size(); j++)
		//for (auto &r : compressedQuaternions.at(i))
		{

			pair<unsigned short, bitset<48> > &r = compressedQuaternions.at(i)[j];
			auto res = rotationsTimeSet.insert(r.first);

			if (!res.second)
			{
				continue;
			}

			float uncompressedTime = uncompressTime(r.first, animationLength);

			bitset<48> mask = 0x7FFF;
			unsigned short flag = (unsigned short)((r.second >> 45).to_ulong());
			unsigned short sx = (unsigned short)((r.second >> 30 & mask).to_ulong());
			unsigned short sy = (unsigned short)((r.second >> 15 & mask).to_ulong());
			unsigned short sz = (unsigned short)((r.second & mask).to_ulong());

			Quat<float> quaterionEntry = uncompressQuaternion(flag, sx, sy, sz);

			quaternion.push_back(pair<float, Quat<float> >(uncompressedTime, quaterionEntry));
		}

		unordered_set<short> scaleTimeSet;
		vector<pair<float, Vec3<float> > > scale;

		for(unsigned int j = 0; j < compressedScales.at(i).size(); j++)
		//for (auto &s : compressedScales.at(i))
		{

			pair<unsigned short, bitset<48> > &s = compressedTranslations.at(i)[j];
			auto res = scaleTimeSet.insert(s.first);

			if (!res.second)
			{
				continue;
			}

			float uncompressedTime = uncompressTime(s.first, animationLength);

			bitset<48> mask = 0xFFFF;
			unsigned short sx = (unsigned short)((s.second & mask).to_ulong());
			unsigned short sy = (unsigned short)((s.second >> 16 & mask).to_ulong());
			unsigned short sz = (unsigned short)((s.second >> 32 & mask).to_ulong());

			Vec3<float> scaleEntry = uncompressVector(minScale, maxScale, sx, sy, sz);

			scale.push_back(pair<float, Vec3<float> >(uncompressedTime, scaleEntry));
		}

		list<float> frame_list;
		for(unordered_set<unsigned short>::iterator itor = frame_set.begin();
				itor != frame_set.end();
				++itor)
			frame_list.push_back(uncompressTime(*itor, animationLength));
		frame_list.sort();

		Vec3<float> lastTranslation = translation.at(0).second;
		Vec3<float> nextTranslation = lastTranslation;
		Quat<float> lastQuaternion = quaternion.at(0).second;
		Quat<float> nextQuaternion = lastQuaternion;
		Vec3<float> lastScale = scale.at(0).second;
		Vec3<float> nextScale = lastScale;
		unsigned int k = 0; unsigned int nk = 0; unsigned int ck = 0;
		unsigned int l = 0; unsigned int nl = 0; unsigned int cl = 0;
		unsigned int m = 0; unsigned int nm = 0; unsigned int cm = 0;
		unsigned int n = 0;
		for(list<float>::iterator itor = frame_list.begin();
				itor != frame_list.end();
				++itor)
		{
			if(m < scale.size())
			{
				if(*itor == scale.at(m).first)
				{
					lastScale = scale.at(m).second;
					m++;
					nm = 0;
					cm = 0;
					boneEntry.anms[n].scale[0] = lastScale.x;
					boneEntry.anms[n].scale[1] = lastScale.y;
					boneEntry.anms[n].scale[2] = lastScale.z;
					if(m < scale.size())
					{
						nextScale = scale.at(m).second;
						for(list<float>::iterator sitor = itor;
								sitor != frame_list.end();
								++sitor)
						{
							if(*sitor != scale.at(m).first)
								cm++;
							else
								break;
						}
					}
					else
						nextScale = lastScale;
				}
				else
				{
					if(lastScale != nextScale)
					{
						nm++;
						float lerpt = 1.0f / (float)(cm) * (float)nm;
						boneEntry.anms[n].scale[0] = Imath::lerp(lastScale.x, nextScale.x, lerpt); 
						boneEntry.anms[n].scale[1] = Imath::lerp(lastScale.y, nextScale.y, lerpt);
						boneEntry.anms[n].scale[2] = Imath::lerp(lastScale.z, nextScale.z, lerpt);
					}
					else
					{
						boneEntry.anms[n].scale[0] = lastScale.x;
						boneEntry.anms[n].scale[1] = lastScale.y;
						boneEntry.anms[n].scale[2] = lastScale.z;
					}
				}
			}
			else
			{
					boneEntry.anms[n].scale[0] = lastScale.x;
					boneEntry.anms[n].scale[1] = lastScale.y;
					boneEntry.anms[n].scale[2] = lastScale.z;
			}

			if(k < translation.size())
			{
				if(*itor == translation.at(k).first)
				{
					lastTranslation = translation.at(k).second;
					k++;
					nk = 0;
					ck = 0;
					boneEntry.anms[n].translation[0] = lastTranslation.x;
					boneEntry.anms[n].translation[1] = lastTranslation.y;
					boneEntry.anms[n].translation[2] = lastTranslation.z;
					if(k < translation.size())
					{
						nextTranslation = translation.at(k).second;
						for(list<float>::iterator sitor = itor;
								sitor != frame_list.end();
								++sitor)
						{
							if(*sitor != translation.at(k).first)
								ck++;
							else
								break;
						}
					}
					else
						nextTranslation = lastTranslation;
				}
				else
				{
					if(lastTranslation != nextTranslation)
					{
						nk++;
						float lerpt = 1.0f / (float)(ck) * (float)nk;
						boneEntry.anms[n].translation[0] = Imath::lerp(lastTranslation.x, nextTranslation.x, lerpt);
						boneEntry.anms[n].translation[1] = Imath::lerp(lastTranslation.y, nextTranslation.y, lerpt);
						boneEntry.anms[n].translation[2] = Imath::lerp(lastTranslation.z, nextTranslation.z, lerpt);
					}
					else
					{
						boneEntry.anms[n].translation[0] = lastTranslation.x;
						boneEntry.anms[n].translation[1] = lastTranslation.y;
						boneEntry.anms[n].translation[2] = lastTranslation.z;
					}
				}
			}
			else
			{
				boneEntry.anms[n].translation[0] = lastTranslation.x;
				boneEntry.anms[n].translation[1] = lastTranslation.y;
				boneEntry.anms[n].translation[2] = lastTranslation.z;
			}

			if(l < quaternion.size())
			{
				if(*itor == quaternion.at(l).first)
				{
					lastQuaternion = quaternion.at(l).second;
					l++;
					cl = 0;
					nl = 0;
					boneEntry.anms[n].rotation[0] = lastQuaternion.v.x;
					boneEntry.anms[n].rotation[1] = lastQuaternion.v.y;
					boneEntry.anms[n].rotation[2] = lastQuaternion.v.z;
					boneEntry.anms[n].rotation[3] = lastQuaternion.r;
					if(l < quaternion.size())
					{
						nextQuaternion = quaternion.at(l).second;
						for(list<float>::iterator sitor = itor;
								sitor != frame_list.end();
								++sitor)
						{
							if(*sitor != quaternion.at(l).first)
								cl++;
							else
								break;
						}
					}
					else
						nextQuaternion = lastQuaternion;
				}
				else
				{
					if(lastQuaternion != nextQuaternion)
					{
						nl++;
						float lerpt = 1.0f / (float)(cl) * (float)nl;
						
						//Quat<float> lerpQuat = Imath::slerp(lastQuaternion, nextQuaternion, lerpt);
						Quat<float> lerpQuat = lastQuaternion;
						boneEntry.anms[n].rotation[0] = lerpQuat.v.x;
						boneEntry.anms[n].rotation[1] = lerpQuat.v.y;
						boneEntry.anms[n].rotation[2] = lerpQuat.v.z;
						boneEntry.anms[n].rotation[3] = lerpQuat.r;
					}
					else
					{
						boneEntry.anms[n].rotation[0] = lastQuaternion.v.x;
						boneEntry.anms[n].rotation[1] = lastQuaternion.v.y;
						boneEntry.anms[n].rotation[2] = lastQuaternion.v.z;
						boneEntry.anms[n].rotation[3] = lastQuaternion.r;
					}
				}
			}
			else
			{
				boneEntry.anms[n].rotation[0] = lastQuaternion.v.x;
				boneEntry.anms[n].rotation[1] = lastQuaternion.v.y;
				boneEntry.anms[n].rotation[2] = lastQuaternion.v.z;
				boneEntry.anms[n].rotation[3] = lastQuaternion.r;
			}

			n++;
		}

		//bones.push_back(boneEntry);
	}
}

static void gLoadAnm_v4(ifstream &in, AnmHeader *anmHeader, const BoneHash *boneHashes, size_t numBones)
{
	in.seekg(16, ios_base::cur);
	in.read((char *)(&anmHeader -> boneCount), 4);
	in.read((char *)(&anmHeader -> frameCount), 4);
	int32 frameDelay;
	in.read((char *)(&frameDelay), 4);
	anmHeader -> fps = 1.0f / frameDelay;
	in.seekg(12, ios_base::cur);

	int translationsOffset, quaternionsOffset, framesOffset;
	in.read((char *)(&translationsOffset), 4);
	translationsOffset += 12;
	in.read((char *)(&quaternionsOffset), 4);
	quaternionsOffset += 12;
	in.read((char *)(&framesOffset), 4);
	framesOffset += 12;

	const int positionBlockSize = 12;
	vector<Vec3<float> > translationEntries;

	int numTranslationEntries = (int)(quaternionsOffset - translationsOffset) / positionBlockSize;

	in.seekg(translationsOffset, ios_base::beg);

	for (int i = 0; i < numTranslationEntries; i++)
	{
		Vec3<float> translationEntry;
		in.read((char *)(&translationEntry), 12);
		translationEntries.push_back(translationEntry);
	}

	const int quaternionBlockSize = 16;
	vector<Quat<float> > quaternionEntries;

	int numQuaternionEntries = (int)(framesOffset - quaternionsOffset) / quaternionBlockSize;

	in.seekg(quaternionsOffset, ios_base::beg);

	for (int i = 0; i < numQuaternionEntries; i++)
	{
		Quat<float> quaternionEntry;
		in.read((char *)(&quaternionEntry.v.x), 4);
		in.read((char *)(&quaternionEntry.v.y), 4);
		in.read((char *)(&quaternionEntry.v.z), 4);
		in.read((char *)(&quaternionEntry.r), 4);
		quaternionEntries.push_back(quaternionEntry);
	}

	map<unsigned int, vector<FrameIndices> > boneMap;

	in.seekg(framesOffset, ios_base::beg);

	anmHeader -> anmBones = (AnmBone *)malloc(sizeof(AnmBone) * anmHeader -> boneCount);
	memset(anmHeader -> anmBones, 0, sizeof(AnmBone) * anmHeader -> boneCount);

	for (int i = 0; i < anmHeader -> boneCount; i++)
	{
		for (int j = 0; j < anmHeader -> frameCount; j++)
		{
			unsigned int boneHash; 
			FrameIndices fi;

			in.read((char *)(&boneHash), 4);
			in.read((char *)(&fi.translationIndex), 2);
			in.read((char *)(&fi.scaleIndex), 2);
			in.read((char *)(&fi.quaternionIndex), 2);
			in.seekg(2, ios_base::cur);

			boneMap[boneHash].push_back(fi);
		}
	}

	int c = 0;

	for(map<unsigned int, vector<FrameIndices> >::iterator itor = boneMap.begin();
			itor != boneMap.end();
			++itor) //for (auto& i : boneMap)
	{
		pair<const unsigned int, vector<FrameIndices> > &i = *itor;
		float cumulativeFrameDelay = 0.0f;

		bool hasBone = false;
		unsigned int l;
		for(l = 0; l < numBones; l++)
		{
			if (boneHashes[l].key == i.first)
			{
				hasBone = true;
				break;
			}
		}

		anmHeader -> anmBones[c].anms = (AnmData *)malloc(sizeof(AnmData) * anmHeader -> frameCount);
		//anmHeader -> anmBones[i].anms = new AnmData[sizeof(AnmData) * anmHeader -> frameCount];
		memset(anmHeader -> anmBones[c].anms, 0, sizeof(AnmData) * anmHeader -> frameCount);
		AnmBone &boneEntry = anmHeader -> anmBones[c];
		if(hasBone)
			strcpy(boneEntry.boneName, boneHashes[l].value);

		//cout<<(boneEntry.boneName)<<endl;
		for(unsigned int m = 0; m < i.second.size(); m++) // for (auto& f : i.second)
		{		
			FrameIndices &f = i.second[m];
			/*
			boneEntry.translation.push_back(pair<float, Vec3<float> >(cumulativeFrameDelay, translationEntries.at(f.translationIndex)));			
			boneEntry.quaternion.push_back(pair<float, Quat<float> >(cumulativeFrameDelay, quaternionEntries.at(f.quaternionIndex)));
			boneEntry.scale.push_back(pair<float, Vec3<float> >(cumulativeFrameDelay, translationEntries.at(f.scaleIndex)));
			*/
			if(f.translationIndex < translationEntries.size())
			{
				const Vec3<float> &t = translationEntries.at(f.translationIndex);
				boneEntry.anms[m].translation[0] = t.x;
				boneEntry.anms[m].translation[1] = t.y;
				boneEntry.anms[m].translation[2] = t.z;
			}

			if(f.quaternionIndex < quaternionEntries.size())
			{
				const Quat<float> &r = quaternionEntries.at(f.quaternionIndex);
				boneEntry.anms[m].rotation[0] = r.v.x;
				boneEntry.anms[m].rotation[1] = r.v.y;
				boneEntry.anms[m].rotation[2] = r.v.z;
				boneEntry.anms[m].rotation[3] = r.r;
			}

			if(f.scaleIndex < translationEntries.size())
			{
				const Vec3<float> &s = translationEntries.at(f.scaleIndex);
				boneEntry.anms[m].scale[0] = s.x;
				boneEntry.anms[m].scale[1] = s.y;
				boneEntry.anms[m].scale[2] = s.z;
			}

			cumulativeFrameDelay += frameDelay;
		}

		//bones.push_back(boneEntry);
		c++;
	}
}

static void gLoadAnm_v5(ifstream &in, AnmHeader *anmHeader, const BoneHash *boneHashes, size_t numBones)
{
	int fileSize;
	in.read((char *)(&fileSize), 4);
	fileSize += 12;

	in.seekg(12, ios_base::cur);

	in.read((char *)(&anmHeader -> boneCount), 4);
	in.read((char *)(&anmHeader -> frameCount), 4);
	int32 frameDelay;
	in.read((char *)(&frameDelay), 4);

	int translationsOffset, quaternionsOffset, framesOffset, hashesOffset;

	in.read((char *)(&hashesOffset), 4);
	in.seekg(8, ios_base::cur);
	in.read((char *)(&translationsOffset), 4);
	in.read((char *)(&quaternionsOffset), 4);
	in.read((char *)(&framesOffset), 4);

	translationsOffset += 12;
	quaternionsOffset += 12;
	framesOffset += 12;
	hashesOffset += 12;

	const int translationBlock = 12;
	vector<Vec3<float> > translationEntries;

	int numTranslationEntries = (int)(quaternionsOffset - translationsOffset) / translationBlock;

	in.seekg(translationsOffset, ios_base::beg);

	for (int i = 0; i < numTranslationEntries; i++)
	{
		Vec3<float> translationEntry;
		in.read((char *)(&translationEntry), 12);
		translationEntries.push_back(translationEntry);
	}

	const int quaternionBlock = 6;
	vector<bitset<48> > quaternionEntries;

	int numQuaternionEntries = (int)(hashesOffset - quaternionsOffset) / quaternionBlock;

	in.seekg(quaternionsOffset, ios_base::beg);

	for (int i = 0; i < numQuaternionEntries; i++)
	{
		bitset<48> quaternionEntry;
		in.read((char *)(&quaternionEntry), 6);
		quaternionEntries.push_back(quaternionEntry);
	}

	const int hashBlock = 4;
	vector<unsigned int> hashEntries;

	int numHashEntries = (int)(framesOffset - hashesOffset) / hashBlock;

	in.seekg(hashesOffset, ios_base::beg);

	for (int i = 0; i < numHashEntries; ++i)
	{
		unsigned int hashEntry;
		in.read((char *)(&hashEntry), 4);
		hashEntries.push_back(hashEntry);
	}

	//bones.resize(numBones);

	in.seekg(framesOffset, ios_base::beg);

	float cumulativeFrameDelay = 0.0f;

	anmHeader -> anmBones = (AnmBone *)malloc(sizeof(AnmBone) * anmHeader -> boneCount);
	memset(anmHeader -> anmBones, 0, sizeof(AnmBone) * anmHeader -> boneCount);
	for (int i = 0; i < anmHeader -> boneCount; i++)
	{
		anmHeader -> anmBones[i].anms = (AnmData *)malloc(sizeof(AnmData) * anmHeader -> frameCount);
		memset(anmHeader -> anmBones[i].anms, 0, sizeof(AnmData) * anmHeader -> frameCount);
	}

	for (int i = 0; i < anmHeader -> frameCount; i++)
	{
		for (int j = 0; j < anmHeader -> boneCount; j++)
		{
			bool hasBone = false;
			unsigned int l;
			for(l = 0; l < numBones; l++)
			{
				if (boneHashes[l].key == hashEntries.at(j))
				{
					hasBone = true;
					break;
				}
			}

			AnmBone &boneEntry = anmHeader -> anmBones[j];
			if(hasBone)
				strcpy(boneEntry.boneName, boneHashes[l].value);

			short translationIndex, quaternionIndex, scaleIndex;
			in.read((char *)(&translationIndex), 2);
			in.read((char *)(&scaleIndex), 2);
			in.read((char *)(&quaternionIndex), 2);

			//bones.at(j).translation.push_back(pair<float, Vec3<float>>(cumulativeFrameDelay, translationEntries.at(translationIndex)));
			boneEntry.anms[i].translation[0] = translationEntries.at(translationIndex).x;
			boneEntry.anms[i].translation[1] = translationEntries.at(translationIndex).y;
			boneEntry.anms[i].translation[2] = translationEntries.at(translationIndex).z;

			bitset<48> mask = 0x7FFF;
			unsigned short flag = (unsigned short)((quaternionEntries.at(quaternionIndex) >> 45).to_ulong());
			unsigned short sx = (unsigned short)((quaternionEntries.at(quaternionIndex) >> 30 & mask).to_ulong());
			unsigned short sy = (unsigned short)((quaternionEntries.at(quaternionIndex) >> 15 & mask).to_ulong());
			unsigned short sz = (unsigned short)((quaternionEntries.at(quaternionIndex) & mask).to_ulong());

			Quat<float> quaterionEntry = uncompressQuaternion(flag, sx, sy, sz);

			//bones.at(j).quaternion.push_back(pair<float, Quat<float>>(cumulativeFrameDelay, quaterionEntry));
			boneEntry.anms[i].rotation[0] = quaterionEntry.v.x;
			boneEntry.anms[i].rotation[1] = quaterionEntry.v.y;
			boneEntry.anms[i].rotation[2] = quaterionEntry.v.z;
			boneEntry.anms[i].rotation[3] = quaterionEntry.r;

			//bones.at(j).scale.push_back(pair<float, Vec3<float> >(cumulativeFrameDelay, translationEntries.at(scaleIndex)));
			boneEntry.anms[i].scale[0] = translationEntries.at(scaleIndex).x;
			boneEntry.anms[i].scale[1] = translationEntries.at(scaleIndex).y;
			boneEntry.anms[i].scale[2] = translationEntries.at(scaleIndex).z;
		}

		cumulativeFrameDelay += frameDelay;
	}

	/*
	auto it = bones.begin();

	while (it != bones.end())
	{
		if (!isalpha(it->name[0]))
		{
			it = bones.erase(it);
		}

		else
		{
			++it;
		}
	}
	*/
}

AnmHeader * gLoadAnm(const char *file, const SklHeader *skl)
{
	if(!file)
		return NULL;

	ifstream in(file, ios::in | ios::binary);
	if (!in.is_open())
	{
		cerr << "Unable to load anm file." << endl;
		return NULL;
	}
	AnmHeader *anmHeader = (AnmHeader *)malloc(sizeof(AnmHeader));
	memset(anmHeader, 0, sizeof(AnmHeader));
	in.read((char *)(&anmHeader -> name), 8);
	in.read((char *)(&anmHeader -> version), 4);
	printf("Anm version -> %d\n", anmHeader -> version);
	if(anmHeader -> version == 3)
		gLoadAnm_v3(in, anmHeader);
	else if(anmHeader -> version == 1 || anmHeader -> version == 4 || anmHeader -> version == 5)
	{
		if(skl)
		{
			if(anmHeader -> version == 1)
				gLoadAnm_v1(in, anmHeader, skl -> boneHashes, skl -> numBones);
			else if(anmHeader -> version == 4)
				gLoadAnm_v4(in, anmHeader, skl -> boneHashes, skl -> numBones);
			else if(anmHeader -> version == 5)
				gLoadAnm_v5(in, anmHeader, skl -> boneHashes, skl -> numBones);
		}
		else
		{
			fprintf(stderr, "Need skl data for parse anm file.\n");
			free(anmHeader);
			anmHeader = NULL;
		}
	}
	else
	{
		fprintf(stderr, "Unsupported anm version -> %d\n", anmHeader -> version);
		free(anmHeader);
		anmHeader = NULL;
	}
	/*
		 for(int i = 0; i < anmHeader -> boneCount; i++)
		 {
		 bool b = false;
		 int j;
		 for(j = 0; j < skl -> numBones; j++)
		 if(strcmp(anmHeader -> anmBones[i].boneName, skl -> bones[j].name) == 0)
		 {
		 b = true;
		 break;
		 }
		 else
		 continue;
		 if(b)
		 cout<<i<<" - "<<j<<" - "<<skl -> bones[j].name<<" - "<<skl-> bones[j].parent<<endl;
		 else
		 cout<<i<<" - "<<j<<" - "<<"§§§§§§§§§"<<" - " <<skl->bones[j].parent<<endl;
		 }
		 */
	in.close();
	return anmHeader;
}

void gFreeAnm(AnmHeader *anm)
{
	if(!anm)
		return;
	if(anm -> anmBones)
	{
		int i;
		for(i = 0; i < anm -> boneCount; i++)
			if(anm -> anmBones[i].anms)
				free(anm -> anmBones[i].anms);
		free(anm -> anmBones);
	}
	free(anm);
}
