#pragma once

#include "Private.h"
#include <vector>
#include <map>

namespace efl
{
	class FbxBuffer
	{
	public:
		std::vector<glm::vec3> Vertices;
		std::vector<glm::vec3> Normals;

		std::vector<glm::mat4> BindPoses;

		std::vector<std::array<int, 4>> Influences;
		std::vector<std::array<float, 4>> Weights;

		std::map<std::string, std::vector<std::vector<glm::mat4>>> Animations;

		int Nf;
		int Nb;

		const std::string& FileMeshExtension = ".mesh";
		const std::string& FileAnimExtension = ".anim";
	};
}

