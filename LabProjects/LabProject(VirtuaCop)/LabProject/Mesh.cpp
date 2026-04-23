#include "stdafx.h"
#include "Mesh.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
CPolygon::CPolygon(int nVertices)
{
    if (nVertices > 0)
    {
        m_nVertices = nVertices;
        m_pVertices = new CVertex[nVertices];
    }
}

CPolygon::~CPolygon()
{
    if (m_pVertices) delete[] m_pVertices;
}

void CPolygon::SetVertex(int nIndex, CVertex& vertex)
{
    if ((0 <= nIndex) && (nIndex < m_nVertices) && m_pVertices)
    {
        m_pVertices[nIndex] = vertex;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
CMesh::CMesh(int nPolygons)
{
    if (nPolygons > 0)
    {
        m_nPolygons = nPolygons;
        m_ppPolygons = new CPolygon*[nPolygons];
        m_nReferences = 0;
    }
    m_xmOOBB = BoundingOrientedBox(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
}

CMesh::~CMesh(void)
{
    if (m_ppPolygons)
    {
        for (int i = 0; i < m_nPolygons; i++) if (m_ppPolygons[i]) delete m_ppPolygons[i];
        delete[] m_ppPolygons;
    }
}

void CMesh::SetPolygon(int nIndex, CPolygon* pPolygon)
{
    if ((0 <= nIndex) && (nIndex < m_nPolygons) && m_ppPolygons)
    {
        m_ppPolygons[nIndex] = pPolygon;
    }
}

// Filled rendering path (replaces the old MoveToEx+LineTo wireframe).
// For each face:
//   1. Compute world-space normal from the first three vertices.
//   2. Backface-cull using dot(normal, face-to-camera).
//   3. Transform vertices to clip space via World * View * Projection.
//   4. Perform NDC range check (reject polys fully outside the frustum).
//   5. Screen-transform to POINT array and call ::Polygon().
// Convex meshes (cubes) stay correct after backface culling alone.
void CMesh::Render(HDC hDCFrameBuffer, XMFLOAT4X4& xmf4x4World, CCamera* pCamera)
{
    XMMATRIX mtxWorld = XMLoadFloat4x4(&xmf4x4World);
    XMFLOAT4X4 xmf4x4Transform = Matrix4x4::Multiply(xmf4x4World, pCamera->m_xmf4x4ViewProject);

    XMVECTOR xmvCameraPos = XMLoadFloat3(&pCamera->m_xmf3Position);

    const float halfW = pCamera->m_d3dViewport.Width * 0.5f;
    const float halfH = pCamera->m_d3dViewport.Height * 0.5f;
    const float offX  = pCamera->m_d3dViewport.TopLeftX + halfW;
    const float offY  = pCamera->m_d3dViewport.TopLeftY + halfH;

    for (int j = 0; j < m_nPolygons; j++)
    {
        CPolygon* pPoly = m_ppPolygons[j];
        int nVertices = pPoly->m_nVertices;
        if (nVertices < 3) continue;

        // --- Backface cull in world space ---
        XMVECTOR v0m = XMLoadFloat3(&pPoly->m_pVertices[0].m_xmf3Position);
        XMVECTOR v1m = XMLoadFloat3(&pPoly->m_pVertices[1].m_xmf3Position);
        XMVECTOR v2m = XMLoadFloat3(&pPoly->m_pVertices[2].m_xmf3Position);
        XMVECTOR v0w = XMVector3TransformCoord(v0m, mtxWorld);
        XMVECTOR v1w = XMVector3TransformCoord(v1m, mtxWorld);
        XMVECTOR v2w = XMVector3TransformCoord(v2m, mtxWorld);
        XMVECTOR normal  = XMVector3Normalize(XMVector3Cross(XMVectorSubtract(v1w, v0w), XMVectorSubtract(v2w, v0w)));
        XMVECTOR toFace  = XMVectorSubtract(v0w, xmvCameraPos);
        float fDot;
        XMStoreFloat(&fDot, XMVector3Dot(normal, toFace));
        if (fDot > 0.0f) continue; // face pointing away from camera

        // --- Transform + simple frustum check + screen-space POINTs ---
        POINT pts[16];
        if (nVertices > 16) nVertices = 16;
        bool bAnyVisible = false;
        for (int i = 0; i < nVertices; i++)
        {
            XMFLOAT3 ndc = Vector3::TransformCoord(pPoly->m_pVertices[i].m_xmf3Position, xmf4x4Transform);
            if ((ndc.z >= 0.0f) && (ndc.z <= 1.0f)) bAnyVisible = true;

            pts[i].x = (LONG)(+ndc.x * halfW + offX);
            pts[i].y = (LONG)(-ndc.y * halfH + offY);
        }
        if (!bAnyVisible) continue;

        ::Polygon(hDCFrameBuffer, pts, nVertices);
    }
}

BOOL CMesh::RayIntersectionByTriangle(XMVECTOR& xmRayOrigin, XMVECTOR& xmRayDirection, XMVECTOR v0, XMVECTOR v1, XMVECTOR v2, float* pfNearHitDistance)
{
    float fHitDistance;
    BOOL bIntersected = TriangleTests::Intersects(xmRayOrigin, xmRayDirection, v0, v1, v2, fHitDistance);
    if (bIntersected && (fHitDistance < *pfNearHitDistance)) *pfNearHitDistance = fHitDistance;
    return(bIntersected);
}

int CMesh::CheckRayIntersection(XMVECTOR& xmvPickRayOrigin, XMVECTOR& xmvPickRayDirection, float* pfNearHitDistance)
{
    int nIntersections = 0;
    bool bIntersected = m_xmOOBB.Intersects(xmvPickRayOrigin, xmvPickRayDirection, *pfNearHitDistance);
    if (bIntersected)
    {
        for (int i = 0; i < m_nPolygons; i++)
        {
            switch (m_ppPolygons[i]->m_nVertices)
            {
                case 3:
                {
                    XMVECTOR v0 = XMLoadFloat3(&(m_ppPolygons[i]->m_pVertices[0].m_xmf3Position));
                    XMVECTOR v1 = XMLoadFloat3(&(m_ppPolygons[i]->m_pVertices[1].m_xmf3Position));
                    XMVECTOR v2 = XMLoadFloat3(&(m_ppPolygons[i]->m_pVertices[2].m_xmf3Position));
                    if (RayIntersectionByTriangle(xmvPickRayOrigin, xmvPickRayDirection, v0, v1, v2, pfNearHitDistance)) nIntersections++;
                    break;
                }
                case 4:
                {
                    XMVECTOR v0 = XMLoadFloat3(&(m_ppPolygons[i]->m_pVertices[0].m_xmf3Position));
                    XMVECTOR v1 = XMLoadFloat3(&(m_ppPolygons[i]->m_pVertices[1].m_xmf3Position));
                    XMVECTOR v2 = XMLoadFloat3(&(m_ppPolygons[i]->m_pVertices[2].m_xmf3Position));
                    if (RayIntersectionByTriangle(xmvPickRayOrigin, xmvPickRayDirection, v0, v1, v2, pfNearHitDistance)) nIntersections++;
                    v0 = XMLoadFloat3(&(m_ppPolygons[i]->m_pVertices[0].m_xmf3Position));
                    v1 = XMLoadFloat3(&(m_ppPolygons[i]->m_pVertices[2].m_xmf3Position));
                    v2 = XMLoadFloat3(&(m_ppPolygons[i]->m_pVertices[3].m_xmf3Position));
                    if (RayIntersectionByTriangle(xmvPickRayOrigin, xmvPickRayDirection, v0, v1, v2, pfNearHitDistance)) nIntersections++;
                    break;
                }
            }
        }
    }
    return(nIntersections);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
CCubeMesh::CCubeMesh(float fWidth, float fHeight, float fDepth) : CMesh(6)
{
    float fHalfWidth = fWidth * 0.5f;
    float fHalfHeight = fHeight * 0.5f;
    float fHalfDepth = fDepth * 0.5f;

    CPolygon* pFrontFace = new CPolygon(4);
    pFrontFace->SetVertex(0, CVertex(-fHalfWidth, +fHalfHeight, -fHalfDepth));
    pFrontFace->SetVertex(1, CVertex(+fHalfWidth, +fHalfHeight, -fHalfDepth));
    pFrontFace->SetVertex(2, CVertex(+fHalfWidth, -fHalfHeight, -fHalfDepth));
    pFrontFace->SetVertex(3, CVertex(-fHalfWidth, -fHalfHeight, -fHalfDepth));
    SetPolygon(0, pFrontFace);

    CPolygon* pTopFace = new CPolygon(4);
    pTopFace->SetVertex(0, CVertex(-fHalfWidth, +fHalfHeight, +fHalfDepth));
    pTopFace->SetVertex(1, CVertex(+fHalfWidth, +fHalfHeight, +fHalfDepth));
    pTopFace->SetVertex(2, CVertex(+fHalfWidth, +fHalfHeight, -fHalfDepth));
    pTopFace->SetVertex(3, CVertex(-fHalfWidth, +fHalfHeight, -fHalfDepth));
    SetPolygon(1, pTopFace);

    CPolygon* pBackFace = new CPolygon(4);
    pBackFace->SetVertex(0, CVertex(-fHalfWidth, -fHalfHeight, +fHalfDepth));
    pBackFace->SetVertex(1, CVertex(+fHalfWidth, -fHalfHeight, +fHalfDepth));
    pBackFace->SetVertex(2, CVertex(+fHalfWidth, +fHalfHeight, +fHalfDepth));
    pBackFace->SetVertex(3, CVertex(-fHalfWidth, +fHalfHeight, +fHalfDepth));
    SetPolygon(2, pBackFace);

    CPolygon* pBottomFace = new CPolygon(4);
    pBottomFace->SetVertex(0, CVertex(-fHalfWidth, -fHalfHeight, -fHalfDepth));
    pBottomFace->SetVertex(1, CVertex(+fHalfWidth, -fHalfHeight, -fHalfDepth));
    pBottomFace->SetVertex(2, CVertex(+fHalfWidth, -fHalfHeight, +fHalfDepth));
    pBottomFace->SetVertex(3, CVertex(-fHalfWidth, -fHalfHeight, +fHalfDepth));
    SetPolygon(3, pBottomFace);

    CPolygon* pLeftFace = new CPolygon(4);
    pLeftFace->SetVertex(0, CVertex(-fHalfWidth, +fHalfHeight, +fHalfDepth));
    pLeftFace->SetVertex(1, CVertex(-fHalfWidth, +fHalfHeight, -fHalfDepth));
    pLeftFace->SetVertex(2, CVertex(-fHalfWidth, -fHalfHeight, -fHalfDepth));
    pLeftFace->SetVertex(3, CVertex(-fHalfWidth, -fHalfHeight, +fHalfDepth));
    SetPolygon(4, pLeftFace);

    CPolygon* pRightFace = new CPolygon(4);
    pRightFace->SetVertex(0, CVertex(+fHalfWidth, +fHalfHeight, -fHalfDepth));
    pRightFace->SetVertex(1, CVertex(+fHalfWidth, +fHalfHeight, +fHalfDepth));
    pRightFace->SetVertex(2, CVertex(+fHalfWidth, -fHalfHeight, +fHalfDepth));
    pRightFace->SetVertex(3, CVertex(+fHalfWidth, -fHalfHeight, -fHalfDepth));
    SetPolygon(5, pRightFace);

    m_xmOOBB = BoundingOrientedBox(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(fHalfWidth, fHalfHeight, fHalfDepth), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
}

CCubeMesh::~CCubeMesh(void)
{
}