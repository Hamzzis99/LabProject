#pragma once

#include "Timer.h"
#include "Scene.h"
#include "Player.h"
#include <vector>

// A single live hit mark: a short-lived 2D overlay drawn on top of the
// 3D scene at the click position. When m_fRemaining reaches 0 it's removed.
struct HitMark
{
    int   x;
    int   y;
    float fRemaining;
    bool  bDidHit;      // true = a cube was hit (cross), false = miss (ring)
};

class CGameFramework
{
public:
    CGameFramework(void);
    ~CGameFramework(void);

    bool OnCreate(HINSTANCE hInstance, HWND hMainWnd);
    void OnDestroy();
    void FrameAdvance();

    void SetActive(bool bActive) { m_bActive = bActive; }

private:
    HINSTANCE                   m_hInstance = NULL;
    HWND                        m_hWnd      = NULL;

    bool                        m_bActive = true;

    CGameTimer                  m_GameTimer;

    HDC                         m_hDCFrameBuffer     = NULL;
    HBITMAP                     m_hBitmapFrameBuffer = NULL;

    CPlayer*                    m_pPlayer = NULL;
    CScene*                     m_pScene  = NULL;

    // Hit marks (2D overlay). Short-lived; size bounded naturally by the
    // mark duration times click rate.
    std::vector<HitMark>        m_HitMarks;
    static constexpr float      HIT_MARK_DURATION = 0.20f;

public:
    void BuildFrameBuffer();
    void ClearFrameBuffer(DWORD dwColor);
    void PresentFrameBuffer();

    void BuildObjects();
    void ReleaseObjects();

    void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
    void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

    // Renders active hit marks directly on the frame buffer DC.
    void RenderHitMarks(HDC hDCFrameBuffer);

    _TCHAR                      m_pszFrameRate[64];
};