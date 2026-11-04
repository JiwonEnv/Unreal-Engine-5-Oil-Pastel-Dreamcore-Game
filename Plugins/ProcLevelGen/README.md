# ProcLevelGen

시드 기반 방 모듈 조립 플러그인 (Unreal Engine 5.8, C++).

미리 정한 방 모듈을 문 소켓으로 이어 붙이고 겹침을 검사합니다. 결과로 그레이박스 바닥과 벽(문 구멍 포함)을 짓습니다.

## 사용

1. 프로젝트의 `Plugins/ProcLevelGen`에 넣고 에디터를 열면 빌드됩니다.
2. 레벨에 `PLGGenerator` 액터를 놓습니다.
3. Details 패널의 **Preview** / **Preview New Seed**로 에디터에서 바로 확인합니다. 미리보기는 레벨에 저장되지 않습니다.
4. 플레이하면 서버가 새 시드를 골라 복제하고, 모든 컴퓨터가 같은 맵을 만듭니다. 시드를 고정하려면 `bNewSeedEachRun`을 끄세요.
5. 생성이 끝나면 `OnGenerated`가 호출됩니다. 콘텐츠는 `GetSpawnSlots(태그)`로 모듈의 스폰 포인트를 받아 배치합니다. 무작위 선택에는 시드에서 만든 `MakeContentStream()`을 씁니다. 책상 같은 고정 소품은 모듈의 `Props`로 넣으면 생성기가 같이 짓습니다.

시작 모듈은 항상 생성기 위치(원점)에 놓이므로, 플레이어 시작 지점은 시작 모듈 안에 고정해 두면 됩니다.

## 파일

| 파일 | 내용 |
|---|---|
| `PLGTypes.h` | 모듈·소켓·테마·배치 결과 구조체 |
| `PLGLayout.h/.cpp` | 조립 알고리즘 (엔진 객체에 의존하지 않는 순수 함수), 규격 검사, 샘플 키트 |
| `PLGGenerator.h/.cpp` | 레벨 액터: 시드 복제, 그레이박스 빌드, 에디터 미리보기 |
| `Docs/ModuleSpec.md` | 모듈 규격: 피벗, 그리드, 벽, 소켓, 생성 순서 |

로그: `LogProcLevelGen` (`-LogCmds="LogProcLevelGen Verbose"`로 모듈별 위치까지).
