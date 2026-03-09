# ColorLight 5A-75B Python 모듈 (`colorlight_module.py`) 사용 설명서

이 모듈은 Falcon Player(FPP)의 C++ 소스 코드(`ColorLight-5a-75.cpp/h`) 로직을 Python으로 충실히 이식한 라이브러리입니다. 이더넷 Raw 소켓을 통해 Colorlight 5A-75 리시빙 카드에 직접 제어 신호와 영상 데이터를 전송합니다.

## 1. 주요 기능
- **Raw Ethernet 통신**: 표준 TCP/IP를 거치지 않고 직접 이더넷 프레임을 조작하여 지연 시간을 최소화합니다.
- **감마 보정(Gamma Correction)**: LED 패널의 특성에 맞춰 밝기 곡선을 조정합니다.
- **색상 순서 변환**: RGB, BGR, GRB 등 다양한 패널의 색상 배열을 지원합니다.
- **동기화 제어**: 모든 픽셀 데이터 전송 후 Sync 패킷을 보내 화면을 한 번에 갱신합니다.

## 2. 요구 사항
- **Python**: 3.7 이상
- **라이브러리**: `scapy`, `numpy`, `pillow`
- **권한**: Raw Socket 제어를 위해 실행 시 **루트 권한(sudo)**이 필요합니다.

## 3. 클래스 개요: `ColorLight5a75Controller`

### 초기화 (`__init__`)
```python
controller = ColorLight5a75Controller(interface, width=160, height=80, color_order="BGR")
```
- `interface`: 패킷을 송출할 NIC 이름 (예: `en5`, `eth0`).
- `width/height`: LED 패널의 전체 가로/세로 해상도.
- `color_order`: 패널의 물리적 색상 순서 (기본값 BGR).

### 주요 메서드

#### `output_frame(image)`
가장 많이 사용되는 메서드로, 아래 세 과정을 순차적으로 수행하여 한 프레임을 화면에 표시합니다.
1. `send_brightness()`: 현재 설정된 밝기 정보 전송.
2. `send_pixel_data(image)`: 이미지 데이터를 행 단위로 쪼개어 전송.
3. `send_sync()`: 화면을 갱신하라는 명령 전송.

#### `set_brightness(brightness)`
- 밝기를 0에서 100 사이의 값으로 설정합니다. 내부적으로는 리시빙 카드가 이해하는 0~255 범위로 변환되어 전송됩니다.

#### `set_gamma(gamma)`
- 감마 값을 설정합니다. 1.0은 보정 없음, 2.2는 일반적인 디스플레이 표준입니다. 설정 시 내부 감마 테이블(256단계)이 즉시 갱신됩니다.

## 4. 상세 동작 원리 (C++ 코드 대조)

### 패킷 구조 (Protocol Specs)
각 패킷은 이더넷 헤더의 **EtherType** 필드를 특수하게 활용합니다 (C++ 코드의 `CL_PACKET_TYPE_OFFSET` 로직 반영).

- **EtherType [12:13]**: `Packet Type` (예: 0x55) + `Data[0]` (데이터의 첫 바이트)
  - 이 방식은 Colorlight 리시빙 카드가 패킷을 빠르게 식별하기 위한 규격입니다.
  - 파이썬 코드에서는 `_build_ether_header` 함수에서 이 구조를 생성합니다.

### 픽셀 데이터 전송 (`send_pixel_data`)
- 한 줄(Row)의 데이터가 약 487픽셀(`CL_MAX_PIXL_PER_PACKET`)을 넘으면 여러 개의 패킷으로 나누어 보냅니다.
- 각 패킷에는 `Row 번호`, `행 내 시작 위치(Offset)`, `픽셀 개수(Count)`가 헤더로 포함됩니다.

### 동기화 (`send_sync`)
- 모든 픽셀 데이터가 카드 내부 버퍼에 쌓인 후, 이 패킷이 도착해야만 실제 LED 패널로 빛이 출력됩니다.
- 펌웨어 버전(`firmware_version`)이 13 이상으로 설정되면, 신뢰성을 위해 동기화 패킷을 두 번 전송합니다.

## 5. 외부 프로젝트 연동 예시

다른 파이썬 스크립트에서 이 모듈을 가져와 사용하는 방법입니다.

```python
from colorlight_module import ColorLight5a75Controller
from PIL import Image

# 1. 컨트롤러 생성
led = ColorLight5a75Controller(interface="en5", width=160, height=80, color_order="BGR")

# 2. 옵션 설정
led.set_brightness(80)  # 밝기 80%
led.set_gamma(2.2)      # 감마 보정 적용

# 3. 이미지 준비 (Pillow 활용)
img = Image.open("photo.png").resize((160, 80)).convert("RGB")

# 4. 출력 명령 (무한 반복 시 화면 유지)
try:
    while True:
        led.output_frame(img)
except KeyboardInterrupt:
    print("종료")
```

## 6. 문제 해결
- **Permission Denied**: `sudo`를 붙여서 실행했는지 확인하세요.
- **No output**: `interface` 이름이 실제 카드가 연결된 NIC와 일치하는지 (`ifconfig` 명령으로 확인) 체크하세요.
- **Wrong Colors**: 색상이 이상하게 나온다면 `color_order`를 `RGB`, `GRB` 등으로 변경해 가며 확인해 보세요.
