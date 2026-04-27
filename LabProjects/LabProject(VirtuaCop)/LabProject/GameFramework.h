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

// Top-level game state. The stage is split into two waves separated by a
// short "clear" cutscene that walks the camera forward and turns it 90 deg
// right into the east corridor.
//
//   WaveOne       : first wave active in the vertical corridor. Clicking
//                   shoots at enemies 0..4. When all five are dead -> ClearWalking.
//   ClearWalking  : camera translates forward along its look direction
//                   with a small vertical footstep bob.
//   ClearTurning  : camera stops translating; yaws its look-at point 90 deg
//                   right using a smoothstep curve.
//   WaveTwo       : second wave active in the east corridor, now visible.
//                   Clicking shoots at enemies 5..9. When all five are dead
//                   -> Done.
//   Done          : both waves clear; nothing left to shoot.
enum class GameState : int
{
    WaveOne,
    ClearWalking,
    ClearTurning,
    WaveTwo,
    Done
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
    static constexpr float      HIT_MARK_HIT_DURATION  = 1.50f;
    static constexpr float      HIT_MARK_MISS_DURATION = 0.35f;

    // ---- Stage-clear camera sequence ----------------------------------
    GameState                   m_gameState    = GameState::WaveOne;
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
    static constexpr float      CLEAR_TURN_YAW_DEG  = 90.0f;  // right-turn angle (matches L-corridor corner)
    static constexpr float      DONE_EXIT_DELAY     = 3.0f;   // seconds after All Clear before quitting

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
