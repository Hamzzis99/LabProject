#pragma once

#include "Mesh.h"
#include "Camera.h"

// Single base class. For this VirtuaCop-style shooter we do not need any
// derived specialisations (no bullets, no explosions, no airplanes).
class CGameObject
{
public:
    CGameObject(CMesh* pMesh);
    CGameObject();
    virtual ~CGameObject();

public:
    bool                        m_bActive = true;

    CMesh*                      m_pMesh = NULL;
    XMFLOAT4X4                  m_xmf4x4World;

    BoundingOrientedBox         m_xmOOBB;

    DWORD                       m_dwColor;

    // "Hit" state machine: when the player clicks and the ray hits this
    // object, we flash it white for HIT_FLASH_DURATION seconds and then
    // remove it from the scene (m_bActive = false).
    bool                        m_bHit = false;
    float                       m_fHitRemainingTime = 0.0f;
    static constexpr float      HIT_FLASH_DURATION = 0.15f;

public:
    void SetActive(bool bActive) { m_bActive = bActive; }
    void SetMesh(CMesh* pMesh) { m_pMesh = pMesh; if (pMesh) pMesh->AddRef(); }
    void SetColor(DWORD dwColor) { m_dwColor = dwColor; }
    void SetPosition(float x, float y, float z);
    void SetPosition(XMFLOAT3& xmf3Position);

    XMFLOAT3 GetPosition();

    void UpdateBoundingBox();

    // Start the hit flash. Safe to call multiple times; we just restart the timer.
    void OnHit();

    virtual void Animate(float fElapsedTime);
    virtual void Render(HDC hDCFrameBuffer, CCamera* pCamera);

    // Picking helpers (unchanged from the base PickingCulling project).
    void GenerateRayForPicking(XMVECTOR& xmvPickPosition, XMMATRIX& xmmtxView, XMVECTOR& xmvPickRayOrigin, XMVECTOR& xmvPickRayDirection);
    int  PickObjectByRayIntersection(XMVECTOR& xmPickPosition, XMMATRIX& xmmtxView, float* pfHitDistance);
};