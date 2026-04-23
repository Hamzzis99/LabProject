# LabProject — DirectXMath Picking & Culling

> 본 문서는 `F:\D3d12\LabProject(DirectXMath-PickingCulling)` 프로젝트 소스에서
> **직접 관측된 내용만** 기록한다.
> 외부 검색·추측·일반 그래픽스 지식은 포함하지 않는다.
> 코드에 없는 동작·의도·용어는 기술하지 않는다.

---

## 1. 프로젝트 개요 (코드로부터의 사실)

- 프로젝트 폴더: `LabProject(DirectXMath-PickingCulling)/LabProject/`
- `.vcxproj`의 `RootNamespace`: `LabProject`
- Win32 데스크톱 응용 프로그램 (`SubSystem=Windows`, 진입점 `_tWinMain`)
- **그리기 출력은 Win32 GDI**로 수행 (`HDC`, `CreateCompatibleDC`, `CreateCompatibleBitmap`, `BitBlt`, `CreatePen`, `MoveToEx`, `LineTo`, `Rectangle`)
- 렌더링 방식은 **와이어프레임** — `CMesh::Render`에서 정점을 변환 후 `MoveToEx + LineTo`로 간선을 그림
- 수학 연산은 **DirectXMath** 사용 (`XMFLOAT3`, `XMFLOAT4X4`, `XMMATRIX`, `XMVECTOR`, `XMMatrixLookAtLH`, `XMMatrixPerspectiveFovLH` 등)
- 충돌/픽킹 연산은 **DirectXCollision** 사용 (`BoundingOrientedBox`, `BoundingFrustum`, `TriangleTests::Intersects`)
- `D3D12_VIEWPORT` 구조체를 카메라의 뷰포트 필드 타입으로 사용 (`d3d12.lib` 링크됨). `d3d12.h`는 `stdafx.h`에서 include되지만, D3D12 디바이스/커맨드/리소스 API는 **코드 내 미관측**.
- 더블 버퍼링: 오프스크린 `HBITMAP`에 그린 뒤 `BitBlt`으로 윈도우 DC 전송

---

## 2. 빌드 구성 (`LabProject/LabProject.vcxproj`)

- ToolsVersion: `15.0`
- 구성: `Debug|Win32`, `Debug|x64`, `Release|Win32`, `Release|x64`
- `PlatformToolset`: `v142` (모든 구성)
- `WindowsTargetPlatformVersion`: `10.0`
- `CharacterSet`: `Unicode`
- `ConfigurationType`: `Application`
- `SubSystem`: `Windows`
- Debug|Win32, Release|Win32: `TargetMachine = MachineX86`
- `RuntimeLibrary`: Debug → `MultiThreadedDebugDLL`, Release → `MultiThreadedDLL`
- PCH: `stdafx.cpp`는 모든 구성에서 `Create`, 나머지는 기본(`Use`)
- 링크 라이브러리(관측 범위):
  - Debug|Win32, Debug|x64, Release|Win32: `winmm.lib` (명시)
  - Release|x64: `kernel32;user32;gdi32;winspool;comdlg32;advapi32;shell32;ole32;oleaut32;uuid;odbc32;odbccp32;winmm`
  - `stdafx.h`에 `#pragma comment(lib, "d3d12.lib")`
- 전처리 정의: `WIN32;_DEBUG;_WINDOWS` / `WIN32;NDEBUG;_WINDOWS`
- `CodeAnalysisRuleSet = AllRules.ruleset` (모든 구성)
- `ProjectGuid`: `{EE18299B-05F9-4236-8D11-55BEA6E6E8FE}`

> 솔루션 파일(`*.sln`)은 경로 직접 조회로 발견되지 않음. 존재 여부는 확인하지 못함.

---

## 3. 파일 목록 (`.vcxproj` 기준)

### 3.1 소스 (`ClCompile`)
- `Camera.cpp`
- `GameFramework.cpp`
- `GameObject.cpp`
- `LabProject.cpp`
- `Mesh.cpp`
- `Player.cpp`
- `Scene.cpp`
- `stdafx.cpp`  ← PCH 생성
- `Timer.cpp`

### 3.2 헤더 (`ClInclude`)
- `Camera.h`
- `GameFramework.h`
- `GameObject.h`
- `LabProject.h`
- `Mesh.h`
- `Player.h`
- `Resource.h`
- `Scene.h`
- `stdafx.h`
- `Timer.h`

### 3.3 리소스 / 기타
- `LabProject.ico` (None)
- `small.ico` (None)
- `ReadMe.txt` (None)
- `LabProject.rc` (ResourceCompile)

> 이전 Software Renderer 프로젝트와 달리 `GraphicsPipeline.h/.cpp`는 **없음**.

---

## 4. 전역 상수 / 매크로 (`stdafx.h`)

```cpp
#define CLIENT_WIDTH        640
#define CLIENT_HEIGHT       480

#define DIR_FORWARD         0x01
#define DIR_BACKWARD        0x02
#define DIR_LEFT            0x04
#define DIR_RIGHT           0x08
#define DIR_UP              0x10
#define DIR_DOWN            0x20

#define RANDOM_COLOR        (0xFF000000 | ((rand() * 0xFFFFFF) / RAND_MAX))
#define EXPLOSION_DEBRISES  240

//#define _WITH_VECTOR_OPERATION    ← 주석 처리됨

#define EPSILON             1.0e-6f

inline bool IsZero (float fValue);      // fabsf(v) < EPSILON
inline bool IsEqual(float fA, float fB);
```

### 인클루드
- Windows: `windows.h`, `Mmsystem.h`
- CRT: `stdlib.h`, `malloc.h`, `memory.h`, `tchar.h`, `math.h`
- **D3D12 / DirectXMath / DirectXCollision**:
  ```cpp
  #include <d3d12.h>
  #include <DirectXMath.h>
  #include <DirectXPackedVector.h>
  #include <DirectXColors.h>
  #include <DirectXCollision.h>
  using namespace DirectX;
  using namespace DirectX::PackedVector;
  #pragma comment(lib, "d3d12.lib")
  ```

### `stdafx.h` 내 수학 유틸 네임스페이스 (인라인 함수)
- `Vector3`
  - `XMVectorToFloat3`
  - `ScalarProduct(v, s, bNormalize=true)`
  - `Add(v1, v2)` / `Add(v1, v2, fScalar)`
  - `Subtract`
  - `DotProduct`, `CrossProduct(bNormalize=true)`
  - `Normalize`, `Length`, `Distance`
  - `Angle(XMVECTOR, XMVECTOR)` / `Angle(XMFLOAT3, XMFLOAT3)`
  - `TransformNormal(v, XMMATRIX)` / `TransformNormal(v, XMFLOAT4X4)`
  - `TransformCoord(v, XMMATRIX)`  / `TransformCoord(v, XMFLOAT4X4)`
- `Vector4`
  - `Add`
- `Matrix4x4`
  - `Identity`, `Translate`
  - `Multiply` (`XMFLOAT4X4×XMFLOAT4X4`, `XMFLOAT4X4×XMMATRIX`, `XMMATRIX×XMFLOAT4X4`, `XMMATRIX×XMMATRIX`)
  - `RotationYawPitchRoll(pitch, yaw, roll)` ← 내부적으로 `XMMatrixRotationRollPitchYaw(radians(pitch), radians(yaw), radians(roll))`
  - `RotationAxis(axis, angleDeg)`
  - `Inverse`, `Transpose`
  - `PerspectiveFovLH(fovDeg, aspect, nearZ, farZ)`
  - `LookAtLH(eye, at, up)`
- `Triangle`
  - `Intersect(rayOrigin, rayDir, v0, v1, v2, &hit)` → `TriangleTests::Intersects` 래퍼
- `Plane`
  - `Normalize(xmf4Plane)` → `XMPlaneNormalize` 래퍼

> 관측된 특이점: `Matrix4x4::RotationYawPitchRoll`의 내부 호출은 `XMMatrixRotationRollPitchYaw(radians(pitch), radians(yaw), radians(roll))` 순서이며, **함수 이름(YawPitchRoll)과 내부 인자 순서가 pitch→yaw→roll로 매칭**됨. 이름 대비 혼동 여지가 있다는 점을 코드 그대로 기록함.

---

## 5. 모듈 의존 관계 (`#include` 관측)

```
LabProject.cpp  ─► LabProject.h ─► resource.h
                ─► GameFramework.h

GameFramework.h ─► Timer.h, Scene.h, Player.h
Scene.h         ─► GameObject.h, Player.h
Player.h        ─► GameObject.h
GameObject.h    ─► Mesh.h, Camera.h
Mesh.h          ─► Camera.h
Camera.h        ─► (전방선언: class CPlayer;)

Camera.cpp      ─► StdAfx.h, Camera.h, Player.h
(모든 .cpp)     ─► stdafx.h
```

> 관측된 특이점: `Camera.cpp`와 `GameObject.cpp`는 `#include "StdAfx.h"` (대소문자 섞임)로 포함. Windows 파일시스템이 대소문자를 구분하지 않으므로 동일 파일로 해석됨.

---

## 6. 클래스 구조

### 6.1 메쉬 (`Mesh.h`, `Mesh.cpp`)

- `CVertex` — `XMFLOAT3 m_xmf3Position`
- `CPolygon`
  - `int m_nVertices`, `CVertex* m_pVertices`
  - 생성자에서 `new CVertex[n]`, 소멸자에서 `delete[]`
  - `SetVertex(int, CVertex&)` — 인덱스·Null 검사 후 대입
- `CMesh`
  - `int m_nPolygons; CPolygon** m_ppPolygons;`
  - `int m_nReferences` 레퍼런스 카운팅 (`AddRef`/`Release`, 0 이하 시 `delete this`)
  - `BoundingOrientedBox m_xmOOBB` (기본값: 중심 (0,0,0), extents (1,1,1), 단위쿼터니언)
  - `virtual void Render(HDC, XMFLOAT4X4& world, CCamera*)` — 아래 8절 참고
  - `BOOL RayIntersectionByTriangle(XMVECTOR& origin, XMVECTOR& dir, XMVECTOR v0, XMVECTOR v1, XMVECTOR v2, float* pfNearHitDistance)`
    - `TriangleTests::Intersects` 호출
    - 교차 && `fHitDistance < *pfNearHitDistance`이면 `*pfNearHitDistance` 갱신
  - `int CheckRayIntersection(XMVECTOR& origin, XMVECTOR& dir, float* pfNearHitDistance)`
    - 먼저 `m_xmOOBB.Intersects(origin, dir, *pfNearHitDistance)`로 OOBB 판정
    - OOBB hit 시 각 폴리곤에 대해 정점 수 3/4별 분기:
      - 3정점: 삼각형 1개
      - 4정점: (v0,v1,v2) + (v0,v2,v3) 두 삼각형
- `CCubeMesh : CMesh(6)` — Front/Top/Back/Bottom/Left/Right, 각 4정점. OOBB extents = (fHalfWidth, fHalfHeight, fHalfDepth)
- `CWallMesh : CMesh(4*n*n + 2)` — 좌/우/상/하 면을 `nSubRects` × `nSubRects`로 분할, 전/후면은 1장. (내부 뷰용 6면 박스)
- `CAirplaneMesh : CMesh(24)` — 상/하/좌/우 플레이트와 꼬리날개를 삼각형 24개로 구성

### 6.2 카메라 (`Camera.h`, `Camera.cpp`)

- `CCamera` 필드:
  - `XMFLOAT3 m_xmf3Position, m_xmf3Right, m_xmf3Up, m_xmf3Look`
  - `XMFLOAT4X4 m_xmf4x4View, m_xmf4x4Projection, m_xmf4x4ViewProject`
  - `D3D12_VIEWPORT m_d3dViewport` (기본값 `{0,0, CLIENT_WIDTH, CLIENT_HEIGHT, 0.0f, 1.0f}`)
  - `BoundingFrustum m_xmFrustumView, m_xmFrustumWorld`
  - `XMFLOAT4X4 m_xmf4x4InverseView`
- `GenerateProjectionMatrix(near, far, fovDeg)`
  - `XMMatrixPerspectiveFovLH(radians(fov), aspect, near, far)`
  - `BoundingFrustum::CreateFromMatrix(m_xmFrustumView, projection)` — **뷰 공간 프러스텀 생성**
- `GenerateViewMatrix()`
  - `Look`, `Right`, `Up` 재정규화 (`Up=Cross(Look,Right)` 등)
  - `m_xmf4x4View`의 회전 3×3 성분과 `_41, _42, _43 = -Dot(pos, basis)`
  - `m_xmf4x4ViewProject = View × Projection`
  - `m_xmf4x4InverseView`: 회전 3×3 전치 + 위치
  - `m_xmFrustumView.Transform(m_xmFrustumWorld, InverseView)` — **월드 공간 프러스텀 산출**
- `IsInFrustum(BoundingOrientedBox&)` → `m_xmFrustumWorld.Intersects(OOBB)`
- `SetLookAt(eye, at, up)` / `SetLookAt(at, up)` — `Matrix4x4::LookAtLH`로 View 생성 후 행에서 basis 추출
- `SetViewport(x, y, w, h, minZ, maxZ)` — `D3D12_VIEWPORT` 필드 대입 (렌더 타겟이 아니라 스크린 매핑 용도)
- `Move(XMFLOAT3&)`, `Move(x,y,z)`
- `Rotate(pitch, yaw, roll)` — 각각 `Right/Up/Look` 축 기준 `XMMatrixRotationAxis`로 basis 회전
- `Update(CPlayer*, XMFLOAT3& lookAt, fTimeElapsed=0.016)` — 플레이어 기준 추격 카메라
  - 플레이어 basis로 회전 행렬 구성 → `pPlayer->m_xmf3CameraOffset`을 월드로 변환
  - 목표 위치로 프레임당 `fLength * (fTimeElapsed / 0.25)` 만큼 선형 이동 (시간 래그 스케일)
  - 이동 후 `SetLookAt(pPlayer->m_xmf3Position, pPlayer->m_xmf3Up)` 수행

### 6.3 게임 오브젝트 계층 (`GameObject.h`, `GameObject.cpp`)

#### 공통 유틸 (파일 상단)
- `inline float RandF(min, max)`
- `XMVECTOR RandomUnitVectorOnSphere()` — 단위 구 내부 벡터 생성 후 정규화 (rejection sampling)

#### `CGameObject`
- `bool m_bActive = true`
- `CMesh* m_pMesh`, `XMFLOAT4X4 m_xmf4x4World`, `BoundingOrientedBox m_xmOOBB`
- `CGameObject* m_pObjectCollided = NULL`
- `DWORD m_dwColor`
- 이동: `XMFLOAT3 m_xmf3MovingDirection`, `float m_fMovingSpeed`, `float m_fMovingRange`
- 회전: `XMFLOAT3 m_xmf3RotationAxis`, `float m_fRotationSpeed` (기본 축 (0,1,0), 기본 속도 0.05)
- `SetMesh(CMesh*)` — Null 아닐 때 `pMesh->AddRef()`
- 소멸자 — `m_pMesh->Release()`
- `SetPosition(x,y,z)`, `SetPosition(XMFLOAT3&)` — `_41,_42,_43`에 직접 기록
- `GetPosition/GetLook/GetUp/GetRight` — World 행의 3/2/1행을 basis로 반환, Look/Up/Right는 정규화
- `MoveStrafe/MoveUp/MoveForward(fDistance)` — basis * distance 누적
- `Move(XMFLOAT3& dir, float speed)` — `dir * speed`로 위치 누적
- `Rotate(pitch, yaw, roll)` — `Matrix4x4::RotationYawPitchRoll` × World
- `Rotate(axis, angleDeg)` — `Matrix4x4::RotationAxis` × World
- `LookTo/LookAt(at, up)` — `Matrix4x4::LookAtLH`로 View 구성 후, **그 View의 행을 열로 전치**해 World의 회전 3×3에 대입
- `UpdateBoundingBox()` — `m_pMesh->m_xmOOBB`를 현재 World로 변환해 `m_xmOOBB`에 저장, quaternion 정규화
- `virtual Animate(dt)` — `Rotate(axis, rotSpeed*dt)`, `Move(dir, movSpeed*dt)`, 마지막에 `UpdateBoundingBox`
- `virtual Render(HDC, CCamera*)` — `pCamera->IsInFrustum(m_xmOOBB)` **통과 시에만** `CreatePen` + `m_pMesh->Render(hDC, World, camera)` 호출 (프러스텀 컬링 지점)
- 픽킹:
  - `GenerateRayForPicking(pickPos, view, &origin, &direction)`
    - `inv(World * View)`로 스크린 투영 좌표계를 모델 공간으로 되돌림
    - origin = 카메라 원점(0,0,0)을 모델 공간으로 변환
    - direction = `normalize(pickPosInModelSpace - origin)`
  - `PickObjectByRayIntersection(pickPos, view, &hitDist)` → `m_pMesh->CheckRayIntersection(...)` 호출

#### `CExplosiveObject : CGameObject`
- `bool m_bBlowingUp = false`
- `XMFLOAT4X4 m_pxmf4x4Transforms[EXPLOSION_DEBRISES]` (=240)
- `float m_fElapsedTimes=0, m_fDuration=2.5, m_fExplosionSpeed=10, m_fExplosionRotation=720`
- static: `m_pxmf3SphereVectors[240]`, `m_pExplosionMesh`
- `PrepareExplosion()` — 240개 랜덤 구 벡터 + `m_pExplosionMesh = new CCubeMesh(0.5, 0.5, 0.5)`
- `Animate(dt)`:
  - 폭발 중: 240개 파편 각각에 대해 이동(SphereVector × speed × elapsed) + 회전(rotation × elapsed) 행렬 계산
  - `elapsed > duration`이면 폭발 종료
  - 비폭발 시 `CGameObject::Animate(dt)` 호출
- `Render` — 폭발 중엔 각 파편을 별도 렌더, 평소엔 `CGameObject::Render`

#### `CWallsObject : CGameObject`
- `BoundingOrientedBox m_xmOOBBPlayerMoveCheck`
- `XMFLOAT4 m_pxmf4WallPlanes[6]` — 각 벽 평면 (법선 + 거리)

#### `CBulletObject : CGameObject`
- `float m_fBulletEffectiveRange = 50.0f` (생성자에서 덮어씀)
- `XMFLOAT3 m_xmf3FirePosition`, `float m_fRotationAngle`
- `SetFirePosition(pos)` — 내부에서 `SetPosition(pos)`도 호출
- `Animate(dt)`:
  - `_WITH_VECTOR_OPERATION` 정의 시: 축-이동방향 사이 각으로 회전 맞춤 + 축 회전 추가
  - **미정의(기본 경로)**: `Matrix4x4::RotationYawPitchRoll(0, rotSpeed*dt, 0)` × World (Yaw만 회전)
  - `MovingDirection × speed × dt` 만큼 위치 누적
  - `UpdateBoundingBox()`
  - `Distance(fire, now) > effectiveRange`이면 `SetActive(false)`

### 6.4 플레이어 (`Player.h`, `Player.cpp`)

#### `CPlayer : CGameObject`
- 자체 basis: `m_xmf3Position, m_xmf3Right, m_xmf3Up, m_xmf3Look`
- `XMFLOAT3 m_xmf3CameraOffset`, `XMFLOAT3 m_xmf3Velocity`
- `float m_fFriction = 125.0f`
- `float m_fPitch/m_fYaw/m_fRoll = 0`
- `CCamera* m_pCamera`
- 생성자:
  - `new CCamera()`
  - `GenerateProjectionMatrix(near=1.01, far=5000, fov=60)`
  - `SetViewport(0, 0, CLIENT_WIDTH, CLIENT_HEIGHT, 0, 1)`
  - basis 단위 초기화
- `SetCameraOffset(offset)` — `m_pCamera->SetLookAt(pos+offset, pos, up)` 후 `GenerateViewMatrix()`
- `Move(DWORD dwDirection, float fDistance)` — DIR_* 비트에 따라 `Look/Right/Up * fDistance`를 누적해 `Move(shift, bUpdateVelocity=true)` 호출
- `Move(XMFLOAT3& shift, bool bUpdateVelocity)`:
  - `true`면 `m_xmf3Velocity += shift`
  - `false`면 `m_xmf3Position += shift` 및 `m_pCamera->Move(shift)`
- `Rotate(pitch, yaw, roll)`:
  1. 카메라 동일 회전
  2. basis를 각 축에 대해 `XMMatrixRotationAxis`로 회전
  3. `Look/Right/Up` 재정규화 및 외적으로 직교화
- `Update(dt)`:
  - `Move(m_xmf3Velocity, false)` — 속도를 위치에 반영
  - `m_pCamera->Update(this, m_xmf3Position, dt)` + `GenerateViewMatrix()`
  - 마찰 감속: 속도 길이만큼까지 `-normalize(velocity) * friction * dt` 적용
- `OnUpdateTransform()` — basis + position을 World 4×4에 반영
- `Animate(dt)` — `OnUpdateTransform()` → `CGameObject::Animate(dt)`

#### `CAirplanePlayer : CPlayer`
- `float m_fBulletEffectiveRange = 150.0f`
- `CBulletObject* m_ppBullets[BULLETS]`  (`BULLETS = 30`)
- 생성자: 공용 `CCubeMesh(1, 4, 1)` 생성 → 30개 총알에 `SetMesh` (공유, AddRef 누적), 각 총알에:
  - `RotationAxis = (0,1,0)`, `RotationSpeed = 360`
  - `MovingSpeed = 120`
  - `SetActive(false)`
- `OnUpdateTransform()` — `CPlayer::OnUpdateTransform()` 후 X축 90° 추가 회전(기체 기준축 보정)
- `FireBullet(CGameObject* pSelectedObject)`
  - 선택된 오브젝트가 있으면 `LookAt(selected.pos, (0,1,0))`
  - `OnUpdateTransform()` 재수행
  - 비활성 총알 중 첫 번째 선택
  - 발사 위치 = 플레이어 위치 + `GetUp() * 6.0`
  - 총알 `m_xmf4x4World = Player.m_xmf4x4World` (복제) 후 위치 `_41/_42/_43` 덮어씀
  - `SetFirePosition(firePos)`, `SetMovingDirection(Up)`, `SetActive(true)`
- `Animate(dt)` — `CPlayer::Animate(dt)` + 활성 총알들 `Animate(dt)`
- `Render` — `CPlayer::Render` + 활성 총알 렌더

### 6.5 씬 (`Scene.h`, `Scene.cpp`)

- `CPlayer* m_pPlayer` (외부에서 주입: `GameFramework::BuildObjects`에서 `m_pScene->m_pPlayer = m_pPlayer`)
- `CGameObject** m_ppObjects; int m_nObjects;`
- `CWallsObject* m_pWallsObject`

- `BuildObjects()`
  - `CExplosiveObject::PrepareExplosion()` 호출
  - 벽 박스: halfSize = (45, 45, 110) → `CWallMesh(90, 90, 220, 30)` + 6개 벽 평면:
    - `(+1,0,0, 45)`, `(-1,0,0, 45)`, `(0,+1,0, 45)`, `(0,-1,0, 45)`, `(0,0,+1, 110)`, `(0,0,-1, 110)`
  - `m_xmOOBBPlayerMoveCheck` = half extents (45, 45, 110*0.05 = 5.5)
  - 공용 `CCubeMesh(4, 4, 4)` 1개 → 10개 `CExplosiveObject`가 **공유**
  - 10개 오브젝트 각각 위치/회전축/회전속도/이동방향/이동속도/색 개별 지정

- `ReleaseObjects()`
  - `CExplosiveObject::m_pExplosionMesh->Release()` (static 폭발 메쉬)
  - 각 `m_ppObjects[i]` `delete` → 배열 `delete[]`
  - `m_pWallsObject` `delete`

- `PickObjectPointedByCursor(xClient, yClient, CCamera*)`
  - 스크린 → NDC 역변환:
    ```
    pick.x = ((2x / viewport.width) - 1) / proj._11
    pick.y = -((2y / viewport.height) - 1) / proj._22
    pick.z = 1.0
    ```
  - 각 오브젝트에 대해 `PickObjectByRayIntersection`로 히트 거리 최소값 추적 → `pNearestObject` 반환

- `CheckObjectByObjectCollisions()` — OOBB pairwise 교차 검사. 두 오브젝트가 충돌하면 `m_xmf3MovingDirection` 및 `m_fMovingSpeed`를 **서로 교환**. (탄성 교환식)
- `CheckObjectByWallCollisions()` — 오브젝트 OOBB가 벽 OOBB에 대해 `DISJOINT` 또는 `INTERSECTS`면 6개 벽 평면 중 `BACK` / `INTERSECTING`인 첫 평면을 찾아 `XMVector3Reflect`로 이동 방향 반사
- `CheckPlayerByWallCollision()` — 벽의 `m_xmOOBBPlayerMoveCheck`를 월드로 변환해 플레이어 OOBB와 교차 **하지 않으면** 벽 전체를 플레이어 위치로 이동(`SetPosition(player.pos)`). 벽 박스가 플레이어를 다시 포함하도록 끌고 다니는 방식으로 관측됨.
- `CheckObjectByBulletCollisions()` — 오브젝트 OOBB ↔ 활성 총알 OOBB 교차 시: 오브젝트를 `CExplosiveObject`로 캐스팅해 `m_bBlowingUp = true`, 총알 `SetActive(false)`

- `Animate(dt)`:
  1. 벽 `Animate(dt)`
  2. 각 오브젝트 `Animate(dt)`
  3. `CheckPlayerByWallCollision()`
  4. `CheckObjectByWallCollisions()`
  5. `CheckObjectByObjectCollisions()`
  6. `CheckObjectByBulletCollisions()`

- `Render(HDC, CCamera*)`:
  - 벽 `Render` → 각 오브젝트 `Render`

- `OnProcessingKeyboardMessage`:
  - `'1'..'9'` → `m_ppObjects[wParam-'1']`를 `CExplosiveObject`로 캐스팅해 `m_bBlowingUp=true`
  - `'A'` → 모든 오브젝트 폭파

### 6.6 프레임워크 (`GameFramework.h`, `GameFramework.cpp`)

- 필드:
  - `HINSTANCE m_hInstance`, `HWND m_hWnd`, `bool m_bActive = true`
  - `CGameTimer m_GameTimer`
  - `HDC m_hDCFrameBuffer`, `HBITMAP m_hBitmapFrameBuffer`
  - `CPlayer* m_pPlayer`, `CScene* m_pScene`, `CGameObject* m_pSelectedObject`
  - `POINT m_ptOldCursorPos`
  - `_TCHAR m_pszFrameRate[50]` — 생성자에서 `"LabProject ("`로 시작
- `OnCreate(hInstance, hWnd)`:
  - `srand(timeGetTime())`
  - `BuildFrameBuffer()` → `BuildObjects()`
- `BuildFrameBuffer()`:
  - `GetClientRect` + `CreateCompatibleDC` + `CreateCompatibleBitmap((w+1)×(h+1))` (이전 프로젝트와 달리 크기 +1)
  - `SetBkMode(TRANSPARENT)`
- `ClearFrameBuffer(color)`:
  - `m_pPlayer->m_pCamera->m_d3dViewport` 영역에 `Rectangle`로 색 칠함
- `PresentFrameBuffer()`:
  - 카메라 뷰포트 영역만 `BitBlt`로 복사
- `BuildObjects()`:
  - `CAirplaneMesh(6, 6, 1)` 생성
  - `CAirplanePlayer` 생성, 위치 (0,0,0), mesh 설정, 색 `RGB(0,0,255)`, CameraOffset `(0, 5, -15)`
  - `CScene` 생성 → `BuildObjects()` → `m_pScene->m_pPlayer = m_pPlayer`
- `ReleaseObjects()`:
  - `Scene->ReleaseObjects()` → `delete m_pScene` → `delete m_pPlayer`
- `OnDestroy()`:
  - `ReleaseObjects()` → `DeleteObject(hBitmap)` → `DeleteDC` → `DestroyWindow`
- `ProcessInput()`:
  - 키보드 비트마스크 `dwDirection`: `VK_UP/DOWN/LEFT/RIGHT/PRIOR/NEXT` → 방향 플래그
  - 마우스 델타(캡처 상태에서만 측정, 델타 /3), 커서 원위치 복귀
  - `VK_RBUTTON` 눌림: `Rotate(cyDelta, 0, -cxDelta)` (roll 사용)
  - 외: `Rotate(cyDelta, cxDelta, 0)`
  - `dwDirection`가 있으면 `m_pPlayer->Move(dwDirection, 0.15f)`
  - 마지막에 `m_pPlayer->Update(m_GameTimer.GetTimeElapsed())` 호출 (2번 호출됨 — 아래 특이점 참조)
- `FrameAdvance()`:
  - `!m_bActive`면 조기 반환
  - `Tick(0.0f)` → `ProcessInput()`
  - `m_pPlayer->Animate(dt)` → `m_pScene->Animate(dt)`
  - `ClearFrameBuffer(RGB(255,255,255))`
  - `m_pScene->Render(hDC, m_pPlayer->m_pCamera)` → `m_pPlayer->Render(hDC, m_pPlayer->m_pCamera)`
  - `PresentFrameBuffer()`
  - `GetFrameRate(m_pszFrameRate+12, 37)` + `SetWindowText`로 타이틀바 갱신
- 입력 메시지 분기 (`OnProcessingMouseMessage`):
  - `WM_RBUTTONDOWN` → `m_pSelectedObject = m_pScene->PickObjectPointedByCursor(LOWORD(lParam), HIWORD(lParam), m_pPlayer->m_pCamera)`
  - `WM_LBUTTONDOWN` → `SetCapture + GetCursorPos(&m_ptOldCursorPos)`
  - `WM_LBUTTONUP / WM_RBUTTONUP` → `ReleaseCapture`
- 키 메시지 분기 (`OnProcessingKeyboardMessage`):
  - `VK_ESCAPE` → 종료
  - `VK_CONTROL` → `((CAirplanePlayer*)m_pPlayer)->FireBullet(m_pSelectedObject)` — **총알 발사**
  - 그 외 → `m_pScene->OnProcessingKeyboardMessage(...)`로 위임 (씬에서 `1..9/A` 처리)

### 6.7 타이머 (`Timer.h`, `Timer.cpp`)

- `CGameTimer` — `QueryPerformanceFrequency/Counter` 사용, 50샘플 이동 평균
- 이전 프로젝트(Software Renderer)와 구현이 동일
- `GetFrameRate(LPTSTR, nChars)`이 숫자 + `" FPS)"`를 버퍼에 기록

---

## 7. 윈도우 / 메시지 루프 (`LabProject.cpp`)

- 윈도우 스타일: `WS_OVERLAPPEDWINDOW`
- 기본 클라이언트 크기: `CLIENT_WIDTH × CLIENT_HEIGHT` (640×480), `AdjustWindowRect`로 보정
- `WndProc` 메시지 라우팅:
  - `WM_SIZE` → `SIZE_MINIMIZED` 시 `SetActive(false)`, 아니면 `SetActive(true)`
  - `WM_LBUTTONDOWN / RBUTTONDOWN / LBUTTONUP / RBUTTONUP / WM_KEYDOWN` → `gGameFramework.OnProcessingWindowMessage`
  - `WM_COMMAND` → 메뉴(ABOUT / EXIT)
  - `WM_DESTROY` → `PostQuitMessage(0)`
- 메시지 없으면 `gGameFramework.FrameAdvance()`

> 관측된 특이점: `WM_MOUSEMOVE`는 `WndProc`에서 분기되어 있지 않음. 즉 `CGameFramework::OnProcessingMouseMessage`의 `WM_MOUSEMOVE` 케이스는 현재 경로에 진입하지 않음(본문 자체는 비어 있음).

- 리소스 식별자 (`Resource.h`): `IDS_APP_TITLE=103`, `IDR_MAINFRAME=128`, `IDD_LABPROJECT_DIALOG=102`, `IDD_ABOUTBOX=103`, `IDM_ABOUT=104`, `IDM_EXIT=105`, `IDI_LABPROJECT=107`, `IDI_SMALL=108`, `IDC_LABPROJECT=109`, `IDC_MYICON=2`

---

## 8. 렌더링 파이프라인 흐름 (코드 동선)

`CGameFramework::FrameAdvance`
  → `m_pScene->Render(hDC, camera)`
      → 벽 → 오브젝트 순으로 `CGameObject::Render(hDC, camera)`
          → `camera->IsInFrustum(m_xmOOBB)` 통과 시에만 진행 (**프러스텀 컬링**)
          → `CreatePen(PS_SOLID, 0, m_dwColor)` 선택
          → `CMesh::Render(hDC, m_xmf4x4World, camera)`
              → `xmf4x4Transform = world × camera.m_xmf4x4ViewProject`
              → 각 폴리곤 정점 루프 (`i = 0 .. nVertices`, `i % nVertices`로 폴리곤 닫음)
                  → `Vector3::TransformCoord(pos, transform)` ← 퍼스펙티브 디비전 포함
                  → `0.0 <= z <= 1.0` 통과 시만 화면 좌표 변환:
                    ```
                    screen.x = +ndc.x * (viewport.width * 0.5) + viewport.TopLeftX + viewport.width * 0.5
                    screen.y = -ndc.y * (viewport.height * 0.5) + viewport.TopLeftY + viewport.height * 0.5
                    ```
                  → 이전 정점이 유효했다면 `MoveToEx + LineTo`로 선 그림
  → `m_pPlayer->Render` (비행기 + 활성 총알들 동일 경로)
  → `PresentFrameBuffer()`

픽킹 흐름:
`CGameFramework::OnProcessingMouseMessage(WM_RBUTTONDOWN)`
  → `CScene::PickObjectPointedByCursor(xClient, yClient, camera)`
      → 스크린 → 카메라 공간 방향 (`proj._11`, `proj._22` 기반 NDC 역변환)
      → 각 오브젝트 `PickObjectByRayIntersection`:
          → `GenerateRayForPicking`: `inverse(World * View)`로 원점/방향을 모델 공간으로 역변환
          → `CMesh::CheckRayIntersection`: OOBB 사전검사 → 폴리곤별 삼각형화 후 `TriangleTests::Intersects`
      → 최단 히트 오브젝트 반환 → `m_pSelectedObject` 설정

---

## 9. 관측된 주의사항 (코드에 그대로 있는 것만)

1. **`ProcessInput`에서 `m_pPlayer->Update`가 호출**되고, 이어 `FrameAdvance`가 `m_pPlayer->Animate`도 호출함. `Animate`는 `OnUpdateTransform`을 거쳐 World를 재구성하므로 프레임당 두 단계를 거침 (코드 그대로 기록).
2. `CGameFramework::BuildFrameBuffer`에서 비트맵 크기를 `(w+1) × (h+1)`로 생성. 이전 Software Renderer 프로젝트는 `w × h`였음.
3. `WndProc`에서 `WM_MOUSEMOVE`가 분기되지 않아, `OnProcessingWindowMessage`의 동명 케이스로 들어오지 않음.
4. `CGameObject::LookTo/LookAt`는 `LookAtLH` 결과를 **전치해** World의 회전 3×3에 대입 (`_11 = view._11`, `_12 = view._21` …).
5. `CAirplanePlayer::OnUpdateTransform`은 부모 호출 후 World에 X축 90° 회전을 **추가로** 곱함 (기체 축 보정).
6. `CScene::CheckObjectByObjectCollisions`의 충돌 반응은 `MovingDirection`과 `MovingSpeed`를 **서로 교환**하는 단순 교환식. 실제 물리 기반 탄성 충돌 공식은 아님.
7. `CScene::CheckPlayerByWallCollision`은 `xmOOBBPlayerMoveCheck`가 플레이어와 **교차하지 않을 때** 벽을 플레이어 위치로 이동. 관측 그대로 기록.
8. `CGameTimer::m_bStopped`는 생성자에서 명시 초기화가 없음.
9. `stdafx.h` 맨 아래의 `_WITH_VECTOR_OPERATION` 매크로는 주석 처리되어 있어, `CBulletObject::Animate`에서 단순 Yaw 회전 경로가 실행됨.
10. `CCubeMesh(1,4,1)` 하나로 30개 총알이 메쉬를 공유 → `AddRef`가 `SetMesh` 호출 시마다 누적.
11. `d3d12.h`를 include하고 `d3d12.lib`를 링크하지만, 코드 내에서 실제 사용되는 타입은 관측 범위에서 `D3D12_VIEWPORT` 구조체뿐. `ID3D12Device`/커맨드 API는 코드에서 사용되지 않음.
12. `Matrix4x4::RotationYawPitchRoll(pitch, yaw, roll)`는 내부적으로 `XMMatrixRotationRollPitchYaw(radians(pitch), radians(yaw), radians(roll))`로 매핑됨. 함수명과 인자 의미에 주의.

---

## 10. 확인 실패 / 미관측 항목

- 솔루션 파일(`*.sln`)은 경로 직접 조회로 발견되지 않음.
- `LabProject.rc` 본문(리소스 스크립트), `ReadMe.txt` 본문, `LabProject.ico` / `small.ico` 바이너리 — 읽지 않음.
- IntelliJ/Rider 인덱스 기반 파일 검색(`find_files_by_*`)은 이번에도 MCP 호출이 실패. `.vcxproj` 등재 파일 + `#include` 체인만 따라 읽었음.
- `d3d12.h`를 include한 의도가 `D3D12_VIEWPORT` 구조체만을 위한 것인지, 향후 확장을 위한 것인지는 코드만으로 판단 불가.
