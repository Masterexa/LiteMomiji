#pragma once

/* include */
#include "pre.hpp"
#include "PipelineState.hpp"
#include <unordered_map>


struct Shader{

	/* Instance */
		/* Fields */
			std::unordered_map<std::string,std::unique_ptr<PipelineState>>	m_techniques;

		/* Methods */
			bool addTechnique(char const* name, PipelineState* pipeline);
			PipelineState* getTechniquePSO(char const* name) const;
};