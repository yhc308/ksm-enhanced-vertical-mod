# KSM Enhanced Vertical Mod (Windows x64 전용)

**K-Shoot Mania 2.1.0을 세로 화면에서도 즐길 수 있도록 확장한 비공식 모드입니다.**
버티컬 플레이, 스킨 프리셋, 화면 커스터마이징, EX 스코어, HARD+APS, 판정 타이밍 설정과 일부 ASIO 기능을 제공합니다.

[다운로드](https://github.com/yhc308/ksm-enhanced-vertical-mod/releases/tag/v0.5.2-beta2) · [소스 구조 및 빌드 안내](../DEVELOPMENT.md) · [전체 소스](https://github.com/yhc308/ksm-enhanced-vertical/tree/enhanced-vertical) · [문제 제보](https://github.com/yhc308/ksm-enhanced-vertical-mod/issues)

▶ English : **[Open](README_EN.md)**<br>
▶ 日本語 : **[開く](README_JA.md)**<br>
▶ 한국어 : **[바로가기](README_KO.md)** **(지금 열고있음)**

## 주요 기능

### ◆ 버티컬 플레이

세로 화면에 맞춘 레인과 HUD 배치로 플레이할 수 있습니다. 기존 가로 플레이도 지원하며, Vertical Option에서 화면 관련 설정을 조정할 수 있습니다.
버티컬을 위해 마련한 UI 이미지와 추가 기능들도 가로모드를 지원합니다.

### ◆ 판정 카운터

가로·세로 모드 각각의 화면에 맞춘 판정 인디케이터를 제공합니다.

### ◆ 세부 판정 추가 및 EX 스코어

P-CRITICAL 판정과 EX 스코어, 리절트 표시, 추가 기록 저장을 지원합니다. EX 기록은 기존 게임 기록과 별도로 보관하며, 해당 채보의 EX 스코어 이론치도 저장합니다.

### ◆ 뷰포트 커스텀 / 사용자 스킨

게임 화면의 프리뷰를 보면서 배경 밝기, 레인과 노트의 굵기, 판정 인디케이터, 스킨 등을 조정할 수 있습니다.

기본 이미지는 `imgs/`, 사용자 스킨은 `skins/<skin name>/`에서 읽습니다. Viewport Custom에서 스킨을 선택하고 프리뷰로 확인할 수 있습니다.
스킨에 누락된 이미지는 `imgs/`의 기본 이미지로 대체할 수 있으므로 모든 이미지를 교체할 필요는 없습니다.

### ◆ HARD+APS 게이지

HARD 게이지 실패 시 NORMAL로 전환하는 APS 기능을 제공합니다. 게이지와 퍼센트 표시가 함께 전환되며, APS 배너에 전환 연출을 적용했습니다.

### ◆ 판정 타이밍 설정

전용 테스트 채보를 반복 플레이하면서 판정과 표시 설정을 조정할 수 있습니다.

## 문제 제보

문제가 발생하면 [Issues](https://github.com/yhc308/ksm-enhanced-vertical-mod/issues)에 사용 중인 Windows 버전, 게임·모드 버전, 재현 방법을 알려주세요. 스크린샷이나 오류 로그도 확인에 도움이 됩니다.

- 게임 로그: `<game folder>/log/session-<number>-pid<PID>/events-0.log`
- 설치기 결과: `%TEMP%/KsmEnhancedVerticalSetup-last-result.json`

## 크레딧 및 라이선스

Codex와 협업하여 제작했으며, 사용된 이미지와 음원은 모두 수작업으로 제작되었습니다.

원본 게임: [K-Shoot Mania](https://www.kshootmania.com/) / [공식 소스](https://github.com/kshootmania/ksm-v2)

- Special thanks: [KSM 개발자](https://x.com/masaka_k)
- UI 스킨 제작 및 모드 제작 기여: [chesia_game](https://x.com/chesia_game)
- 프리뷰·판정 타이밍 설정용 음악 제공: [Hyung](https://open.spotify.com/artist/2soZF7u1mCALXnpwCbyGvh?si=7gvRzWqdTeqV2lEYtfmo5w)

이 프로젝트는 비공식 모드입니다. 원본의 저작권 고지와 라이선스를 유지합니다.
