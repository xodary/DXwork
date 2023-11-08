//------------------------------------------------------- ----------------------
// File: Object.h
//-----------------------------------------------------------------------------
#pragma once

#include "Mesh.h"
#include "Camera.h"
#include "Shader.h"

#define DIR_FORWARD					0x01
#define DIR_BACKWARD				0x02
#define DIR_LEFT					0x04
#define DIR_RIGHT					0x08
#define DIR_UP						0x10
#define DIR_DOWN					0x20

#define RESOURCE_TEXTURE2D			0x01
#define RESOURCE_TEXTURE2D_ARRAY	0x02	//[]
#define RESOURCE_TEXTURE2DARRAY		0x03
#define RESOURCE_TEXTURE_CUBE		0x04
#define RESOURCE_BUFFER				0x05

class CShader;

struct MATERIAL
{
	XMFLOAT4						m_xmf4Ambient;
	XMFLOAT4						m_xmf4Diffuse;
	XMFLOAT4						m_xmf4Specular; //(r,g,b,a=power)
	XMFLOAT4						m_xmf4Emissive;
};
struct CB_GAMEOBJECT_INFO
{
	XMFLOAT4X4						m_xmf4x4World;
	MATERIAL						m_material;

	UINT							m_nType;
};

class CGameObject;

class CTexture
{
public:
	CTexture(int nTextureResources, UINT nResourceType, int nSamplers, int nRootParameters);
	virtual ~CTexture();

private:
	int								m_nReferences = 0;

	UINT							m_nTextureType;
	UINT							*m_pnResourceTypes = NULL;

	int								m_nTextures = 0;
	_TCHAR							(*m_ppstrTextureNames)[64] = NULL;
	ID3D12Resource					**m_ppd3dTextures = NULL;
	ID3D12Resource					**m_ppd3dTextureUploadBuffers;
	DXGI_FORMAT						*m_pdxgiBufferFormats = NULL;
	int								*m_pnBufferElements = NULL;

	int								m_nRootParameters = 0;
	int								*m_pnRootParameterIndices = NULL;
	D3D12_GPU_DESCRIPTOR_HANDLE		*m_pd3dSrvGpuDescriptorHandles = NULL;

	int								m_nSamplers = 0;
	D3D12_GPU_DESCRIPTOR_HANDLE		*m_pd3dSamplerGpuDescriptorHandles = NULL;


public:

	void AddRef() { m_nReferences++; }
	void Release() { if (--m_nReferences <= 0) delete this; }

	void UpdateShaderVariable(ID3D12GraphicsCommandList* pd3dCommandList, int nParameterIndex, int nTextureIndex);
	void ReleaseUploadBuffers();
	void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);

	void LoadTextureFromDDSFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, const wchar_t* pszFileName, UINT nResourceType, UINT nIndex);
	void ReleaseShaderVariables();

	int GetTextures() { return(m_nTextures); }
	ID3D12Resource* GetResource(int nIndex) { return(m_ppd3dTextures[nIndex]); }
	int LoadTextureFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CGameObject* pParent, FILE* pInFile, CShader* pShader, UINT nIndex);
	_TCHAR* GetTextureName(int nIndex) { return(m_ppstrTextureNames[nIndex]); }

	UINT GetTextureType() { return(m_nTextureType); }
	UINT GetTextureType(int nIndex) { return(m_pnResourceTypes[nIndex]); }
	D3D12_SHADER_RESOURCE_VIEW_DESC GetShaderResourceViewDesc(int nIndex);

	void SetRootParameterIndex(int nIndex, UINT nRootParameterIndex);
	void SetGpuDescriptorHandle(int nIndex, D3D12_GPU_DESCRIPTOR_HANDLE d3dSrvGpuDescriptorHandle);
	int GetRootParameter(int nIndex) { return(m_pnRootParameterIndices[nIndex]); }
	int GetRootParameters() { return(m_nRootParameters); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle(int nIndex) { return(m_pd3dSrvGpuDescriptorHandles[nIndex]); }
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
#define MATERIAL_ALBEDO_MAP			0x01
#define MATERIAL_SPECULAR_MAP		0x02
#define MATERIAL_NORMAL_MAP			0x04
#define MATERIAL_METALLIC_MAP		0x08
#define MATERIAL_EMISSION_MAP		0x10
#define MATERIAL_DETAIL_ALBEDO_MAP	0x20
#define MATERIAL_DETAIL_NORMAL_MAP	0x40


class CMaterial
{
public:
	CMaterial();
	virtual ~CMaterial();

private:
	int								m_nReferences = 0;

public:
	void AddRef() { m_nReferences++; }
	void Release() { if (--m_nReferences <= 0) delete this; }

public:
	CShader							*m_pShader = NULL;
	CTexture						*m_pTexture = NULL;

	XMFLOAT4						m_xmf4AlbedoColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	XMFLOAT4						m_xmf4EmissiveColor = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4						m_xmf4SpecularColor = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4						m_xmf4AmbientColor = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	void SetShader(CShader* pShader);
	void SetMaterialType(UINT nType) { m_nType |= nType; }
	void SetTexture(CTexture* pTexture);

	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void ReleaseUploadBuffers();
	virtual void ReleaseShaderVariables();

	UINT							m_nType = 0x00;

	float							m_fGlossiness = 0.0f;
	float							m_fSmoothness = 0.0f;
	float							m_fSpecularHighlight = 0.0f;
	float							m_fMetallic = 0.0f;
	float							m_fGlossyReflection = 0.0f;
};

class CGameObject
{
private:
	int m_nReferences = 0;
public:
	void AddRef();
	void Release();

	CGameObject(int nMeshes, int nMaterials);
	~CGameObject();

	void UpdateTransform(XMFLOAT4X4* pxmf4x4Parent=NULL);

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera=NULL);
	virtual void OnPrepareRender() { }

	XMFLOAT3 GetPosition();
	XMFLOAT3 GetLook();
	XMFLOAT3 GetUp();
	XMFLOAT3 GetRight();

	virtual void SetMesh(int nIndex, CMesh* pMesh);
	void SetActive(bool bActive) { m_bActive = bActive; }
	void SetMovingSpeed(float fSpeed) { m_fMovingSpeed = fSpeed; }
	void SetShader(int nMaterial, CShader* pShader);
	void SetMaterial(int nMaterial, CMaterial* pMaterial);
	void SetChild(CGameObject* pChild);
	void SetPosition(float x, float y, float z);
	void SetPosition(XMFLOAT3 xmf3Position);
	void SetScale(float x, float y, float z);
	void SetBoundingBoxMesh(int nIndex, CBoundingBoxMesh* pMesh);
	void Rotate(float fPitch = 10.0f, float fYaw = 10.0f, float fRoll = 10.0f, XMFLOAT4X4* parent=NULL);
	void Rotate(XMFLOAT3* pxmf3Axis, float fAngle);
	void Rotate(XMFLOAT4* pxmf4Quaternion);

	void UpdateBoundingBox();
	void RenderBoundingBox(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);

	virtual void PrepareAnimate() { }
	virtual void Animate(float fTimeElapsed, XMFLOAT4X4* pxmf4x4Parent = NULL);

	virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void ReleaseShaderVariables();

	virtual void UpdateShaderVariable(ID3D12GraphicsCommandList* pd3dCommandList, XMFLOAT4X4* pxmf4x4World);

	virtual void ReleaseUploadBuffers();

	int FindReplicatedTexture(_TCHAR* pstrTextureName, D3D12_GPU_DESCRIPTOR_HANDLE* pd3dSrvGpuDescriptorHandle);

	UINT GetMeshType(UINT nIndex) { return((m_ppMeshes[nIndex]) ? m_ppMeshes[nIndex]->GetType() : 0x00); }

	void LoadMaterialsFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CGameObject* pParent, FILE* pInFile, CShader* pShader);

	static CGameObject* LoadFrameHierarchyFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CGameObject* pParent, FILE* pInFile, CShader* pShader);
	static CGameObject* LoadGeometryFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, char* pstrFileName, CShader* pShader);
	CGameObject* FindFrame(char* pstrFrameName);
	CGameObject* GetParent() { return(m_pParent); }

	static void PrintFrameInfo(CGameObject* pGameObject, CGameObject* pParent);

	bool							m_bActive = true;

	int								m_nMeshes = 0;
	CMesh							**m_ppMeshes = NULL;

	int								m_nMaterials = 0;
	CMaterial						**m_ppMaterials = NULL;

	XMFLOAT4X4						m_xmf4x4Transform;
	XMFLOAT4X4						m_xmf4x4World;

	CGameObject						*m_pParent = NULL;
	CGameObject						*m_pChild = NULL;
	CGameObject						*m_pSibling = NULL;

	ID3D12Resource					*m_pd3dcbGameObject = NULL;
	CB_GAMEOBJECT_INFO				*m_pcbMappedGameObject = NULL;

	char							m_pstrFrameName[64];

	BoundingOrientedBox				*m_pxmBoundingBoxes;
	CBoundingBoxMesh				**m_ppBoundingBoxMeshes = NULL;
	
	float							m_fMovingSpeed = 0.0f;
	float							m_fMovingRange = 0.0f;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CSkyBox : public CGameObject
{
public:
	CSkyBox(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature);
	virtual ~CSkyBox();

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL);
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CHeightMapTerrain : public CGameObject
{
public:
	CHeightMapTerrain(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, LPCTSTR pFileName, int nWidth, int nLength, int nBlockWidth, int nBlockLength, XMFLOAT3 xmf3Scale, XMFLOAT4 xmf4Color);
	virtual ~CHeightMapTerrain();

private:
	CHeightMapImage* m_pHeightMapImage;

	int								m_nWidth;
	int								m_nLength;

	XMFLOAT3						m_xmf3Scale;

public:
	float GetHeight(float x, float z, bool bReverseQuad = false) { return(m_pHeightMapImage->GetHeight(x, z, bReverseQuad) * m_xmf3Scale.y); } //World
	XMFLOAT3 GetNormal(float x, float z) { return(m_pHeightMapImage->GetHeightMapNormal(int(x / m_xmf3Scale.x), int(z / m_xmf3Scale.z))); }

	int GetHeightMapWidth() { return(m_pHeightMapImage->GetHeightMapWidth()); }
	int GetHeightMapLength() { return(m_pHeightMapImage->GetHeightMapLength()); }

	XMFLOAT3 GetScale() { return(m_xmf3Scale); }
	float GetWidth() { return(m_nWidth * m_xmf3Scale.x); }
	float GetLength() { return(m_nLength * m_xmf3Scale.z); }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CRippleWater : public CGameObject
{
public:
	CRippleWater(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nWidth, int nLength, int nBlockWidth, int nBlockLength, XMFLOAT3 xmf3Scale, XMFLOAT4 xmf4Color);
	virtual ~CRippleWater();

private:
	int								m_nWidth;
	int								m_nLength;

	XMFLOAT3						m_xmf3Scale;

public:
	XMFLOAT3 GetScale() { return(m_xmf3Scale); }
	float GetWidth() { return(m_nWidth * m_xmf3Scale.x); }
	float GetLength() { return(m_nLength * m_xmf3Scale.z); }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

class CBulletObject : public CGameObject
{
public:
	XMFLOAT3					m_xmf3FirePosition = XMFLOAT3(0.0f, 0.0f, 1.0f);

	CBulletObject();
	virtual ~CBulletObject();
	virtual void PrepareAnimate();
	virtual void Animate(float fTimeElapsed, XMFLOAT4X4* pxmf4x4Parent = NULL);

	void SetFirePosition(XMFLOAT3 xmf3FirePosition);
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CTankObject : public CGameObject
{
public:
	CGameObject					*m_pBodyFrame;
	CGameObject					*m_pTurretFrame;
	CGameObject					*m_pTracksBackLeftFrame;
	CGameObject					*m_pTracksBackRightFrame;
	CGameObject					*m_pTracksFrontLeftFrame;
	CGameObject					*m_pTracksFrontRightFrame;

	CTankObject();
	virtual ~CTankObject();

	virtual void PrepareAnimate();
};