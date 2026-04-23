#pragma once

#include "Camera.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CVertex
{
public:
    CVertex() { }
    CVertex(float x, float y, float z) { m_xmf3Position = XMFLOAT3(x, y, z); }

    XMFLOAT3                    m_xmf3Position;
};

class CPolygon
{
public:
    CPolygon(int nVertices);
    virtual ~CPolygon();

    int                         m_nVertices;
    CVertex*                    m_pVertices;

    void SetVertex(int nIndex, CVertex& vertex);
};

class CMesh
{
public:
    CMesh(int nPolygons);
    virtual ~CMesh();

private:
    int                         m_nReferences;

protected:
    int                         m_nPolygons;
    CPolygon**                  m_ppPolygons;

public:
    BoundingOrientedBox         m_xmOOBB;

public:
    void AddRef() { m_nReferences++; }
    void Release() { m_nReferences--; if (m_nReferences <= 0) delete this; }

public:
    void SetPolygon(int nIndex, CPolygon* pPolygon);

    // Filled rendering with Win32 GDI (Polygon).
    // Performs per-face backface culling (world-space normal vs camera).
    virtual void Render(HDC hDCFrameBuffer, XMFLOAT4X4& xmf4x4World, CCamera* pCamera);

    // Ray-mesh intersection for picking (quick OOBB then per-triangle).
    BOOL RayIntersectionByTriangle(XMVECTOR& xmRayOrigin, XMVECTOR& xmRayDirection, XMVECTOR v0, XMVECTOR v1, XMVECTOR v2, float* pfNearHitDistance);
    int  CheckRayIntersection(XMVECTOR& xmvPickRayOrigin, XMVECTOR& xmvPickRayDirection, float* pfHitDistance);

    void SetOOBB(XMFLOAT3& xmCenter, XMFLOAT3& xmExtents, XMFLOAT4& xmOrientation) { m_xmOOBB = BoundingOrientedBox(xmCenter, xmExtents, xmOrientation); }
};

class CCubeMesh : public CMesh
{
public:
    CCubeMesh(float fWidth = 4.0f, float fHeight = 4.0f, float fDepth = 4.0f);
    virtual ~CCubeMesh();
};