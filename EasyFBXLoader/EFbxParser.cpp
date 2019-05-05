#include "EFbxParser.h"
#include "FbxBuffer.h"

#include <fstream>
#include <iostream>
#include <sstream>

int efl::EFbxParser::FileLength = 0;

bool efl::EFbxParser::WriteCompressedFile(const std::string& File, int Stack, const std::string& AnimName, std::vector<Bytef>& OutDataCompressed, uLongf& OutDataSize)
{
	FbxBuffer FbxBuffer;

	bool Success = false;

	std::string MeshFileName, AnimFileName;

	if (!EFbxParser::CheckFileExists(File, FbxBuffer.FileMeshExtension))
	{
		Success = EFbxParser::ParseFbxFile(File, FbxBuffer);

		if (!Success) 
		{
			std::cerr << "something went wrong while parsing mesh file.." << std::endl;
			return false;
		}

		Success = InternalWriteFile(FbxBuffer, File, MeshFileName);

		if (!Success) 
		{
			std::cerr << "[Internal Abrupt]: something went wrong while writing '.mesh' file" << std::endl;
			return false;
		}
	}

	WriteCompressedAnimationFile(File, Stack, AnimName, FbxBuffer);

	/*CompressFile(MeshFileName, FileLength, OutDataCompressed, OutDataSize);
	CompressFile(MeshFileName, FileLength, OutDataCompressed, OutDataSize);*/

	return Success;
}

bool efl::EFbxParser::WriteCompressedAnimationFile(const std::string& Path, int Stack, const std::string& AnimationName, FbxBuffer& InBuffer)
{
	const std::string& FileAnimExtension = ".anim";

	if (!EFbxParser::CheckFileExists(Path, FileAnimExtension))
	{
		bool Result = ParseAnimationFile(Path, Stack, AnimationName, InBuffer);

		if (!Result) {
			//todo: raise error
			return false;
		}

		//if we are here it means we can write down the animations
		InternalWriteAnimFile(Path, AnimationName, InBuffer);
	}

	return true;
}

void efl::EFbxParser::InternalWriteAnimFile(const std::string& File, const std::string& AnimName, FbxBuffer& InBuffer)
{
	std::string FullName = NewExtensionNameByFile(File, ".anim");

	std::ofstream OutStream;
	OutStream.open(FullName, std::ios_base::binary);

	if (!OutStream.is_open())
	{
		//todo: raise error
		return;
	}

	// Header 
	OutStream.write("Anim", 5);

	// write Numfer of frames 
	int Nf = static_cast<int>(InBuffer.Animations[AnimName].size());
	OutStream.write((char*)&Nf, sizeof(int));

	// write number of influences
	int Nb = static_cast<int>(InBuffer.Animations[AnimName][0].size());
	OutStream.write((char*)&Nb, sizeof(int));

	// not sure it's working
	for (int i = 0; i < Nf; i++)
	{
		OutStream.write(reinterpret_cast<char*>(InBuffer.Animations[AnimName][i].data()), Nb * sizeof(float) * 16);
	}

	OutStream.close();
}

bool efl::EFbxParser::InternalWriteFile(FbxBuffer& InBuffer, const std::string& File, std::string& OutFileName)
{
	std::string FullName = NewExtensionNameByFile(File, ".mesh");

	std::ofstream OutStream;
	OutStream.open(FullName, std::ios_base::binary);

	if (!OutStream.is_open())
	{
		std::cerr << "could not open file " << FullName << std::endl;
		return false;
	}

	// Header
	OutStream.write("Mesh", 5);

	// Vertices
	int Nv = static_cast<int>(InBuffer.Vertices.size());
	OutStream.write((char*)&Nv, sizeof(int));

	// Bind poses
	int Nb = static_cast<int>(InBuffer.BindPoses.size());
	OutStream.write((char*)&Nb, sizeof(int));

	// Write down all buffers
	OutStream.write(reinterpret_cast<char*>(InBuffer.Vertices.data()), Nv * sizeof(float) * 3);
	OutStream.write(reinterpret_cast<char*>(InBuffer.Normals.data()), Nv * sizeof(float) * 3);
	OutStream.write(reinterpret_cast<char*>(InBuffer.Influences.data()), Nv * sizeof(float) * 4);
	OutStream.write(reinterpret_cast<char*>(InBuffer.Weights.data()), Nv * sizeof(float) * 4);
	OutStream.write(reinterpret_cast<char*>(&InBuffer.BindPoses[0][0][0]), Nb * sizeof(float) * 16);

	// Write down length of file in order to compress it
	int Length = OutStream.tellp();
	OutStream.write((char*)&Length, sizeof(int));
	FileLength = Length;

	// close stream
	OutStream.close();

	return true;
}

void efl::EFbxParser::CompressFile(const std::string& File, size_t FileLength, std::vector<Bytef>& OutDataCompressed, uLongf& OutDataSize)
{
	std::ifstream InStream;

	std::vector<char> DataToCompress(FileLength);
	InStream.open(File, std::ios_base::binary);
	InStream.read(DataToCompress.data(), DataToCompress.size());

	InStream.close();

	std::vector<Bytef> DataCompressed(compressBound(DataToCompress.size()));
	uLongf DataCompressedSize = DataCompressed.size();
	compress(DataCompressed.data(), &DataCompressedSize, (const Bytef*)DataToCompress.data(), DataToCompress.size());

	OutDataCompressed = DataCompressed;
	OutDataSize = DataCompressedSize;

	std::ofstream Os;
	std::stringstream Ss;
	Ss << std::string(File) << std::string(".compressed");

	Os.open(Ss.str(), std::ios_base::binary);

	Os.write((char *)&FileLength, sizeof(int));
	Os.write((char *)DataCompressed.data(), DataCompressedSize);
	Os.close();
}

bool efl::EFbxParser::ParseFbxFile(const std::string& FileName, FbxBuffer& OutFbxBuffer)
{
	bool success = false;

	auto Manager = fbxsdk::FbxManager::Create();

	// ignore
	auto io_settings = fbxsdk::FbxIOSettings::Create(Manager, IOSROOT);
	Manager->SetIOSettings(io_settings);

	auto importer = fbxsdk::FbxImporter::Create(Manager, "");

	if (importer->Initialize(FileName.c_str(), -1, Manager->GetIOSettings()))
	{
		auto scene = fbxsdk::FbxScene::Create(Manager, "");
		if (importer->Import(scene))
		{
			fbxsdk::FbxGeometryConverter converter(Manager);
			converter.Triangulate(scene, true);
			IterateNode(scene->GetRootNode(), OutFbxBuffer);
		}
		success = true;
	}
	else
	{
		success = false;
	}

	Manager->Destroy();
	return success;
}

bool efl::EFbxParser::ParseAnimationFile(const std::string& File, int Stack, const std::string& Name, FbxBuffer& OutFbxBuffer)
{
	bool success = false;

	auto Manager = fbxsdk::FbxManager::Create();

	// ignore
	auto io_settings = fbxsdk::FbxIOSettings::Create(Manager, IOSROOT);
	Manager->SetIOSettings(io_settings);

	auto importer = fbxsdk::FbxImporter::Create(Manager, "");

	if (importer->Initialize(File.c_str(), -1, Manager->GetIOSettings()))
	{
		auto scene = fbxsdk::FbxScene::Create(Manager, "");
		if (importer->Import(scene))
		{
			auto AnimStack = scene->GetSrcObject<fbxsdk::FbxAnimStack>(Stack);
			scene->SetCurrentAnimationStack(AnimStack);

			auto TakeInfo = scene->GetTakeInfo(AnimStack->GetName());
			auto FirstFrame = TakeInfo->mLocalTimeSpan.GetStart().GetFrameCount();
			auto LastFrame = TakeInfo->mLocalTimeSpan.GetStop().GetFrameCount();

			auto Frames = std::vector<std::vector<glm::mat4>>((LastFrame - FirstFrame) + 1);

			IterateBoneForAnimation(scene->GetRootNode(), FirstFrame, LastFrame, Frames);

			OutFbxBuffer.Animations[Name] = Frames;
		}
		success = true;
	}
	else
	{
		success = !success;
		std::cout << " unable t initialize fbx importer" << std::endl;
	}

	Manager->Destroy();
	return success;
}

void efl::EFbxParser::IterateBoneForAnimation(fbxsdk::FbxNode* node, int FirstFrame, int LastFrame, std::vector<std::vector<glm::mat4>>& OutFrames)
{
	auto Skeleton = node->GetSkeleton();

	if (Skeleton)
	{
		// store animation frames
		for (int i = FirstFrame; i <= LastFrame; i++)
		{
			FbxTime time;
			time.SetFrame(i);
			auto FbxMatrix = node->EvaluateGlobalTransform(time);
			OutFrames[i].push_back(glm::make_mat4(&FbxMatrix[0][0]));
		}
	}

	for (int i = 0; i < node->GetChildCount(); i++)
	{
		IterateBoneForAnimation(node->GetChild(i), FirstFrame, LastFrame, OutFrames);
	}
}

bool efl::EFbxParser::IterateNode(fbxsdk::FbxNode* node, FbxBuffer& OutFbxBuffer)
{
	auto mesh = node->GetMesh();

	if (mesh)
	{
		bool HasSkeleton = false;

		std::map<int, std::vector<std::pair<int, float>>> BonesMapping;

		for (int i = 0; i < mesh->GetDeformerCount(); i++)
		{
			auto Skeleton = reinterpret_cast<fbxsdk::FbxSkin*>(mesh->GetDeformer(i, fbxsdk::FbxDeformer::eSkin));

			if (!Skeleton)
				continue;

			for (int j = 0; j < Skeleton->GetClusterCount(); j++)
			{
				auto Cluster = Skeleton->GetCluster(j);

				auto BoneMatrix = Cluster->GetLink()->EvaluateGlobalTransform();
				OutFbxBuffer.BindPoses.push_back(glm::make_mat4(&BoneMatrix[0][0]));

				int* Bones = Cluster->GetControlPointIndices();
				double* Weights = Cluster->GetControlPointWeights();

				// get the number f vertex influenced by the cluster/bone
				for (int k = 0; k < Cluster->GetControlPointIndicesCount(); k++)
				{
					BonesMapping[Bones[k]].push_back({ j, Weights[k] });
				}
			}

			HasSkeleton = true;
			break;
		}

		for (int i = 0; i < mesh->GetPolygonCount(); i++)
		{
			int vertex_index0 = mesh->GetPolygonVertex(i, 0);
			int vertex_index1 = mesh->GetPolygonVertex(i, 1);
			int vertex_index2 = mesh->GetPolygonVertex(i, 2);

			fbxsdk::FbxVector4 VertexA = mesh->GetControlPointAt(vertex_index0);
			fbxsdk::FbxVector4 VertexB = mesh->GetControlPointAt(vertex_index1);
			fbxsdk::FbxVector4 VertexC = mesh->GetControlPointAt(vertex_index2);

			OutFbxBuffer.Vertices.push_back(glm::vec3(VertexA[0], VertexA[1], VertexA[2]));
			OutFbxBuffer.Vertices.push_back(glm::vec3(VertexB[0], VertexB[1], VertexB[2]));
			OutFbxBuffer.Vertices.push_back(glm::vec3(VertexC[0], VertexC[1], VertexC[2]));

			fbxsdk::FbxVector4 NorA, NorB, NorC;

			mesh->GetPolygonVertexNormal(i, 0, NorA);
			mesh->GetPolygonVertexNormal(i, 1, NorB);
			mesh->GetPolygonVertexNormal(i, 2, NorC);

			OutFbxBuffer.Normals.push_back(glm::vec3(NorA[0], NorA[1], NorA[2]));
			OutFbxBuffer.Normals.push_back(glm::vec3(NorB[0], NorB[1], NorB[2]));
			OutFbxBuffer.Normals.push_back(glm::vec3(NorC[0], NorC[1], NorC[2]));

			if (HasSkeleton)
			{
				FillBonesAndWeights(vertex_index0, BonesMapping, OutFbxBuffer);
				FillBonesAndWeights(vertex_index1, BonesMapping, OutFbxBuffer);
				FillBonesAndWeights(vertex_index2, BonesMapping, OutFbxBuffer);
			}
		}

		return true;
	}

	for (int i = 0; i < node->GetChildCount(); i++)
	{
		if (IterateNode(node->GetChild(i), OutFbxBuffer))
		{
			return true;
		}
	}

	return false;
}

void efl::EFbxParser::FillBonesAndWeights(int Index, std::map<int, std::vector<std::pair<int, float>>>& BonesMapping, FbxBuffer& OutFbxBuffer)
{
	std::array<int, 4> DefaultInfluences = { 0, 0, 0, 0 };
	std::array<float, 4> DefaultWeights = { 0, 0, 0, 0 };

	for (int j = 0; j < std::min<unsigned long long>(4ULL, BonesMapping[Index].size()); j++)
	{
		DefaultInfluences[j] = BonesMapping[Index][j].first;
		DefaultWeights[j] = BonesMapping[Index][j].second;
	}

	OutFbxBuffer.Influences.push_back(DefaultInfluences);
	OutFbxBuffer.Weights.push_back(DefaultWeights);
}

bool efl::EFbxParser::CheckFileExists(const std::string& File, const std::string& Extension)
{
	std::string FullName = NewExtensionNameByFile(File, Extension);

	std::ifstream In;

	In.open(FullName);

	if (In.is_open())
	{
		In.close();
		return true;
	}

	In.close();
	return false;
}

std::string efl::EFbxParser::NewExtensionNameByFile(const std::string& File, const std::string& NewExtension = std::string(""))
{
	size_t LastIndex = File.find_last_of(".");
	std::string RawName = File.substr(0, LastIndex);
	std::string FullName = RawName + NewExtension;

	return FullName;
}
