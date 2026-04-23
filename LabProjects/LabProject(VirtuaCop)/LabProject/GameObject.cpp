#include "stdafx.h"
#include "GameObject.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
CGameObject::CGameObject()
{
    m_pMesh = NULL;
    m_xmf4x4World = Matrix4x4::Identity();
    m_dwColor = RGB(0, 0, 0);
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
    return(XMFLOAT3(m_xmf4x4World._41, m_xmf4x4World._42, m_xmf4x4World._43));
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
    m_bHit = true;
    m_fHitRemainingTime = HIT_FLASH_DURATION;
}

void CGameObject::Animate(float fElapsedTime)
{
    UpdateBoundingBox();

    if (m_bHit)
    {
        m_fHitRemainingTime -= fElapsedTime;
        if (m_fHitRemainingTime <= 0.0f)
        {
            m_bHit = false;
            m_bActive = false; // vanish from the scene once the flash finishes
        }
    }
}

void CGameObject::Render(HDC hDCFrameBuffer, CCamera* pCamera)
{
    if (!m_pMesh) return;
    if (!pCamera->IsInFrustum(m_xmOOBB)) return;

    DWORD dwDrawColor = m_bHit ? RGB(255, 255, 255) : m_dwColor;

    // Set both the pen (edge) and brush (fill) to the draw colour.
    // GDI Polygon() uses the currently-selected brush to fill interiors.
    HPEN   hPen     = ::CreatePen(PS_SOLID, 1, dwDrawColor);
    HPEN   hOldPen  = (HPEN)::SelectObject(hDCFrameBuffer, hPen);
    HBRUSH hBrush   = ::CreateSolidBrush(dwDrawColor);
    HBRUSH hOldBrush = (HBRUSH)::SelectObject(hDCFrameBuffer, hBrush);

    m_pMesh->Render(hDCFrameBuffer, m_xmf4x4World, pCamera);

    ::SelectObject(hDCFrameBuffer, hOldBrush);
    ::DeleteObject(hBrush);
    ::SelectObject(hDCFrameBuffer, hOldPen);
    ::DeleteObject(hPen);
}

void CGameObject::GenerateRayForPicking(XMVECTOR& xmvPickPosition, XMMATRIX& xmmtxView, XMVECTOR& xmvPickRayOrigin, XMVECTOR& xmvPickRayDirection)
{
    XMMATRIX xmmtxToModel = XMMatrixInverse(NULL, XMLoadFloat4x4(&m_xmf4x4World) * xmmtxView);

    XMFLOAT3 xmf3CameraOrigin(0.0f, 0.0f, 0.0f);
    xmvPickRayOrigin = XMVector3TransformCoord(XMLoadFloat3(&xmf3CameraOrigin), xmmtxToModel);
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
    return(nIntersected);
}