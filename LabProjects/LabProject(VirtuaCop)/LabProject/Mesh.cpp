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
//   3. Transform vertices to view space (for correct near-plane clipping).
//   4. Sutherland-Hodgman clip against the near plane (view_z >= zNear).
//      This is essential whenever the camera is inside the z-extent of a
//      mesh (e.g. walking through a corridor). Without it, vertices behind
//      the camera project with negative w and the polygon tears.
//   5. Project clipped vertices to NDC via the projection matrix.
//   6. Screen-transform to POINT array and call ::Polygon().
// Convex meshes (cubes) stay correct after backface culling alone.
void CMesh::Render(HDC hDCFrameBuffer, XMFLOAT4X4& xmf4x4World, CCamera* pCamera)
{
    XMMATRIX mtxWorld = XMLoadFloat4x4(&xmf4x4World);
    XMFLOAT4X4 xmf4x4WorldView = Matrix4x4::Multiply(xmf4x4World, pCamera->m_xmf4x4View);
    XMMATRIX mtxWorldView = XMLoadFloat4x4(&xmf4x4WorldView);
    XMMATRIX mtxProj = XMLoadFloat4x4(&pCamera->m_xmf4x4Projection);

    XMVECTOR xmvCameraPos = XMLoadFloat3(&pCamera->m_xmf3Position);

    const float halfW = pCamera->m_d3dViewport.Width * 0.5f;
    const float halfH = pCamera->m_d3dViewport.Height * 0.5f;
    const float offX  = pCamera->m_d3dViewport.TopLeftX + halfW;
    const float offY  = pCamera->m_d3dViewport.TopLeftY + halfH;
    const float zNear = 1.01f;   // matches CPlayer::CPlayer() near plane

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

        // --- Transform vertices to view space ---
        if (nVertices > 16) nVertices = 16;
        XMFLOAT3 viewVerts[16];
        for (int i = 0; i < nVertices; i++)
        {
            XMVECTOR v = XMLoadFloat3(&pPoly->m_pVertices[i].m_xmf3Position);
            XMVECTOR vView = XMVector3TransformCoord(v, mtxWorldView);
            XMStoreFloat3(&viewVerts[i], vView);
        }

        // --- Sutherland-Hodgman clip against the near plane (view_z >= zNear) ---
        // A convex polygon with n vertices produces at most n+1 vertices after
        // clipping against one plane. clipped[16] is safe since inputs are <=16.
        XMFLOAT3 clipped[16];
        int nClipped = 0;
        for (int i = 0; i < nVertices; i++)
        {
            const int prev_i = (i == 0) ? (nVertices - 1) : (i - 1);
            const XMFLOAT3& prev = viewVerts[prev_i];
            const XMFLOAT3& curr = viewVerts[i];
            const bool prev_in = prev.z >= zNear;
            const bool curr_in = curr.z >= zNear;
            if (curr_in)
            {
                if (!prev_in && nClipped < 16)
                {
                    const float t = (zNear - prev.z) / (curr.z - prev.z);
                    clipped[nClipped].x = prev.x + t * (curr.x - prev.x);
                    clipped[nClipped].y = prev.y + t * (curr.y - prev.y);
                    clipped[nClipped].z = zNear;
                    ++nClipped;
                }
                if (nClipped < 16) clipped[nClipped++] = curr;
            }
            else if (prev_in && nClipped < 16)
            {
                const float t = (zNear - prev.z) / (curr.z - prev.z);
                clipped[nClipped].x = prev.x + t * (curr.x - prev.x);
                clipped[nClipped].y = prev.y + t * (curr.y - prev.y);
                clipped[nClipped].z = zNear;
                ++nClipped;
            }
        }
        if (nClipped < 3) continue;

        // --- Project clipped verts to screen space ---
        POINT pts[16];
        for (int i = 0; i < nClipped; i++)
        {
            XMVECTOR v = XMVectorSet(clipped[i].x, clipped[i].y, clipped[i].z, 1.0f);
            XMVECTOR vClip = XMVector4Transform(v, mtxProj);
            float w = XMVectorGetW(vClip);
            if (w < 1e-6f) w = 1e-6f;  // clip verts guarantee w > 0, this is just safety
            const float ndc_x = XMVectorGetX(vClip) / w;
            const float ndc_y = XMVectorGetY(vClip) / w;
            pts[i].x = (LONG)(+ndc_x * halfW + offX);
            pts[i].y = (LONG)(-ndc_y * halfH + offY);
        }

        ::Polygon(hDCFrameBuffer, pts, nClipped);
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