#pragma once

#include "Private.h"

#include <fbxsdk.h>
#include <string>
#include <map>
#include <array>
#include <vector>
#include <zlib.h>

namespace efl
{
	class EFbxParser
	{
	public:
		static EFL_API bool WriteCompressedFile(const std::string& File, int Stack, const std::string& AnimName, std::vector<Bytef>& OutDataCompressed, uLongf& OutDataSize);
	private:
		static bool WriteCompressedAnimationFile(const std::string& Path, int Stack, const std::string& AnimationName, class FbxBuffer& InBuffer);
		static bool InternalWriteFile(class FbxBuffer& InBuffer, const std::string& File, std::string& NewFileName);
		static void CompressFile(const std::string& File, size_t FileLength, std::vector<Bytef>& OutDataCompressed, uLongf& OutDataSize);
		static bool ParseFbxFile(const std::string& FileName, class FbxBuffer& OutFbxBuffer);
		static bool IterateNode(fbxsdk::FbxNode* node, class FbxBuffer& OutMesh);
		static void IterateBoneForAnimation(fbxsdk::FbxNode* node, int FirstFrame, int LastFrame, std::vector<std::vector<glm::mat4>>& OutFrames);
		static void FillBonesAndWeights(int Index, std::map<int, std::vector<std::pair<int, float>>>& BonesMapping, class FbxBuffer& OutFbxBuffer);
		static bool ParseAnimationFile(const std::string& File, int Stack, const std::string& Name, class FbxBuffer& OutFbxBuffer);
		static void InternalWriteAnimFile(const std::string& File, const std::string& AnimName, FbxBuffer& InBuffer);
		static bool CheckFileExists(const std::string& File, const std::string& Extension);
		static std::string NewExtensionNameByFile(const std::string& File, const std::string& NewExtension);
		static int FileLength;
	};
}

