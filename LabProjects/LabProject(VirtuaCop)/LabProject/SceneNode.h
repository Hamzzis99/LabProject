#pragma once

#include "GameObject.h"
#include <vector>

// Simple scene graph node (a "tree of transforms")
// - Each node carries a local transform relative to its parent
// - Optionally holds a CGameObject payload
// - Parents propagate world transform down to children recursively
// - Root node has no parent and typically no payload
class CSceneNode
{
public:
    CSceneNode();
    virtual ~CSceneNode();

    void SetParent(CSceneNode* pParent) { m_pParent = pParent; }
    void AddChild(CSceneNode* pChild);

    void SetLocalTransform(const XMFLOAT4X4& xmf4x4Local) { m_xmf4x4Local = xmf4x4Local; }
    const XMFLOAT4X4& GetLocalTransform() const { return m_xmf4x4Local; }
    const XMFLOAT4X4& GetWorldTransform() const { return m_xmf4x4World; }

    void SetLocalPosition(float x, float y, float z);

    void SetObject(CGameObject* pObject) { m_pObject = pObject; }
    CGameObject* GetObject() const { return m_pObject; }

    void UpdateWorldTransform();
    void Animate(float fElapsedTime);
    void CollectRenderable(std::vector<CSceneNode*>& out);

    XMFLOAT4X4                  m_xmf4x4Local;
    XMFLOAT4X4                  m_xmf4x4World;
    CSceneNode*                 m_pParent = nullptr;
    std::vector<CSceneNode*>    m_Children;
    CGameObject*                m_pObject = nullptr;
};