//-----------------------------------------------------------------------------
// File: Scene.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "Scene.h"

CScene::CScene()
{
	m_xmf4x4WaterAnimation = XMFLOAT4X4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

CScene::~CScene()
{
}

void CScene::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	CreateShaderVariables(pd3dDevice, pd3dCommandList);
	m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);

	m_pBoundingBoxShader = new CBoundingBoxShader();
	m_pBoundingBoxShader->CreateShader(pd3dDevice, m_pd3dGraphicsRootSignature, D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);

	m_pSkyBox = new CSkyBox(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	XMFLOAT3 xmf3Scale = XMFLOAT3(8.0f, 2.0f, 8.0f);
	XMFLOAT4 xmf4Color(0.0f, 0.5f, 0.0f, 0.0f);

	m_pTerrain = new CHeightMapTerrain(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, _T("Image/HeightMap.raw"), 257, 257, 13, 13, xmf3Scale, xmf4Color);
	
	m_pCubeMapSkyboxShader = new CCubeMapSkyboxShader(2048,256);
	m_pCubeMapSkyboxShader->CreateShader(pd3dDevice, m_pd3dGraphicsRootSignature);
	m_pCubeMapSkyboxShader->BuildObjects(pd3dDevice, pd3dCommandList);

	// m_pTerrainWater = new CRippleWater(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, 257, 257, 17, 17, xmf3Scale, xmf4Color);
	// m_pTerrainWater->SetPosition(+(257 * 0.5f), 155.0f, +(257 * 0.5f));

	m_nShaders = 4;
	m_ppShaders = new CShader * [m_nShaders];
	
	CBulletShader* pBulletsShader = new CBulletShader();
	pBulletsShader->CreateShader(pd3dDevice, m_pd3dGraphicsRootSignature);
	pBulletsShader->BuildObjects(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, this, m_pTerrain);
	m_ppShaders[0] = pBulletsShader;
	
	CEnermyShader* pEnermyShader = new CEnermyShader();
	pEnermyShader->CreateShader(pd3dDevice, m_pd3dGraphicsRootSignature);
	pEnermyShader->BuildObjects(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, this, m_pTerrain);
	m_ppShaders[1] = pEnermyShader;
	pEnermyShader->m_pBulletShader = pBulletsShader;
	
	CTreeShader* pTreeShader = new CTreeShader();
	pTreeShader->CreateShader(pd3dDevice, m_pd3dGraphicsRootSignature);
	pTreeShader->BuildObjects(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, this, m_pTerrain);
	m_ppShaders[2] = pTreeShader;
	AddCollisionObject(pTreeShader, m_ppCollisionObjects, m_nCollisionObject);
	
	CBillboardObjectsShader* pBillboardObjectShader = new CBillboardObjectsShader();
	pBillboardObjectShader->CreateShader(pd3dDevice, m_pd3dGraphicsRootSignature);
	pBillboardObjectShader->BuildObjects(pd3dDevice, pd3dCommandList, m_pTerrain);
	m_ppShaders[3] = pBillboardObjectShader;

	m_nEnvironmentMappingShaders = 1;
	m_ppEnvironmentMappingShaders = new CDynamicCubeMappingShader * [m_nEnvironmentMappingShaders];
	
	m_ppEnvironmentMappingShaders[0] = new CDynamicCubeMappingShader(256);
	m_ppEnvironmentMappingShaders[0]->CreateShader(pd3dDevice, m_pd3dGraphicsRootSignature);
	m_ppEnvironmentMappingShaders[0]->BuildObjects(pd3dDevice, pd3dCommandList, m_pTerrain);

	m_nEnvironmentMapObjects = 1;
	m_ppEnvironmentMapObjects = new CGameObject * [m_nEnvironmentMapObjects];
	m_ppEnvironmentMapObjects[0] = m_ppEnvironmentMappingShaders[0]->m_ppObjects[0];
}

void CScene::AddCollisionObject(CShader* pShader, CGameObject**& ppObject, int& nObject)
{
	int newObjects = pShader->m_nObject;
	CGameObject** ppCollisionObjects = new CGameObject * [nObject + newObjects];
	for (int i = 0; i < nObject; ++i)
	{
		ppCollisionObjects[i] = ppObject[i];
	}
	for (int i = 0; i < newObjects; ++i)
	{
		ppCollisionObjects[nObject + i] = pShader->m_ppObjects[i];
	}
	nObject += newObjects;

	delete[] ppObject;
	ppObject = ppCollisionObjects;
}

ID3D12RootSignature* CScene::CreateGraphicsRootSignature(ID3D12Device* pd3dDevice)
{
	ID3D12RootSignature* pd3dGraphicsRootSignature = NULL;

	D3D12_DESCRIPTOR_RANGE pd3dDescriptorRanges[7];

	pd3dDescriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[0].NumDescriptors = 1;
	pd3dDescriptorRanges[0].BaseShaderRegister = 0; //t0: skybox
	pd3dDescriptorRanges[0].RegisterSpace = 0;
	pd3dDescriptorRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[1].NumDescriptors = 3;
	pd3dDescriptorRanges[1].BaseShaderRegister = 1; //t1~t3: water
	pd3dDescriptorRanges[1].RegisterSpace = 0;
	pd3dDescriptorRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[2].NumDescriptors = 4;
	pd3dDescriptorRanges[2].BaseShaderRegister = 4; //t4~t7: terrain
	pd3dDescriptorRanges[2].RegisterSpace = 0;
	pd3dDescriptorRanges[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[3].NumDescriptors = 6;
	pd3dDescriptorRanges[3].BaseShaderRegister = 8; //t8 ~ t14: gtxtAlbedoTexture
	pd3dDescriptorRanges[3].RegisterSpace = 0;
	pd3dDescriptorRanges[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[4].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	pd3dDescriptorRanges[4].NumDescriptors = 1;
	pd3dDescriptorRanges[4].BaseShaderRegister = 2; //GameObject
	pd3dDescriptorRanges[4].RegisterSpace = 0;
	pd3dDescriptorRanges[4].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[5].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[5].NumDescriptors = 1;
	pd3dDescriptorRanges[5].BaseShaderRegister = 15; //t15: gtxtTexture
	pd3dDescriptorRanges[5].RegisterSpace = 0;
	pd3dDescriptorRanges[5].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[6].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[6].NumDescriptors = 1;
	pd3dDescriptorRanges[6].BaseShaderRegister = 16; //t16: gtxtTextureCube
	pd3dDescriptorRanges[6].RegisterSpace = 0;
	pd3dDescriptorRanges[6].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER pd3dRootParameters[11];

	pd3dRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[0].Descriptor.ShaderRegister = 0; //Player
	pd3dRootParameters[0].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	pd3dRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[1].Descriptor.ShaderRegister = 1; //Camera
	pd3dRootParameters[1].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	
	pd3dRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[2].DescriptorTable.pDescriptorRanges = &pd3dDescriptorRanges[4];	//GameObject
	pd3dRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;	// skybox
	pd3dRootParameters[3].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[3].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[0]);
	pd3dRootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;	// water
	pd3dRootParameters[4].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[4].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[1]);
	pd3dRootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;	// terrain
	pd3dRootParameters[5].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[5].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[2]);
	pd3dRootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[6].Descriptor.ShaderRegister = 4;		//Time, ElapsedTime
	pd3dRootParameters[6].Descriptor.RegisterSpace = 0;			//Time
	pd3dRootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[7].Descriptor.ShaderRegister = 5;		//Texture Animation (4x4) Matrix
	pd3dRootParameters[7].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;	// Standard Texture
	pd3dRootParameters[8].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[8].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[3]);
	pd3dRootParameters[8].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[9].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[9].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[9].DescriptorTable.pDescriptorRanges = &pd3dDescriptorRanges[5];	// gtxtTexture
	pd3dRootParameters[9].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[10].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[10].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[10].DescriptorTable.pDescriptorRanges = &pd3dDescriptorRanges[6]; //t8: gtxtTextureCube
	pd3dRootParameters[10].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_STATIC_SAMPLER_DESC pd3dSamplerDescs[2];

	pd3dSamplerDescs[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	pd3dSamplerDescs[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pd3dSamplerDescs[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pd3dSamplerDescs[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pd3dSamplerDescs[0].MipLODBias = 0;
	pd3dSamplerDescs[0].MaxAnisotropy = 1;
	pd3dSamplerDescs[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	pd3dSamplerDescs[0].MinLOD = 0;
	pd3dSamplerDescs[0].MaxLOD = D3D12_FLOAT32_MAX;
	pd3dSamplerDescs[0].ShaderRegister = 0;
	pd3dSamplerDescs[0].RegisterSpace = 0;
	pd3dSamplerDescs[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dSamplerDescs[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	pd3dSamplerDescs[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pd3dSamplerDescs[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pd3dSamplerDescs[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pd3dSamplerDescs[1].MipLODBias = 0;
	pd3dSamplerDescs[1].MaxAnisotropy = 1;
	pd3dSamplerDescs[1].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	pd3dSamplerDescs[1].MinLOD = 0;
	pd3dSamplerDescs[1].MaxLOD = D3D12_FLOAT32_MAX;
	pd3dSamplerDescs[1].ShaderRegister = 1;
	pd3dSamplerDescs[1].RegisterSpace = 0;
	pd3dSamplerDescs[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags = 
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
	D3D12_ROOT_SIGNATURE_DESC d3dRootSignatureDesc;
	::ZeroMemory(&d3dRootSignatureDesc, sizeof(D3D12_ROOT_SIGNATURE_DESC));
	d3dRootSignatureDesc.NumParameters = _countof(pd3dRootParameters);
	d3dRootSignatureDesc.pParameters = pd3dRootParameters;
	d3dRootSignatureDesc.NumStaticSamplers = _countof(pd3dSamplerDescs);
	d3dRootSignatureDesc.pStaticSamplers = pd3dSamplerDescs;
	d3dRootSignatureDesc.Flags = d3dRootSignatureFlags;

	ID3DBlob* pd3dSignatureBlob = NULL;
	ID3DBlob* pd3dErrorBlob = NULL;
	D3D12SerializeRootSignature(&d3dRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pd3dSignatureBlob, &pd3dErrorBlob);
	pd3dDevice->CreateRootSignature(0, pd3dSignatureBlob->GetBufferPointer(), pd3dSignatureBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&pd3dGraphicsRootSignature);
	if (pd3dSignatureBlob) pd3dSignatureBlob->Release();
	if (pd3dErrorBlob) pd3dErrorBlob->Release();

	return(pd3dGraphicsRootSignature);
}

void CScene::OnPreRender(ID3D12Device* pd3dDevice, ID3D12CommandQueue* pd3dCommandQueue, ID3D12Fence* pd3dFence, HANDLE hFenceEvent)
{
	for (int i = 0; i < m_nEnvironmentMappingShaders; i++)
	{
		if(m_ppEnvironmentMappingShaders[i])
			m_ppEnvironmentMappingShaders[i]->OnPreRender(pd3dDevice, pd3dCommandQueue, pd3dFence, hFenceEvent, this);
	}
	if(m_pCubeMapSkyboxShader && inMirror)
		m_pCubeMapSkyboxShader->OnPreRender(pd3dDevice, pd3dCommandQueue, pd3dFence, hFenceEvent, this);
}

void CScene::AnimateObjects(float fTimeElapsed)
{
	m_ppShaders[0]->AnimateObjects(fTimeElapsed);
	if (inMirror) return;
	if(m_nShaders>1)
		((CEnermyShader*)m_ppShaders[1])->m_pPlayer = m_pPlayer;
	for (int i = 0; i < m_nCollisionObject; i++)
	{
		m_ppCollisionObjects[i]->UpdateBoundingBox();
	}
	for (int i = 0; i < m_nEnvironmentMapObjects; i++)
	{
		m_ppEnvironmentMapObjects[i]->UpdateBoundingBox();
	}
	for (int i = 1; i < m_nShaders; i++)
	{
		if (m_ppShaders[i])
		{
			m_ppShaders[i]->AnimateObjects(fTimeElapsed);
		}
	}
	for (int i = 0; i < m_nEnvironmentMappingShaders; i++)
	{
		if (m_ppEnvironmentMappingShaders[i])
		{
			m_ppEnvironmentMappingShaders[i]->AnimateObjects(fTimeElapsed);
		}
	}
}

void CScene::OnPrepareRender(ID3D12GraphicsCommandList* pd3dCommandList)
{
	pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);

	UpdateShaderVariables(pd3dCommandList);

	D3D12_GPU_VIRTUAL_ADDRESS d3dcbAnimationGpuVirtualAddress = m_pd3dcbAnimation->GetGPUVirtualAddress();
	pd3dCommandList->SetGraphicsRootConstantBufferView(PARAMETER_ANIMATION, d3dcbAnimationGpuVirtualAddress); //Animation

	//if (m_pd3dcbLights)
	//{
	//	D3D12_GPU_VIRTUAL_ADDRESS d3dcbLightsGpuVirtualAddress = m_pd3dcbLights->GetGPUVirtualAddress();
	//	pd3dCommandList->SetGraphicsRootConstantBufferView(4, d3dcbLightsGpuVirtualAddress); //Lights
	//}
	//if (m_pd3dcbMaterials)
	//{
	//	D3D12_GPU_VIRTUAL_ADDRESS d3dcbMaterialsGpuVirtualAddress = m_pd3dcbMaterials->GetGPUVirtualAddress();
	//	pd3dCommandList->SetGraphicsRootConstantBufferView(3, d3dcbMaterialsGpuVirtualAddress); //Materials
	//}
}

void CScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	pCamera->SetViewportsAndScissorRects(pd3dCommandList);
	pCamera->UpdateShaderVariables(pd3dCommandList);

	if (m_pSkyBox) m_pSkyBox->Render(pd3dCommandList, pCamera);
	if (m_pCubeMapSkyboxShader && inMirror) m_pCubeMapSkyboxShader->Render(pd3dCommandList, pCamera);
	if (m_pTerrain) m_pTerrain->Render(pd3dCommandList, pCamera);
	if (m_pTerrainWater) m_pTerrainWater->Render(pd3dCommandList, pCamera);
	
	for (int i = 0; i < m_nShaders; i++) if (m_ppShaders[i]) m_ppShaders[i]->Render(pd3dCommandList, pCamera);
	
	for (int i = 0; i < m_nEnvironmentMappingShaders; i++) if (m_ppEnvironmentMappingShaders[i]) m_ppEnvironmentMappingShaders[i]->Render(pd3dCommandList, pCamera);
}

void CScene::RenderBoundingBox(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	m_pBoundingBoxShader->Render(pd3dCommandList, pCamera);
	for (int i = 0; i < m_nCollisionObject; i++)
	{
		if (m_ppCollisionObjects[i])
		{
			m_ppCollisionObjects[i]->RenderBoundingBox(pd3dCommandList, pCamera);
		}
	}
	for (int i = 0; i < ((CEnermyShader*)m_ppShaders[1])->m_nBullets; ++i)
	{
		isCollided = false;
		if (((CEnermyShader*)m_ppShaders[1])->m_ppBullets[i] == NULL) continue;
		((CEnermyShader*)m_ppShaders[1])->m_ppBullets[i]->RenderBoundingBox(pd3dCommandList, pCamera);
	}

	m_pPlayer->RenderBoundingBox(pd3dCommandList, pCamera);
}

void CScene::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	UINT ncbElementBytes = ((sizeof(XMFLOAT4X4) + 255) & ~255); //256의 배수
	m_pd3dcbAnimation = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	m_pd3dcbAnimation->Map(0, NULL, (void**)&m_pcbMappedAnimation);
}

void CScene::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	::memcpy(m_pcbMappedAnimation, &m_xmf4x4WaterAnimation, sizeof(XMFLOAT4));
}

void CScene::ReleaseShaderVariables()
{
	if (m_pd3dcbAnimation)
	{
		m_pd3dcbAnimation->Unmap(0, NULL);
		m_pd3dcbAnimation->Release();
	}

	if (m_pSkyBox) m_pSkyBox->ReleaseShaderVariables();
	if (m_pTerrain) m_pTerrain->ReleaseShaderVariables();
	if (m_pCubeMapSkyboxShader) m_pCubeMapSkyboxShader->ReleaseShaderVariables();
	if (m_pTerrainWater) m_pTerrainWater->ReleaseShaderVariables();

	for (int i = 0; i < m_nShaders; i++) if (m_ppShaders[i]) m_ppShaders[i]->ReleaseShaderVariables();
}

void CScene::ReleaseObjects()
{
	if (m_pd3dGraphicsRootSignature) m_pd3dGraphicsRootSignature->Release();

	if (m_ppShaders)
	{
		for (int i = 0; i < m_nShaders; i++)
		{
			m_ppShaders[i]->ReleaseShaderVariables();
			m_ppShaders[i]->ReleaseObjects();
			m_ppShaders[i]->Release();
		}
		delete[] m_ppShaders;
	}

	if (m_ppEnvironmentMappingShaders)
	{
		for (int i = 0; i < m_nEnvironmentMappingShaders; i++)
		{
			m_ppEnvironmentMappingShaders[i]->ReleaseShaderVariables();
			m_ppEnvironmentMappingShaders[i]->ReleaseObjects();
			m_ppEnvironmentMappingShaders[i]->Release();
		}
		delete[] m_ppEnvironmentMappingShaders;
	}

	if (m_pSkyBox) delete m_pSkyBox;
	if (m_pTerrain) delete m_pTerrain;
	if (m_pCubeMapSkyboxShader) delete m_pCubeMapSkyboxShader;
	if (m_pTerrainWater) delete m_pTerrainWater;

	ReleaseShaderVariables();
}

void CScene::ReleaseUploadBuffers()
{
	for (int i = 0; i < m_nShaders; i++) m_ppShaders[i]->ReleaseUploadBuffers();
	for (int i = 0; i < m_nEnvironmentMappingShaders; i++) m_ppEnvironmentMappingShaders[i]->ReleaseUploadBuffers();

	if (m_pSkyBox) m_pSkyBox->ReleaseUploadBuffers();
	if (m_pTerrain) m_pTerrain->ReleaseUploadBuffers();
	if (m_pTerrainWater) m_pTerrainWater->ReleaseUploadBuffers();
	if (m_pCubeMapSkyboxShader) m_pCubeMapSkyboxShader->ReleaseUploadBuffers();

	// for (int i = 0; i < m_nObjects; i++) m_ppObjects[i]->ReleaseUploadBuffers();
}

bool CScene::CheckSceneCollisions(CGameObject* pTargetGameObject)
{
	if (inMirror) return false;
	for (int i = 0; i < m_nCollisionObject; i++)
	{
		isCollided = false;
		if (CheckObjectByObjectCollisions(m_ppCollisionObjects[i], pTargetGameObject))
		{
			std::cout << "충돌발생 CScene::CheckSceneCollisions" << std::endl;
			return true;
		}
	}
	for (int i = 0; i < ((CEnermyShader*)m_ppShaders[1])->m_nBullets; ++i)
	{
		isCollided = false;
		if (((CEnermyShader*)m_ppShaders[1])->m_ppBullets[i] == NULL) continue;
 		if (CheckObjectByObjectCollisions(((CEnermyShader*)m_ppShaders[1])->m_ppBullets[i], pTargetGameObject))
		{
			std::cout << "충돌발생 CScene::CheckSceneCollisions" << std::endl;
			return true;
		}
	}
	return(false);
}

bool CScene::CheckEnvironmentMapCollision()
{
	if (inMirror) return false;
	if (m_nEnvironmentMappingShaders < 1) return false;
	for (int i = 0; i < m_nEnvironmentMapObjects; i++)
	{
		if (CheckObjectByObjectCollisions(m_ppEnvironmentMapObjects[i], m_pPlayer, true))
		{
			std::cout << "충돌발생 CScene::CheckEnvironmentMapCollision" << std::endl;
			return true;
		}
	}
	return false;
}

bool CScene::CheckObjectByObjectCollisions(CGameObject* pObjectA, CGameObject* pObjectB, bool isAsphere)
{
	if (isAsphere)
	{
		for (int p = 0; p < pObjectA->m_nMeshes; ++p) {
			for (int q = 0; q < pObjectB->m_nMeshes; ++q) {
				if (pObjectA->m_xmBoundingSpheres[p].Intersects(pObjectB->m_pxmBoundingBoxes[q]))
				{
					std::cout << "충돌발생 CScene::CheckObjectByObjectCollisions isAsphere" << std::endl;
					isCollided = true;
					return isCollided;
				}
			}
		}
	}
	else
	{
		for (int p = 0; p < pObjectA->m_nMeshes; ++p) {
			for (int q = 0; q < pObjectB->m_nMeshes; ++q) {
				if (pObjectA->m_pxmBoundingBoxes[p].Intersects(pObjectB->m_pxmBoundingBoxes[q]))
				{
					std::cout << "충돌발생 CScene::CheckObjectByObjectCollisions Asphere" << std::endl;
					std::cout << pObjectA->m_pstrFrameName << " & " << pObjectB->m_pstrFrameName << std::endl;
					isCollided = true;
					return isCollided;
				}
			}
		}
	}
	if (pObjectB->m_pSibling) CheckObjectByObjectCollisions(pObjectA, pObjectB->m_pSibling, isAsphere);
	if (pObjectB->m_pChild) CheckObjectByObjectCollisions(pObjectA, pObjectB->m_pChild, isAsphere);
	if (pObjectA->m_pSibling) CheckObjectByObjectCollisions(pObjectA->m_pSibling, pObjectB, isAsphere);
	if (pObjectA->m_pChild) CheckObjectByObjectCollisions(pObjectA->m_pChild, pObjectB, isAsphere);
	return(isCollided);
}

bool CScene::CheckInMirrorCollision()
{
	if (!inMirror) return false;
	XMFLOAT3 MirroPos = m_pCubeMapSkyboxShader->m_ppObjects[0]->GetPosition();
	float boxsize = m_pCubeMapSkyboxShader->m_nBoxSize / 2;
	XMFLOAT3 playerPos = m_pPlayer->GetPosition();
	if (playerPos.z < MirroPos.z - boxsize || playerPos.z > MirroPos.z + boxsize)
	{
		return true;
	}
	if (playerPos.x < MirroPos.x - boxsize || playerPos.x > MirroPos.x + boxsize)
	{
		return true;
	}
	return false;
}