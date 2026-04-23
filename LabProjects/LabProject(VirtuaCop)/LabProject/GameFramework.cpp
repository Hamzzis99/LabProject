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

void CGameFramework::UpdateClearSequence(float fElapsedTime)
{
    switch (m_gameState)
    {
    case GameState::Playing:
    {
        // Trigger when the last cube has fully finished its Exploding -> Dead
        // transition (m_bActive becomes false only at that point, which is
        // what RemainingEnemyCount counts against).
        if (m_pScene->RemainingEnemyCount() == 0)
        {
            m_gameState   = GameState::ClearWalking;
            m_fClearTimer = 0.0f;

            // Snapshot current camera pose.
            m_xmf3ClearStartPos    = m_pPlayer->m_pCamera->m_xmf3Position;
            m_xmf3ClearStartLookAt = XMFLOAT3(0.0f, 0.0f, 0.0f); // camera was aimed at origin

            // Walk direction = unit vector from camera to its look-at point.
            XMVECTOR vDir = XMVectorSubtract(XMLoadFloat3(&m_xmf3ClearStartLookAt),
                                             XMLoadFloat3(&m_xmf3ClearStartPos));
            vDir = XMVector3Normalize(vDir);
            XMStoreFloat3(&m_xmf3ClearWalkDir, vDir);
        }
        break;
    }

    case GameState::ClearWalking:
    {
        m_fClearTimer += fElapsedTime;
        if (m_fClearTimer >= CLEAR_WALK_DURATION)
        {
            // Transition to ClearTurning. Freeze current pose and compute
            // the yaw-rotated look-at target the turn will lerp toward.
            m_gameState   = GameState::ClearTurning;
            m_fClearTimer = 0.0f;

            m_xmf3ClearStartPos    = m_pPlayer->m_pCamera->m_xmf3Position;
            m_xmf3ClearStartLookAt = XMFLOAT3(0.0f, 0.0f, 0.0f); // still aimed at origin here

            XMVECTOR vPos    = XMLoadFloat3(&m_xmf3ClearStartPos);
            XMVECTOR vLook   = XMLoadFloat3(&m_xmf3ClearStartLookAt);
            XMVECTOR vDir    = XMVector3Normalize(XMVectorSubtract(vLook, vPos));
            XMMATRIX mYaw    = XMMatrixRotationY(XMConvertToRadians(CLEAR_TURN_YAW_DEG));
            XMVECTOR vNewDir = XMVector3Normalize(XMVector3TransformNormal(vDir, mYaw));
            // Pick any point along the new direction; only the direction matters
            // for the view matrix, distance just makes the number numerically nice.
            XMVECTOR vTarget = XMVectorAdd(vPos, XMVectorScale(vNewDir, 20.0f));
            XMStoreFloat3(&m_xmf3ClearTurnLookAt, vTarget);
        }
        else
        {
            float t = m_fClearTimer / CLEAR_WALK_DURATION;  // 0..1

            // Linear forward walk from the snapshot.
            XMFLOAT3 pos = m_xmf3ClearStartPos;
            pos.x += m_xmf3ClearWalkDir.x * CLEAR_WALK_DISTANCE * t;
            pos.y += m_xmf3ClearWalkDir.y * CLEAR_WALK_DISTANCE * t;
            pos.z += m_xmf3ClearWalkDir.z * CLEAR_WALK_DISTANCE * t;

            // Footstep bob: small vertical sine on top of the walk.
            pos.y += ::sinf(m_fClearTimer * CLEAR_WALK_BOB_FREQ) * CLEAR_WALK_BOB_AMP;

            XMFLOAT3 up(0.0f, 1.0f, 0.0f);
            m_pPlayer->m_pCamera->SetLookAt(pos, m_xmf3ClearStartLookAt, up);
            m_pPlayer->m_pCamera->GenerateViewMatrix();
        }
        break;
    }

    case GameState::ClearTurning:
    {
        m_fClearTimer += fElapsedTime;
        if (m_fClearTimer >= CLEAR_TURN_DURATION)
        {
            m_gameState = GameState::Done;

            XMFLOAT3 up(0.0f, 1.0f, 0.0f);
            m_pPlayer->m_pCamera->SetLookAt(m_xmf3ClearStartPos, m_xmf3ClearTurnLookAt, up);
            m_pPlayer->m_pCamera->GenerateViewMatrix();
        }
        else
        {
            float t = m_fClearTimer / CLEAR_TURN_DURATION;
            float s = t * t * (3.0f - 2.0f * t);  // smoothstep for ease-in-out

            // Lerp the look-at point from its starting value toward the target.
            XMFLOAT3 la;
            la.x = m_xmf3ClearStartLookAt.x + (m_xmf3ClearTurnLookAt.x - m_xmf3ClearStartLookAt.x) * s;
            la.y = m_xmf3ClearStartLookAt.y + (m_xmf3ClearTurnLookAt.y - m_xmf3ClearStartLookAt.y) * s;
            la.z = m_xmf3ClearStartLookAt.z + (m_xmf3ClearTurnLookAt.z - m_xmf3ClearStartLookAt.z) * s;

            XMFLOAT3 up(0.0f, 1.0f, 0.0f);
            m_pPlayer->m_pCamera->SetLookAt(m_xmf3ClearStartPos, la, up);
            m_pPlayer->m_pCamera->GenerateViewMatrix();
        }
        break;
    }

    case GameState::Done:
        // Nothing to do; camera stays put at the final pose.
        break;
    }
}

void CGameFramework::FrameAdvance()
{
    if (!m_bActive) return;

    m_GameTimer.Tick(0.0f);
    const float fElapsed = m_GameTimer.GetTimeElapsed();

    // --- simulation ---
    m_pScene->Animate(fElapsed);
    UpdateClearSequence(fElapsed);

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

    // Title bar: FPS plus current state.
    _TCHAR szTitle[96];
    _TCHAR szFps[32] = _T("");
    m_GameTimer.GetFrameRate(szFps, 32);
    switch (m_gameState)
    {
    case GameState::Playing:
        _stprintf_s(szTitle, _T("VirtuaCop - Enemies left: %d  [%s"),
                    m_pScene->RemainingEnemyCount(), szFps);
        break;
    case GameState::ClearWalking:
        _stprintf_s(szTitle, _T("VirtuaCop - Cleared! (walking)  [%s"), szFps);
        break;
    case GameState::ClearTurning:
        _stprintf_s(szTitle, _T("VirtuaCop - Cleared! (turning)  [%s"), szFps);
        break;
    case GameState::Done:
        _stprintf_s(szTitle, _T("VirtuaCop - Cleared!  [%s"), szFps);
        break;
    }
    ::SetWindowText(m_hWnd, szTitle);
}