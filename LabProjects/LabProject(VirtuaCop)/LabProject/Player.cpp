#include "stdafx.h"
#include "Player.h"

CPlayer::CPlayer()
{
    m_pCamera = new CCamera();
    m_pCamera->GenerateProjectionMatrix(1.01f, 5000.0f, 60.0f);
    m_pCamera->SetViewport(0, 0, CLIENT_WIDTH, CLIENT_HEIGHT, 0.0f, 1.0f);
}

CPlayer::~CPlayer()
{
    if (m_pCamera) delete m_pCamera;
}

void CPlayer::Setup(XMFLOAT3 xmf3Position, XMFLOAT3 xmf3LookAt, XMFLOAT3 xmf3Up)
{
    m_xmf3Position = xmf3Position;
    m_xmf3LookAt   = xmf3LookAt;
    m_xmf3Up       = xmf3Up;

    // Position camera, aim it, and bake the view/view-proj matrices once.
    m_pCamera->SetLookAt(m_xmf3Position, m_xmf3LookAt, m_xmf3Up);
    m_pCamera->GenerateViewMatrix();
}