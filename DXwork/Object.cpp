//------------------------------------------------------- ----------------------
// File: Object.cpp
//-----------------------------------------------------------------------------
#include "stdafx.h"
#include "Object.h"
#include "Scene.h"

CTexture::CTexture(int nTextures, UINT nTextureType, int nSamplers, int nRootParameters)
{
	m_nTextureType = nTextureType;

	m_nTextures = nTextures;
	if (m_nTextures > 0)
	{
		m_ppd3dTextureUploadBuffers = new ID3D12Resource * [m_nTextures];
		m_ppd3dTextures = new ID3D12Resource * [m_nTextures];
		for (int i = 0; i < m_nTextures; i++) m_ppd3dTextureUploadBuffers[i] = m_ppd3dTextures[i] = NULL;

		m_ppstrTextureNames = new _TCHAR[m_nTextures][64];
		for (int i = 0; i < m_nTextures; i++) m_ppstrTextureNames[i][0] = '\0';

		m_pd3dSrvGpuDescriptorHandles = new D3D12_GPU_DESCRIPTOR_HANDLE[m_nTextures];
		for (int i = 0; i < m_nTextures; i++) m_pd3dSrvGpuDescriptorHandles[i].ptr = NULL;

		m_pnResourceTypes = new UINT[m_nTextures];
		m_pdxgiBufferFormats = new DXGI_FORMAT[m_nTextures];
		m_pnBufferElements = new int[m_nTextures];
	}
	m_nRootParameters = nRootParameters;
	if (nRootParameters > 0) m_pnRootParameterIndices = new int[nRootParameters];
	for (int i = 0; i < m_nRootParameters; i++) m_pnRootParameterIndices[i] = -1;

	m_nSamplers = nSamplers;
	if (m_nSamplers > 0) m_pd3dSamplerGpuDescriptorHandles = new D3D12_GPU_DESCRIPTOR_HANDLE[m_nSamplers];
}

CTexture::~CTexture()
{
	if (m_ppd3dTextures)
	{
		for (int i = 0; i < m_nTextures; i++) if (m_ppd3dTextures[i]) m_ppd3dTextures[i]->Release();
		delete[] m_ppd3dTextures;
	}

	if (m_ppstrTextureNames) delete[] m_ppstrTextureNames;

	if (m_pnResourceTypes) delete[] m_pnResourceTypes;
	if (m_pdxgiBufferFormats) delete[] m_pdxgiBufferFormats;
	if (m_pnBufferElements) delete[] m_pnBufferElements;

	if (m_pnRootParameterIndices) delete[] m_pnRootParameterIndices;
	if (m_pd3dSrvGpuDescriptorHandles) delete[] m_pd3dSrvGpuDescriptorHandles;

	if (m_pd3dSamplerGpuDescriptorHandles) delete[] m_pd3dSamplerGpuDescriptorHandles;
}

void CTexture::LoadTextureFromDDSFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, const wchar_t* pszFileName, UINT nResourceType, UINT nIndex)
{
	m_pnResourceTypes[nIndex] = nResourceType;
	m_ppd3dTextures[nIndex] = ::CreateTextureResourceFromDDSFile(pd3dDevice, pd3dCommandList, pszFileName, &m_ppd3dTextureUploadBuffers[nIndex], D3D12_RESOURCE_STATE_GENERIC_READ/*D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE*/);
}

void CTexture::SetRootParameterIndex(int nIndex, UINT nRootParameterIndex)
{
	m_pnRootParameterIndices[nIndex] = nRootParameterIndex;
}

void CTexture::SetGpuDescriptorHandle(int nIndex, D3D12_GPU_DESCRIPTOR_HANDLE d3dSrvGpuDescriptorHandle)
{
	m_pd3dSrvGpuDescriptorHandles[nIndex] = d3dSrvGpuDescriptorHandle;
}

void CTexture::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (m_nRootParameters == m_nTextures)
	{
		for (int i = 0; i < m_nRootParameters; i++)
		{
			if (m_pd3dSrvGpuDescriptorHandles[i].ptr && (m_pnRootParameterIndices[i] != -1)) pd3dCommandList->SetGraphicsRootDescriptorTable(m_pnRootParameterIndices[i], m_pd3dSrvGpuDescriptorHandles[i]);
		}
	}
	else
	{
		if (m_pd3dSrvGpuDescriptorHandles[0].ptr) pd3dCommandList->SetGraphicsRootDescriptorTable(m_pnRootParameterIndices[0], m_pd3dSrvGpuDescriptorHandles[0]);
	}
}

void CTexture::ReleaseShaderVariables()
{
}

void CTexture::ReleaseUploadBuffers()
{
	if (m_ppd3dTextureUploadBuffers)
	{
		for (int i = 0; i < m_nTextures; i++) if (m_ppd3dTextureUploadBuffers[i]) m_ppd3dTextureUploadBuffers[i]->Release();
		delete[] m_ppd3dTextureUploadBuffers;
		m_ppd3dTextureUploadBuffers = NULL;
	}
}

D3D12_SHADER_RESOURCE_VIEW_DESC CTexture::GetShaderResourceViewDesc(int nIndex)
{
	ID3D12Resource* pShaderResource = GetResource(nIndex);
	D3D12_RESOURCE_DESC d3dResourceDesc = pShaderResource->GetDesc();

	D3D12_SHADER_RESOURCE_VIEW_DESC d3dShaderResourceViewDesc;
	d3dShaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	int nTextureType = GetTextureType(nIndex);
	switch (nTextureType)
	{
	case RESOURCE_TEXTURE2D: //(d3dResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)(d3dResourceDesc.DepthOrArraySize == 1)
	case RESOURCE_TEXTURE2D_ARRAY: //[]
		d3dShaderResourceViewDesc.Format = d3dResourceDesc.Format;
		d3dShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		d3dShaderResourceViewDesc.Texture2D.MipLevels = -1;
		d3dShaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
		d3dShaderResourceViewDesc.Texture2D.PlaneSlice = 0;
		d3dShaderResourceViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		break;
	case RESOURCE_TEXTURE2DARRAY: //(d3dResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)(d3dResourceDesc.DepthOrArraySize != 1)
		d3dShaderResourceViewDesc.Format = d3dResourceDesc.Format;
		d3dShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		d3dShaderResourceViewDesc.Texture2DArray.MipLevels = -1;
		d3dShaderResourceViewDesc.Texture2DArray.MostDetailedMip = 0;
		d3dShaderResourceViewDesc.Texture2DArray.PlaneSlice = 0;
		d3dShaderResourceViewDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
		d3dShaderResourceViewDesc.Texture2DArray.FirstArraySlice = 0;
		d3dShaderResourceViewDesc.Texture2DArray.ArraySize = d3dResourceDesc.DepthOrArraySize;
		break;
	case RESOURCE_TEXTURE_CUBE: //(d3dResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)(d3dResourceDesc.DepthOrArraySize == 6)
		d3dShaderResourceViewDesc.Format = d3dResourceDesc.Format;
		d3dShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		d3dShaderResourceViewDesc.TextureCube.MipLevels = 1;
		d3dShaderResourceViewDesc.TextureCube.MostDetailedMip = 0;
		d3dShaderResourceViewDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		break;
	case RESOURCE_BUFFER: //(d3dResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
		d3dShaderResourceViewDesc.Format = m_pdxgiBufferFormats[nIndex];
		d3dShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		d3dShaderResourceViewDesc.Buffer.FirstElement = 0;
		d3dShaderResourceViewDesc.Buffer.NumElements = m_pnBufferElements[nIndex];
		d3dShaderResourceViewDesc.Buffer.StructureByteStride = 0;
		d3dShaderResourceViewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		break;
	}
	return(d3dShaderResourceViewDesc);
}

int CTexture::LoadTextureFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CGameObject* pParent, FILE* pInFile, CShader* pShader, UINT nIndex)
{
	char pstrTextureName[64] = { '\0' };

	BYTE nStrLength = 64;
	UINT nReads = (UINT)::fread(&nStrLength, sizeof(BYTE), 1, pInFile);
	nReads = (UINT)::fread(pstrTextureName, sizeof(char), nStrLength, pInFile);
	pstrTextureName[nStrLength] = '\0';

	bool bDuplicated = false;
	bool bLoaded = false;
	if (strcmp(pstrTextureName, "null"))
	{
		bLoaded = true;
		char pstrFilePath[64] = { '\0' };
		strcpy_s(pstrFilePath, 64, "Model/Textures/");

		bDuplicated = (pstrTextureName[0] == '@');
		strcpy_s(pstrFilePath + 15, 64 - 15, (bDuplicated) ? (pstrTextureName + 1) : pstrTextureName);
		strcpy_s(pstrFilePath + 15 + ((bDuplicated) ? (nStrLength - 1) : nStrLength), 64 - 15 - ((bDuplicated) ? (nStrLength - 1) : nStrLength), ".dds");

		size_t nConverted = 0;
		mbstowcs_s(&nConverted, m_ppstrTextureNames[nIndex], 64, pstrFilePath, _TRUNCATE);

		static int nTextures = 0, nRepeatedTextures = 0;
		TCHAR pstrDebug[256] = { 0 };
		_stprintf_s(pstrDebug, 256, _T("Texture Name: %d %c %s\n"), (pstrTextureName[0] == '@') ? nRepeatedTextures++ : nTextures++, (pstrTextureName[0] == '@') ? '@' : ' ', m_ppstrTextureNames[nIndex]);
		OutputDebugString(pstrDebug);
		if (!bDuplicated)
		{
			LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, m_ppstrTextureNames[nIndex], RESOURCE_TEXTURE2D, nIndex);
			pShader->CreateShaderResourceView(pd3dDevice, this, nIndex);
		}
		else
		{
			if (pParent)
			{
				CGameObject* pRootGameObject = pParent;
				while (pRootGameObject)
				{
					if (!pRootGameObject->m_pParent) break;
					pRootGameObject = pRootGameObject->m_pParent;
				}
				D3D12_GPU_DESCRIPTOR_HANDLE d3dSrvGpuDescriptorHandle;
				int nParameterIndex = pRootGameObject->FindReplicatedTexture(m_ppstrTextureNames[nIndex], &d3dSrvGpuDescriptorHandle);
				if (nParameterIndex >= 0)
				{
					m_pd3dSrvGpuDescriptorHandles[nIndex] = d3dSrvGpuDescriptorHandle;
				}
			}
		}
	}
	else
	{
		char cfilename[64] = { "Model/Textures/dummy.dds" };

		size_t n = 0;
		mbstowcs_s(&n, m_ppstrTextureNames[nIndex], 64, cfilename, _TRUNCATE);

		if (pParent)
		{
			CGameObject* pRootGameObject2 = pParent;
			while (pRootGameObject2)
			{
				if (!pRootGameObject2->m_pParent) break;
				pRootGameObject2 = pRootGameObject2->m_pParent;
			}
			D3D12_GPU_DESCRIPTOR_HANDLE d3dSrvGpuDescriptorHandle2;
			if (pRootGameObject2->FindReplicatedTexture(m_ppstrTextureNames[nIndex], &d3dSrvGpuDescriptorHandle2))
			{
				m_pd3dSrvGpuDescriptorHandles[nIndex] = d3dSrvGpuDescriptorHandle2;
			}
		}
		else
		{
			LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, m_ppstrTextureNames[nIndex], RESOURCE_TEXTURE2D, nIndex);
			pShader->CreateShaderResourceView(pd3dDevice, this, nIndex);
		}
	}
	return(bLoaded);
}

ID3D12Resource* CTexture::CreateTexture(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
	UINT nWidth, UINT nHeight, UINT nElements, UINT nMipLevels, DXGI_FORMAT dxgiFormat, D3D12_RESOURCE_FLAGS d3dResourceFlags, D3D12_RESOURCE_STATES d3dResourceStates,
	D3D12_CLEAR_VALUE* pd3dClearValue, UINT nResourceType, UINT nIndex)
{
	m_pnResourceTypes[nIndex] = nResourceType;
	m_ppd3dTextures[nIndex] = ::CreateTexture2DResource(pd3dDevice, pd3dCommandList, nWidth, nHeight, nElements, nMipLevels, dxgiFormat, d3dResourceFlags, d3dResourceStates, pd3dClearValue);
	return(m_ppd3dTextures[nIndex]);
}


// == CMaterial ==


CMaterial::CMaterial()
{
}

CMaterial::~CMaterial()
{
	if (m_pTexture) m_pTexture->Release();
	if (m_pShader) m_pShader->Release();
}

void CMaterial::SetShader(CShader* pShader)
{
	if (m_pShader) m_pShader->Release();
	m_pShader = pShader;
	if (m_pShader) m_pShader->AddRef();
}

void CMaterial::SetTexture(CTexture* pTexture)
{
	if (m_pTexture) m_pTexture->Release();
	m_pTexture = pTexture;
	if (m_pTexture) m_pTexture->AddRef();
}

void CMaterial::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (m_pTexture) m_pTexture->UpdateShaderVariables(pd3dCommandList);
}

void CMaterial::ReleaseUploadBuffers()
{
	if (m_pShader) m_pShader->ReleaseUploadBuffers();
	if (m_pTexture) m_pTexture->ReleaseUploadBuffers();
}

void CMaterial::ReleaseShaderVariables()
{
	if (m_pShader) m_pShader->ReleaseShaderVariables();
	if (m_pTexture) m_pTexture->ReleaseShaderVariables();
}

// CGameObject ===================================================================================

void CGameObject::AddRef()
{
	m_nReferences++;

	if (m_pSibling) m_pSibling->AddRef();
	if (m_pChild) m_pChild->AddRef();
}

void CGameObject::Release()
{
	if (m_pSibling) m_pSibling->Release();
	if (m_pChild) m_pChild->Release();

	if (--m_nReferences <= 0) delete this;
}

CGameObject::CGameObject(int nMeshes, int nMaterials)
{
	m_xmf4x4Transform = Matrix4x4::Identity();
	m_xmf4x4World = Matrix4x4::Identity();

	m_nMeshes = nMeshes;
	m_ppMeshes = NULL;
	m_ppBoundingBoxMeshes = NULL;
	m_pxmBoundingBoxes = NULL;

	if (m_nMeshes > 0)
	{
		m_ppMeshes = new CMesh * [m_nMeshes];
		m_ppBoundingBoxMeshes = new CBoundingBoxMesh * [m_nMeshes];
		m_pxmBoundingBoxes = new BoundingOrientedBox[m_nMeshes];
		for (int i = 0; i < m_nMeshes; i++) {
			m_ppMeshes[i] = NULL;
			m_ppBoundingBoxMeshes[i] = NULL;
			m_pxmBoundingBoxes[i] = BoundingOrientedBox(
				XMFLOAT3(0.0f, 0.0f, 0.0f), 
				XMFLOAT3(0.1f, 0.1f, 0.1f), 
				XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
		}
	}

	m_nMaterials = nMaterials;
	if (m_nMaterials > 0)
	{
		m_ppMaterials = new CMaterial * [m_nMaterials];
		for (int i = 0; i < m_nMaterials; i++) m_ppMaterials[i] = NULL;
	}
}

CGameObject::~CGameObject()
{
	delete m_pxmBoundingBoxes;
	m_pxmBoundingBoxes = NULL;

	if (m_ppMeshes)
	{
		for (int i = 0; i < m_nMeshes; i++)
		{
			if (m_ppMeshes[i]) m_ppMeshes[i]->Release();
			if (m_ppBoundingBoxMeshes[i]) m_ppBoundingBoxMeshes[i]->Release();
			m_ppMeshes[i] = NULL;
			m_ppBoundingBoxMeshes[i] = NULL;
		}
		delete[] m_ppMeshes;
		delete[] m_ppBoundingBoxMeshes;
	}

	if (m_nMaterials > 0)
	{
		for (int i = 0; i < m_nMaterials; i++)
		{
			if (m_ppMaterials[i]) m_ppMaterials[i]->Release();
		}
	}
	if (m_ppMaterials) delete[] m_ppMaterials;

}

void CGameObject::Animate(float fTimeElapsed, XMFLOAT4X4* pxmf4x4Parent)
{
	UpdateBoundingBox();

	if (m_pSibling) m_pSibling->Animate(fTimeElapsed, pxmf4x4Parent);
	if (m_pChild) m_pChild->Animate(fTimeElapsed, &m_xmf4x4World);
}

void CGameObject::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	UINT ncbElementBytes = ((sizeof(CB_GAMEOBJECT_INFO) + 255) & ~255); //256의 배수
	m_pd3dcbGameObject = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	m_pd3dcbGameObject->Map(0, NULL, (void**)&m_pcbMappedGameObject);
}

void CGameObject::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (m_pcbMappedGameObject)
	{
		XMStoreFloat4x4(&m_pcbMappedGameObject->m_xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4World)));
		if(m_ppMaterials) memcpy(&m_pcbMappedGameObject->m_nType, &m_ppMaterials[0]->m_nType, sizeof(UINT));
	}
}

//void CGameObject::UpdateShaderVariable(ID3D12GraphicsCommandList* pd3dCommandList, XMFLOAT4X4* pxmf4x4World)
//{
//	XMFLOAT4X4 xmf4x4World;
//	XMStoreFloat4x4(&xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(pxmf4x4World)));
//	pd3dCommandList->SetGraphicsRoot32BitConstants(2, 16, &xmf4x4World, 0);
//}

void CGameObject::ReleaseShaderVariables()
{
	if (m_pd3dcbGameObject)
	{
		m_pd3dcbGameObject->Unmap(0, NULL);
		m_pd3dcbGameObject->Release();
	}
	for (int i = 0; i < m_nMaterials; ++i)
	{
		if (m_ppMaterials) m_ppMaterials[i]->ReleaseShaderVariables();
	}
}

void CGameObject::ReleaseUploadBuffers()
{
	for (int i = 0; i < m_nMeshes; i++)
	{
		if (m_ppMeshes[i]) m_ppMeshes[i]->ReleaseUploadBuffers();
		if (m_ppBoundingBoxMeshes[i]) m_ppBoundingBoxMeshes[i]->ReleaseUploadBuffers();
	}

	for (int i = 0; i < m_nMaterials; i++)
	{
		if (m_ppMaterials[i]) m_ppMaterials[i]->ReleaseUploadBuffers();
	}

	if (m_pSibling) m_pSibling->ReleaseUploadBuffers();
	if (m_pChild) m_pChild->ReleaseUploadBuffers();
}

void CGameObject::UpdateTransform(XMFLOAT4X4* pxmf4x4Parent)
{
	m_xmf4x4World = (pxmf4x4Parent) ? Matrix4x4::Multiply(m_xmf4x4Transform, *pxmf4x4Parent) : m_xmf4x4Transform;

	if (m_pSibling) m_pSibling->UpdateTransform(pxmf4x4Parent);
	if (m_pChild) m_pChild->UpdateTransform(&m_xmf4x4World);
}

void CGameObject::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	OnPrepareRender();

	UpdateShaderVariables(pd3dCommandList);

	if (m_nMaterials <= 1) {
		if ((m_nMaterials == 1) && (m_ppMaterials[0]))
		{
			if (m_ppMaterials[0]->m_pShader)
			{
				m_ppMaterials[0]->m_pShader->Render(pd3dCommandList, pCamera);
			}
			m_ppMaterials[0]->UpdateShaderVariables(pd3dCommandList);
			if (m_ppMaterials[0]->m_pTexture)
			{
				m_ppMaterials[0]->m_pTexture->UpdateShaderVariables(pd3dCommandList);
				if (m_pcbMappedGameObject) XMStoreFloat4x4(&m_pcbMappedGameObject->m_xmf4x4Texture, XMMatrixTranspose(XMLoadFloat4x4(&m_ppMaterials[0]->m_pTexture->m_xmf4x4Texture)));
			}
		}	
		
		pd3dCommandList->SetGraphicsRootDescriptorTable(PARAMETER_OBJECT, m_d3dCbvGPUDescriptorHandle);

		if (m_ppMeshes)
		{
			for (int i = 0; i < m_nMeshes; i++)
			{
				if (m_ppMeshes[i]) m_ppMeshes[i]->Render(pd3dCommandList);
				//if (m_ppBoundingBoxMeshes[i]) m_ppBoundingBoxMeshes[i]->Render(pd3dCommandList);
			}
		}
	}
	else {
		std::cout << "nMaterial 이 1을 넘었다." << std::endl;
	}

	if (m_pSibling) m_pSibling->Render(pd3dCommandList, pCamera);
	if (m_pChild) m_pChild->Render(pd3dCommandList, pCamera);
}

XMFLOAT3 CGameObject::GetPosition()
{
	return(XMFLOAT3(m_xmf4x4World._41, m_xmf4x4World._42, m_xmf4x4World._43));
}

XMFLOAT3 CGameObject::GetLook()
{
	return(Vector3::Normalize(XMFLOAT3(m_xmf4x4World._31, m_xmf4x4World._32, m_xmf4x4World._33)));
}

XMFLOAT3 CGameObject::GetUp()
{
	return(Vector3::Normalize(XMFLOAT3(m_xmf4x4World._21, m_xmf4x4World._22, m_xmf4x4World._23)));
}

XMFLOAT3 CGameObject::GetRight()
{
	return(Vector3::Normalize(XMFLOAT3(m_xmf4x4World._11, m_xmf4x4World._12, m_xmf4x4World._13)));
}

void CGameObject::SetMesh(int nIndex, CMesh* pMesh)
{
	if (m_ppMeshes)
	{
		if (m_ppMeshes[nIndex]) m_ppMeshes[nIndex]->Release();
		m_ppMeshes[nIndex] = pMesh;
		if (pMesh) pMesh->AddRef();
	}
}

void CGameObject::SetBoundingBoxMesh(int nIndex, CBoundingBoxMesh* pMesh)
{
	if (m_ppBoundingBoxMeshes) 
	{
		if (m_ppBoundingBoxMeshes[nIndex]) m_ppBoundingBoxMeshes[nIndex]->Release();
		m_ppBoundingBoxMeshes[nIndex] = pMesh;
		if (pMesh) pMesh->AddRef();
	}
}

void CGameObject::SetShader(int nMaterial, CShader* pShader)
{
	if (m_ppMaterials) m_ppMaterials[nMaterial]->SetShader(pShader);
}

void CGameObject::SetMaterial(int nMaterial, CMaterial* pMaterial)
{
	if (m_ppMaterials[nMaterial]) m_ppMaterials[nMaterial]->Release();
	m_ppMaterials[nMaterial] = pMaterial;
	if (m_ppMaterials[nMaterial]) m_ppMaterials[nMaterial]->AddRef();
}

void CGameObject::SetChild(CGameObject* pChild)
{
	if (m_pChild)
	{
		if (pChild) pChild->m_pSibling = m_pChild->m_pSibling;
		m_pChild->m_pSibling = pChild;
	}
	else
	{
		m_pChild = pChild;
	}
	if (pChild)
	{
		pChild->m_pParent = this;
	}
}

void CGameObject::SetPosition(float x, float y, float z)
{
	m_xmf4x4Transform._41 = x;
	m_xmf4x4Transform._42 = y;
	m_xmf4x4Transform._43 = z;

	UpdateTransform(NULL);
}

void CGameObject::SetPosition(XMFLOAT3 xmf3Position)
{
	SetPosition(xmf3Position.x, xmf3Position.y, xmf3Position.z);
}

void CGameObject::SetRight(XMFLOAT3 xmf3Right)
{
	m_xmf4x4Transform._11 = xmf3Right.x;
	m_xmf4x4Transform._12 = xmf3Right.y;
	m_xmf4x4Transform._13 = xmf3Right.z;

	UpdateTransform(NULL);
}

void CGameObject::SetUp(XMFLOAT3 xmf3Up)
{
	m_xmf4x4Transform._21 = xmf3Up.x;
	m_xmf4x4Transform._22 = xmf3Up.y;
	m_xmf4x4Transform._23 = xmf3Up.z;

	UpdateTransform(NULL);
}

void CGameObject::SetLook(XMFLOAT3 xmf3Look)
{
	m_xmf4x4Transform._31 = xmf3Look.x;
	m_xmf4x4Transform._32 = xmf3Look.y;
	m_xmf4x4Transform._33 = xmf3Look.z;

	UpdateTransform(NULL);
}

void CGameObject::SetLookAt(XMFLOAT3& xmf3Target, XMFLOAT3& xmf3Up)
{
	XMFLOAT3 xmf3Position(m_xmf4x4World._41, m_xmf4x4World._42, m_xmf4x4World._43);
	XMFLOAT4X4 mtxLookAt = Matrix4x4::LookAtLH(xmf3Position, xmf3Target, xmf3Up);
	m_xmf4x4World._11 = mtxLookAt._11; m_xmf4x4World._12 = mtxLookAt._21; m_xmf4x4World._13 = mtxLookAt._31;
	m_xmf4x4World._21 = mtxLookAt._12; m_xmf4x4World._22 = mtxLookAt._22; m_xmf4x4World._23 = mtxLookAt._32;
	m_xmf4x4World._31 = mtxLookAt._13; m_xmf4x4World._32 = mtxLookAt._23; m_xmf4x4World._33 = mtxLookAt._33;
}

void CGameObject::SetScale(float x, float y, float z)
{
	XMMATRIX mtxScale = XMMatrixScaling(x, y, z);
	m_xmf4x4Transform = Matrix4x4::Multiply(mtxScale, m_xmf4x4Transform);

	UpdateTransform(NULL);
}

void CGameObject::Rotate(float fPitch, float fYaw, float fRoll, XMFLOAT4X4* parent)
{
	XMMATRIX mtxRotate = XMMatrixRotationRollPitchYaw(XMConvertToRadians(fPitch), XMConvertToRadians(fYaw), XMConvertToRadians(fRoll));
	m_xmf4x4Transform = Matrix4x4::Multiply(mtxRotate, m_xmf4x4Transform);

	UpdateTransform(parent);
}

void CGameObject::Rotate(XMFLOAT3* pxmf3Axis, float fAngle)
{
	XMMATRIX mtxRotate = XMMatrixRotationAxis(XMLoadFloat3(pxmf3Axis), XMConvertToRadians(fAngle));
	m_xmf4x4Transform = Matrix4x4::Multiply(mtxRotate, m_xmf4x4Transform);

	UpdateTransform(NULL);
}

void CGameObject::Rotate(XMFLOAT4* pxmf4Quaternion)
{
	XMMATRIX mtxRotate = XMMatrixRotationQuaternion(XMLoadFloat4(pxmf4Quaternion));
	m_xmf4x4Transform = Matrix4x4::Multiply(mtxRotate, m_xmf4x4Transform);

	UpdateTransform(NULL);
}

void CGameObject::UpdateBoundingBox()
{
	for (int i = 0; i < m_nMeshes; ++i) {
		m_ppMeshes[i]->m_xmBoundingBox.Transform(m_pxmBoundingBoxes[i], XMLoadFloat4x4(&m_xmf4x4World));
		XMStoreFloat4(&m_pxmBoundingBoxes[i].Orientation, XMQuaternionNormalize(XMLoadFloat4(&m_pxmBoundingBoxes[i].Orientation)));
	}
	if (m_pSibling) m_pSibling->UpdateBoundingBox();
	if (m_pChild) m_pChild->UpdateBoundingBox();
}

void CGameObject::RenderBoundingBox(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	for (int i = 0; i < m_nMeshes; ++i) {
		if (m_ppBoundingBoxMeshes[i])
		{
			m_ppBoundingBoxMeshes[i]->UpdateVertexPosition(&m_pxmBoundingBoxes[i]);
			m_ppBoundingBoxMeshes[i]->Render(pd3dCommandList);
		}
	}
	if (m_pSibling) m_pSibling->RenderBoundingBox(pd3dCommandList, pCamera);
	if (m_pChild) m_pChild->RenderBoundingBox(pd3dCommandList, pCamera);
}

bool CGameObject::FindReplicatedTexture(_TCHAR* pstrTextureName, D3D12_GPU_DESCRIPTOR_HANDLE* pd3dSrvGpuDescriptorHandle)
{
	int nParameterIndex = -1;

	for (int i = 0; i < m_nMaterials; i++)
	{
		if (m_ppMaterials[i] && m_ppMaterials[i]->m_pTexture)
		{
			int nTextures = m_ppMaterials[i]->m_pTexture->GetTextures();
			for (int j = 0; j < nTextures; j++)
			{
				// wprintf(L"%ls\n", m_ppMaterials[i]->m_pTexture->GetTextureName(j));
				// std::cout << m_ppMaterials[i]->m_pTexture->GetTextureName(j) << std::endl;
				if (!_tcsncmp(m_ppMaterials[i]->m_pTexture->GetTextureName(j), pstrTextureName, _tcslen(pstrTextureName)))
				{
					*pd3dSrvGpuDescriptorHandle = m_ppMaterials[i]->m_pTexture->GetGpuDescriptorHandle(j);
					return true;
				}
			}
		}
	}
	if (m_pSibling) if (m_pSibling->FindReplicatedTexture(pstrTextureName, pd3dSrvGpuDescriptorHandle)) return(true);
	if (m_pChild) if (m_pChild->FindReplicatedTexture(pstrTextureName, pd3dSrvGpuDescriptorHandle)) return(true);

	return(false);
}

void CGameObject::LoadMaterialsFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CGameObject* pParent, FILE* pInFile, CShader* pShader)
{
	char pstrToken[64] = { '\0' };

	int nMaterial = 0;
	BYTE nStrLength = 0;

	UINT nReads = (UINT)::fread(&m_nMaterials, sizeof(int), 1, pInFile);

	m_ppMaterials = new CMaterial * [m_nMaterials];
	for (int i = 0; i < m_nMaterials; i++) m_ppMaterials[i] = NULL;

	CMaterial* pMaterial = NULL;
	CTexture* pTexture = NULL;

	for (; ; )
	{
		nReads = (UINT)::fread(&nStrLength, sizeof(BYTE), 1, pInFile);
		nReads = (UINT)::fread(pstrToken, sizeof(char), nStrLength, pInFile);
		pstrToken[nStrLength] = '\0';

		if (!strcmp(pstrToken, "<Material>:"))
		{
			nReads = (UINT)::fread(&nMaterial, sizeof(int), 1, pInFile);

			pMaterial = new CMaterial();
			pTexture = new CTexture(6, RESOURCE_TEXTURE2D, 0, 1); //0:Albedo, 2:Metallic, 3:Normal, 4:Emission, 5:DetailAlbedo, 6:DetailNormal
			pMaterial->SetTexture(pTexture);
			SetMaterial(nMaterial, pMaterial);

			UINT nMeshType = GetMeshType(0);
		}
		else if (!strcmp(pstrToken, "<AlbedoColor>:"))
		{
			nReads = (UINT)::fread(&(pMaterial->m_xmf4AlbedoColor), sizeof(float), 4, pInFile);
		}
		else if (!strcmp(pstrToken, "<EmissiveColor>:"))
		{
			nReads = (UINT)::fread(&(pMaterial->m_xmf4EmissiveColor), sizeof(float), 4, pInFile);
		}
		else if (!strcmp(pstrToken, "<SpecularColor>:"))
		{
			nReads = (UINT)::fread(&(pMaterial->m_xmf4SpecularColor), sizeof(float), 4, pInFile);
		}
		else if (!strcmp(pstrToken, "<Glossiness>:"))
		{
			nReads = (UINT)::fread(&(pMaterial->m_fGlossiness), sizeof(float), 1, pInFile);
		}
		else if (!strcmp(pstrToken, "<Smoothness>:"))
		{
			nReads = (UINT)::fread(&(pMaterial->m_fSmoothness), sizeof(float), 1, pInFile);
		}
		else if (!strcmp(pstrToken, "<Metallic>:"))
		{
			nReads = (UINT)::fread(&(pMaterial->m_fSpecularHighlight), sizeof(float), 1, pInFile);
		}
		else if (!strcmp(pstrToken, "<SpecularHighlight>:"))
		{
			nReads = (UINT)::fread(&(pMaterial->m_fMetallic), sizeof(float), 1, pInFile);
		}
		else if (!strcmp(pstrToken, "<GlossyReflection>:"))
		{
			nReads = (UINT)::fread(&(pMaterial->m_fGlossyReflection), sizeof(float), 1, pInFile);
		}
		else if (!strcmp(pstrToken, "<AlbedoMap>:"))
		{
			if (pTexture->LoadTextureFromFile(pd3dDevice, pd3dCommandList, pParent, pInFile, pShader, 0)) pMaterial->SetMaterialType(MATERIAL_ALBEDO_MAP);
		}
		// else if (!strcmp(pstrToken, "<SpecularMap>:"))
		// {
		// 	if (pTexture->LoadTextureFromFile(pd3dDevice, pd3dCommandList, pParent, pInFile, pShader, 1)) pMaterial->SetMaterialType(MATERIAL_SPECULAR_MAP);
		// }
		else if (!strcmp(pstrToken, "<MetallicMap>:"))
		{
			if (pTexture->LoadTextureFromFile(pd3dDevice, pd3dCommandList, pParent, pInFile, pShader, 1)) pMaterial->SetMaterialType(MATERIAL_METALLIC_MAP);
		}
		else if (!strcmp(pstrToken, "<NormalMap>:"))
		{
			if (pTexture->LoadTextureFromFile(pd3dDevice, pd3dCommandList, pParent, pInFile, pShader, 2)) pMaterial->SetMaterialType(MATERIAL_NORMAL_MAP);
		}
		else if (!strcmp(pstrToken, "<EmissionMap>:"))
		{
			if (pTexture->LoadTextureFromFile(pd3dDevice, pd3dCommandList, pParent, pInFile, pShader, 3)) pMaterial->SetMaterialType(MATERIAL_EMISSION_MAP);
		}
		else if (!strcmp(pstrToken, "<DetailAlbedoMap>:"))
		{
			if (pTexture->LoadTextureFromFile(pd3dDevice, pd3dCommandList, pParent, pInFile, pShader, 4)) pMaterial->SetMaterialType(MATERIAL_DETAIL_ALBEDO_MAP);
		}
		else if (!strcmp(pstrToken, "<DetailNormalMap>:"))
		{
			if (pTexture->LoadTextureFromFile(pd3dDevice, pd3dCommandList, pParent, pInFile, pShader, 5)) pMaterial->SetMaterialType(MATERIAL_DETAIL_NORMAL_MAP);
		}
		else if (!strcmp(pstrToken, "</Materials>"))
		{
			break;
		}
	}		
	//pShader->CreateShaderResourceViews(pd3dDevice, pTexture, 0, PARAMETER_STANDARD_TEXTURE);
	pTexture->SetRootParameterIndex(0, PARAMETER_STANDARD_TEXTURE);
}

CGameObject* CGameObject::LoadFrameHierarchyFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CGameObject* pParent, FILE* pInFile, CShader* pShader)
{
	char pstrToken[64] = { '\0' };

	BYTE nStrLength = 0;
	UINT nReads = 0;

	int nFrame = 0, nTextures = 0;

	CGameObject* pGameObject = NULL;

	for (; ; )
	{
		nReads = (UINT)::fread(&nStrLength, sizeof(BYTE), 1, pInFile);
		nReads = (UINT)::fread(pstrToken, sizeof(char), nStrLength, pInFile);
		pstrToken[nStrLength] = '\0';

		if (!strcmp(pstrToken, "<Frame>:"))
		{
			pGameObject = new CGameObject(1, 1);

			nReads = (UINT)::fread(&nFrame, sizeof(int), 1, pInFile);
			nReads = (UINT)::fread(&nTextures, sizeof(int), 1, pInFile);

			nReads = (UINT)::fread(&nStrLength, sizeof(BYTE), 1, pInFile);
			nReads = (UINT)::fread(pGameObject->m_pstrFrameName, sizeof(char), nStrLength, pInFile);
			pGameObject->m_pstrFrameName[nStrLength] = '\0';
		}
		else if (!strcmp(pstrToken, "<Transform>:"))
		{
			XMFLOAT3 xmf3Position, xmf3Rotation, xmf3Scale;
			XMFLOAT4 xmf4Rotation;
			nReads = (UINT)::fread(&xmf3Position, sizeof(float), 3, pInFile);
			nReads = (UINT)::fread(&xmf3Rotation, sizeof(float), 3, pInFile); //Euler Angle
			nReads = (UINT)::fread(&xmf3Scale, sizeof(float), 3, pInFile);
			nReads = (UINT)::fread(&xmf4Rotation, sizeof(float), 4, pInFile); //Quaternion
		}
		else if (!strcmp(pstrToken, "<TransformMatrix>:"))
		{
			nReads = (UINT)::fread(&pGameObject->m_xmf4x4Transform, sizeof(float), 16, pInFile);
		}
		else if (!strcmp(pstrToken, "<Mesh>:"))
		{
			CStandardMesh* pMesh = new CStandardMesh(pd3dDevice, pd3dCommandList);
			pMesh->LoadMeshFromFile(pd3dDevice, pd3dCommandList, pInFile);
			pGameObject->SetMesh(0, pMesh);
			CBoundingBoxMesh* pBoundingBoxMesh = new CBoundingBoxMesh(pd3dDevice, pd3dCommandList);
			pGameObject->SetBoundingBoxMesh(0, pBoundingBoxMesh);
			pGameObject->m_pxmBoundingBoxes = new BoundingOrientedBox[1];
			pGameObject->m_pxmBoundingBoxes[0] = BoundingOrientedBox(
				XMFLOAT3(0.0f, 0.0f, 0.0f),
				XMFLOAT3(0.1f, 0.1f, 0.1f),
				XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
		}
		else if (!strcmp(pstrToken, "<Materials>:"))
		{
			pGameObject->LoadMaterialsFromFile(pd3dDevice, pd3dCommandList, pParent, pInFile, pShader);
		}
		else if (!strcmp(pstrToken, "<Children>:"))
		{
			int nChilds = 0;
			nReads = (UINT)::fread(&nChilds, sizeof(int), 1, pInFile);
			pGameObject->m_nObjects += nChilds;
			if (nChilds > 0)
			{
				for (int i = 0; i < nChilds; i++)
				{
					CGameObject* pChild = CGameObject::LoadFrameHierarchyFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, pGameObject, pInFile, pShader);
					if (pChild) pGameObject->SetChild(pChild);
#ifdef _WITH_DEBUG_FRAME_HIERARCHY
					TCHAR pstrDebug[256] = { 0 };
					_stprintf_s(pstrDebug, 256, _T("(Frame: %p) (Parent: %p)\n"), pChild, pGameObject);
					OutputDebugString(pstrDebug);
#endif
				}
			}
		}
		else if (!strcmp(pstrToken, "</Frame>"))
		{
			break;
		}
	}
	return(pGameObject);
}

void CGameObject::PrintFrameInfo(CGameObject* pGameObject, CGameObject* pParent)
{
	TCHAR pstrDebug[256] = { 0 };

	_stprintf_s(pstrDebug, 256, _T("(Frame: %p) (Parent: %p)\n"), pGameObject, pParent);
	OutputDebugString(pstrDebug);

	if (pGameObject->m_pSibling) CGameObject::PrintFrameInfo(pGameObject->m_pSibling, pParent);
	if (pGameObject->m_pChild) CGameObject::PrintFrameInfo(pGameObject->m_pChild, pGameObject);
}

void CGameObject::PrintBoundingBox()
{
	//UpdateTransform();
	if (m_pxmBoundingBoxes) {
		std::cout << m_pstrFrameName << std::endl;

		XMFLOAT3 a = GetPosition();
		std::cout << a.x << ' ';
		std::cout << a.y << ' ';
		std::cout << a.z << ' ';
		std::cout << std::endl;

		std::cout << m_pxmBoundingBoxes[0].Center.x << ' ';
		std::cout << m_pxmBoundingBoxes[0].Center.y << ' ';
		std::cout << m_pxmBoundingBoxes[0].Center.z << ' ';
		std::cout << std::endl << std::endl;
	}
	if (m_pSibling) m_pSibling->PrintBoundingBox();
	if (m_pChild) m_pChild->PrintBoundingBox();
	return;
}

CGameObject* CGameObject::LoadGeometryFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, char* pstrFileName, CShader* pShader)
{
	FILE* pInFile = NULL;
	::fopen_s(&pInFile, pstrFileName, "rb");
	::rewind(pInFile);

	CGameObject* pGameObject = CGameObject::LoadFrameHierarchyFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, NULL, pInFile, pShader);

#ifdef _WITH_DEBUG_FRAME_HIERARCHY
	TCHAR pstrDebug[256] = { 0 };
	_stprintf_s(pstrDebug, 256, _T("Frame Hierarchy\n"));
	OutputDebugString(pstrDebug);

	CGameObject::PrintFrameInfo(pGameObject, NULL);
#endif

	return(pGameObject);
}

CGameObject* CGameObject::FindFrame(char* pstrFrameName)
{
	CGameObject* pFrameObject = NULL;
	if (!strncmp(m_pstrFrameName, pstrFrameName, strlen(pstrFrameName))) return(this);

	if (m_pSibling) if (pFrameObject = m_pSibling->FindFrame(pstrFrameName)) return(pFrameObject);
	if (m_pChild) if (pFrameObject = m_pChild->FindFrame(pstrFrameName)) return(pFrameObject);

	return(NULL);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
CSkyBox::CSkyBox(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature) : CGameObject(1, 1)
{
	CreateShaderVariables(pd3dDevice, pd3dCommandList);

	CSkyBoxMesh* pSkyBoxMesh = new CSkyBoxMesh(pd3dDevice, pd3dCommandList, 3000.0f, 3000.0f, 3000.0f);
	SetMesh(0, pSkyBoxMesh);

	CTexture* pSkyBoxTexture = new CTexture(1, RESOURCE_TEXTURE_CUBE, 0, 1);
	pSkyBoxTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"SkyBox/SkyBox_0.dds", RESOURCE_TEXTURE_CUBE, 0);
	
	UINT ncbElementBytes = ((sizeof(CB_GAMEOBJECT_INFO) + 255) & ~255); //256의 배수

	CSkyBoxShader* pSkyBoxShader = new CSkyBoxShader();
	pSkyBoxShader->CreateShader(pd3dDevice, pd3dGraphicsRootSignature);
	pSkyBoxShader->CreateCbvSrvDescriptorHeaps(pd3dDevice, 1, 1);
	pSkyBoxShader->CreateShaderResourceViews(pd3dDevice, pSkyBoxTexture, 0, PARAMETER_SKYBOX_CUBE_TEXTURE);
	D3D12_GPU_DESCRIPTOR_HANDLE d3dCbvGPUDescriptorHandle = pSkyBoxShader->CreateConstantBufferView(pd3dDevice, m_pd3dcbGameObject, ncbElementBytes);
	SetCbvGPUDescriptorHandle(d3dCbvGPUDescriptorHandle);

	CMaterial* pSkyBoxMaterial = new CMaterial();
	pSkyBoxMaterial->SetTexture(pSkyBoxTexture);
	pSkyBoxMaterial->SetShader(pSkyBoxShader);

	SetMaterial(0, pSkyBoxMaterial);
}

CSkyBox::~CSkyBox()
{
}

void CSkyBox::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	XMFLOAT3 xmf3CameraPos = pCamera->GetPosition();
	SetPosition(xmf3CameraPos.x, xmf3CameraPos.y, xmf3CameraPos.z);

	CGameObject::Render(pd3dCommandList, pCamera);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

CHeightMapTerrain::CHeightMapTerrain(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, LPCTSTR pFileName, int nWidth, int nLength, int nBlockWidth, int nBlockLength, XMFLOAT3 xmf3Scale, XMFLOAT4 xmf4Color)
	: CGameObject(((nWidth - 1) / (nBlockWidth - 1))* ((nLength - 1) / (nBlockLength - 1)), 1)
{
	m_nWidth = nWidth;
	m_nLength = nLength;
	m_xmf3Scale = xmf3Scale;

	int cxQuadsPerBlock = nBlockWidth - 1;
	int czQuadsPerBlock = nBlockLength - 1;

	m_pHeightMapImage = new CHeightMapImage(pFileName, nWidth, nLength);

	long cxBlocks = (m_nWidth - 1) / cxQuadsPerBlock;
	long czBlocks = (m_nLength - 1) / czQuadsPerBlock;

	m_nMeshes = cxBlocks * czBlocks;
	m_ppMeshes = new CMesh * [m_nMeshes];
	for (int i = 0; i < m_nMeshes; i++)	m_ppMeshes[i] = NULL;

	CHeightMapGridMesh* pHeightMapGridMesh = NULL;
	for (int z = 0, zStart = 0; z < czBlocks; z++)
	{
		for (int x = 0, xStart = 0; x < cxBlocks; x++)
		{
			xStart = x * (nBlockWidth - 1);
			zStart = z * (nBlockLength - 1);
			pHeightMapGridMesh = new CHeightMapGridMesh(pd3dDevice, pd3dCommandList, xStart, zStart, nBlockWidth, nBlockLength, xmf3Scale, xmf4Color, m_pHeightMapImage);
			SetMesh(x + (z * cxBlocks), pHeightMapGridMesh);
		}
	}

	CreateShaderVariables(pd3dDevice, pd3dCommandList);

	CTexture* pTerrainTexture = new CTexture(4, RESOURCE_TEXTURE2D, 0, 1);
	pTerrainTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/Base_Texture.dds", RESOURCE_TEXTURE2D, 0);
	pTerrainTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/Detail_Texture_7.dds", RESOURCE_TEXTURE2D, 1);
	pTerrainTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/WaveFoam.dds", RESOURCE_TEXTURE2D, 2);
	pTerrainTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/HeightMap(Flipped).dds", RESOURCE_TEXTURE2D, 3);
	
	UINT ncbElementBytes = ((sizeof(CB_GAMEOBJECT_INFO) + 255) & ~255); //256의 배수

#ifdef _WITH_TERRAIN_TESSELATION
	CTerrainTessellationShader* pTerrainShader = new CTerrainTessellationShader();
#else
	CTerrainShader* pTerrainShader = new CTerrainShader();
#endif

	pTerrainShader->CreateShader(pd3dDevice, pd3dGraphicsRootSignature);
	pTerrainShader->CreateCbvSrvDescriptorHeaps(pd3dDevice, 1, 4);
	pTerrainShader->CreateShaderResourceViews(pd3dDevice, pTerrainTexture, 0, PARAMETER_TERRAIN);
	D3D12_GPU_DESCRIPTOR_HANDLE d3dCbvGPUDescriptorHandle = pTerrainShader->CreateConstantBufferView(pd3dDevice, m_pd3dcbGameObject, ncbElementBytes);
	SetCbvGPUDescriptorHandle(d3dCbvGPUDescriptorHandle);

	CMaterial* pTerrainMaterial = new CMaterial();
	pTerrainMaterial->SetTexture(pTerrainTexture);
	pTerrainMaterial->SetShader(pTerrainShader);

	SetMaterial(0, pTerrainMaterial);
}

float CHeightMapTerrain::GetHeight(float fx, float fz) //World Coordinates
{
	float fHeight = m_pHeightMapImage->GetHeight(fx, fz, m_xmf3Scale);

	return(fHeight);
}

CHeightMapTerrain::~CHeightMapTerrain(void)
{
	if (m_pHeightMapImage) delete m_pHeightMapImage;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
CRippleWater::CRippleWater(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nWidth, int nLength, int nBlockWidth, int nBlockLength, XMFLOAT3 xmf3Scale, XMFLOAT4 xmf4Color) 
	: CGameObject(((nWidth - 1) / (nBlockWidth - 1)) * ((nLength - 1) / (nBlockLength - 1)), 1)
{
	m_nWidth = nWidth;
	m_nLength = nLength;

	int cxQuadsPerBlock = nBlockWidth - 1;
	int czQuadsPerBlock = nBlockLength - 1;

	m_xmf3Scale = xmf3Scale;

	long cxBlocks = (m_nWidth - 1) / cxQuadsPerBlock;
	long czBlocks = (m_nLength - 1) / czQuadsPerBlock;

	m_nMeshes = cxBlocks * czBlocks;
	m_ppMeshes = new CMesh * [m_nMeshes];
	for (int i = 0; i < m_nMeshes; i++)	m_ppMeshes[i] = NULL;

	CGridMesh* pGridMesh = NULL;
	for (int z = 0, zStart = 0; z < czBlocks; z++)
	{
		for (int x = 0, xStart = 0; x < cxBlocks; x++)
		{
			xStart = x * (nBlockWidth - 1);
			zStart = z * (nBlockLength - 1);
			pGridMesh = new CGridMesh(pd3dDevice, pd3dCommandList, xStart, zStart, nBlockWidth, nBlockLength, xmf3Scale, xmf4Color, NULL);
			SetMesh(x + (z * cxBlocks), pGridMesh);
		}
	}
	
	CreateShaderVariables(pd3dDevice, pd3dCommandList);

	CTexture* pWaterTexture = new CTexture(3, RESOURCE_TEXTURE2D, 0, 1);
	pWaterTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/Water_Base_Texture_0.dds", RESOURCE_TEXTURE2D, 0);
	pWaterTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/Water_Detail_Texture_0.dds", RESOURCE_TEXTURE2D, 1);
	pWaterTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/Lava(Diffuse).dds", RESOURCE_TEXTURE2D, 2);

	UINT ncbElementBytes = ((sizeof(CB_GAMEOBJECT_INFO) + 255) & ~255); //256의 배수

	CRippleWaterShader* pRippleWaterShader = new CRippleWaterShader();
	pRippleWaterShader->CreateShader(pd3dDevice, pd3dGraphicsRootSignature);
	pRippleWaterShader->CreateCbvSrvDescriptorHeaps(pd3dDevice, 1, 3);
	pRippleWaterShader->CreateShaderResourceViews(pd3dDevice, pWaterTexture, 0, PARAMETER_WATER_TEXTURE);
	D3D12_GPU_DESCRIPTOR_HANDLE d3dCbvGPUDescriptorHandle = pRippleWaterShader->CreateConstantBufferView(pd3dDevice, m_pd3dcbGameObject, ncbElementBytes);
	SetCbvGPUDescriptorHandle(d3dCbvGPUDescriptorHandle);

	CMaterial* pWaterMaterial = new CMaterial();
	pWaterMaterial->SetTexture(pWaterTexture);
	pWaterMaterial->SetShader(pRippleWaterShader);

	SetMaterial(0, pWaterMaterial);
}

CRippleWater::~CRippleWater()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CTankObject

CTankObject::CTankObject() : CGameObject(0,0)
{
	m_fMovingSpeed = 10.0f;
}

CTankObject::~CTankObject()
{
}

void CTankObject::PrepareAnimate()
{
	//m_pBodyFrame = FindFrame("TankFree_Blue");
	//m_pTurretFrame = FindFrame("TankFree_Tower");
	//m_pTracksFrontLeftFrame = FindFrame("TankFree_Wheel_f_left");
	//m_pTracksFrontRightFrame = FindFrame("TankFree_Wheel_f_right");
	//m_pTracksBackLeftFrame = FindFrame("TankFree_Wheel_b_left");
	//m_pTracksBackRightFrame = FindFrame("TankFree_Wheel_b_right");
}

void CTankObject::Animate(float fTimeElapsed, XMFLOAT4X4* pxmf4x4Parent, CPlayer* pPlayer)
{
	if (Vector3::Length(Vector3::Subtract(GetPosition(), ((CGameObject*)pPlayer)->GetPosition())) > 40)
		SetPosition(Vector3::Add(GetPosition(), GetLook(), m_fMovingSpeed * fTimeElapsed));
	

	XMFLOAT3 xmf3Scale = m_pTerrain->GetScale();
	XMFLOAT3 xmf3PlayerPosition = GetPosition();
	int z = (int)(xmf3PlayerPosition.z / xmf3Scale.z);
	//bool bReverseQuad = ((z % 2) != 0);
	float fHeight = m_pTerrain->GetHeight(xmf3PlayerPosition.x, xmf3PlayerPosition.z) + 6.0f;
	XMFLOAT3 terrainNormal = m_pTerrain->GetNormal(xmf3PlayerPosition.x, xmf3PlayerPosition.z);
	XMFLOAT3 xmf3Look = Vector3::Subtract(((CGameObject*)pPlayer)->GetPosition(), GetPosition());
	xmf3Look = Vector3::Normalize(XMFLOAT3(xmf3Look.x, 0.0f, xmf3Look.z));

	XMVECTOR vTerrainNormal = XMLoadFloat3(&terrainNormal);
	XMFLOAT3 xmf3Up = GetUp();
	XMVECTOR vTankNormal = XMLoadFloat3(&xmf3Up);
	XMVECTOR lerpedVector = XMVectorLerp(vTerrainNormal, vTankNormal, min(0.7, fTimeElapsed * 40));

	XMStoreFloat3(&xmf3Up, lerpedVector);
	XMFLOAT3 xmf3Right = GetRight();

	XMStoreFloat3(&xmf3Right, XMVector3Cross(XMLoadFloat3(&xmf3Up), XMLoadFloat3(&xmf3Look)));
	XMStoreFloat3(&xmf3Look, XMVector3Cross(XMLoadFloat3(&xmf3Right), XMLoadFloat3(&xmf3Up)));

	xmf3Right = Vector3::Normalize(xmf3Right);
	xmf3Up = Vector3::Normalize(xmf3Up);
	xmf3Look = Vector3::Normalize(xmf3Look);
	
	SetRight(xmf3Right);
	SetUp(xmf3Up);
	SetLook(xmf3Look);

	XMFLOAT4X4 xmf4x4RUL = Matrix4x4::Identity();
	xmf4x4RUL._11 = xmf3Right.x; xmf4x4RUL._12 = xmf3Right.y; xmf4x4RUL._13 = xmf3Right.z;
	xmf4x4RUL._21 = xmf3Up.x; xmf4x4RUL._22 = xmf3Up.y; xmf4x4RUL._23 = xmf3Up.z;
	xmf4x4RUL._31 = xmf3Look.x; xmf4x4RUL._32 = xmf3Look.y; xmf4x4RUL._33 = xmf3Look.z;
	m_xmf4x4Transform = Matrix4x4::Multiply(xmf4x4RUL, XMMatrixScaling(p_fScale, p_fScale, p_fScale));
	xmf3PlayerPosition.y = fHeight;
	SetPosition(xmf3PlayerPosition);
	UpdateTransform();

	CGameObject::Animate(fTimeElapsed, pxmf4x4Parent);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CBulletObject

CBulletObject::CBulletObject(float fEffectiveRange) : CGameObject(0, 0)
{
	m_fBulletEffectiveRange = fEffectiveRange;
}

CBulletObject::~CBulletObject()
{
}

void CBulletObject::PrepareAnimate()
{
}

void CBulletObject::Animate(float fTimeElapsed, XMFLOAT4X4* pxmf4x4Parent)
{
	m_fElapsedTimeAfterFire += fTimeElapsed;
	m_fElapsedTimeAfterFire += fTimeElapsed;

	float fDistance = m_fMovingSpeed * fTimeElapsed;

	XMFLOAT3 xmf3Position = GetPosition();
	if ((m_fElapsedTimeAfterFire > m_fLockingDelayTime) && m_pLockedObject)
	{
		XMVECTOR xmvPosition = XMLoadFloat3(&xmf3Position);

		XMFLOAT3 xmf3LockedObjectPosition = m_pLockedObject->GetPosition();
		XMVECTOR xmvLockedObjectPosition = XMLoadFloat3(&xmf3LockedObjectPosition);
		XMVECTOR xmvToLockedObject = xmvLockedObjectPosition - xmvPosition;
		xmvToLockedObject = XMVector3Normalize(xmvToLockedObject);

		XMVECTOR xmvMovingDirection = XMLoadFloat3(&m_xmf3MovingDirection);
		xmvMovingDirection = XMVector3Normalize(XMVectorLerp(xmvMovingDirection, xmvToLockedObject, 0.125f));
		XMStoreFloat3(&m_xmf3MovingDirection, xmvMovingDirection);
	}
	Rotate(30 * fTimeElapsed, 0, 0);
	XMStoreFloat3(&xmf3Position, XMLoadFloat3(&xmf3Position) + (XMLoadFloat3(&GetLook()) * fDistance));
	m_xmf4x4Transform._41 = xmf3Position.x; m_xmf4x4Transform._42 = xmf3Position.y; m_xmf4x4Transform._43 = xmf3Position.z;
	m_fMovingDistance += fDistance;

	UpdateTransform();
	CGameObject::Animate(fTimeElapsed, pxmf4x4Parent);

	if ((m_fMovingDistance > m_fBulletEffectiveRange) || (m_fElapsedTimeAfterFire > m_fLockingTime)) Reset();
}

void CBulletObject::SetFirePosition(XMFLOAT3 xmf3FirePosition)
{
	m_xmf3FirePosition = xmf3FirePosition;
	SetPosition(xmf3FirePosition);
}

void CBulletObject::Reset()
{
	m_pLockedObject = NULL;
	m_fElapsedTimeAfterFire = 0;
	m_fMovingDistance = 0;

	m_bActive = false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
CGrassObject::CGrassObject()
{
}

CGrassObject::~CGrassObject()
{
}

void CGrassObject::Animate(float fTimeElapsed)
{
	if (m_fRotationAngle <= -1.5f) m_fRotationDelta = 1.0f;
	if (m_fRotationAngle >= +1.5f) m_fRotationDelta = -1.0f;
	m_fRotationAngle += m_fRotationDelta * fTimeElapsed;

	Rotate(0.0f, 0.0f, m_fRotationAngle);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
CDynamicCubeMappingObject::CDynamicCubeMappingObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, LONG nCubeMapSize, UINT nBoxSize, D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle, D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle, CShader* pShader)
{
	m_nMeshes = 1;
	m_ppBoundingBoxMeshes = new CBoundingBoxMesh * [m_nMeshes];
	m_xmBoundingSpheres = new BoundingSphere[m_nMeshes];
	for (int i = 0; i < m_nMeshes; i++) {
		m_ppMeshes[i] = NULL;
		m_ppBoundingBoxMeshes[i] = NULL;
		m_xmBoundingSpheres[i] = BoundingSphere(XMFLOAT3(0.0f, 0.0f, 0.0f), 0.1f);
	}

	//Camera[6]
	for (int j = 0; j < 6; j++)
	{
		m_ppCameras[j] = new CCamera();
		m_ppCameras[j]->SetViewport(0, 0, nCubeMapSize, nCubeMapSize, 0.0f, 1.0f);
		m_ppCameras[j]->SetScissorRect(0, 0, nCubeMapSize, nCubeMapSize);
		m_ppCameras[j]->CreateShaderVariables(pd3dDevice, pd3dCommandList);
		m_ppCameras[j]->GenerateProjectionMatrix(0.1f, 5000.0f, 1.0f/*Aspect Ratio*/, 90.0f/*FOV*/);
	}

	//Depth Buffer & View
	D3D12_CLEAR_VALUE d3dDsbClearValue = { DXGI_FORMAT_D24_UNORM_S8_UINT, { 1.0f, 0 } };
	m_pd3dDepthStencilBuffer = ::CreateTexture2DResource(pd3dDevice, pd3dCommandList, nCubeMapSize, nCubeMapSize, 1, 1,
		DXGI_FORMAT_D24_UNORM_S8_UINT, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, D3D12_RESOURCE_STATE_DEPTH_WRITE, &d3dDsbClearValue);

	m_d3dDsvCPUDescriptorHandle = d3dDsvCPUDescriptorHandle;
	pd3dDevice->CreateDepthStencilView(m_pd3dDepthStencilBuffer, NULL, m_d3dDsvCPUDescriptorHandle);

	CTexture* pTexture = new CTexture(1, RESOURCE_TEXTURE_CUBE, 0, 1);
	D3D12_CLEAR_VALUE d3dRtbClearValue = { DXGI_FORMAT_R8G8B8A8_UNORM, { 0.0f, 0.0f, 0.0f, 1.0f } };
	ID3D12Resource* pd3dResource = pTexture->CreateTexture(pd3dDevice, pd3dCommandList, nCubeMapSize, nCubeMapSize, 6, 1,
		DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ, &d3dRtbClearValue, RESOURCE_TEXTURE_CUBE, 0);
	pShader->CreateShaderResourceViews(pd3dDevice, pTexture, 0, PARAMETER_CUBE);

	D3D12_RENDER_TARGET_VIEW_DESC d3dRTVDesc;
	d3dRTVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3dRTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
	d3dRTVDesc.Texture2DArray.MipSlice = 0;
	d3dRTVDesc.Texture2DArray.PlaneSlice = 0;
	d3dRTVDesc.Texture2DArray.ArraySize = 1;

	for (int j = 0; j < 6; j++)
	{
		m_pd3dRtvCPUDescriptorHandles[j] = d3dRtvCPUDescriptorHandle;
		d3dRTVDesc.Texture2DArray.FirstArraySlice = j; //i-번째 렌더 타겟 뷰는 텍스쳐 큐브의 i-번째 버퍼에서 시작
		pd3dDevice->CreateRenderTargetView(pd3dResource, &d3dRTVDesc, m_pd3dRtvCPUDescriptorHandles[j]);
		d3dRtvCPUDescriptorHandle.ptr += ::gnRtvDescriptorIncrementSize;
	}

	CMaterial* pMaterial = new CMaterial();
	pMaterial->m_pReflection = new MATERIAL;
	pMaterial->m_pReflection->m_xmf4Ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	pMaterial->m_pReflection->m_xmf4Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	pMaterial->m_pReflection->m_xmf4Specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 3.0f);
	pMaterial->m_pReflection->m_xmf4Emissive = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	pMaterial->SetTexture(pTexture);

	SetMaterial(0, pMaterial);
}

CDynamicCubeMappingObject::~CDynamicCubeMappingObject()
{
	for (int j = 0; j < 6; j++)
	{
		if (m_ppCameras[j])
		{
			m_ppCameras[j]->ReleaseShaderVariables();
			delete m_ppCameras[j];
		}
	}

	if (m_pd3dDepthStencilBuffer) m_pd3dDepthStencilBuffer->Release();
}

void CDynamicCubeMappingObject::OnPreRender(ID3D12GraphicsCommandList* pd3dCommandList, CScene* pScene)
{
	pScene->OnPrepareRender(pd3dCommandList);
	// pPlayer->OnPrepareRender(pd3dCommandList);

	static XMFLOAT3 pxmf3LookAts[6] = { XMFLOAT3(+100.0f, 0.0f, 0.0f), XMFLOAT3(-100.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, +100.0f, 0.0f), XMFLOAT3(0.0f, -100.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, +100.0f), XMFLOAT3(0.0f, 0.0f, -100.0f) };
	//static XMFLOAT3 pxmf3LookAts[6] = { XMFLOAT3(-100.0f, 0.0f, 0.0f), XMFLOAT3(+100.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, -100.0f, 0.0f), XMFLOAT3(0.0f, +100.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -100.0f), XMFLOAT3(0.0f, 0.0f, +100.0f) };
	static XMFLOAT3 pxmf3Ups[6] = { XMFLOAT3(0.0f, +1.0f, 0.0f), XMFLOAT3(0.0f, +1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, +1.0f), XMFLOAT3(0.0f, +1.0f, 0.0f), XMFLOAT3(0.0f, +1.0f, 0.0f) };

	float pfClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	::SynchronizeResourceTransition(pd3dCommandList, m_ppMaterials[0]->m_pTexture->GetResource(0), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);

	XMFLOAT3 xmf3Position = GetPosition();
	for (int j = 0; j < 6; j++)
	{
		m_ppCameras[j]->SetPosition(xmf3Position);
		m_ppCameras[j]->GenerateViewMatrix(xmf3Position, Vector3::Add(xmf3Position, pxmf3LookAts[j]), pxmf3Ups[j]);

		pd3dCommandList->ClearRenderTargetView(m_pd3dRtvCPUDescriptorHandles[j], pfClearColor, 0, NULL);
		pd3dCommandList->ClearDepthStencilView(m_d3dDsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);

		pd3dCommandList->OMSetRenderTargets(1, &m_pd3dRtvCPUDescriptorHandles[j], TRUE, &m_d3dDsvCPUDescriptorHandle);

		pScene->Render(pd3dCommandList, m_ppCameras[j]);
		//pScene->m_pPlayer->m_pShader->Render(pd3dCommandList, m_ppCameras[j]);
		//pScene->m_pPlayer->CGameObject::Render(pd3dCommandList, m_ppCameras[j]);
	}
	::SynchronizeResourceTransition(pd3dCommandList, m_ppMaterials[0]->m_pTexture->GetResource(0), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
}

void CDynamicCubeMappingObject::UpdateBoundingBox()
{
	for (int i = 0; i < m_nMeshes; ++i) {
		m_ppMeshes[i]->m_xmBoundingSphere.Transform(m_xmBoundingSpheres[i], XMLoadFloat4x4(&m_xmf4x4World));
	}
	if (m_pSibling) m_pSibling->UpdateBoundingBox();
	if (m_pChild) m_pChild->UpdateBoundingBox();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
CSkyboxMappingObject::CSkyboxMappingObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, LONG nCubeMapSize, UINT nBoxSize, D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle, D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle, CShader* pShader)
{
	m_nCubeMapSize = nCubeMapSize;
	m_nBoxSize = nBoxSize;
	m_nMeshes = 1;
	m_ppBoundingBoxMeshes = new CBoundingBoxMesh * [m_nMeshes];
	m_xmBoundingSpheres = new BoundingSphere[m_nMeshes];

	//Camera[6]
	for (int j = 0; j < 6; j++)
	{
		m_ppCameras[j] = new CCamera();
		m_ppCameras[j]->SetViewport(0, 0, nCubeMapSize, nCubeMapSize, 0.0f, 1.0f);
		m_ppCameras[j]->SetScissorRect(0, 0, nCubeMapSize, nCubeMapSize);
		m_ppCameras[j]->CreateShaderVariables(pd3dDevice, pd3dCommandList);
		//m_ppCameras[j]->GenerateProjectionMatrix(0.1f, 5000.0f, 1.0f/*Aspect Ratio*/, 90.0f/*FOV*/);
		m_ppCameras[j]->GenerateOrthoProjectionMatrix(m_nBoxSize, m_nBoxSize, 0.0f, m_nBoxSize);
	}

	//Depth Buffer & View
	D3D12_CLEAR_VALUE d3dDsbClearValue = { DXGI_FORMAT_D24_UNORM_S8_UINT, { 1.0f, 0 } };
	m_pd3dDepthStencilBuffer = ::CreateTexture2DResource(pd3dDevice, pd3dCommandList, nCubeMapSize, nCubeMapSize, 1, 1,
		DXGI_FORMAT_D24_UNORM_S8_UINT, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, D3D12_RESOURCE_STATE_DEPTH_WRITE, &d3dDsbClearValue);

	m_d3dDsvCPUDescriptorHandle = d3dDsvCPUDescriptorHandle;
	pd3dDevice->CreateDepthStencilView(m_pd3dDepthStencilBuffer, NULL, m_d3dDsvCPUDescriptorHandle);

	CTexture* pTexture = new CTexture(1, RESOURCE_TEXTURE_CUBE, 0, 1);
	D3D12_CLEAR_VALUE d3dRtbClearValue = { DXGI_FORMAT_R8G8B8A8_UNORM, { 0.0f, 0.0f, 0.0f, 1.0f } };
	ID3D12Resource* pd3dResource = pTexture->CreateTexture(pd3dDevice, pd3dCommandList, nCubeMapSize, nCubeMapSize, 6, 1,
		DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ, &d3dRtbClearValue, RESOURCE_TEXTURE_CUBE, 0);
	pShader->CreateShaderResourceViews(pd3dDevice, pTexture, 0, PARAMETER_CUBE);
	//pShader->CreateShaderResourceViews(pd3dDevice, pTexture, 0, PARAMETER_SKYBOX_CUBE_TEXTURE);

	D3D12_RENDER_TARGET_VIEW_DESC d3dRTVDesc;
	d3dRTVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3dRTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
	d3dRTVDesc.Texture2DArray.MipSlice = 0;
	d3dRTVDesc.Texture2DArray.PlaneSlice = 0;
	d3dRTVDesc.Texture2DArray.ArraySize = 1;

	for (int j = 0; j < 6; j++)
	{
		m_pd3dRtvCPUDescriptorHandles[j] = d3dRtvCPUDescriptorHandle;
		d3dRTVDesc.Texture2DArray.FirstArraySlice = j; //i-번째 렌더 타겟 뷰는 텍스쳐 큐브의 i-번째 버퍼에서 시작
		pd3dDevice->CreateRenderTargetView(pd3dResource, &d3dRTVDesc, m_pd3dRtvCPUDescriptorHandles[j]);
		d3dRtvCPUDescriptorHandle.ptr += ::gnRtvDescriptorIncrementSize;
	}

	CMaterial* pMaterial = new CMaterial();
	pMaterial->m_pReflection = new MATERIAL;
	pMaterial->m_pReflection->m_xmf4Ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	pMaterial->m_pReflection->m_xmf4Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	pMaterial->m_pReflection->m_xmf4Specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 3.0f);
	pMaterial->m_pReflection->m_xmf4Emissive = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	pMaterial->SetTexture(pTexture);

	SetMaterial(0, pMaterial);
}

CSkyboxMappingObject::~CSkyboxMappingObject()
{
	for (int j = 0; j < 6; j++)
	{
		if (m_ppCameras[j])
		{
			m_ppCameras[j]->ReleaseShaderVariables();
			delete m_ppCameras[j];
		}
	}

	if (m_pd3dDepthStencilBuffer) m_pd3dDepthStencilBuffer->Release();
}

void CSkyboxMappingObject::OnPreRender(ID3D12GraphicsCommandList* pd3dCommandList, CScene* pScene)
{
	pScene->OnPrepareRender(pd3dCommandList);

	static XMFLOAT3 pxmf3LookAts[6] = {
		XMFLOAT3(m_nBoxSize / 2, 0.0f, 0.0f), XMFLOAT3(-m_nBoxSize / 2, 0.0f, 0.0f),
		XMFLOAT3(0.0f, -m_nBoxSize / 2, 0.0f), XMFLOAT3(0.0f, m_nBoxSize / 2, 0.0f),
		XMFLOAT3(0.0f, 0.0f, -m_nBoxSize / 2), XMFLOAT3(0.0f, 0.0f, m_nBoxSize / 2) };
	static XMFLOAT3 pxmf3Ups[6] = {
		XMFLOAT3(0.0f, +1.0f, 0.0f), XMFLOAT3(0.0f, +1.0f, 0.0f), 
		XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), 
		XMFLOAT3(0.0f, +1.0f, 0.0f), XMFLOAT3(0.0f, +1.0f, 0.0f) };

	float pfClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	::SynchronizeResourceTransition(pd3dCommandList, m_ppMaterials[0]->m_pTexture->GetResource(0), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);

	XMFLOAT3 xmf3Position = GetPosition();
	for (int j = 0; j < 6; j++)
	{
		m_ppCameras[j]->SetPosition(xmf3Position);
		m_ppCameras[j]->GenerateViewMatrix(
			Vector3::Subtract(xmf3Position, pxmf3LookAts[j]),
			Vector3::Add(xmf3Position, pxmf3LookAts[j]), pxmf3Ups[j]);
		
		pd3dCommandList->ClearRenderTargetView(m_pd3dRtvCPUDescriptorHandles[j], pfClearColor, 0, NULL);
		pd3dCommandList->ClearDepthStencilView(m_d3dDsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);

		pd3dCommandList->OMSetRenderTargets(1, &m_pd3dRtvCPUDescriptorHandles[j], TRUE, &m_d3dDsvCPUDescriptorHandle);

		pScene->Render(pd3dCommandList, m_ppCameras[j]);
		pScene->m_pPlayer->m_pShader->Render(pd3dCommandList, m_ppCameras[j]);
		pScene->m_pPlayer->CGameObject::Render(pd3dCommandList, m_ppCameras[j]);
	}
	::SynchronizeResourceTransition(pd3dCommandList, m_ppMaterials[0]->m_pTexture->GetResource(0), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
}
