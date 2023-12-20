//-----------------------------------------------------------------------------
// File: Scene.h
//-----------------------------------------------------------------------------

#pragma once
#include "Shader.h"
#include "Player.h"

class CScene
{
public:
	CScene();
	~CScene();

	virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void ReleaseShaderVariables();
	virtual void ReleaseObjects();
	virtual void ReleaseUploadBuffers();

	bool CheckSceneCollisions(CGameObject* pTargetGameObject);
	bool CheckEnvironmentMapCollision();
	bool CheckObjectByObjectCollisions(CGameObject* pObjectA, CGameObject* pObjectB, bool isAsphere=false);
	bool CheckInMirrorCollision();
	bool CheckObjectByBoundingBoxCollision(BoundingOrientedBox box, CGameObject* object);
	void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void AddCollisionObject(CShader* pShader, CGameObject**& ppObject, int& nObject);
	ID3D12RootSignature* CreateGraphicsRootSignature(ID3D12Device* pd3dDevice);
	void OnPreRender(ID3D12Device* pd3dDevice, ID3D12CommandQueue* pd3dCommandQueue, ID3D12Fence* pd3dFence, HANDLE hFenceEvent);
	void OnPrepareRender(ID3D12GraphicsCommandList* pd3dCommandList);
	ID3D12RootSignature* GetGraphicsRootSignature() { return(m_pd3dGraphicsRootSignature); }

	void AnimateObjects(float fTimeElapsed);
	void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL);
	void RenderBoundingBox(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
	CHeightMapTerrain* GetTerrain() { return(m_pTerrain); }


	ID3D12RootSignature* m_pd3dGraphicsRootSignature = NULL;

	CPlayer						*m_pPlayer = NULL;
	CSkyBox						*m_pSkyBox = NULL;
	CCubeMapSkyboxShader		*m_pCubeMapSkyboxShader = NULL;
	CHeightMapTerrain			*m_pTerrain = NULL;

	CDynamicCubeMappingShader	**m_ppEnvironmentMappingShaders = NULL;
	int							m_nEnvironmentMappingShaders = 0;

	CRippleWater				*m_pTerrainWater = NULL;
	XMFLOAT4X4					m_xmf4x4WaterAnimation;
	ID3D12Resource*				m_pd3dcbAnimation = NULL;
	XMFLOAT4X4*					m_pcbMappedAnimation = NULL;

	CBoundingBoxShader*			m_pBoundingBoxShader = NULL;

	int							m_nShaders = 0;
	CShader						**m_ppShaders = NULL;

	CGameObject					**m_ppCollisionObjects = NULL;
	int							m_nCollisionObject = 0;
	
	CGameObject					**m_ppEnvironmentMapObjects = NULL;
	int							m_nEnvironmentMapObjects = 0;

	bool						isCollided = false;

	bool						inMirror = false;
};