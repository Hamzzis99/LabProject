#include "stdafx.h"
#include "SceneNode.h"

CSceneNode::CSceneNode()
{
    m_xmf4x4Local = Matrix4x4::Identity();
    m_xmf4x4World = Matrix4x4::Identity();
}

CSceneNode::~CSceneNode()
{
    for (CSceneNode* pChild : m_Children)
    {
        if (pChild) delete pChild;
    }
    // Note: m_pObject lifetime is owned by CScene's flat object list,
    // so do NOT delete it here.
}

void CSceneNode::AddChild(CSceneNode* pChild)
{
    if (!pChild) return;
    pChild->SetParent(this);
    m_Children.push_back(pChild);
}

void CSceneNode::SetLocalPosition(float x, float y, float z)
{
    m_xmf4x4Local._41 = x;
    m_xmf4x4Local._42 = y;
    m_xmf4x4Local._43 = z;
}

void CSceneNode::UpdateWorldTransform()
{
    if (m_pParent)
    {
        m_xmf4x4World = Matrix4x4::Multiply(m_xmf4x4Local, m_pParent->m_xmf4x4World);
    }
    else
    {
        m_xmf4x4World = m_xmf4x4Local;
    }

    // Push world transform into the payload so rendering/picking stays simple
    if (m_pObject)
    {
        m_pObject->m_xmf4x4World = m_xmf4x4World;
    }

    for (CSceneNode* pChild : m_Children)
    {
        pChild->UpdateWorldTransform();
    }
}

void CSceneNode::Animate(float fElapsedTime)
{
    if (m_pObject && m_pObject->m_bActive)
    {
        m_pObject->Animate(fElapsedTime);
    }
    for (CSceneNode* pChild : m_Children)
    {
        pChild->Animate(fElapsedTime);
    }
}

void CSceneNode::CollectRenderable(std::vector<CSceneNode*>& out)
{
    if (m_pObject && m_pObject->m_bActive)
    {
        out.push_back(this);
    }
    for (CSceneNode* pChild : m_Children)
    {
        pChild->CollectRenderable(out);
    }
}