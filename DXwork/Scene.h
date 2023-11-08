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

	void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	ID3D12RootSignature* CreateGraphicsRootSignature(ID3D12Device* pd3dDevice);
	void PrepareRender(ID3D12GraphicsCommandList* pd3dCommandList);
	ID3D12RootSignature* GetGraphicsRootSignature() { return(m_pd3dGraphicsRootSignature); }

	void AnimateObjects(float fTimeElapsed);
	void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL);
	void RenderBoundingBox(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
	CHeightMapTerrain* GetTerrain() { return(m_pTerrain); }


	ID3D12RootSignature* m_pd3dGraphicsRootSignature = NULL;

	CPlayer						*m_pPlayer = NULL;
	CSkyBox						*m_pSkyBox = NULL;

	CHeightMapTerrain			*m_pTerrain = NULL;

	CRippleWater				*m_pTerrainWater = NULL;
	XMFLOAT4X4					m_xmf4x4WaterAnimation;
	ID3D12Resource*				m_pd3dcbAnimation = NULL;
	XMFLOAT4X4*					m_pcbMappedAnimation = NULL;

	CBoundingBoxShader*			m_pBoundingBoxShader = NULL;

	int							m_nShaders = 0;
	CShader						**m_ppShaders = NULL;

};