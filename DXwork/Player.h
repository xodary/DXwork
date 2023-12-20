//-----------------------------------------------------------------------------
// File: Player.h
//-----------------------------------------------------------------------------
#pragma once
#include "Object.h"
#include "Camera.h"
#include "Shader.h"

struct CB_PLAYER_INFO
{
	XMFLOAT4X4					m_xmf4x4World;
};

class CPlayer : public CGameObject
{
public:
	XMFLOAT3					m_xmf3Position;
	XMFLOAT3					m_xmf3Right;
	XMFLOAT3					m_xmf3Up;
	XMFLOAT3					m_xmf3Look;

	float           			m_fPitch;
	float           			m_fYaw;
	float           			m_fRoll;

	XMFLOAT3					m_xmf3Velocity;
	XMFLOAT3     				m_xmf3Gravity;
	float           			m_fMaxVelocityXZ;
	float           			m_fMaxVelocityY;
	float           			m_fFriction;

	LPVOID						m_pPlayerUpdatedContext;
	LPVOID						m_pCameraUpdatedContext;

	CCamera						*m_pCamera = NULL;

	CShader						*m_pShader = NULL;

	XMFLOAT3					m_xmf3BeforeCollidedPosition = XMFLOAT3(0.0f, 0.0f, 0.0f);

public:
	CPlayer(int nMesh, int nMaterial);
	CPlayer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, void* pContext=NULL, int nMesh = 1, int nMaterials = 1);
	void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	void ReleaseShaderVariables();
	~CPlayer();

	CCamera* OnChangeCamera(DWORD nNewCameraMode, DWORD nCurrentCameraMode);

	CCamera* GetCamera() { return(m_pCamera); }

	XMFLOAT3 GetPosition() { return(m_xmf3Position); }
	XMFLOAT3 GetLookVector() { return(m_xmf3Look); }
	XMFLOAT3 GetUpVector() { return(m_xmf3Up); }
	XMFLOAT3 GetRightVector() { return(m_xmf3Right); }
	const XMFLOAT3& GetVelocity() const { return(m_xmf3Velocity); }

	void SetFriction(float fFriction) { m_fFriction = fFriction; }
	void SetGravity(const XMFLOAT3& xmf3Gravity) { m_xmf3Gravity = xmf3Gravity; }
	void SetMaxVelocityXZ(float fMaxVelocity) { m_fMaxVelocityXZ = fMaxVelocity; }
	void SetMaxVelocityY(float fMaxVelocity) { m_fMaxVelocityY = fMaxVelocity; }
	void SetVelocity(const XMFLOAT3& xmf3Velocity) { m_xmf3Velocity = xmf3Velocity; }
	void SetPosition(const XMFLOAT3& xmf3Position, bool bMove = true);

	virtual void OnPlayerUpdateCallback(float fTimeElapsed) { }
	void SetPlayerUpdatedContext(LPVOID pContext) { m_pPlayerUpdatedContext = pContext; }

	virtual void OnCameraUpdateCallback(float fTimeElapsed) { }
	void SetCameraUpdatedContext(LPVOID pContext) { m_pCameraUpdatedContext = pContext; }

	virtual CCamera* ChangeCamera();

	void Move(DWORD nDirection, float fDistance, bool bVelocity = false);
	void Move(const XMFLOAT3& xmf3Shift, bool bVelocity = false);
	void Rotate(float x=0.f, float y = 0.f, float z = 0.f, bool World=false);

	virtual void Animate(float fTimeElapsed, XMFLOAT4X4* pxmf4x4Parent = NULL) { }
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
	virtual void OnPrepareRender();
	virtual void Update(XMFLOAT3& xmf3LookAt, float fTimeElapsed) { }
	void Update(float fTimeElapsed);

protected:
	ID3D12Resource					*m_pd3dcbPlayer = NULL;
	CB_PLAYER_INFO					*m_pcbMappedPlayer = NULL;

};

class CTerrainPlayer : public CPlayer
{
public:
	CTerrainPlayer(int nMesh, int nMaterial);
	CTerrainPlayer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, void* pContext = NULL, int nMesh=1, int nMaterials=1);
	virtual ~CTerrainPlayer();

	virtual CCamera* ChangeCamera(DWORD nNewCameraMode, float fTimeElapsed);

	virtual void OnPlayerUpdateCallback(float fTimeElapsed);
	virtual void OnCameraUpdateCallback(float fTimeElapsed);
};

class CTankPlayer : public CTerrainPlayer
{
public:
	CGameObject						*m_pBodyFrame;
	CGameObject						*m_pTurretFrame;
	CGameObject						*m_pTracksBackLeftFrame;
	CGameObject						*m_pTracksBackRightFrame;
	CGameObject						*m_pTracksFrontLeftFrame;
	CGameObject						*m_pTracksFrontRightFrame;
	
	float							m_fwheelRotationY = 0.0f;
	float							m_fTurretRotationY = 0.0f;

	CTankPlayer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, void* pContext = NULL, int nMesh = 1, int nMaterials = 1);
	virtual ~CTankPlayer();

	virtual void PrepareAnimate();
	virtual void Animate(float fTimeElapsed, XMFLOAT4X4* pxmf4x4Parent = NULL);
	void RotateTurret(float fAngle);
	CGameObject* GetTurret() { return m_pTurretFrame; }

	int								m_killedEnermy = 0;
};