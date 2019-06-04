#include "Model.hpp"
#include "Mesh.hpp"
#include <thread>
#include <stdlib.h>

bool loadWavefrontFromFile(char const* path, Model** out_model)
{
	FILE* fp;
	errno_t err = fopen_s(&fp, path, "r");
	if(err!=0)
	{
		return false;
	}

	fclose(fp);
	return false;
}