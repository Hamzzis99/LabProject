#pragma once

#include "GameObject.h"
#include "Player.h"
#include "SceneNode.h"
#include <vector>

// Holds:
//   - the shared cube mesh used by every enemy (ref-counted)
//   - a flat owning list of CGameObject* (lifetime control)
//   - a scene-graph root with one CSceneNode per enemy as direct children
//     (the "simple tree" data structure requested in the spec)
class CScene
{
public:
    CScene();
    virtual ~CScene();

    CPlayer*                        m_pPlayer = NULL;

    CSceneNode*                     m_pRootNode = NULL;
    std::vector<CGameObject*>       m_AllObjects;   // owning: enemies only
    std::vector<CSceneNode*>        m_EnemyNodes;   // non-owning, points into tree

    // Boundary walls: visual only, not pickable, not counted as enemies.
    // Stored in a separate owning list so they can be deleted on shutdown.
    std::vector<CGameObject*>       m_WallObjects;

    virtual void BuildObjects();
    virtual void ReleaseObjects();

    virtual void Animate(float fElapsedTime);
    virtual void Render(HDC hDCFrameBuffer, CCamera* pCamera);

    // Returns the nearest enemy hit by the ray through the given screen pixel.
    // NULL if the ray misses every active enemy.
    CGameObject* PickObjectPointedByCursor(int xClient, int yClient, CCamera* pCamera);

    // Number of still-active enemies (for the title-bar "Cleared" notice).
    int RemainingEnemyCount() const;

    // Per-wave remaining counts. Wave 1 = first 5 entries of m_AllObjects
    // (vertical corridor), Wave 2 = next 5 entries (east corridor). Used by
    // the game state machine to decide when to trigger the walk+turn (wave
    // 1 cleared) and when the whole stage is done (both waves cleared).
    static constexpr int WAVE_SIZE = 5;
    int WaveRemaining(int nWave) const;
};