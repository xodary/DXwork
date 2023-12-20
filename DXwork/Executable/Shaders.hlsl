cbuffer cbPlayerInfo : register(b0)
{
	matrix		gmtxPlayerWorld : packoffset(c0);
};

cbuffer cbCameraInfo : register(b1)
{
	matrix		gmtxView : packoffset(c0);
	matrix		gmtxProjection : packoffset(c4);
	matrix		gmtxViewProjection : packoffset(c8);
	float3		gvCameraPosition : packoffset(c12);
};

struct MATERIAL
{
	float4					m_cAmbient;
	float4					m_cDiffuse;
	float4					m_cSpecular; //a = power
	float4					m_cEmissive;
};

cbuffer cbGameObjectInfo : register(b2)
{
	matrix		gmtxGameObject : packoffset(c0);
    MATERIAL	gMaterial : packoffset(c4);
    matrix		gmtxTexture: packoffset(c8);
	uint		gnTexturesMask : packoffset(c12);
};

cbuffer cbFrameworkInfo : register(b4)
{
	float 		gfCurrentTime : packoffset(c0.x);
	float		gfElapsedTime : packoffset(c0.y);
	uint		gnRenderMode : packoffset(c0.z);
};

cbuffer cbWaterInfo : register(b5)
{
	matrix		gf4x4TextureAnimation : packoffset(c0);
};

#define DYNAMIC_TESSELLATION		0x10

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

static float3	gf3TerrainScale = float3(16.0f, 3.0f, 16.0f);
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

	//float x = input.position.x / gf3TerrainScale.x;
	//float z = input.position.z / gf3TerrainScale.z;
	//input.position.y = GetTerrainHeight(x, z) * 255.0f * gf3TerrainScale.y;
	////input.position.y = gtxtTerrainHeightMapTexture.Load(float3(x, z, 0)) * 255.0f * gf3TerrainScale.y;

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
	//if ((150.0f < input.positionW.y) && (input.positionW.y < 155.5f))
	//{
	//	cColor.rgb += gtxtTerrainWaterTexture.Sample(gSamplerState, float2(input.uv0.x * 50.0f, (input.positionW.y - 155.0f) / 3.0f + 0.65f)).rgb * (1.0f - (input.positionW.y - 155.0f) / 5.5f);
	//}
	return(cColor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Standard

#define MATERIAL_ALBEDO_MAP			0x01
#define MATERIAL_NORMAL_MAP			0x04
#define MATERIAL_METALLIC_MAP		0x08
#define MATERIAL_EMISSION_MAP		0x10
#define MATERIAL_DETAIL_ALBEDO_MAP	0x20
#define MATERIAL_DETAIL_NORMAL_MAP	0x40

Texture2D gtxtAlbedoTexture : register(t8);
Texture2D gtxtMetallicTexture : register(t9);
Texture2D gtxtNormalTexture : register(t10);
Texture2D gtxtEmissionTexture : register(t11);
Texture2D gtxtDetailAlbedoTexture : register(t12);
Texture2D gtxtDetailNormalTexture : register(t13);

struct VS_STANDARD_INPUT
{
    float3 position : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

struct VS_STANDARD_OUTPUT
{
    float4 position : SV_POSITION;
    float3 positionW : POSITION;
    float3 normalW : NORMAL;
    float3 tangentW : TANGENT;
    float3 bitangentW : BITANGENT;
    float2 uv : TEXCOORD;
};

VS_STANDARD_OUTPUT VSStandard(VS_STANDARD_INPUT input)
{
    VS_STANDARD_OUTPUT output;

    output.positionW = (float3) mul(float4(input.position, 1.0f), gmtxGameObject);
    output.normalW = mul(input.normal, (float3x3) gmtxGameObject);
    output.tangentW = (float3) mul(float4(input.tangent, 1.0f), gmtxGameObject);
    output.bitangentW = (float3) mul(float4(input.bitangent, 1.0f), gmtxGameObject);
    output.position = mul(mul(float4(output.positionW, 1.0f), gmtxView), gmtxProjection);
    output.uv = input.uv;

    return (output);
}

float4 PSStandard(VS_STANDARD_OUTPUT input) : SV_TARGET
{
    float4 cAlbedoColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float4 cSpecularColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float4 cNormalColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float4 cMetallicColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float4 cEmissionColor = float4(0.0f, 0.0f, 0.0f, 1.0f);

    if (gnTexturesMask & MATERIAL_ALBEDO_MAP)
        cAlbedoColor = gtxtAlbedoTexture.Sample(gSamplerState, input.uv);
    //if (gnTexturesMask & MATERIAL_SPECULAR_MAP)
    //    cSpecularColor = gtxtSpecularTexture.Sample(gSamplerState, input.uv);
    if (gnTexturesMask & MATERIAL_NORMAL_MAP)
        cNormalColor = gtxtNormalTexture.Sample(gSamplerState, input.uv);
    if (gnTexturesMask & MATERIAL_METALLIC_MAP)
        cMetallicColor = gtxtMetallicTexture.Sample(gSamplerState, input.uv);
    if (gnTexturesMask & MATERIAL_EMISSION_MAP)
        cEmissionColor = gtxtEmissionTexture.Sample(gSamplerState, input.uv);

    float4 cIllumination = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float4 cColor = cAlbedoColor + cSpecularColor + cEmissionColor;
    if (gnTexturesMask & MATERIAL_NORMAL_MAP)
    {
        float3 normalW = input.normalW;
        float3x3 TBN = float3x3(normalize(input.tangentW), normalize(input.bitangentW), normalize(input.normalW));
        float3 vNormal = normalize(cNormalColor.rgb * 2.0f - 1.0f); //[0, 1] ¡æ [-1, 1]
        normalW = normalize(mul(vNormal, TBN));
//      cIllumination = Lighting(input.positionW, normalW);
        cColor = lerp(cColor, cIllumination, 0.5f);
    }
    return cColor;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
struct VS_BOUNDINGBOX_INPUT
{
    float3 position : POSITION;
};

struct VS_BOUNDINGBOX_OUTPUT
{
    float4 positionH : SV_POSITION;
};

VS_BOUNDINGBOX_OUTPUT VSBoundingBox(VS_BOUNDINGBOX_INPUT input)
{
    VS_BOUNDINGBOX_OUTPUT output;
	
    output.positionH = mul(mul(float4(input.position, 1.0f), gmtxView), gmtxProjection);
    //output.positionH = float4(input.position, 1.0f);
    return (output);
}

float4 PSBoundingBox(VS_BOUNDINGBOX_OUTPUT input) : SV_TARGET
{
    return (float4(1.0f, 0.0f, 0.0f, 1.0f));
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
Texture2D gtxtTexture : register(t15);

struct VS_TEXTURED_INPUT
{
    float3 position : POSITION;
    float2 uv : TEXCOORD;
};

struct VS_TEXTURED_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

VS_TEXTURED_OUTPUT VSTextured(VS_TEXTURED_INPUT input)
{
    VS_TEXTURED_OUTPUT output;

    output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxGameObject), gmtxView), gmtxProjection);
    output.uv = input.uv;

    return (output);
}

//VS_TEXTURED_OUTPUT VSSpriteAnimation(VS_TEXTURED_INPUT input)
//{
//    VS_TEXTURED_OUTPUT output;

//    output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxGameObject), gmtxView), gmtxProjection);
//    output.uv = mul(float3(input.uv, 1.0f), (float3x3) (gMaterial.gmtxTexture)).xy;

//    return (output);
//}

float4 PSTextured(VS_TEXTURED_OUTPUT input) : SV_TARGET
{
    float4 cColor = gtxtTexture.Sample(gSamplerState, input.uv);

    return (cColor);
}

// Tessellation
//--------------------------------------------------------------------------------------
//
struct VS_TERRAIN_TESSELLATION_OUTPUT
{
    float3 position : POSITION;
    float3 positionW : POSITION1;
    float4 color : COLOR;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
};

VS_TERRAIN_TESSELLATION_OUTPUT VSTerrainTessellation(VS_TERRAIN_INPUT input)
{
    VS_TERRAIN_TESSELLATION_OUTPUT output;

    output.position = input.position;
    output.positionW = mul(float4(input.position, 1.0f), gmtxGameObject).xyz;
    output.color = input.color;
    output.uv0 = input.uv0;
    output.uv1 = input.uv1;

    return (output);
}

struct HS_TERRAIN_TESSELLATION_CONSTANT
{
    float fTessEdges[4] : SV_TessFactor;
    float fTessInsides[2] : SV_InsideTessFactor;
};

struct HS_TERRAIN_TESSELLATION_OUTPUT
{
    float3 position : POSITION;
    float4 color : COLOR;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
};

struct DS_TERRAIN_TESSELLATION_OUTPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    float4 tessellation : TEXCOORD2;
};

[domain("quad")]
//[partitioning("fractional_even")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(25)]
[patchconstantfunc("HSTerrainTessellationConstant")]
[maxtessfactor(64.0f)]
HS_TERRAIN_TESSELLATION_OUTPUT HSTerrainTessellation(InputPatch<VS_TERRAIN_TESSELLATION_OUTPUT, 25> input, uint i : SV_OutputControlPointID)
{
    HS_TERRAIN_TESSELLATION_OUTPUT output;

    output.position = input[i].position;
    output.color = input[i].color;
    output.uv0 = input[i].uv0;
    output.uv1 = input[i].uv1;

    return (output);
}

float CalculateTessFactor(float3 f3Position)
{
    float fDistToCamera = distance(f3Position, gvCameraPosition);
    float s = saturate((fDistToCamera - 10.0f) / (500.0f - 10.0f));

    return (lerp(64.0f, 1.0f, s));
	//	return(pow(2, lerp(20.0f, 4.0f, s)));
}

HS_TERRAIN_TESSELLATION_CONSTANT HSTerrainTessellationConstant(InputPatch<VS_TERRAIN_TESSELLATION_OUTPUT, 25> input)
{
    HS_TERRAIN_TESSELLATION_CONSTANT output;

    if (gnRenderMode & DYNAMIC_TESSELLATION)
    {
        float3 e0 = 0.5f * (input[0].positionW + input[4].positionW);
        float3 e1 = 0.5f * (input[0].positionW + input[20].positionW);
        float3 e2 = 0.5f * (input[4].positionW + input[24].positionW);
        float3 e3 = 0.5f * (input[20].positionW + input[24].positionW);

        output.fTessEdges[0] = CalculateTessFactor(e0);
        output.fTessEdges[1] = CalculateTessFactor(e1);
        output.fTessEdges[2] = CalculateTessFactor(e2);
        output.fTessEdges[3] = CalculateTessFactor(e3);

        float3 f3Sum = float3(0.0f, 0.0f, 0.0f);
        for (int i = 0; i < 25; i++)
            f3Sum += input[i].positionW;
        float3 f3Center = f3Sum / 25.0f;
        output.fTessInsides[0] = output.fTessInsides[1] = CalculateTessFactor(f3Center);
    }
    else
    {
        output.fTessEdges[0] = 20.0f;
        output.fTessEdges[1] = 20.0f;
        output.fTessEdges[2] = 20.0f;
        output.fTessEdges[3] = 20.0f;

        output.fTessInsides[0] = 20.0f;
        output.fTessInsides[1] = 20.0f;
    }

    return (output);
}

void BernsteinCoeffcient5x5(float t, out float fBernstein[5])
{
    float tInv = 1.0f - t;
    fBernstein[0] = tInv * tInv * tInv * tInv;
    fBernstein[1] = 4.0f * t * tInv * tInv * tInv;
    fBernstein[2] = 6.0f * t * t * tInv * tInv;
    fBernstein[3] = 4.0f * t * t * t * tInv;
    fBernstein[4] = t * t * t * t;
}

float3 CubicBezierSum5x5(OutputPatch<HS_TERRAIN_TESSELLATION_OUTPUT, 25> patch, float uB[5], float vB[5])
{
    float3 f3Sum = float3(0.0f, 0.0f, 0.0f);
    f3Sum = vB[0] * (uB[0] * patch[0].position + uB[1] * patch[1].position + uB[2] * patch[2].position + uB[3] * patch[3].position + uB[4] * patch[4].position);
    f3Sum += vB[1] * (uB[0] * patch[5].position + uB[1] * patch[6].position + uB[2] * patch[7].position + uB[3] * patch[8].position + uB[4] * patch[9].position);
    f3Sum += vB[2] * (uB[0] * patch[10].position + uB[1] * patch[11].position + uB[2] * patch[12].position + uB[3] * patch[13].position + uB[4] * patch[14].position);
    f3Sum += vB[3] * (uB[0] * patch[15].position + uB[1] * patch[16].position + uB[2] * patch[17].position + uB[3] * patch[18].position + uB[4] * patch[19].position);
    f3Sum += vB[4] * (uB[0] * patch[20].position + uB[1] * patch[21].position + uB[2] * patch[22].position + uB[3] * patch[23].position + uB[4] * patch[24].position);

    return (f3Sum);
}

[domain("quad")]
DS_TERRAIN_TESSELLATION_OUTPUT DSTerrainTessellation(HS_TERRAIN_TESSELLATION_CONSTANT patchConstant, float2 uv : SV_DomainLocation, OutputPatch<HS_TERRAIN_TESSELLATION_OUTPUT, 25> patch)
{
    DS_TERRAIN_TESSELLATION_OUTPUT output = (DS_TERRAIN_TESSELLATION_OUTPUT) 0;

    float uB[5], vB[5];
    BernsteinCoeffcient5x5(uv.x, uB);
    BernsteinCoeffcient5x5(uv.y, vB);

    output.color = lerp(lerp(patch[0].color, patch[4].color, uv.x), lerp(patch[20].color, patch[24].color, uv.x), uv.y);
    output.uv0 = lerp(lerp(patch[0].uv0, patch[4].uv0, uv.x), lerp(patch[20].uv0, patch[24].uv0, uv.x), uv.y);
    output.uv1 = lerp(lerp(patch[0].uv1, patch[4].uv1, uv.x), lerp(patch[20].uv1, patch[24].uv1, uv.x), uv.y);

    float3 position = CubicBezierSum5x5(patch, uB, vB);
    matrix mtxWorldViewProjection = mul(mul(gmtxGameObject, gmtxView), gmtxProjection);
    output.position = mul(float4(position, 1.0f), mtxWorldViewProjection);

    output.tessellation = float4(patchConstant.fTessEdges[0], patchConstant.fTessEdges[1], patchConstant.fTessEdges[2], patchConstant.fTessEdges[3]);

    return (output);
}

float4 PSTerrainTessellation(DS_TERRAIN_TESSELLATION_OUTPUT input) : SV_TARGET
{
    float4 cColor = float4(0.0f, 0.0f, 0.0f, 1.0f);

    if (gnRenderMode & (DYNAMIC_TESSELLATION))
    {
		if (input.tessellation.w <= 5.0f) cColor = float4(1.0f, 0.0f, 0.0f, 1.0f);
		else if (input.tessellation.w <= 10.0f) cColor = float4(0.0f, 1.0f, 0.0f, 1.0f);
		else if (input.tessellation.w <= 20.0f) cColor = float4(0.0f, 0.0f, 1.0f, 1.0f);
		else if (input.tessellation.w <= 30.0f) cColor = float4(1.0f, 0.0f, 1.0f, 1.0f);
		else if (input.tessellation.w <= 40.0f) cColor = float4(1.0f, 1.0f, 0.0f, 1.0f);
		else if (input.tessellation.w <= 50.0f) cColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
		else if (input.tessellation.w <= 55.0f) cColor = float4(0.2f, 0.2f, 0.72f, 1.0f);
		else if (input.tessellation.w <= 60.0f) cColor = float4(0.5f, 0.75f, 0.75f, 1.0f);
		else cColor = float4(0.87f, 0.17f, 1.0f, 1.0f);
    }
    else
    {
        float4 cBaseTexColor = gtxtTerrainBaseTexture.Sample(gSamplerState, input.uv0);
        float4 cDetailTexColor = gtxtTerrainDetailTexture.Sample(gSamplerState, input.uv1);
        // float fAlpha = gtxtTerrainAlphaTexture.Sample(gSamplerState, input.uv0);

        //cColor = saturate(lerp(cBaseTexColor, cDetailTexColor, fAlpha));
        cColor = cBaseTexColor * 0.5f + cDetailTexColor * 0.5f;
    }
    return (cColor);
}


struct VS_LIGHTING_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct VS_LIGHTING_OUTPUT
{
    float4 position : SV_POSITION;
    float3 positionW : POSITION;
    float3 normalW : NORMAL;
};

VS_LIGHTING_OUTPUT VSCubeMapping(VS_LIGHTING_INPUT input)
{
    VS_LIGHTING_OUTPUT output;

    output.positionW = mul(float4(input.position, 1.0f), gmtxGameObject).xyz;
//	output.positionW = (float3)mul(float4(input.position, 1.0f), gmtxWorld);
    output.normalW = mul(float4(input.normal, 0.0f), gmtxGameObject).xyz;
//	output.normalW = mul(input.normal, (float3x3)gmtxWorld);
    output.position = mul(mul(float4(output.positionW, 1.0f), gmtxView), gmtxProjection);

    return (output);
}

TextureCube gtxtCubeMap : register(t16);

float4 PSCubeMapping(VS_LIGHTING_OUTPUT input) : SV_Target
{
    input.normalW = normalize(input.normalW);

    // float4 cIllumination = Lighting(input.positionW, input.normalW);
     
    float3 vFromCamera = normalize(input.positionW - gvCameraPosition.xyz);
    float3 vReflected = normalize(reflect(vFromCamera, input.normalW));
    float4 cCubeTextureColor = gtxtCubeMap.Sample(gSamplerState, vReflected);

//	return(float4(vReflected * 0.5f + 0.5f, 1.0f));
    return (cCubeTextureColor);
//	return(cIllumination * cCubeTextureColor);
    
//    return (float4(1.0f, 1.0f, 1.0f, 1.0f));

}

VS_LIGHTING_OUTPUT VSMapping(VS_LIGHTING_INPUT input)
{
    VS_LIGHTING_OUTPUT output;

    // output.positionW = mul(float4(input.position, 1.0f), gmtxGameObject).xyz;
    // output.normalW = mul(float4(input.normal, 0.0f), gmtxGameObject).xyz;
    // output.position = mul(mul(float4(output.positionW, 1.0f), gmtxView), gmtxProjection);
    
   output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxGameObject), gmtxView), gmtxProjection);
   output.positionW = input.position;

    return (output);
}

float4 PSMapping(VS_LIGHTING_OUTPUT input) : SV_Target
{
   // input.normalW = normalize(input.normalW);
   // float3 vFromCamera = normalize(input.positionW - gvCameraPosition.xyz);
   // float3 vReflected = normalize(reflect(vFromCamera, input.normalW));
   // float4 cColor = gtxtCubeMap.Sample(gSamplerState, vReflected);
   input.positionW.x = 1.f - input.positionW.x;
   //input.positionW.z = 1.f - input.positionW.z;
    float4 cColor = gtxtCubeMap.Sample(gssClamp, input.positionW);

    return (cColor);


}
