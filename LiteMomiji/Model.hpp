#pragma once

#include "pre.hpp"
#include <vector>

struct MeshSet
{
	Mesh*		mesh;
	uint32_t	materials_count;
	Material**	materials;
};

// contains only pair
struct Model{

	/* Instance */
		/* Fields */
			std::vector<MeshSet>	m_meshsets;
};


bool loadWavefrontFromFile(char const* path, Model** out_model);