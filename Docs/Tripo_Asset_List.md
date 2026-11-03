# Tripo 에셋 목록 (목표 예산 4,000 크레딧, 2026-11 재계산)

> 크레딧은 [Tripo 제품 문서 요금 및 크레딧](https://www.tripo3d.ai/ko/product-doc/getting-started/pricing-credits) (2026년 10월) 기준으로 다시 계산했습니다.
> 이전 버전은 "생성 1회 ≈ 25크레딧"으로 잡아 실제보다 크게 낮았습니다.
> **텍스처 비용(T)은 해상도 등급마다 달라 문서에 숫자가 없습니다.** 첫 텍스처 생성 때 화면에 표시되는 값을 확인해 아래 표에 채우세요.

## 1. 예산 요약

### 1.1 작업별 크레딧 (공식)

| 작업 | 크레딧 | 메모 |
|---|---|---|
| Smart Mesh P2.0 | 100 / 생성 1회 | 1회에 폴리곤 수를 달리한 변형 최대 4개, 크레딧 동일 |
| HD Model H3.1 | 15 / 생성 1회 | 하이폴리, 텍스처 동시 생성 가능 |
| Smart UV | 20 / 언랩 1회 | |
| 리깅 | 20 / 모델 | |
| 애니메이션 | 20 / 모델 | |
| 리토폴로지 (일반) | 10 / 모델 | |
| 이미지 편집 | 10 / 1회 | 컨셉 정리, 배경 제거, 포즈 맞추기 |
| 텍스처 | **T** / 1회 | 2K < 4K < 8K. 생성 전 표시 |

- 생성에 실패하면 크레딧이 차감되지 않는다.

### 1.2 에셋 1개당 예상 (텍스처 제외)

| 분류 | 작업 구성 | 1개당 |
|---|---|---|
| 리깅 캐릭터 (플레이어·몬스터) | P2.0 ×2 (200) + Smart UV 20 + 리깅 20 + 애니메이션 20 | **260 + 2T** |
| 운반 골동품 — P안 | P2.0 평균 ×1.5 (150) | **150 + T** |
| 운반 골동품 — H안 | H3.1 ×2 (30) + 리토폴로지 10 | **40 + T** |
| 랜드마크·장식 (정적) | H3.1 ×2 (30) | **30 + T** |

- 리깅 캐릭터의 "×2"는 첫 생성(변형 4개) + 재생성 1회. 텍스처는 재시도 1회를 포함해 2회.
- 운반 골동품 H안은 하이폴리를 리토폴로지로 3천 tri 이하까지 줄이는 방식. **품질과 텍스처 유지 여부가 검증되지 않았다.** 아래 1.5의 테스트로 먼저 확인한다.
- 랜드마크·장식은 리깅이 없고 고정 배치라 H3.1이 맞다. 랜드마크는 Nanite 사용.

### 1.3 전체 합계

| 분류 | 개수 | P안 (골동품 P2.0) | H안 (골동품 H3.1) | 텍스처 횟수 |
|---|---|---|---|---|
| 플레이어 캐릭터 | 4 | 1,040 | 1,040 | 8 |
| 몬스터 | 2 | 520 | 520 | 4 |
| 운반 골동품 | 20 | 3,000 | 800 | 20 |
| 랜드마크 | 6 | 180 | 180 | 6 |
| 장식 소품 | 10 | 300 | 300 | 10 |
| 컨셉 이미지 편집 | 약 40회 | 400 | 400 | — |
| **소계 (텍스처 제외)** | **42** | **5,440** | **3,240** | **48** |
| 텍스처 (48 × T) | | 예: T=20이면 +960, T=40이면 +1,920 | 같음 | |
| 여유분 (약 10%) | | +600 | +400 | |

> T 값은 예시입니다. 실제 값을 확인한 뒤 고치세요.

**결론**
- **P안은 4,000크레딧으로 불가능.** 텍스처를 빼도 5,440.
- **H안도 텍스처를 넣으면 4,000을 넘을 가능성이 높다.** (T=20이면 약 4,600)
- 4,000크레딧에 맞추려면 **우선순위 C를 무료 에셋으로 대체**하는 것을 기본 계획으로 한다. (1.4)

### 1.4 우선순위별 합계 (H안, 텍스처 제외)

| 우선순위 | 포함 에셋 | 크레딧 | 텍스처 횟수 | 누적 |
|---|---|---|---|---|
| A | 캐릭터 4, 촛대 집사, 골동품 7, 랜드마크 4, 이미지 편집 약 25회 | 1,040 + 260 + 280 + 120 + 250 = **1,950** | 21 | 1,950 + 21T |
| B | 종 달린 말, 골동품 8, 랜드마크 2, 이미지 편집 약 10회 | 260 + 320 + 60 + 100 = **740** | 12 | 2,690 + 33T |
| C | 골동품 5, 장식 10, 이미지 편집 약 5회 | 200 + 300 + 50 = **550** | 15 | 3,240 + 48T |

- T=20 가정 시 A+B = 약 3,350. **A+B까지는 4,000 안에 들어오고 C는 남는 만큼만.**
- 협업으로 받는 Tripo 이용 환경의 크레딧으로 캐릭터 4종(약 1,040 + 8T)을 만들면 자체 예산에 여유가 생긴다. 협업 환경에서 만든 모델의 상업적 이용 조건은 따로 확인.

### 1.5 예산을 확정하기 전에 할 테스트

1. 캐릭터 1종(PC_Bear)을 P2.0으로 생성 → 텍스처 1회 → **T 값 확인** 후 이 표의 예시 값을 교체.
2. 골동품 1종(예: VAL_Teacup)을 **P2.0과 H3.1 + 리토폴로지 두 방식으로** 만들어 품질·폴리곤·텍스처 유지 비교 → P안/H안 결정.
3. 결정한 안으로 1.3과 1.4 다시 계산.

**만들지 않는 것**: 벽, 바닥, 계단, 문틀, 선반 → 언리얼 모델링 모드나 무료 키트 사용.

## 2. 생성 규칙

### 공통 스타일 문구 (모든 프롬프트 끝에 붙이기)
```
, stylized vintage dreamcore, hand-crafted antique toy look, soft rounded shapes, clean silhouette, single object, game asset, neutral lighting, no background, no base
```

### 캐릭터 공통 문구 (플레이어 4종에 동일하게)
```
cute plush clown doll character, 2.5 heads tall chibi proportions, round soft body, short stubby arms and legs, standing in A-pose, ruffled clown collar, cone party hat with pompom, sewn buttons, visible stitched fabric seams, full body, symmetrical
```

### 작업 원칙
- **우선순위 A → B → C 순서로 생성.** 크레딧이 모자라면 C부터 무료 에셋으로 대체.
- **이미지 기반 생성 권장**: 텍스트보다 스타일이 일정하게 나옴. 단, 다른 작가의 그림을 그대로 넣기보다 **직접 그린 스케치나 직접 만든 컨셉 이미지**를 입력으로 쓰는 게 상업적으로 안전함.
- 플레이어 4종은 **같은 체형·A포즈**로 뽑아서 언리얼 IK Retargeter로 애니메이션 한 세트를 공유.
- 로우폴리 / 리토폴로지 옵션 사용. 목표: 캐릭터 1만 tri 이하, 골동품 3천 tri 이하.
- 충돌은 언리얼에서 단순 콜리전(박스, 캡슐, 컨벡스)으로 따로 지정.
- 생성 후 바로 `Content/Dream/` 아래 분류별 폴더로 정리하고 이 문서의 상태 칸을 갱신.

## 3. 플레이어 캐릭터 (리깅 O)

| ID | 우선 | 이름 | 프롬프트 (캐릭터 공통 문구 + 아래) | 상태 |
|---|---|---|---|---|
| PC_Mouse | A | 쥐 광대 | `white mouse head with round pink ears and red nose, red clown jumpsuit with yellow buttons, blue ruffled collar, striped red and yellow hat` | ☐ |
| PC_Bear | A | 곰 광대 | `brown teddy bear head, half blue checkered half rust red clown suit, white ruffled collar, brown cone hat with yellow pompom` | ☐ |
| PC_Frog | A | 개구리 광대 | `green frog head with big round eyes, yellow and light blue harlequin diamond clown suit, cream ruffled collar, yellow cone hat with blue pompoms` | ☐ |
| PC_Rabbit | A | 토끼 광대 | `cream rabbit head with long floppy ears, pastel pink and lavender clown suit with star patches, white ruffled collar, purple hat` | ☐ |

## 4. 몬스터 (리깅 O)

| ID | 우선 | 이름 | 프롬프트 | 상태 |
|---|---|---|---|---|
| MON_Candle | A | 촛대 집사 | `tall thin eerie butler creature, brass candlestick with lit candle instead of a head, wide brass dish around the neck, red velvet tailcoat covered in dripping white candle wax, long thin dark legs, holding small brass hand bells in long black fingers, standing in A-pose` | ☐ |
| MON_BellHorse | B | 종 달린 말 | `eyeless bronze horse sculpture creature with green patina, body made of hanging brass bells of many sizes, thin skeletal legs, long drooping neck, standing pose` | ☐ |

> 종 달린 말은 사족보행이라 Tripo 자동 리깅이 안 맞을 수 있음. 안 되면 정적 메시 + 다리만 간단히 애니메이션하거나, 종만 흔들리게 처리.

## 5. 운반 골동품 (리깅 X)

크기 등급: **S** = 한 손, 가벼움 / **M** = 혼자 들 수 있지만 무거움 / **L** = 2명 필요
가격 단위: 별가루

| ID | 우선 | 이름 | 등급 | 내구 | 가격 | 프롬프트 | 상태 |
|---|---|---|---|---|---|---|---|
| VAL_CoupeGlass | A | 크리스탈 쿠페 잔 | S | 매우 약함 | 1,200 | `crystal champagne coupe glass with gold rim and blue and gold diamond gems band, octagonal base` | ☐ |
| VAL_Teacup | A | 도자기 찻잔 세트 | S | 약함 | 600 | `porcelain teacup on saucer, pastel floral painting, gold handle` | ☐ |
| VAL_RabbitKey | A | 토끼 머리 열쇠 | S | 강함 | 800 | `ornate antique silver key with rabbit head handle, baroque scroll details` | ☐ |
| VAL_PocketWatch | B | 회중시계 | S | 보통 | 700 | `open brass pocket watch with moon phase dial and chain` | ☐ |
| VAL_Perfume | B | 향수병 | S | 매우 약함 | 900 | `antique cut glass perfume bottle with pink liquid and gold atomizer bulb` | ☐ |
| VAL_MusicBoxSmall | B | 작은 오르골 | S | 보통 | 750 | `small wooden music box with tiny carousel horse on top, gold trim` | ☐ |
| VAL_DollHead | C | 유리 눈 인형 머리 | S | 약함 | 500 | `porcelain doll head with glass eyes and painted rosy cheeks, cracked bonnet` | ☐ |
| VAL_GoldEgg | B | 금박 달걀 | S | 보통 | 1,500 | `jeweled golden decorative egg with blue enamel and pearl details` | ☐ |
| VAL_OilLamp | A | 황동 오일 램프 | M | 약함 | 1,400 | `antique brass oil lamp with tall clear glass chimney, amber glass oil reservoir, curled brass handle` | ☐ |
| VAL_Reliquary | A | 붉은 하트 성물함 | M | 약함 | 2,500 | `red glass heart vessel on ornate gold stand, gold flame crown on top, gold lilies around, thorny gold wire wrapped around the heart` | ☐ |
| VAL_Lantern | A | 유리 랜턴 | M | 약함 | 1,800 | `hanging baroque brass lantern with etched blue glass star panels, scrollwork top` | ☐ |
| VAL_ClownFigure | B | 도자기 광대 인형 | M | 약함 | 1,100 | `porcelain clown figurine sitting, pastel harlequin suit, ruffled collar` | ☐ |
| VAL_Globe | C | 별자리 지구본 | M | 보통 | 1,000 | `antique celestial globe with constellations on brass stand` | ☐ |
| VAL_FaceMirror | B | 소녀 얼굴 손거울 | M | 약함 | 1,300 | `ornate hand mirror shaped like a girl's face with curly baroque hair frame, long handle` | ☐ |
| VAL_Candelabra | C | 은 촛대 세트 | M | 강함 | 900 | `three-arm silver candelabra with melted candles and wax drips` | ☐ |
| VAL_CarouselHorse | A | 회전목마 말 | L | 보통 | 3,500 | `carousel horse with pastel saddle blanket, flowers, gold twisted pole through body` | ☐ |
| VAL_GrandMusicBox | B | 대형 오르골 | L | 약함 | 4,000 | `large ornate music box with open lid, dancing ballerina, mirror inside, gold feet` | ☐ |
| VAL_Gramophone | B | 축음기 | L | 보통 | 3,000 | `antique gramophone with large brass flower horn and wooden base` | ☐ |
| VAL_Birdcage | C | 유리 돔 새장 | L | 매우 약함 | 3,200 | `glass dome birdcage with brass frame and porcelain bird inside` | ☐ |
| VAL_Painting | C | 금박 액자 그림 | L | 보통 | 2,800 | `large ornate gold frame with painting of a moon over carousel` | ☐ |

## 6. 랜드마크 (리깅 X)

| ID | 우선 | 이름 | 용도 | 프롬프트 | 상태 |
|---|---|---|---|---|---|
| LM_MusicBoxBed | A | 오르골 침대 | 탈출 지점 | `child's bed shaped like an open music box, quilted blanket, crank handle on side, small carousel canopy` | ☐ |
| LM_Carousel | A | 대형 회전목마 | 회전목마 홀 중앙 | `small indoor carousel with striped canopy, four animal seats, gold poles, round platform` | ☐ |
| LM_GrandClock | A | 괘종시계 | 시간 표시 | `tall grandfather clock with moon face, crooked shape, swinging pendulum` | ☐ |
| LM_Chandelier | B | 촛불 샹들리에 | 예배실 천장 | `large brass chandelier with many candles and crystal drops` | ☐ |
| LM_MoonDealer | B | 달 머리 골동품상 | 로비 NPC (정적) | `tall slender shopkeeper figure with crescent moon head, long dark coat with stars, sitting behind antique counter` | ☐ |
| LM_Wardrobe | A | 대형 서랍장 | 숨는 장소 | `tall antique wardrobe with carved doors and keyhole, slightly open` | ☐ |

## 7. 장식 소품 (리깅 X, 우선순위 C)

| ID | 이름 | 프롬프트 | 상태 |
|---|---|---|---|
| DEC_Balloons | 풍선 다발 | `bunch of pastel balloons tied with ribbon` | ☐ |
| DEC_Armchair | 안락의자 | `velvet armchair with curved wooden legs, faded rose fabric` | ☐ |
| DEC_RoundTable | 원형 탁자 | `small round wooden side table with lace cloth` | ☐ |
| DEC_BookStack | 책 더미 | `stack of old leather books, slightly tilted` | ☐ |
| DEC_ToyTrain | 장난감 기차 | `tin toy train locomotive, red and blue` | ☐ |
| DEC_StarMobile | 별 모빌 | `hanging mobile with paper stars and moon` | ☐ |
| DEC_HatStand | 모자걸이 | `wooden coat stand with clown hats hanging` | ☐ |
| DEC_Vase | 꽃병 | `pastel ceramic vase with dried roses and daisies` | ☐ |
| DEC_Umbrella | 우산꽂이 | `brass umbrella stand with striped umbrellas` | ☐ |
| DEC_EmptyCage | 빈 새장 | `hanging brass birdcage, empty, door open` | ☐ |

## 8. 생성 순서 체크리스트

1. ☐ GDD 0~1단계(물리 멀티 검증, 그레이박스 루프) 완료 — **이전에는 생성하지 않기**
2. ☐ 테스트 1개 생성 (PC_Bear)으로 스타일·폴리곤·리깅 품질 확인 후 공통 문구 조정. 텍스처 크레딧(T) 기록
3. ☐ 골동품 1개를 P2.0 / H3.1 + 리토폴로지로 비교해 P안·H안 결정 (1.5), 예산 표 갱신
4. ☐ 우선순위 A 전부 (캐릭터 4, 집사, 골동품 7, 랜드마크 4)
5. ☐ 우선순위 B
6. ☐ 남은 크레딧 확인 후 우선순위 C 또는 무료 에셋 대체
