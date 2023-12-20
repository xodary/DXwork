//-----------------------------------------------------------------------------
// File: Player.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "Player.h"
#include "Shader.h"

CPlayer::CPlayer(int nMesh, int nMaterial) : CGameObject(nMesh, nMaterial)
{
	m_pCamera = NULL;

	m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_xmf3Right = XMFLOAT3(1.0f, 0.0f, 0.0f);
	m_xmf3Up = XMFLOAT3(0.0f, 1.0f, 0.0f);
	m_xmf3Look = XMFLOAT3(0.0f, 0.0f, 1.0f);

	m_xmf3Velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_xmf3Gravity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_fMaxVelocityXZ = 0.0f;
	m_fMaxVelocityY = 0.0f;
	m_fFriction = 0.0f;

	m_fPitch = 0.0f;
	m_fRoll = 0.0f;
	m_fYaw = 0.0f;

	m_pPlayerUpdatedContext = NULL;
	m_pCameraUpdatedContext = NULL;
}

CPlayer::CPlayer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, void* pContext, int nMesh, int nMaterials) : CPlayer(nMesh, nMaterials)
{
	m_pCamera = ChangeCamera();
	if (m_pCamera->GetMode() == THIRD_PERSON_CAMERA) m_pCamera->SetLookAt(m_xmf3Position);

	CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void CPlayer::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (m_pCamera) m_pCamera->CreateShaderVariables(pd3dDevice, pd3dCommandList);

	UINT ncbElementBytes = ((sizeof(CB_PLAYER_INFO) + 255) & ~255); //256의 배수
	m_pd3dcbPlayer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);
	
	m_pd3dcbPlayer->Map(0, NULL, (void**)&m_pcbMappedPlayer);

	CGameObject::CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void CPlayer::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	XMStoreFloat4x4(&m_pcbMappedPlayer->m_xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4World)));

	D3D12_GPU_VIRTUAL_ADDRESS d3dGpuVirtualAddress = m_pd3dcbPlayer->GetGPUVirtualAddress();
	pd3dCommandList->SetGraphicsRootConstantBufferView(0, d3dGpuVirtualAddress);
}

void CPlayer::ReleaseShaderVariables()
{
	if (m_pCamera) m_pCamera->ReleaseShaderVariables();

	if (m_pd3dcbPlayer)
	{
		m_pd3dcbPlayer->Unmap(0, NULL);
		m_pd3dcbPlayer->Release();
	}
}


CPlayer::~CPlayer()
{
}

CCamera* CPlayer::OnChangeCamera(DWORD nNewCameraMode, DWORD nCurrentCameraMode)
{
	CCamera* pNewCamera = NULL;
	switch (nNewCameraMode)
	{
	case FIRST_PERSON_CAMERA:
		pNewCamera = new CFirstPersonCamera(m_pCamera);
		break;
	case THIRD_PERSON_CAMERA:
		pNewCamera = new CThirdPersonCamera(m_pCamera);
		break;
	case SPACESHIP_CAMERA:
		pNewCamera = new CSpaceShipCamera(m_pCamera);
		break;
	}
	if (nCurrentCameraMode == SPACESHIP_CAMERA)
	{
		m_xmf3Right = Vector3::Normalize(XMFLOAT3(m_xmf3Right.x, 0.0f, m_xmf3Right.z));
		m_xmf3Up = Vector3::Normalize(XMFLOAT3(0.0f, 1.0f, 0.0f));
		m_xmf3Look = Vector3::Normalize(XMFLOAT3(m_xmf3Look.x, 0.0f, m_xmf3Look.z));

		m_fPitch = 0.0f;
		m_fRoll = 0.0f;
		m_fYaw = Vector3::Angle(XMFLOAT3(0.0f, 0.0f, 1.0f), m_xmf3Look);
		if (m_xmf3Look.x < 0.0f) m_fYaw = -m_fYaw;
	}
	else if ((nNewCameraMode == SPACESHIP_CAMERA) && m_pCamera)
	{
		m_xmf3Right = m_pCamera->GetRightVector();
		m_xmf3Up = m_pCamera->GetUpVector();
		m_xmf3Look = m_pCamera->GetLookVector();
	}

	if (pNewCamera)
	{
		pNewCamera->SetMode(nNewCameraMode);
		pNewCamera->SetPlayer(this);
	}

	if (m_pCamera) delete m_pCamera;

	return(pNewCamera);
}

CCamera* CPlayer::ChangeCamera()
{
	DWORD nCurrentCameraMode = 0x00;

	SetFriction(2.0f);
	SetGravity(XMFLOAT3(0.0f, 0.0f, 0.0f));
	SetMaxVelocityXZ(2.5f);
	SetMaxVelocityY(40.0f);
	m_pCamera = OnChangeCamera(FIRST_PERSON_CAMERA, nCurrentCameraMode);
	m_pCamera->SetTimeLag(0.0f);
	m_pCamera->SetOffset(XMFLOAT3(0.0f, 20.0f, 0.0f));
	m_pCamera->GenerateProjectionMatrix(1.01f, 5000.0f, ASPECT_RATIO, 60.0f);
	m_pCamera->SetViewport(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, 0.0f, 1.0f);
	m_pCamera->SetScissorRect(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);

	return(m_pCamera);
}

void CPlayer::Rotate(float fPitch, float fYaw, float fRoll, bool World)
{
	m_pCamera->Rotate(fPitch, fYaw, fRoll, true);
	if (fPitch != 0.0f)
	{
		XMMATRIX mtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&m_xmf3Right), XMConvertToRadians(fPitch));
		m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, mtxRotate);
		m_xmf3Up = Vector3::TransformNormal(m_xmf3Up, mtxRotate);
	}
	if (fYaw != 0.0f)
	{
		XMMATRIX mtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&m_xmf3Up), XMConvertToRadians(fYaw));
		m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, mtxRotate);
		m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, mtxRotate);
	}
	if (fRoll != 0.0f)
	{
		XMMATRIX mtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&m_xmf3Look), XMConvertToRadians(fRoll));
		m_xmf3Up = Vector3::TransformNormal(m_xmf3Up, mtxRotate);
		m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, mtxRotate);
	}

	m_xmf3Look = Vector3::Normalize(m_xmf3Look);
	m_xmf3Right = Vector3::Normalize(Vector3::CrossProduct(m_xmf3Up, m_xmf3Look));
	m_xmf3Up = Vector3::Normalize(Vector3::CrossProduct(m_xmf3Look, m_xmf3Right));

}

void CPlayer::SetPosition(const XMFLOAT3& xmf3Position, bool bMove)
{
	if (bMove) Move(XMFLOAT3(xmf3Position.x - m_xmf3Position.x, xmf3Position.y - m_xmf3Position.y, xmf3Position.z - m_xmf3Position.z), false);
	else m_xmf3Position = xmf3Position;
}

void CPlayer::Update(float fTimeElapsed)
{
	m_xmf3Velocity = Vector3::Add(m_xmf3Velocity, Vector3::ScalarProduct(m_xmf3Gravity, fTimeElapsed, false));
	float fLength = sqrtf(m_xmf3Velocity.x * m_xmf3Velocity.x + m_xmf3Velocity.z * m_xmf3Velocity.z);
	float fMaxVelocityXZ = m_fMaxVelocityXZ * fTimeElapsed;
	if (fLength > m_fMaxVelocityXZ)
	{
		m_xmf3Velocity.x *= (fMaxVelocityXZ / fLength);
		m_xmf3Velocity.z *= (fMaxVelocityXZ / fLength);
	}
	float fMaxVelocityY = m_fMaxVelocityY * fTimeElapsed;
	fLength = sqrtf(m_xmf3Velocity.y * m_xmf3Velocity.y);
	if (fLength > m_fMaxVelocityY) m_xmf3Velocity.y *= (fMaxVelocityY / fLength);

	Move(m_xmf3Velocity, false);

	if (m_pPlayerUpdatedContext) OnPlayerUpdateCallback(fTimeElapsed);

	DWORD nCurrentCameraMode = m_pCamera->GetMode();
	if (nCurrentCameraMode == THIRD_PERSON_CAMERA) m_pCamera->Update(m_xmf3Position, fTimeElapsed);
	if (m_pCameraUpdatedContext) OnCameraUpdateCallback(fTimeElapsed);
	m_pCamera->RegenerateViewMatrix();

	fLength = Vector3::Length(m_xmf3Velocity);
	float fDeceleration = (m_fFriction * fTimeElapsed);
	if (fDeceleration > fLength) fDeceleration = fLength;
	m_xmf3Velocity = Vector3::Add(m_xmf3Velocity, Vector3::ScalarProduct(m_xmf3Velocity, -fDeceleration, true));
}

void CPlayer::Move(DWORD dwDirection, float fDistance, bool bUpdateVelocity)
{
	if (dwDirection)
	{
		XMFLOAT3 xmf3Shift = XMFLOAT3(0, 0, 0);
		if (dwDirection & DIR_FORWARD) xmf3Shift = Vector3::Add(xmf3Shift, m_xmf3Look, fDistance);
		if (dwDirection & DIR_BACKWARD) xmf3Shift = Vector3::Add(xmf3Shift, m_xmf3Look, -fDistance);
		if (dwDirection & DIR_RIGHT) xmf3Shift = Vector3::Add(xmf3Shift, m_xmf3Right, fDistance);
		if (dwDirection & DIR_LEFT) xmf3Shift = Vector3::Add(xmf3Shift, m_xmf3Right, -fDistance);
		if (dwDirection & DIR_UP) xmf3Shift = Vector3::Add(xmf3Shift, m_xmf3Up, fDistance);
		if (dwDirection & DIR_DOWN) xmf3Shift = Vector3::Add(xmf3Shift, m_xmf3Up, -fDistance);

		Move(xmf3Shift, bUpdateVelocity);
	}
}

void CPlayer::Move(const XMFLOAT3& xmf3Shift, bool bUpdateVelocity)
{
	if (bUpdateVelocity)
	{
		m_xmf3Velocity = Vector3::Add(m_xmf3Velocity, xmf3Shift);
	}
	else
	{
		m_xmf3Position = Vector3::Add(m_xmf3Position, xmf3Shift);
		m_pCamera->Move(xmf3Shift);
	}
}

void CPlayer::OnPrepareRender()
{
	m_xmf4x4Transform._11 = m_xmf3Right.x; m_xmf4x4Transform._12 = m_xmf3Right.y; m_xmf4x4Transform._13 = m_xmf3Right.z;
	m_xmf4x4Transform._21 = m_xmf3Up.x; m_xmf4x4Transform._22 = m_xmf3Up.y; m_xmf4x4Transform._23 = m_xmf3Up.z;
	m_xmf4x4Transform._31 = m_xmf3Look.x; m_xmf4x4Transform._32 = m_xmf3Look.y; m_xmf4x4Transform._33 = m_xmf3Look.z;
	m_xmf4x4Transform._41 = m_xmf3Position.x; m_xmf4x4Transform._42 = m_xmf3Position.y; m_xmf4x4Transform._43 = m_xmf3Position.z;

	UpdateTransform(NULL);
}

void CPlayer::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	UpdateShaderVariables(pd3dCommandList);
	DWORD nCameraMode = (pCamera) ? pCamera->GetMode() : 0x00;
	if (nCameraMode == THIRD_PERSON_CAMERA)
	{
		if (m_pShader) m_pShader->Render(pd3dCommandList, pCamera);
		CGameObject::Render(pd3dCommandList, pCamera);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CTerrainPlayer
//
CTerrainPlayer::CTerrainPlayer(int nMesh, int nMaterial) : CPlayer(nMesh, nMaterial)
{
}

CTerrainPlayer::CTerrainPlayer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, void* pContext, int nMesh, int nMaterial) : CPlayer(nMesh, nMaterial)
{
	m_pCamera = ChangeCamera(THIRD_PERSON_CAMERA, 0.0f);

	CreateShaderVariables(pd3dDevice, pd3dCommandList);

	CCubeMeshDiffused* pCubeMesh = new CCubeMeshDiffused(pd3dDevice, pd3dCommandList, 4.0f, 12.0f, 4.0f);
	SetMesh(0, pCubeMesh);

	m_pShader = new CPlayerShader();
	m_pShader->CreateShader(pd3dDevice, pd3dGraphicsRootSignature);
	m_pShader->CreateCbvSrvDescriptorHeaps(pd3dDevice, 0, 1);

	CHeightMapTerrain* pTerrain = (CHeightMapTerrain*)pContext;
	SetPosition(XMFLOAT3(pTerrain->GetWidth() * 0.5f, 2000.0f, pTerrain->GetLength() * 0.5f), true);
	SetPlayerUpdatedContext(pTerrain);
	SetCameraUpdatedContext(pTerrain);
}

CTerrainPlayer::~CTerrainPlayer()
{
}

CCamera* CTerrainPlayer::ChangeCamera(DWORD nNewCameraMode, float fTimeElapsed)
{
	DWORD nCurrentCameraMode = (m_pCamera) ? m_pCamera->GetMode() : 0x00;
	if (nCurrentCameraMode == nNewCameraMode) return(m_pCamera);
	switch (nNewCameraMode)
	{
	case FIRST_PERSON_CAMERA:
		SetFriction(250.0f);
		SetGravity(XMFLOAT3(0.0f, -400.0f, 0.0f));
		SetMaxVelocityXZ(300.0f);
		SetMaxVelocityY(400.0f);
		m_pCamera = OnChangeCamera(FIRST_PERSON_CAMERA, nCurrentCameraMode);
		m_pCamera->SetTimeLag(0.0f);
		m_pCamera->SetOffset(XMFLOAT3(0.0f, 20.0f, 0.0f));
		m_pCamera->GenerateProjectionMatrix(1.01f, 50000.0f, ASPECT_RATIO, 60.0f);
		break;
	case SPACESHIP_CAMERA:
		SetFriction(125.0f);
		SetGravity(XMFLOAT3(0.0f, 0.0f, 0.0f));
		SetMaxVelocityXZ(300.0f);
		SetMaxVelocityY(400.0f);
		m_pCamera = OnChangeCamera(SPACESHIP_CAMERA, nCurrentCameraMode);
		m_pCamera->SetTimeLag(0.0f);
		m_pCamera->SetOffset(XMFLOAT3(0.0f, 0.0f, 0.0f));
		m_pCamera->GenerateProjectionMatrix(1.01f, 50000.0f, ASPECT_RATIO, 60.0f);
		break;
	case THIRD_PERSON_CAMERA:
		SetFriction(250.0f);
		SetGravity(XMFLOAT3(0.0f, -250.0f, 0.0f));
		//SetGravity(XMFLOAT3(0.0f, 0.0f, 0.0f));
		SetMaxVelocityXZ(300.0f);
		SetMaxVelocityY(400.0f);
		m_pCamera = OnChangeCamera(THIRD_PERSON_CAMERA, nCurrentCameraMode);
		m_pCamera->SetTimeLag(0.25f);
		m_pCamera->SetOffset(XMFLOAT3(0.0f, 20.0f, -50.0f));
		m_pCamera->SetLookAt(Vector3::Add(m_xmf3Position, m_pCamera->GetOffset()), m_xmf3Position, m_xmf3Up);
		m_pCamera->GenerateProjectionMatrix(1.01f, 50000.0f, ASPECT_RATIO, 60.0f);
		break;
	default:
		break;
	}
	m_pCamera->SetPosition(Vector3::Add(m_xmf3Position, m_pCamera->GetOffset()));
	Update(fTimeElapsed);

	return(m_pCamera);
}

void CTerrainPlayer::OnPlayerUpdateCallback(float fTimeElapsed)
{
	CHeightMapTerrain* pTerrain = (CHeightMapTerrain*)m_pPlayerUpdatedContext;
	XMFLOAT3 xmf3Scale = pTerrain->GetScale();
	XMFLOAT3 xmf3PlayerPosition = GetPosition();
	int z = (int)(xmf3PlayerPosition.z / xmf3Scale.z);
	//bool bReverseQuad = ((z % 2) != 0);
	float fHeight = pTerrain->GetHeight(xmf3PlayerPosition.x, xmf3PlayerPosition.z) + 6.0f;
	if (xmf3PlayerPosition.y < fHeight)
	{
		XMFLOAT3 terrainNormal;
		terrainNormal = pTerrain->GetNormal(xmf3PlayerPosition.x, xmf3PlayerPosition.z);
		XMVECTOR vTerrainNormal = XMLoadFloat3(&terrainNormal);
		XMVECTOR vTankNormal = XMLoadFloat3(&m_xmf3Up);

		XMVECTOR lerpedVector = XMVectorLerp(vTerrainNormal, vTankNormal, min(0.7, fTimeElapsed * 40));
		
		XMStoreFloat3(&m_xmf3Up, lerpedVector);
		XMStoreFloat3(&m_xmf3Right, XMVector3Cross(XMLoadFloat3(&m_xmf3Up), XMLoadFloat3(&m_xmf3Look)));
		XMStoreFloat3(&m_xmf3Look, XMVector3Cross(XMLoadFloat3(&m_xmf3Right), XMLoadFloat3(&m_xmf3Up)));

		m_xmf3Right = Vector3::Normalize(m_xmf3Right);
		m_xmf3Up = Vector3::Normalize(m_xmf3Up);
		m_xmf3Look = Vector3::Normalize(m_xmf3Look);

		XMFLOAT3 xmf3PlayerVelocity = GetVelocity();
		xmf3PlayerVelocity.y = 0.0f;
		SetVelocity(xmf3PlayerVelocity);
		xmf3PlayerPosition.y = fHeight;
		SetPosition(xmf3PlayerPosition, true);
	}
}

void CTerrainPlayer::OnCameraUpdateCallback(float fTimeElapsed)
{
	CHeightMapTerrain* pTerrain = (CHeightMapTerrain*)m_pCameraUpdatedContext;
	XMFLOAT3 xmf3Scale = pTerrain->GetScale();
	XMFLOAT3 xmf3CameraPosition = m_pCamera->GetPosition();
	int z = (int)(xmf3CameraPosition.z / xmf3Scale.z);
	// bool bReverseQuad = ((z % 2) != 0);
	float fHeight = pTerrain->GetHeight(xmf3CameraPosition.x, xmf3CameraPosition.z) + 5.0f;
	if (xmf3CameraPosition.y <= fHeight)
	{
		xmf3CameraPosition.y = fHeight;
		m_pCamera->SetPosition(xmf3CameraPosition);
	}
}

CTankPlayer::CTankPlayer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, void* pContext, int nMesh, int nMaterials)
	: CTerrainPlayer(nMesh, nMaterials)
{
	m_pCamera = ChangeCamera(THIRD_PERSON_CAMERA, 0.0f);

	CreateShaderVariables(pd3dDevice, pd3dCommandList);

	// m_pShader = new CStandardShader();를 먼저 해버리면
	// m_pShader가 CShader* 타입이어서 뭘 호출하던 CShader::함수() 로 불린다.
	CStandardShader* pShader = new CStandardShader();
	pShader->CreateShader(pd3dDevice, pd3dGraphicsRootSignature);
	pShader->CreateCbvSrvDescriptorHeaps(pd3dDevice, 10, 6);

	CGameObject* pGameObject = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, "Model/TankFree_Blue.bin", pShader);
	SetChild(pGameObject);
	pGameObject->SetScale(8, 8, 8);
	pShader->CreateShaderVariablesAndCreateCBV(pd3dDevice, pd3dCommandList, this);
	m_pShader = pShader;

	PrepareAnimate();

	CHeightMapTerrain* pTerrain = (CHeightMapTerrain*)pContext;
	//SetPosition(XMFLOAT3(1030.0f, 1000.0f, 1400.0f));
	if (pTerrain)
		SetPosition(XMFLOAT3(pTerrain->GetWidth() * 0.5f, 400.0f, pTerrain->GetLength() * 0.5f), false);
	else
		SetPosition(XMFLOAT3(0, 0, 0), false);
	SetPlayerUpdatedContext(pTerrain);
	SetCameraUpdatedContext(pTerrain);
}

CTankPlayer::~CTankPlayer()
{
}

void CTankPlayer::PrepareAnimate()
{
	m_pBodyFrame = FindFrame("TankFree_Blue");
	m_pTurretFrame = FindFrame("TankFree_Tower");
	m_pTracksFrontLeftFrame = FindFrame("TankFree_Wheel_f_left");
	m_pTracksFrontRightFrame = FindFrame("TankFree_Wheel_f_right");
	m_pTracksBackLeftFrame = FindFrame("TankFree_Wheel_b_left");
	m_pTracksBackRightFrame = FindFrame("TankFree_Wheel_b_right");
	// Body
	// ㄴ Turret, FrontTrack, BackTrack ...  

}

void CTankPlayer::Animate(float fTimeElapsed, XMFLOAT4X4* pxmf4x4Parent)
{
	XMFLOAT4X4 preWorld[4] = { m_pTracksFrontLeftFrame->m_xmf4x4World, m_pTracksFrontRightFrame->m_xmf4x4World,
							   m_pTracksBackLeftFrame->m_xmf4x4World, m_pTracksBackRightFrame->m_xmf4x4World };
	XMFLOAT4X4 preTrans[4] = { m_pTracksFrontLeftFrame->m_xmf4x4Transform, m_pTracksFrontRightFrame->m_xmf4x4Transform,
							   m_pTracksBackLeftFrame->m_xmf4x4Transform, m_pTracksBackRightFrame->m_xmf4x4Transform };

	XMMATRIX xmmtxRotate = XMMatrixRotationY(XMConvertToRadians(m_fwheelRotationY * fTimeElapsed));
	if (m_pTracksFrontLeftFrame) m_pTracksFrontLeftFrame->m_xmf4x4Transform = Matrix4x4::Multiply(xmmtxRotate, m_pTracksFrontLeftFrame->m_xmf4x4Transform);
	if (m_pTracksFrontRightFrame) m_pTracksFrontRightFrame->m_xmf4x4Transform = Matrix4x4::Multiply(xmmtxRotate, m_pTracksFrontRightFrame->m_xmf4x4Transform);
	if (m_pTracksBackLeftFrame) m_pTracksBackLeftFrame->m_xmf4x4Transform = Matrix4x4::Multiply(xmmtxRotate, m_pTracksBackLeftFrame->m_xmf4x4Transform);
	if (m_pTracksBackRightFrame) m_pTracksBackRightFrame->m_xmf4x4Transform = Matrix4x4::Multiply(xmmtxRotate, m_pTracksBackRightFrame->m_xmf4x4Transform);
	UpdateTransform(NULL);
	XMFLOAT3 parent = GetLook();
	XMFLOAT3 child = m_pTracksFrontLeftFrame->GetLook();
	int angle = Vector3::Angle(parent, child);

	if (angle >= 40) {
		m_pTracksFrontLeftFrame->m_xmf4x4World = preWorld[0];
		m_pTracksFrontLeftFrame->m_xmf4x4Transform = preTrans[0];
		m_pTracksFrontRightFrame->m_xmf4x4World = preWorld[1];
		m_pTracksFrontRightFrame->m_xmf4x4Transform = preTrans[1];

		m_pTracksBackLeftFrame->m_xmf4x4World = preWorld[2];
		m_pTracksBackLeftFrame->m_xmf4x4Transform = preTrans[2];
		m_pTracksBackRightFrame->m_xmf4x4World = preWorld[3];
		m_pTracksBackRightFrame->m_xmf4x4Transform = preTrans[3];
	}
	CPlayer::Animate(fTimeElapsed, pxmf4x4Parent);
}

void CTankPlayer::RotateTurret(float fAngle)
{
	m_pCamera->Rotate(0.0f, fAngle, 0.0f, true);
	m_pTurretFrame->Rotate(0.0f, fAngle, 0.0f, &m_xmf4x4World);
	m_fTurretRotationY += fAngle;
}
