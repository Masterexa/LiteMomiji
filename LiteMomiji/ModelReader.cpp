#include "Model.hpp"
#include "Mesh.hpp"
#include "Graphics.hpp"
#include <thread>
#include <stdlib.h>
namespace{
	using namespace DirectX;
}

void calculateTangentAndBinormal(Vertex* in_triangle, size_t triangle_offset, size_t triangle_length, XMFLOAT3* out_tangent, XMFLOAT3* out_binormal)
{
	XMVECTOR v1, v2;
	XMVECTOR tuv;
	float den =0.f;

	v1 = XMVectorSubtract(XMLoadFloat3(&in_triangle[triangle_offset+1].position), XMLoadFloat3(&in_triangle[triangle_offset+0].position));
	v2 = XMVectorSubtract(XMLoadFloat3(&in_triangle[triangle_offset+2].position), XMLoadFloat3(&in_triangle[triangle_offset+0].position));

	XMVECTOR uv[3];
	for(size_t i=0; i<3; i++)
	{
		uv[i] = XMLoadFloat2(&in_triangle[i].uv);
	}

	tuv = XMVectorSwizzle<0,2,1,3>(XMVectorSubtract(uv[1],uv[0]));
	tuv = XMVectorAdd(tuv, XMVectorSwizzle<2,0,3,1>(XMVectorSubtract(uv[2],uv[0])) );
	den = 1.f / (XMVectorGetX(tuv)*XMVectorGetW(tuv)-XMVectorGetY(tuv)*XMVectorGetZ(tuv));

	auto tangent = XMVectorScale(
		XMVectorSubtract(
			XMVectorMultiply(XMVectorSwizzle<3,3,3,3>(tuv),v1), XMVectorMultiply(XMVectorSwizzle<2,2,2,2>(tuv),v2)
		),den
	);

	auto binormal = XMVectorScale(
		XMVectorSubtract(
			XMVectorMultiply(XMVectorSwizzle<0,0,0,0>(tuv),v2), XMVectorMultiply(XMVectorSwizzle<1,1,1,1>(tuv),v1)
		), den
	);

	XMStoreFloat3(out_tangent, XMVector3Normalize(tangent));
	XMStoreFloat3(out_binormal, XMVector3Normalize(binormal));
}


bool triangleFanToList(uint32_t* out_dst, size_t* inout_dst_capacity, uint32_t* src, size_t src_fan_cnt, uint32_t src_fan_base)
{
	size_t required = src_fan_cnt;
	if(required<3)
	{
		return false;
	}
	required = (required-2)*3;

	bool is_few = (*inout_dst_capacity<required);
	*inout_dst_capacity = required;
	
	if(!out_dst || is_few)
	{
		return false;
	}

	for(size_t src_i=2,dst_i=0; src_i<src_fan_cnt; src_i++,dst_i+=2)
	{
		out_dst[dst_i]		= src_fan_base;
		out_dst[dst_i+1]	= src_fan_base+src_i-1;
		out_dst[dst_i+2]	= src_fan_base+src_i;
	}

	return true;
}


bool loadWavefrontFromFile(char const* path, Graphics* graphics, Mesh** out_model)
{
	std::vector<Vertex>		vertices;
	std::vector<uint32_t>	indices;

	
	FILE* fp;
	char buf[256];
	std::vector<long>	Vs;
	std::vector<long>	VTs;
	std::vector<long>	VNs;
	std::vector<long>	Fs;

	auto loadPosIfMatched =[](std::vector<long>& container, bool& matched, char const* tag, char const* tagbuf, long fpos)
	{
		if(!matched && strcmp(tag, tagbuf)==0)
		{
			container.push_back(fpos);
			matched = true;
		}
	};
	auto loadVector = [](FILE* fp,std::vector<long>& pos, size_t num)->XMVECTOR
	{
		XMFLOAT4 v;
		if(num==0)
		{
			return XMVectorZero();
		}

		fseek(fp, pos[num-1], SEEK_SET);
		fscanf_s(fp, "%*s %f %f %f %f", &v.x, &v.y, &v.z, &v.w);

		return XMLoadFloat4(&v);
	};
	auto addVertex = [&](size_t v_, size_t vt_, size_t vn_)
	{
		Vertex vert;
		memset(&vert, 0, sizeof(vert));

		long pos = ftell(fp);
		XMStoreFloat3(&vert.position, loadVector(fp, Vs, v_));
		XMStoreFloat2(&vert.uv, loadVector(fp, VTs, vt_));
		XMStoreFloat3(&vert.normal, XMVectorMultiply(loadVector(fp, VNs, vn_),XMVectorSet(1,1,1,1)) );
		XMStoreFloat4(&vert.color, XMVectorSet(1, 1, 1, 1));

		fseek(fp, pos, SEEK_SET);
		vertices.push_back(vert);
	};
	auto addIndex =[&indices](uint32_t base, uint32_t num)
	{
		for(size_t src_i=2, dst_i=0; src_i<num; src_i++, dst_i+=2)
		{
			indices.push_back(base+src_i);
			indices.push_back(base+src_i-1);
			indices.push_back(base);
		}
	};



	// load file
	errno_t err = fopen_s(&fp, path, "rb");
	if(err!=0)
	{
		return false;
	}

	// load positions
	for(; feof(fp)==0; )
	{
		bool matched	= false;
		long pos		= ftell(fp);

		if(fscanf_s(fp, "%15s%*[^\n\r]", buf, 16)<1)
		{
			break;
		}

		loadPosIfMatched(Vs, matched, "v", buf, pos);
		loadPosIfMatched(VTs, matched, "vt", buf, pos);
		loadPosIfMatched(VNs, matched, "vn", buf, pos);
		loadPosIfMatched(Fs, matched, "f", buf, pos);
	}

	// convet to vertices and indices
	for(auto& it : Fs)
	{
		uint32_t base	= vertices.size();
		uint32_t faces	= 0;

		// seek f line
		fseek(fp, it, SEEK_SET);
		fscanf_s(fp, "%*s");

		// load vertices
		while(true)
		{
			long v, vt, vn; v=vt=vn=0;
			long lp = ftell(fp);

			fscanf_s(fp, "%s", buf, 256);
			fseek(fp, lp, SEEK_SET);

			if(fscanf_s(fp, "%d//%d", &v, &vn)==2)
			{
				addVertex(v, vt, vn);
				continue;
			}

			fseek(fp, lp, SEEK_SET);
			if(fscanf_s(fp, "%d/%d/%d", &v, &vt, &vn)>0)
			{
				addVertex(v, vt, vn);
				continue;
			}

			break;
		}

		// generate index as triangle list
		faces = vertices.size()-base;
		addIndex(base, faces);
	}
	// close file
	fclose(fp);
	


	MeshInitDesc desc;
	desc.verts_ptr		= vertices.data();
	desc.verts_count	= vertices.size();
	desc.indices_ptr	= indices.data();
	desc.indices_count	= indices.size();
	desc.submesh_pairs_ptr	= nullptr;
	desc.submesh_pairs_count=0;
	
	auto mesh = std::make_unique<Mesh>();
	if(FAILED(mesh->init(graphics, &desc)))
	{
		return false;
	}
	*out_model = mesh.get();
	mesh.release();

	return true;
}

bool loadMeshFromPMXFile(char const* path, Graphics* graphics, Mesh** out_model)
{
	std::vector<uint8_t>	text_buffer(128);
	FILE* fp;


	struct Head{
		union{
			uint8_t		bytes[4];
			uint32_t	u32;
		} id;
		float	number;
		uint8_t	data_bytes;
	};
	struct HeadDataByteBase
	{
		uint8_t encode;
		uint8_t additional_uvs;
		uint8_t vertex_indices;
		uint8_t material_indices;
		uint8_t bone_indices;
		uint8_t morph_indices;
		uint8_t rigidbody_indices;
	};
	const Head		CORRECT_HEAD={0x50,0x4d,0x58,0x20};
	const uint8_t	ENCODE_UTF16=0, ENCODE_UTF8=1;
	
	Head head;
	HeadDataByteBase*	headdata_base;
	std::unique_ptr<uint8_t[]> head_data;


	auto readTest=[&]()
	{
	};

	// load file
	errno_t err = fopen_s(&fp, path, "rb");
	if(err!=0)
	{
		return false;
	}


	do{
		// read head and check 
		fread(&head, sizeof(head), 1, fp);
		if(CORRECT_HEAD.id.u32!=head.id.u32)
		{
			fclose(fp);
			return false;
		}
		
		// load head data
		head_data.reset(new uint8_t[head.data_bytes]);
		headdata_base = reinterpret_cast<HeadDataByteBase*>(head_data.get());
		fread(head_data.get(), sizeof(uint8_t), head.data_bytes, fp);


	} while(false);
	fclose(fp);

	return true;
}