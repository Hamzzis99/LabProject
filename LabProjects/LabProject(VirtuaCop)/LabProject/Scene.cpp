#include "stdafx.h"
#include "Scene.h"

CScene::CScene()
{
}

CScene::~CScene()
{
}

void CScene::BuildObjects()
{
    // Shared cube mesh; every enemy is the same colour-swapped cube.
    CCubeMesh* pCubeMesh = new CCubeMesh(4.0f, 4.0f, 4.0f);

    // Hard-coded enemy layout: 10 cubes placed in the scene at fixed points.
    // (Single-project, hardcoding is fine per the spec.)
    struct EnemySpec { float x, y, z; COLORREF color; };
    const EnemySpec kEnemies[] = {
        { -13.5f,  0.0f, -14.0f, RGB(255,   0,   0) },
        { +13.5f,  0.0f, -14.0f, RGB(  0,   0, 255) },
        {   0.0f, +5.0f,  20.0f, RGB(  0, 200,   0) },
        {   0.0f,  0.0f,   0.0f, RGB(  0, 220, 220) },
        {  10.0f,  0.0f,   0.0f, RGB(160,   0, 255) },
        { -10.0f,  0.0f, -10.0f, RGB(255,   0, 255) },
        { -10.0f, 10.0f, -10.0f, RGB(255, 120,   0) },
        { -10.0f, 10.0f, -20.0f, RGB(255,   0, 128) },
        { -12.0f,  8.0f,   5.0f, RGB(128,   0, 255) },
        { +15.0f, 10.0f,   0.0f, RGB(255,  64,  64) },
    };
    constexpr int kEnemyCount = sizeof(kEnemies) / sizeof(kEnemies[0]);

    m_pRootNode = new CSceneNode();
    m_AllObjects.reserve(kEnemyCount);
    m_EnemyNodes.reserve(kEnemyCount);

    for (int i = 0; i < kEnemyCount; ++i)
    {
        CGameObject* pObj = new CGameObject();
        pObj->SetMesh(pCubeMesh);
        pObj->SetColor(kEnemies[i].color);

        CSceneNode* pNode = new CSceneNode();
        pNode->SetObject(pObj);
        pNode->SetLocalPosition(kEnemies[i].x, kEnemies[i].y, kEnemies[i].z);

        m_pRootNode->AddChild(pNode);   // scene graph: depth-2 tree

        m_AllObjects.push_back(pObj);
        m_EnemyNodes.push_back(pNode);
    }

    // Prime world transforms + OOBBs so picking works on frame 0.
    m_pRootNode->UpdateWorldTransform();
    for (CGameObject* pObj : m_AllObjects) pObj->UpdateBoundingBox();
}

void CScene::ReleaseObjects()
{
    // Delete the tree first (it owns the CSceneNodes but NOT the objects).
    if (m_pRootNode)
    {
        delete m_pRootNode;
        m_pRootNode = NULL;
    }
    m_EnemyNodes.clear();

    // Then delete the flat list of game objects.
    for (CGameObject* pObj : m_AllObjects) delete pObj;
    m_AllObjects.clear();
}

CGameObject* CScene::PickObjectPointedByCursor(int xClient, int yClient, CCamera* pCamera)
{
    // Screen pixel -> near-plane point in camera (view) space.
    XMFLOAT3 xmf3PickPosition;
    xmf3PickPosition.x = (((2.0f * xClient) / pCamera->m_d3dViewport.Width) - 1.0f) / pCamera->m_xmf4x4Projection._11;
    xmf3PickPosition.y = -(((2.0f * yClient) / pCamera->m_d3dViewport.Height) - 1.0f) / pCamera->m_xmf4x4Projection._22;
    xmf3PickPosition.z = 1.0f;

    XMVECTOR xmvPickPosition = XMLoadFloat3(&xmf3PickPosition);
    XMMATRIX xmmtxView       = XMLoadFloat4x4(&pCamera->m_xmf4x4View);

    float        fNearest       = FLT_MAX;
    CGameObject* pNearestObject = NULL;

    for (CGameObject* pObj : m_AllObjects)
    {
        if (!pObj->m_bActive) continue;
        float fHitDistance = FLT_MAX;
        int nIntersected = pObj->PickObjectByRayIntersection(xmvPickPosition, xmmtxView, &fHitDistance);
        if ((nIntersected > 0) && (fHitDistance < fNearest))
        {
            fNearest       = fHitDistance;
            pNearestObject = pObj;
        }
    }
    return pNearestObject;
}

int CScene::RemainingEnemyCount() const
{
    int n = 0;
    for (CGameObject* pObj : m_AllObjects) if (pObj->m_bActive) ++n;
    return n;
}

void CScene::Animate(float fElapsedTime)
{
    // Per-node animation (ticks each enemy's hit-flash timer).
    if (m_pRootNode) m_pRootNode->Animate(fElapsedTime);

    // Nothing below needs recomputing since nothing is moving,
    // but doing it once per frame keeps picking safe if we later add motion.
    if (m_pRootNode) m_pRootNode->UpdateWorldTransform();
}

void CScene::Render(HDC hDCFrameBuffer, CCamera* pCamera)
{
    // Walk the tree, gather every active node, then render.
    // (No inter-object depth sort needed: the enemies are placed so that
    // backface culling per-cube alone gives correct results.)
    std::vector<CSceneNode*> renderList;
    renderList.reserve(m_EnemyNodes.size());
    if (m_pRootNode) m_pRootNode->CollectRenderable(renderList);

    for (CSceneNode* pNode : renderList)
    {
        pNode->GetObject()->Render(hDCFrameBuffer, pCamera);
    }
}