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
    // One-time: populate the 240 random sphere vectors and create the
    // shared 0.5-cube mesh used for all debris pieces.
    CGameObject::PrepareExplosion();

    m_pRootNode = new CSceneNode();

    // -----------------------------------------------------------------
    // 1) Walls first.
    //
    // Render order matters: the scene has no depth buffer and no painter's
    // sort, so whichever object is added last draws on top. Walls are the
    // background, enemies are the foreground. Adding walls to the tree
    // BEFORE enemies makes CollectRenderable yield walls first -> enemies
    // overdraw walls in the pixels where they overlap.
    //
    // Layout (top-down, +x right, +z forward; camera enters from the south
    // end of the vertical corridor, walks +z, turns 90 deg right at the
    // corner, ends up looking into the east corridor):
    //
    //              x=-18        x=+18                     x=+50
    //    z=+5      +-----VN-----+                            (north end, blocked)
    //              |            |
    //              |  wave 1    |
    //              |  (5 cubes) |
    //    z=-8      |            +---------EN-----------+     (east corridor top)
    //              |                                   |
    //              |  [camera walks this aisle]        HE    (east corridor end)
    //              |            wave 2 (5 cubes)       |
    //    z=-22     |            +---------ES-----------+     (east corridor bottom)
    //              |            |
    //              |  (still    |
    //              |  vertical  |
    //              |  corridor) |
    //    z=-40     +-----HS-----+                            (south end, camera's back)
    //              VL           VR (split into south + north pieces)
    //
    // User requested no rendering optimization -- each wall is its own thin
    // CCubeMesh and all walls sit in the scene graph, even ones the camera
    // never really sees (HS behind the camera, etc).
    struct WallSpec
    {
        float fW, fH, fD;   // mesh dimensions (x, y, z)
        float x,  y,  z;    // center position in world space
    };
    const WallSpec kWalls[] = {
        //   W      H      D        x       y       z      purpose
        {  0.8f,  14.0f,  45.0f,  -18.0f,  4.0f,  -17.5f },  // VL : left wall of vertical corridor
        {  0.8f,  14.0f,  18.0f,  +18.0f,  4.0f,  -31.0f },  // VR-south : right wall, south piece
        {  0.8f,  14.0f,  13.0f,  +18.0f,  4.0f,   -1.5f },  // VR-north : right wall, north piece (opening for east corridor sits between them)
        { 36.0f,  14.0f,   0.8f,    0.0f,  4.0f,   +5.0f },  // VN : vertical corridor north end
        { 32.0f,  14.0f,   0.8f,  +34.0f,  4.0f,   -8.0f },  // EN : east corridor top wall
        { 32.0f,  14.0f,   0.8f,  +34.0f,  4.0f,  -22.0f },  // ES : east corridor bottom wall
        {  0.8f,  14.0f,  14.0f,  +50.0f,  4.0f,  -15.0f },  // HE : east corridor end
        { 36.0f,  14.0f,   0.8f,    0.0f,  4.0f,  -40.0f },  // HS : south end, behind the camera (corridor feel)
    };
    constexpr int kWallCount = sizeof(kWalls) / sizeof(kWalls[0]);
    const COLORREF kWallColor = RGB(95, 95, 95);

    m_WallObjects.reserve(kWallCount);
    for (int i = 0; i < kWallCount; ++i)
    {
        // Each wall is its own mesh (different dimensions per wall).
        CCubeMesh* pWallMesh = new CCubeMesh(kWalls[i].fW, kWalls[i].fH, kWalls[i].fD);

        CGameObject* pWall = new CGameObject();
        pWall->SetMesh(pWallMesh);     // refcount: 0 -> 1
        pWall->SetColor(kWallColor);

        CSceneNode* pNode = new CSceneNode();
        pNode->SetObject(pWall);
        pNode->SetLocalPosition(kWalls[i].x, kWalls[i].y, kWalls[i].z);
        m_pRootNode->AddChild(pNode);

        m_WallObjects.push_back(pWall);
    }

    // -----------------------------------------------------------------
    // 2) Enemies second (so they draw on top of walls where they overlap).
    //
    // Exactly 10 cubes, split into two waves of 5:
    //   Wave 1 (m_AllObjects[0..4]): inside the vertical corridor,
    //     visible from the camera's initial pose. Shooting all 5 triggers
    //     the walk + turn cutscene.
    //   Wave 2 (m_AllObjects[5..9]): inside the east corridor, hidden
    //     behind the corner until the camera turns. Shooting all 5
    //     finishes the stage.
    //
    // Cube half-extent is 2. Vertical corridor interior is
    //   x in [-17.6, +17.6], y in [-3, +11], z in [-39.6, +4.6].
    // East corridor interior is
    //   x in [+18.4, +49.6], y in [-3, +11], z in [-21.6, -8.4].
    // All positions below are inside those boxes with >=2 units of margin.
    struct EnemySpec { float x, y, z; COLORREF color; };
    const EnemySpec kEnemies[] = {
        // ---- Wave 1 : vertical corridor (indices 0..4) ----
        {  -6.0f,  1.0f, -16.0f, RGB(255,   0,   0) },
        {  +6.0f,  1.0f, -16.0f, RGB(  0,   0, 255) },
        {  -4.0f,  5.0f, -20.0f, RGB(255,   0, 255) },
        {  +4.0f,  5.0f, -20.0f, RGB(255, 120,   0) },
        {   0.0f,  3.0f, -24.0f, RGB(  0, 220, 220) },
        // ---- Wave 2 : east corridor  (indices 5..9) ----
        { +25.0f,  0.0f, -12.0f, RGB(255,  64,  64) },
        { +30.0f,  0.0f, -18.0f, RGB(160,   0, 255) },
        { +35.0f,  5.0f, -14.0f, RGB(  0, 200,   0) },
        { +42.0f,  3.0f, -16.0f, RGB(128,   0, 255) },
        { +45.0f,  7.0f, -12.0f, RGB(255,   0, 128) },
    };
    constexpr int kEnemyCount = sizeof(kEnemies) / sizeof(kEnemies[0]);

    // Shared cube mesh for every enemy (all enemies are the same 4x4x4 cube,
    // just differently coloured). Refcount: 0 -> kEnemyCount after the loop.
    CCubeMesh* pEnemyMesh = new CCubeMesh(4.0f, 4.0f, 4.0f);

    m_AllObjects.reserve(kEnemyCount);
    m_EnemyNodes.reserve(kEnemyCount);
    for (int i = 0; i < kEnemyCount; ++i)
    {
        CGameObject* pObj = new CGameObject();
        pObj->SetMesh(pEnemyMesh);
        pObj->SetColor(kEnemies[i].color);

        CSceneNode* pNode = new CSceneNode();
        pNode->SetObject(pObj);
        pNode->SetLocalPosition(kEnemies[i].x, kEnemies[i].y, kEnemies[i].z);

        m_pRootNode->AddChild(pNode);

        m_AllObjects.push_back(pObj);
        m_EnemyNodes.push_back(pNode);
    }

    // Prime world transforms + OOBBs so picking works on frame 0.
    m_pRootNode->UpdateWorldTransform();
    for (CGameObject* pObj : m_WallObjects) pObj->UpdateBoundingBox();
    for (CGameObject* pObj : m_AllObjects)  pObj->UpdateBoundingBox();
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

    // Delete the boundary walls (also game objects, owned separately).
    for (CGameObject* pObj : m_WallObjects) delete pObj;
    m_WallObjects.clear();

    // Release the shared debris mesh (safe even if nothing ever exploded).
    CGameObject::ReleaseExplosion();
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
        if (!pObj->IsPickable()) continue;
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

int CScene::WaveRemaining(int nWave) const
{
    // nWave is 1 or 2. Wave 1 occupies m_AllObjects[0..WAVE_SIZE-1]; wave 2
    // occupies m_AllObjects[WAVE_SIZE..2*WAVE_SIZE-1]. Defensive clamp in
    // case the enemy list is shorter than expected.
    const int start = (nWave == 1) ? 0 : WAVE_SIZE;
    const int end   = (nWave == 1) ? WAVE_SIZE : 2 * WAVE_SIZE;
    const int limit = static_cast<int>(m_AllObjects.size());
    int n = 0;
    for (int i = start; i < end && i < limit; ++i)
        if (m_AllObjects[i]->m_bActive) ++n;
    return n;
}

void CScene::Animate(float fElapsedTime)
{
    // Per-node animation (ticks each enemy's hit state machine + spin matrix).
    // NOTE: we intentionally do NOT call UpdateWorldTransform() here. The
    // scene is static and the root's local transforms never change after
    // BuildObjects. Calling it every frame would overwrite the per-object
    // spin matrix written by Animate, which would visually freeze the spin.
    if (m_pRootNode) m_pRootNode->Animate(fElapsedTime);
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