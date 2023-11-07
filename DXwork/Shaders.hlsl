cbuffer cbPlayerInfo : register(b0)
{
	matrix		gmtxPlayerWorld : packoffset(c0);
};

struct MATERIAL
{
	float4					m_cAmbient;
	float4					m_cDiffuse;
	float4					m_cSpecular; //a = power
	float4					m_cEmissive;
};

cbuffer cbCameraInfo : register(b1)
{
	matrix		gmtxView : packoffset(c0);
	matrix		gmtxProjection : packoffset(c4);
};

cbuffer cbGameObjectInfo : register(b2)
{
	matrix		gmtxGameObject : packoffset(c0);
	MATERIAL	gMaterial : packoffset(c4);
	uint		gnTexturesMask : packoffset(c8);
};

cbuffer cbFrameworkInfo : register(b4)
{
	float 		gfCurrentTime;
	float		gfElapsedTime;
	float2		gf2CursorPos;
};

cbuffer cbWaterInfo : register(b5)
{
	matrix		gf4x4TextureAnimation : packoffset(c0);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
struct VS_DIFFUSED_INPUT
{
	float3 position : POSITION;
	float4 color : COLOR;
};

struct VS_DIFFUSED_OUTPUT
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

VS_DIFFUSED_OUTPUT VSPlayer(VS_DIFFUSED_INPUT input)
{
	VS_DIFFUSED_OUTPUT output;

	output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxPlayerWorld), gmtxView), gmtxProjection);
	//output.position = float4(input.position, 1.0f);
	output.color = input.color;

	return(output);
}

float4 PSPlayer(VS_DIFFUSED_OUTPUT input) : SV_TARGET
{
	return(input.color);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

TextureCube gtxtSkyCubeTexture : register(t0);
SamplerState gSamplerState : register(s0);
SamplerState gssClamp : register(s1);

struct VS_SKYBOX_CUBEMAP_INPUT
{
	float3 position : POSITION;
};

struct VS_SKYBOX_CUBEMAP_OUTPUT
{
	float3	positionL : POSITION;
	float4	position : SV_POSITION;
};

VS_SKYBOX_CUBEMAP_OUTPUT VSSkyBox(VS_SKYBOX_CUBEMAP_INPUT input)
{
	VS_SKYBOX_CUBEMAP_OUTPUT output;

	output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxGameObject), gmtxView), gmtxProjection);
	output.positionL = input.position;

	return(output);
}

float4 PSSkyBox(VS_SKYBOX_CUBEMAP_OUTPUT input) : SV_TARGET
{
	float4 cColor = gtxtSkyCubeTexture.Sample(gssClamp, input.positionL);

	return(cColor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

Texture2D<float4> gtxtWaterBaseTexture : register(t1);
Texture2D<float4> gtxtWaterDetail0Texture : register(t2);
Texture2D<float4> gtxtWaterDetail1Texture : register(t3);


struct VS_RIPPLE_WATER_INPUT
{
	float3 position : POSITION;
	float4 color : COLOR;
	float2 uv0 : TEXCOORD0;
};

struct VS_RIPPLE_WATER_OUTPUT
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
	float2 uv0 : TEXCOORD0;
};

VS_RIPPLE_WATER_OUTPUT VSRippleWater(VS_RIPPLE_WATER_INPUT input)
{
	VS_RIPPLE_WATER_OUTPUT output;

	input.position.y += sin(gfCurrentTime * 0.35f + input.position.x * 0.35f) * 2.95f + cos(gfCurrentTime * 0.30f + input.position.z * 0.35f) * 2.05f;

	output.position = mul(float4(input.position, 1.0f), gmtxGameObject);
	if (155.0f < output.position.y) output.position.y = 155.0f;
	output.position = mul(mul(output.position, gmtxView), gmtxProjection);

	output.color = (input.position.y / 200.0f) + 0.55f;
	output.uv0 = input.uv0;

	return(output);
}

float4 PSRippleWater(VS_RIPPLE_WATER_OUTPUT input) : SV_TARGET
{
	float2 uv = input.uv0;

	uv.y += 0.00125f;

	float4 cBaseTexColor = gtxtWaterBaseTexture.SampleLevel(gSamplerState, uv, 0);
	float4 cDetail0TexColor = gtxtWaterDetail0Texture.SampleLevel(gSamplerState, uv * 10.0f, 0);
	float4 cDetail1TexColor = gtxtWaterDetail1Texture.SampleLevel(gSamplerState, uv * 5.0f, 0);

	float4 cColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
	cColor = lerp(cBaseTexColor * cDetail0TexColor, cDetail1TexColor.r * 0.5f, 0.35f);

	return(cColor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Terrain

Texture2D<float4> gtxtTerrainBaseTexture : register(t4);
Texture2D<float4> gtxtTerrainDetailTexture : register(t5);
Texture2D<float4> gtxtTerrainWaterTexture : register(t6);
Texture2D<float> gtxtTerrainHeightMapTexture : register(t7);

static float3	gf3TerrainScale = float3(8.0f, 2.0f, 8.0f);
static float2	gf2TerrainHeightMapSize = float2(257.0f, 257.0f);

struct VS_TERRAIN_INPUT
{
	float3 position : POSITION;
	float4 color : COLOR;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
};

struct VS_TERRAIN_OUTPUT
{
	float4 position : SV_POSITION;
	float4 positionW : POSITION;
	float4 color : COLOR;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
};

float GetTerrainHeight(float fx, float fz)
{
	if ((fx < 0.0f) || (fz < 0.0f) || (fx >= gf2TerrainHeightMapSize.x) || (fz >= gf2TerrainHeightMapSize.y)) return(0.0f);

	uint x = (uint)fx;
	uint z = (uint)fz;
	float fxPercent = fx - x;
	float fzPercent = fz - z;
	bool bReverseQuad = ((z % 2) != 0);

	float fBottomLeft = (float)gtxtTerrainHeightMapTexture.Load(float3(x, z, 0));
	float fBottomRight = (float)gtxtTerrainHeightMapTexture.Load(float3((x + 1), z, 0));
	float fTopLeft = (float)gtxtTerrainHeightMapTexture.Load(float3(x, (z + 1), 0));
	float fTopRight = (float)gtxtTerrainHeightMapTexture.Load(float3((x + 1), (z + 1), 0));

	if (bReverseQuad)
	{
		if (fzPercent >= fxPercent)
			fBottomRight = fBottomLeft + (fTopRight - fTopLeft);
		else
			fTopLeft = fTopRight + (fBottomLeft - fBottomRight);
	}
	else
	{
		if (fzPercent < (1.0f - fxPercent))
			fTopRight = fTopLeft + (fBottomRight - fBottomLeft);
		else
			fBottomLeft = fTopLeft + (fBottomRight - fTopRight);
	}
	float fTopHeight = fTopLeft * (1 - fxPercent) + fTopRight * fxPercent;
	float fBottomHeight = fBottomLeft * (1 - fxPercent) + fBottomRight * fxPercent;
	float fHeight = fBottomHeight * (1 - fzPercent) + fTopHeight * fzPercent;

	return(fHeight);
}

VS_TERRAIN_OUTPUT VSTerrain(VS_TERRAIN_INPUT input)
{
	VS_TERRAIN_OUTPUT output;

	float x = input.position.x / gf3TerrainScale.x;
	float z = input.position.z / gf3TerrainScale.z;
	input.position.y = GetTerrainHeight(x, z) * 255.0f * gf3TerrainScale.y;
	//input.position.y = gtxtTerrainHeightMapTexture.Load(float3(x, z, 0)) * 255.0f * gf3TerrainScale.y;

	output.positionW = mul(float4(input.position, 1.0f), gmtxGameObject);
	output.position = mul(mul(output.positionW, gmtxView), gmtxProjection);
	output.color = input.color;
    output.uv0 = input.uv0;
    output.uv1 = input.uv1;

	return(output);
}

float4 PSTerrain(VS_TERRAIN_OUTPUT input) : SV_TARGET
{
	float4 cBaseTexColor = gtxtTerrainBaseTexture.Sample(gSamplerState, input.uv0);
    float4 cDetailTexColor = gtxtTerrainDetailTexture.Sample(gSamplerState, input.uv1);
    //float4 cColor = input.color * cBaseTexColor;
    float4 cColor = cBaseTexColor * 0.5f + cDetailTexColor * 0.5f;
	if ((154.975f < input.positionW.y) && (input.positionW.y < 155.5f))
	{
		cColor.rgb += gtxtTerrainWaterTexture.Sample(gSamplerState, float2(input.uv0.x * 50.0f, (input.positionW.y - 155.0f) / 3.0f + 0.65f)).rgb * (1.0f - (input.positionW.y - 155.0f) / 5.5f);
	}
	return(cColor);
}
