# 라즈베리파이 설치 및 실행 가이드 (Raspberry Pi Lite)

라즈베리파이 Lite(GUI 없음) 환경에서 이 프로젝트를 문제없이 실행하기 위한 가이드입니다.

## 1. 전제 조건 및 시스템 의존성 설치
라즈베리파이 Lite는 최소 패키지로 구성되어 있어 이미지 처리와 패킷 전송을 위한 추가 라이브러리 설치가 필요합니다.

```bash
sudo apt update
sudo apt install -y python3-pip libopenjp2-7 libtiff6 python3-dev libpcap-dev
```

## 2. 프로젝트 준비 및 라이브러리 설치
프로젝트 폴더 내에서 필요한 Python 패키지를 설치합니다.

```bash
# 프로젝트 폴더로 이동 (예시)
# cd colorlight_v3_working

# 필수 라이브러리 설치
pip3 install -r requirements.txt
```

## 3. 실행 방법 (권한 필수)
네트워크의 Raw Socket에 직접 접근하므로 반드시 **sudo** 권한이 필요합니다.

```bash
# 관리자 권한으로 실행
sudo python3 main.py
```

## 4. 실행 전 체크리스트
- **네트워크 인터페이스**: 라즈베리파이의 유선 랜 포트는 보통 `eth0`입니다. 현재 코드는 `eth0`를 최우선으로 찾도록 설정되어 있습니다.
- **폰트 파일**: `./font/GothicBold.ttf` 파일이 해당 경로에 존재해야 한글 및 텍스트 출력이 정상적으로 이루어집니다.
- **설정 파일**: `LED_Config` 파일을 통해 기본 해상도나 밝기를 미리 설정할 수 있습니다.

## 5. 트러블슈팅
- **Operation not permitted**: `sudo`를 붙이지 않고 실행했을 때 발생합니다.
- **Interface not found**: 랜선이 연결되어 있지 않거나 인터페이스 이름이 다를 경우 발생합니다. `ifconfig`로 이름을 확인하세요.
- **OSError: cannot open resource**: 폰트 파일 경로가 틀렸을 때 발생합니다.

---
*마지막 업데이트: 2026-03-12*
