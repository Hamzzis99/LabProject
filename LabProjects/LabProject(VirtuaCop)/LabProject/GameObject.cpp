#include "stdafx.h"
#include "GameObject.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Shared explosion resources (definitions).
CMesh*    CGameObject::s_pDebrisMesh = nullptr;
XMFLOAT3  CGameObject::s_pxmf3SphereVectors[CGameObject::EXPLOSION_DEBRIS];

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Random helpers (local; same rejection-sampling trick as PickingCulling).
static inline float RandF(float fMin, float fMax)
{
    return fMin + ((float)rand() / (float)RAND_MAX) * (fMax - fMin);
}

static XMVECTOR RandomUnitVectorOnSphere()
{
    XMVECTOR xmvOne = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
    while (true)
    {
        XMVECTOR v = XMVectorSet(RandF(-1.0f, 1.0f), RandF(-1.0f, 1.0f), RandF(-1.0f, 1.0f), 0.0f);
        // Only keep vectors inside the unit ball; otherwise distribution is cube-biased.
        if (!XMVector3Greater(XMVector3LengthSq(v), xmvOne)) return XMVector3Normalize(v);
    }
}

void CGameObject::PrepareExplosion()
{
    if (s_pDebrisMesh) return; // idempotent: safe if called twice
    for (int i = 0; i < EXPLOSION_DEBRIS; ++i)
    {
        XMStoreFloat3(&s_pxmf3SphereVectors[i], RandomUnitVectorOnSphere());
    }
    s_pDebrisMesh = new CCubeMesh(0.5f, 0.5f, 0.5f);
}

void CGameObject::ReleaseExplosion()
{
    if (s_pDebrisMesh)
    {
        s_pDebrisMesh->Release();
        s_pDebrisMesh = nullptr;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
CGameObject::CGameObject()
{
    m_pMesh          = NULL;
    m_xmf4x4World    = Matrix4x4::Identity();
    m_xmf4x4SpinBase = Matrix4x4::Identity();
    m_dwColor        = RGB(0, 0, 0);
}

CGameObject::CGameObject(CMesh* pMesh) : CGameObject()
{
    m_pMesh = pMesh;
}

CGameObject::~CGameObject(void)
{
    if (m_pMesh) m_pMesh->Release();
}

void CGameObject::SetPosition(float x, float y, float z)
{
    m_xmf4x4World._41 = x;
    m_xmf4x4World._42 = y;
    m_xmf4x4World._43 = z;
}

void CGameObject::SetPosition(XMFLOAT3& xmf3Position)
{
    m_xmf4x4World._41 = xmf3Position.x;
    m_xmf4x4World._42 = xmf3Position.y;
    m_xmf4x4World._43 = xmf3Position.z;
}

XMFLOAT3 CGameObject::GetPosition()
{
    return XMFLOAT3(m_xmf4x4World._41, m_xmf4x4World._42, m_xmf4x4World._43);
}

void CGameObject::UpdateBoundingBox()
{
    if (m_pMesh)
    {
        m_pMesh->m_xmOOBB.Transform(m_xmOOBB, XMLoadFloat4x4(&m_xmf4x4World));
        XMStoreFloat4(&m_xmOOBB.Orientation, XMQuaternionNormalize(XMLoadFloat4(&m_xmOOBB.Orientation)));
    }
}

void CGameObject::OnHit()
{
    // Ignore re-hits. Scene::PickObjectPointedByCursor also filters on
    // IsPickable(), so this is belt-and-suspenders.
    if (m_hitState != HitState::Alive) return;

    m_hitState       = HitState::Spinning;
    m_fPhaseTimer    = 0.0f;
    m_xmf4x4SpinBase = m_xmf4x4World;
}

void CGameObject::Animate(float fElapsedTime)
{
    switch (m_hitState)
    {
    case HitState::Alive:
    {
        UpdateBoundingBox();
        break;
    }
    case HitState::Spinning:
    {
        m_fPhaseTimer += fElapsedTime;
        if (m_fPhaseTimer >= SPIN_DURATION)
        {
            // Rotation ends and the explosion begins in the same instant:
            // the spinning cube is replaced by a debris cloud, which
            // visually carries the momentum forward. Centre of the cloud
            // is the cube's last rendered position.
            m_hitState            = HitState::Exploding;
            m_fPhaseTimer         = 0.0f;
            m_xmf3ExplosionCenter = GetPosition();
        }
        else
        {
            // W = Ry(angle) * SpinBase.
            // DirectX row-major (v' = v * W): left matrix is applied first,
            // so the vertex is rotated in local space then placed back into
            // world space by SpinBase. That is "spin at own position".
            float fAngle = m_fPhaseTimer * SPIN_SPEED;
            XMFLOAT4X4 xmf4x4Rot = Matrix4x4::RotationYawPitchRoll(0.0f, fAngle, 0.0f);
            m_xmf4x4World = Matrix4x4::Multiply(xmf4x4Rot, m_xmf4x4SpinBase);
            UpdateBoundingBox();
        }
        break;
    }
    case HitState::Exploding:
    {
        m_fPhaseTimer += fElapsedTime;
        if (m_fPhaseTimer >= EXPLOSION_DURATION)
        {
            m_hitState = HitState::Dead;
            m_bActive  = false;
        }
        else
        {
            // Each debris: translate outward along its sphere vector,
            // then spin around that same vector. Same recipe as
            // CExplosiveObject::Animate in PickingCulling.
            for (int i = 0; i < EXPLOSION_DEBRIS; ++i)
            {
                XMFLOAT4X4 m = Matrix4x4::Identity();
                m._41 = m_xmf3ExplosionCenter.x + s_pxmf3SphereVectors[i].x * EXPLOSION_SPEED * m_fPhaseTimer;
                m._42 = m_xmf3ExplosionCenter.y + s_pxmf3SphereVectors[i].y * EXPLOSION_SPEED * m_fPhaseTimer;
                m._43 = m_xmf3ExplosionCenter.z + s_pxmf3SphereVectors[i].z * EXPLOSION_SPEED * m_fPhaseTimer;
                XMFLOAT4X4 r = Matrix4x4::RotationAxis(s_pxmf3SphereVectors[i], EXPLOSION_ROT_SPEED * m_fPhaseTimer);
                m_pxmf4x4DebrisTransforms[i] = Matrix4x4::Multiply(r, m);
            }
        }
        break;
    }
    case HitState::Dead:
        break;
    }
}

void CGameObject::Render(HDC hDCFrameBuffer, CCamera* pCamera)
{
    if (!m_pMesh) return;

    // Cull by the main OOBB for Alive/Spinning. Skip it while Exploding,
    // since the debris spread well outside the original box and we still
    // want them drawn as long as the cloud is on-screen somewhere.
    if (m_hitState != HitState::Exploding)
    {
        if (!pCamera->IsInFrustum(m_xmOOBB)) return;
    }

    // Always render the cube in its own colour. During Spinning/Pause it is
    // simply the rotating/frozen cube; we no longer flash white.
    DWORD dwDrawColor = m_dwColor;

    HPEN   hPen      = ::CreatePen(PS_SOLID, 1, dwDrawColor);
    HPEN   hOldPen   = (HPEN)::SelectObject(hDCFrameBuffer, hPen);
    HBRUSH hBrush    = ::CreateSolidBrush(dwDrawColor);
    HBRUSH hOldBrush = (HBRUSH)::SelectObject(hDCFrameBuffer, hBrush);

    if (m_hitState == HitState::Exploding)
    {
        if (s_pDebrisMesh)
        {
            for (int i = 0; i < EXPLOSION_DEBRIS; ++i)
            {
                s_pDebrisMesh->Render(hDCFrameBuffer, m_pxmf4x4DebrisTransforms[i], pCamera);
            }
        }
    }
    else
    {
        // Alive or Spinning: render the original cube with the selected colour.
        m_pMesh->Render(hDCFrameBuffer, m_xmf4x4World, pCamera);
    }

    ::SelectObject(hDCFrameBuffer, hOldBrush);
    ::DeleteObject(hBrush);
    ::SelectObject(hDCFrameBuffer, hOldPen);
    ::DeleteObject(hPen);
}

void CGameObject::GenerateRayForPicking(XMVECTOR& xmvPickPosition, XMMATRIX& xmmtxView, XMVECTOR& xmvPickRayOrigin, XMVECTOR& xmvPickRayDirection)
{
    XMMATRIX xmmtxToModel = XMMatrixInverse(NULL, XMLoadFloat4x4(&m_xmf4x4World) * xmmtxView);

    XMFLOAT3 xmf3CameraOrigin(0.0f, 0.0f, 0.0f);
    xmvPickRayOrigin    = XMVector3TransformCoord(XMLoadFloat3(&xmf3CameraOrigin), xmmtxToModel);
    xmvPickRayDirection = XMVector3TransformCoord(xmvPickPosition, xmmtxToModel);
    xmvPickRayDirection = XMVector3Normalize(xmvPickRayDirection - xmvPickRayOrigin);
}

int CGameObject::PickObjectByRayIntersection(XMVECTOR& xmvPickPosition, XMMATRIX& xmmtxView, float* pfHitDistance)
{
    int nIntersected = 0;
    if (m_pMesh)
    {
        XMVECTOR xmvPickRayOrigin, xmvPickRayDirection;
        GenerateRayForPicking(xmvPickPosition, xmmtxView, xmvPickRayOrigin, xmvPickRayDirection);
        nIntersected = m_pMesh->CheckRayIntersection(xmvPickRayOrigin, xmvPickRayDirection, pfHitDistance);
    }
    return nIntersected;
}
