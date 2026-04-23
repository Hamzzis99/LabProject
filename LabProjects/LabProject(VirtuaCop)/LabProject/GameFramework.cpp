//-----------------------------------------------------------------------------
// File: GameFramework.cpp (VirtuaCop build)
//
// Game loop is the classic one from the tutorial but stripped to the minimum:
//   Tick -> Animate scene -> Clear frame buffer -> Render scene
//        -> Draw hit-mark overlay -> Present -> Update title bar
//
// Input:
//   Left mouse click  -> ray-pick an enemy; flash + schedule removal; mark overlay
//   ESC               -> quit (handled in LabProject.cpp WndProc)
//
// The player never moves. The camera is placed in BuildObjects() and never
// updated after that.
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "GameFramework.h"

CGameFramework::CGameFramework()
{
    _tcscpy_s(m_pszFrameRate, _T("VirtuaCop ("));
}

CGameFramework::~CGameFramework()
{
}

bool CGameFramework::OnCreate(HINSTANCE hInstance, HWND hMainWnd)
{
    ::srand(::timeGetTime());

    m_hInstance = hInstance;
    m_hWnd      = hMainWnd;

    BuildFrameBuffer();
    BuildObjects();
    return true;
}

void CGameFramework::BuildFrameBuffer()
{
    HDC hDC = ::GetDC(m_hWnd);

    RECT rcClient;
    ::GetClientRect(m_hWnd, &rcClient);

    m_hDCFrameBuffer     = ::CreateCompatibleDC(hDC);
    m_hBitmapFrameBuffer = ::CreateCompatibleBitmap(hDC, (rcClient.right - rcClient.left) + 1, (rcClient.bottom - rcClient.top) + 1);
    ::SelectObject(m_hDCFrameBuffer, m_hBitmapFrameBuffer);

    ::ReleaseDC(m_hWnd, hDC);
    ::SetBkMode(m_hDCFrameBuffer, TRANSPARENT);
}

void CGameFramework::ClearFrameBuffer(DWORD dwColor)
{
    HBRUSH hBrush    = ::CreateSolidBrush(dwColor);
    HBRUSH hOldBrush = (HBRUSH)::SelectObject(m_hDCFrameBuffer, hBrush);
    ::Rectangle(m_hDCFrameBuffer,
        (int)m_pPlayer->m_pCamera->m_d3dViewport.TopLeftX,
        (int)m_pPlayer->m_pCamera->m_d3dViewport.TopLeftY,
        (int)m_pPlayer->m_pCamera->m_d3dViewport.Width,
        (int)m_pPlayer->m_pCamera->m_d3dViewport.Height);
    ::SelectObject(m_hDCFrameBuffer, hOldBrush);
    ::DeleteObject(hBrush);
}

void CGameFramework::PresentFrameBuffer()
{
    HDC hDC = ::GetDC(m_hWnd);
    ::BitBlt(hDC,
        (int)m_pPlayer->m_pCamera->m_d3dViewport.TopLeftX,
        (int)m_pPlayer->m_pCamera->m_d3dViewport.TopLeftY,
        (int)m_pPlayer->m_pCamera->m_d3dViewport.Width,
        (int)m_pPlayer->m_pCamera->m_d3dViewport.Height,
        m_hDCFrameBuffer,
        (int)m_pPlayer->m_pCamera->m_d3dViewport.TopLeftX,
        (int)m_pPlayer->m_pCamera->m_d3dViewport.TopLeftY,
        SRCCOPY);
    ::ReleaseDC(m_hWnd, hDC);
}

void CGameFramework::OnProcessingMouseMessage(HWND /*hWnd*/, UINT nMessageID, WPARAM /*wParam*/, LPARAM lParam)
{
    if (nMessageID != WM_LBUTTONDOWN) return;

    const int xClient = LOWORD(lParam);
    const int yClient = HIWORD(lParam);

    // Hitscan: ray through (xClient, yClient) into the scene.
    CGameObject* pHit = m_pScene->PickObjectPointedByCursor(xClient, yClient, m_pPlayer->m_pCamera);

    HitMark mark;
    mark.x          = xClient;
    mark.y          = yClient;
    mark.fRemaining = HIT_MARK_DURATION;
    mark.bDidHit    = (pHit != NULL);
    m_HitMarks.push_back(mark);

    if (pHit)
    {
        // Start the enemy's flash-then-disappear state.
        pHit->OnHit();
    }
}

void CGameFramework::OnProcessingKeyboardMessage(HWND /*hWnd*/, UINT nMessageID, WPARAM wParam, LPARAM /*lParam*/)
{
    // No keyboard-driven gameplay: the player is stationary and only shoots
    // via mouse click. Only ESC is handled (quit).
    if (nMessageID == WM_KEYDOWN && wParam == VK_ESCAPE)
    {
        ::PostQuitMessage(0);
    }
}

LRESULT CALLBACK CGameFramework::OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
    switch (nMessageID)
    {
        case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_INACTIVE) m_GameTimer.Stop();
            else                               m_GameTimer.Start();
            break;
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MOUSEMOVE:
            OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam);
            break;
        case WM_KEYDOWN:
        case WM_KEYUP:
            OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
            break;
        default:
            break;
    }
    return 0;
}

void CGameFramework::BuildObjects()
{
    // Fixed camera: back a bit and slightly elevated, looking at the origin.
    m_pPlayer = new CPlayer();
    m_pPlayer->Setup(
        /*position*/ XMFLOAT3(0.0f, 6.0f, -40.0f),
        /*lookAt  */ XMFLOAT3(0.0f, 0.0f,  0.0f),
        /*up      */ XMFLOAT3(0.0f, 1.0f,  0.0f));

    m_pScene = new CScene();
    m_pScene->BuildObjects();
    m_pScene->m_pPlayer = m_pPlayer;
}

void CGameFramework::ReleaseObjects()
{
    if (m_pScene)  { m_pScene->ReleaseObjects(); delete m_pScene;  m_pScene  = NULL; }
    if (m_pPlayer) { delete m_pPlayer;                             m_pPlayer = NULL; }
}

void CGameFramework::OnDestroy()
{
    ReleaseObjects();
    if (m_hBitmapFrameBuffer) ::DeleteObject(m_hBitmapFrameBuffer);
    if (m_hDCFrameBuffer)     ::DeleteDC(m_hDCFrameBuffer);
    if (m_hWnd)               ::DestroyWindow(m_hWnd);
}

void CGameFramework::RenderHitMarks(HDC hDCFrameBuffer)
{
    // A HIT mark is a filled cross (red); a MISS mark is a thin ring (gray).
    for (const HitMark& m : m_HitMarks)
    {
        if (m.bDidHit)
        {
            HPEN hPen    = ::CreatePen(PS_SOLID, 3, RGB(255, 0, 0));
            HPEN hOldPen = (HPEN)::SelectObject(hDCFrameBuffer, hPen);
            const int r = 10;
            ::MoveToEx(hDCFrameBuffer, m.x - r, m.y - r, NULL);
            ::LineTo  (hDCFrameBuffer, m.x + r, m.y + r);
            ::MoveToEx(hDCFrameBuffer, m.x + r, m.y - r, NULL);
            ::LineTo  (hDCFrameBuffer, m.x - r, m.y + r);
            ::SelectObject(hDCFrameBuffer, hOldPen);
            ::DeleteObject(hPen);
        }
        else
        {
            HPEN   hPen      = ::CreatePen(PS_SOLID, 2, RGB(160, 160, 160));
            HPEN   hOldPen   = (HPEN)::SelectObject(hDCFrameBuffer, hPen);
            HBRUSH hOldBrush = (HBRUSH)::SelectObject(hDCFrameBuffer, ::GetStockObject(NULL_BRUSH));
            const int r = 8;
            ::Ellipse(hDCFrameBuffer, m.x - r, m.y - r, m.x + r, m.y + r);
            ::SelectObject(hDCFrameBuffer, hOldBrush);
            ::SelectObject(hDCFrameBuffer, hOldPen);
            ::DeleteObject(hPen);
        }
    }
}

void CGameFramework::FrameAdvance()
{
    if (!m_bActive) return;

    m_GameTimer.Tick(0.0f);
    const float fElapsed = m_GameTimer.GetTimeElapsed();

    // --- simulation ---
    m_pScene->Animate(fElapsed);

    // Tick hit-mark lifetimes and remove expired entries.
    for (auto it = m_HitMarks.begin(); it != m_HitMarks.end(); )
    {
        it->fRemaining -= fElapsed;
        if (it->fRemaining <= 0.0f) it = m_HitMarks.erase(it);
        else                        ++it;
    }

    // --- rendering ---
    ClearFrameBuffer(RGB(245, 245, 245));
    m_pScene->Render(m_hDCFrameBuffer, m_pPlayer->m_pCamera);
    RenderHitMarks(m_hDCFrameBuffer);
    PresentFrameBuffer();

    // Title bar: FPS plus remaining enemy count or "Cleared".
    _TCHAR szTitle[96];
    const int remaining = m_pScene->RemainingEnemyCount();
    if (remaining > 0)
    {
        _TCHAR szFps[32] = _T("");
        m_GameTimer.GetFrameRate(szFps, 32);
        _stprintf_s(szTitle, _T("VirtuaCop - Enemies left: %d  [%s"), remaining, szFps);
    }
    else
    {
        _TCHAR szFps[32] = _T("");
        m_GameTimer.GetFrameRate(szFps, 32);
        _stprintf_s(szTitle, _T("VirtuaCop - Cleared!  [%s"), szFps);
    }
    ::SetWindowText(m_hWnd, szTitle);
}