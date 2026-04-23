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

// Top-level game state. After every enemy is Dead we play a short "clear"
// camera move: walk forward, then turn. Classic light-gun shooter cadence.
enum class GameState : int
{
    Playing,       // normal play
    ClearWalking,  // cam translates forward with a small vertical bob
    ClearTurning,  // cam stops; yaws its look-at direction around Y
    Done           // frozen at final pose
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

    // ---- Stage-clear camera sequence ----------------------------------
    GameState                   m_gameState    = GameState::Playing;
    float                       m_fClearTimer  = 0.0f;

    // Captured at the moment each phase begins.
    XMFLOAT3                    m_xmf3ClearStartPos;     // cam pos
    XMFLOAT3                    m_xmf3ClearStartLookAt;  // cam look-at point
    XMFLOAT3                    m_xmf3ClearWalkDir;      // unit forward vector
    XMFLOAT3                    m_xmf3ClearTurnLookAt;   // look-at at end of turn

    // Tunables (all exposed here so the feel is easy to tweak).
    static constexpr float      CLEAR_WALK_DURATION = 2.0f;   // seconds walking
    static constexpr float      CLEAR_WALK_DISTANCE = 18.0f;  // units forward
    static constexpr float      CLEAR_WALK_BOB_AMP  = 0.35f;  // vertical bob amplitude
    static constexpr float      CLEAR_WALK_BOB_FREQ = 7.0f;   // rad/s (footstep rate)
    static constexpr float      CLEAR_TURN_DURATION = 1.5f;   // seconds turning
    static constexpr float      CLEAR_TURN_YAW_DEG  = 70.0f;  // right-turn angle

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

    // Advances the stage-clear camera sequence (walk -> turn -> done).
    void UpdateClearSequence(float fElapsedTime);

    _TCHAR                      m_pszFrameRate[64];
};