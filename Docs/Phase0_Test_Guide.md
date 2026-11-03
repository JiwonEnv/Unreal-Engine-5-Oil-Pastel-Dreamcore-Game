# 0단계 테스트 가이드

목표: **큐브만으로 "같이 들고 옮기는 재미"와 네트워크 동기화가 참을 만한지** 확인하기.

## 1. 열기

1. `DreamMovers.uproject`를 더블클릭해 언리얼 5.8 에디터로 엽니다.
   - "모듈을 다시 빌드할까요?"라고 물으면 **예**.
2. 시작 맵 `Content/Maps/Lvl_Proto`가 자동으로 열립니다.

## 2. 멀티 테스트 설정 (PIE)

툴바의 ▶ 옆 `⋮` → **Net Mode: Play As Listen Server**, **Number of Players: 2**.
▶를 누르면 창 두 개가 뜹니다 (서버 겸 플레이어 1, 클라이언트 플레이어 2).

랙 테스트: `⋮` → Advanced Settings → **Enable Network Emulation** 체크 → Emulation Profile `Average`(약 30~60ms) 또는 `Bad`.
실제 인터넷 친구와 비슷한 조건이 됩니다.

## 3. 조작

| 키 | 동작 |
|---|---|
| WASD / 마우스 | 이동 / 시점 |
| Space | 점프 |
| Shift | 달리기 |
| 마우스 왼쪽 (누르고 있기) | 실타래로 잡기, 떼면 놓기 |
| 마우스 휠 | 잡은 물건 당기기 / 밀기 |

## 4. 맵 구성

- **현관 홀** (시작): 금색 원판이 탈출 지점. 물건을 원판 위에서 **놓으면** 정산됩니다.
- **응접실**: 탁자 위 작은 물건(S), 바닥의 중간 물건(M), 가운데 좁은 장식장 틈.
- **뒷방**: 2인용 무거운 물건(L, 25~35kg), 경사로 위 선반의 깨지기 쉬운 물건.
- 목표 금액 = 맵 전체 가치의 50%, 제한 시간 12분. 달성하면 성공, 8초 뒤 재시작.

## 5. 체크리스트

| # | 확인할 것 | 결과 |
|---|---|---|
| 1 | 두 창 모두 상단에 한글 HUD(모은 금액, 남은 시간)가 보인다 | ☐ |
| 2 | 작은 물건을 잡아 옮길 수 있고, 상대 창에서도 실과 물건이 같이 움직인다 | ☐ |
| 3 | 벽에 세게 부딪히면 금이 가고(색이 어두워짐) 가격이 줄어든다. 3번이면 깨져 사라진다 | ☐ |
| 4 | 회전목마 말(32kg)은 혼자 잡으면 바닥에 끌리기만 하고, 둘이 잡으면 들린다 | ☐ |
| 5 | 둘이 같이 들고 문을 통과할 수 있다 | ☐ |
| 6 | 탈출 원판에 놓으면 금액이 오르고 물건이 사라진다 | ☐ |
| 7 | Network Emulation `Average`에서 클라이언트 쪽 잡기가 "참을 만하다" | ☐ |
| 8 | 같은 조건으로 `Bad`에서는 어떤지 (기록만) | ☐ |

**7번이 0단계의 핵심 통과 기준**입니다. 통과하면 1단계(그레이박스 저택)로 넘어갑니다.

## 6. 빌드 없이 바로 바꿀 수 있는 값

에디터에서 골동품 액터를 선택 → Details 패널 **Valuable** 항목:
- `Mass Kg`: 무게 (20 초과면 2인 필요)
- `Crack Speed`: 이 속도(cm/s) 이상 부딪히면 금이 감. 낮을수록 잘 깨짐
- `Base Value`, `Max Cracks`, `Loss Per Crack`, `Tint`

맵은 `Tools/ue/build_level.py`로 다시 생성할 수 있지만, **다시 생성하면 에디터에서 손으로 바꾼 배치는 사라집니다.**
손으로 다듬기 시작했다면 스크립트는 더 이상 돌리지 마세요.

## 7. C++ 값 (수정 시 빌드 필요, 에디터에서 `Ctrl+Alt+F11` Live Coding)

| 파일 | 값 | 의미 |
|---|---|---|
| `DMGrabComponent.h` | `MaxLiftKg` 20 | 한 명이 들 수 있는 최대 무게 |
| | `SpringFrequency` 9 | 높을수록 조준점에 빨리 붙지만 네트워크에서 떨림 |
| | `GrabRange` 400 | 잡기 거리 (cm) |
| | `BreakDistance` 250 | 물건이 끼었을 때 실이 끊어지는 거리 |
| `DMGameMode.h` | `RoundSeconds` 720 | 제한 시간 |
| | `TargetShare` 0.5 | 목표 금액 비율 |
| `DMCharacter.h` | `WalkSpeed` / `SprintSpeed` | 이동 속도 |

## 8. 구조 요약

| 클래스 | 역할 |
|---|---|
| `ADMCharacter` | 1인칭 인형 (키 110cm), 입력은 코드로 생성 |
| `UDMGrabComponent` | 실타래 잡기. 서버가 힘 상한이 있는 스프링으로 끌어당김 → 힘이 합산되어 2인 들기 |
| `ADMValuable` | 골동품. 충돌 속도로 금, 가치 감소, 파손 |
| `ADMExtractionZone` | 탈출 지점. 놓인 물건만 정산 |
| `ADMGameMode` / `ADMGameState` | 목표 금액, 타이머, 성공·실패, 자동 재시작 |
| `ADMHUD` | 점수, 타이머, 가격표, 조준 정보 |

물리 동기화: 서버가 물리를 전담하고, 물건은 엔진의 `Predictive Interpolation` 복제 모드를 씁니다.

## 9. 오일 파스텔 셰이더

맵의 `PP_OilPastel`(무한 범위 Post Process Volume)이 `Content/Proto/Shader/MI_PP_OilPastel`을 씁니다.
**MI를 열어 값을 바꾸면 에디터 뷰포트에 바로 반영됩니다** (빌드 불필요).

| 그룹 | 파라미터 | 효과 |
|---|---|---|
| 1 Paint | `BrushSize` 7 | Kuwahara 붓 크기(px). 클수록 유화처럼 뭉개짐 |
| | `Saturation` 1.15 | 채도 |
| 2 Paper | `PaperScale` 2.2 | 종이 결 크기 |
| | `Grain` 0.75 | 종이 결이 드러나는 정도. 0이면 꽉 칠한 유화, 1이면 거친 파스텔 |
| | `StrokeScale` / `Stroke` | 표면 방향을 따라가는 색연필 해칭 크기 / 세기 |
| 3 Lines | `LineWidth` / `LineStrength` | 연필 외곽선 굵기 / 진하기 |
| | `BoilFPS` 6 | 선이 초당 몇 번 떨리는지(손그림 애니메이션 느낌). 0이면 고정 |
| 4 Glints | `GlintSize` / `Glint` / `GlintThreshold` | 유리·황동(Roughness 낮은 표면)의 별 반짝임 |
| 5 Dream | `Collapse` | **건드리지 말 것.** 게임 중 HUD가 타이머에 맞춰 조절 |
| | `PaperColor` / `InkColor` / `GlintColor` | 종이색, 선 색, 반짝임 색 |

**꿈 붕괴 연출**: 남은 시간이 50% 아래로 내려가면 색이 서서히 빠지고, 0이 되면 연필 스케치만 남습니다.
조절 값은 `DMHUD.h`의 `CollapseStartsAt`(0.5), `CollapseAtWake`(0.75). 실패하면 1(스케치만).
빨리 보고 싶으면 `DMGameMode.h`의 `RoundSeconds`를 60으로 줄여서 테스트하세요.

**반짝임 대상**: 골동품 액터의 `Roughness`(낮을수록 반짝임)와 `Metallic`. 테스트 맵에서는 크리스탈 잔, 향수병, 유리 랜턴, 성물함, 황동 램프, 회중시계, 금박 달걀, 액자가 반짝이도록 설정됨.

셰이더 코드: `Tools/ue/oil_pastel.hlsl` (Custom 노드 하나에 전부 들어 있음). 고친 뒤 아래를 실행하면 머티리얼이 갱신됩니다.

```
UnrealEditor-Cmd.exe DreamMovers.uproject -ExecutePythonScript="Tools/ue/build_shader.py"
```
(에디터가 열려 있으면 먼저 닫기. 또는 머티리얼 에디터에서 Custom 노드 코드를 직접 수정)

## 10. 스크립트 주의

- `build_level.py`는 `Lvl_Proto`의 액터를 **전부 지우고 다시 배치**합니다. 맵을 손으로 다듬기 시작했다면 다시 돌리지 마세요.
- 두 스크립트 모두 기존 에셋을 지우지 않고 그 자리에서 갱신합니다.

## 11. 몬스터

| | 촛대 집사 (`ADMButler`) | 종 달린 말 (`ADMBellHorse`) |
|---|---|---|
| 위치 | 응접실 ↔ 뒷방 순찰 (4지점) | 뒷방 가운데 대기 |
| 감지 | **촛불 빛 = 시야.** 빛 원뿔 안 + 가려지지 않음 → 0.6초 후 발각 (가까우면 더 빨리) | **소리만.** 눈이 없음. 가만히 있으면 바로 옆이어도 안전 |
| 발각 후 | 손종을 울림(반경 40m, 말이 듣고 달려옴) → 걷는 인형보다 느리게(270) 추격, 3초 놓치면 마지막 위치 수색 후 순찰 복귀 | 소리 난 곳으로 돌진(600, 달리기보다 빠름) → 3초 수색 → 천천히 복귀 |
| 플레이어에게 보이는 단서 | 머리 위 "?"(눈치채는 중) / 빨간 "!"(발각), 빛이 붉어짐 | 화면 하단 "딸랑… 딸랑…" (달려오면 빨간 글씨) |

**소리가 나는 것** (`dm.ShowNoise 1` 콘솔 명령으로 주황 구체 표시, 서버 화면에서만)
- 골동품 충돌: 150cm/s 이상 부딪히면 3~15m, 금이 가면 1.5배, 깨지면 25m
- 달리기 발소리 8m, 점프 착지 5m. **걷기는 조용함**
- 집사의 종 40m

**잡히면**: 인형이 솜뭉치가 됩니다(3kg, 안 깨짐). 동료가 실로 잡아 탈출 지점에 놓으면 그 자리에서 부활합니다.
전원이 솜뭉치가 되면 라운드 실패. 혼자 플레이할 때는 잡히면 바로 실패입니다.

**튜닝**: 맵의 `Butler` / `BellHorse` 액터를 선택 → Details의 `Butler` / `Bell Horse` 항목 (빌드 불필요).
집사 순찰 지점은 뷰포트에서 다이아몬드 위젯을 드래그해서 옮길 수 있습니다.

몬스터 상태 변화는 로그 `LogDreamMovers`에 찍힙니다 (`-LogCmds="LogDreamMovers Verbose"`로 순찰 지점까지).

## 12. 자동 점검 (`-dmselftest`)

키보드 없이 핵심 동작을 확인하는 개발용 모드입니다 (출시 빌드에는 포함되지 않음). 약 30초 후 자동 종료됩니다.

```
UnrealEditor.exe DreamMovers.uproject /Game/Maps/Lvl_Proto?listen -game -windowed -ResX=1280 -ResY=720 -dmselftest
```

확인 항목 11개: 5kg 물건 잡기·들기, 32kg 물건 혼자 못 들기, 소리에 말 반응, 집사 발각·포획, 솜뭉치 생성, 잡힌 뒤 솜뭉치 시점, 탈출 지점 부활.
결과: 로그(`Saved/Logs/DreamMovers.log`)의 `[SelfTest] PASS/FAIL` 줄, 스크린샷 `Saved/Screenshots/WindowsEditor/SelfTest_*.png`.

**자동 점검이 못 하는 것**: 실제 2인 같이 들기, 네트워크 지연 체감, 조작감. 이건 PIE 2인 테스트로 확인하세요.
