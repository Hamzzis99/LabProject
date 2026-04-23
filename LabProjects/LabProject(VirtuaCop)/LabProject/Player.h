#pragma once

#include "GameObject.h"

// In VirtuaCop mode the "player" does not move or rotate. It only exists
// as a thin holder for the fixed camera used for rendering and picking.
class CPlayer
{
public:
    CPlayer();
    virtual ~CPlayer();

    CCamera* m_pCamera = NULL;

    XMFLOAT3 m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
    XMFLOAT3 m_xmf3LookAt   = XMFLOAT3(0.0f, 0.0f, 1.0f);
    XMFLOAT3 m_xmf3Up       = XMFLOAT3(0.0f, 1.0f, 0.0f);

    // One-shot setup: place camera, build view/projection, fix viewport.
    void Setup(XMFLOAT3 xmf3Position, XMFLOAT3 xmf3LookAt, XMFLOAT3 xmf3Up);
};