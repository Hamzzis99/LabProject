#pragma once

#include "Mesh.h"
#include "Camera.h"

// Life-cycle of an enemy cube after being shot:
//
//   Alive --(OnHit)--> Spinning --(SPIN_DURATION)--> Exploding --(EXPLOSION_DURATION)--> Dead
//
//   Spinning   : the original cube spins fast around its own Y axis while
//                keeping its original colour. Rotation continues right up
//                until the explosion kicks in, preserving momentum into
//                the debris cloud (no dead-air pause).
//   Exploding  : the original cube disappears; 240 small cube debris fly
//                outward on random unit-sphere vectors, each piece also
//                rotating around its own axis. Same pattern as the
//                PickingCulling CExplosiveObject effect (A-key demo).
//   Dead       : m_bActive is set to false; the scene skips the object.
enum class HitState : int
{
    Alive,
    Spinning,
    Exploding,
    Dead
};

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

    // ---- Hit state machine ---------------------------------------------
    HitState                    m_hitState    = HitState::Alive;
    float                       m_fPhaseTimer = 0.0f;

    // Spin phase: the cube's transform at hit-time is frozen into SpinBase,
    // then W = RotationY(angle) * SpinBase each frame. Rotation continues
    // all the way until the explosion takes over; there is no idle pause.
    XMFLOAT4X4                  m_xmf4x4SpinBase;
    static constexpr float      SPIN_DURATION = 1.4f;     // seconds
    static constexpr float      SPIN_SPEED    = 1440.0f;  // deg/s  (= 4 rev/s)

    // Explosion phase: 240 debris, each piece owns a transform.
    static constexpr int        EXPLOSION_DEBRIS    = 240;
    static constexpr float      EXPLOSION_DURATION  = 1.2f;    // seconds
    static constexpr float      EXPLOSION_SPEED     = 12.0f;   // units/s radial
    static constexpr float      EXPLOSION_ROT_SPEED = 720.0f;  // deg/s per piece
    XMFLOAT3                    m_xmf3ExplosionCenter;
    XMFLOAT4X4                  m_pxmf4x4DebrisTransforms[EXPLOSION_DEBRIS];

    // Shared across every exploding object (initialised once).
    static CMesh*               s_pDebrisMesh;
    static XMFLOAT3             s_pxmf3SphereVectors[EXPLOSION_DEBRIS];
    static void PrepareExplosion();   // call once before any enemy might be hit
    static void ReleaseExplosion();   // call once on shutdown

public:
    void SetActive(bool bActive) { m_bActive = bActive; }
    void SetMesh(CMesh* pMesh) { m_pMesh = pMesh; if (pMesh) pMesh->AddRef(); }
    void SetColor(DWORD dwColor) { m_dwColor = dwColor; }
    void SetPosition(float x, float y, float z);
    void SetPosition(XMFLOAT3& xmf3Position);

    XMFLOAT3 GetPosition();

    // Pickable only while Alive. Prevents re-hitting a cube that is already
    // spinning or exploding.
    bool IsPickable() const { return m_bActive && (m_hitState == HitState::Alive); }

    void UpdateBoundingBox();

    // Start the hit pipeline. No-op if the cube is not currently Alive.
    void OnHit();

    virtual void Animate(float fElapsedTime);
    virtual void Render(HDC hDCFrameBuffer, CCamera* pCamera);

    // Picking helpers (unchanged from the PickingCulling base project).
    void GenerateRayForPicking(XMVECTOR& xmvPickPosition, XMMATRIX& xmmtxView, XMVECTOR& xmvPickRayOrigin, XMVECTOR& xmvPickRayDirection);
    int  PickObjectByRayIntersection(XMVECTOR& xmPickPosition, XMMATRIX& xmmtxView, float* pfHitDistance);
};
