# ColorLight 5A-75B CLI 도구 (`main.py`) 사용 설명서

이 도구는 `colorlight_module.py`를 사용하여 LED 패널에 텍스트를 출력하는 명령행 인터페이스(CLI) 프로그램입니다. 사용자는 다양한 옵션을 통해 출력 내용을 실시간으로 변경할 수 있습니다.

## 1. 기본 실행 방법

가장 기본적인 실행 방법입니다. 별도의 인자가 없으면 미리 설정된 기본값으로 전송됩니다.
이더넷 패킷 직접 제어를 위해 반드시 `sudo` 권한이 필요합니다.

```bash
sudo python3 main.py
```

- **기본 설정값:**
  - 텍스트: `Hello!`
  - 인터페이스: `en5`
  - 해상도: `160x80`
  - 색상 순서: `BGR`
  - 글자 크기: `30`
  - 글자 색상: `white`
  - 배경 색상: `black`

## 2. 주요 커맨드라인 인자 (Arguments)

| 인자 | 단축키 | 설명 | 기본값 |
| :--- | :--- | :--- | :--- |
| `--text` | `-t` | 출력할 문자열 | `Hello!` |
| `--interface` | `-i` | 네트워크 인터페이스 이름 | `en5` |
| `--width` | `-W` | 패널 가로 해상도 | `160` |
| `--height` | `-H` | 패널 세로 해상도 | `80` |
| `--font-size` | `-fs` | 글자 크기 (픽셀) | `30` |
| `--color-order`| `-co` | 색상 배열 (RGB, BGR, GRB 등) | `BGR` |
| `--text-color` | - | 글자 색상 (이름 또는 HEX) | `white` |
| `--bg-color` | - | 배경 색상 (이름 또는 HEX) | `black` |
| `--brightness` | `-b` | 밝기 (0-100) | `100` |
| `--gamma` | `-g` | 감마 보정값 | `1.0` |
| `--loop` | `-l` | 무한 반복 전송 모드 활성화 | `False` |

## 3. 핵심 명령어 예시 (Best 5)

다음은 실제 사용 환경에서 가장 자주 쓰이는 5가지 명령어 예시와 설명입니다.

### 1) 기본 설정으로 즉시 출력
```bash
sudo python3 main.py
```
*   **설명**: 아무런 인자 없이 실행하면 기본 네트워크 인터페이스(`en5`)를 통해 `160x80` 사이즈 패널에 흰색 글씨로 "Hello!"를 한 번 출력합니다. 시스템 설정이 기본값과 일치할 때 가장 빠르게 테스트할 수 있는 방법입니다.

### 2) 커스텀 문구와 색상 적용
```bash
sudo python3 main.py --text "Welcome" --text-color yellow --bg-color "#111111" --font-size 40
```
*   **설명**: "Welcome"이라는 문구를 노란색으로, 배경은 짙은 회색(`#111111`)으로 설정하여 출력합니다. 글자 크기를 40으로 키워 시인성을 높였습니다. 텍스트 컬러는 이름(yellow)이나 HEX 코드 모두 사용 가능합니다.

### 3) 화면 유지 (무한 반복 모드)
```bash
sudo python3 main.py --text 'System OK!' --loop
```
*   **설명**: `--loop` 옵션을 추가하면 프로그램이 종료되지 않고 패킷을 계속해서 재전송합니다. 일부 리시빙 카드는 신호가 끊기면 화면을 즉시 끄기 때문에, 정지된 문구를 계속 띄워두고 싶을 때 반드시 사용해야 하는 옵션입니다.

### 4) 패널 사양에 맞춘 최적화 (128x64 해상도)
```bash
sudo python3 main.py -t "LED ON" -W 128 -H 64 -co RGB -i en0
```
*   **설명**: 기본값과 다른 사양의 패널(`128x64`)이나 다른 인터페이스(`en0`)를 사용할 때의 예시입니다. 또한 색상 순서가 `RGB`인 패널에 맞춰 `-co` 옵션을 지정했습니다. 단축키(`-t`, `-W`, `-H`)를 사용하여 더 짧게 입력할 수 있습니다.

### 5) 시인성 및 밝기 최적화
```bash
sudo python3 main.py --text "Night Mode" --brightness 30 --gamma 2.2 --text-color red
```
*   **설명**: 야간 등 너무 밝은 환경이 부적절할 때 밝기를 30%로 낮추고, 색감이 더 진하게 표현되도록 감마값을 2.2로 조정했습니다. 특히 빨간색(`red`) 텍스트는 야간에 눈의 피로도를 줄이는 데 유용합니다.

## 4. 고급 옵션 활용

### 밝기 제어
눈이 부시거나 전력 소모를 줄여야 할 때 사용합니다.
```bash
sudo python3 main.py --brightness 50
```

### 색상 순서 맞추기
화면의 빨간색과 파란색이 바뀌어 나온다면 이 옵션을 변경해 보세요.
```bash
sudo python3 main.py --color-order RGB
```

## 6. Debian / Linux 환경에서의 사용

이 도구는 데비안 기반 리눅스(Ubuntu, Raspberry Pi OS 등)에서도 정상적으로 작동합니다. 몇 가지 차이점만 유의해 주세요.

### 네트워크 인터페이스 이름
- macOS는 `en0`, `en5` 등을 사용하지만, 데비안은 보통 `eth0`, `enp0s3`, `usb0` 등의 이름을 사용합니다. `ip link` 또는 `ifconfig` 명령어로 이름을 먼저 확인하세요.

### 파이썬 패키지 설치 (`externally-managed-environment` 발생 시)
최신 데비안/라즈비안 버전에서는 시스템 환경 보호를 위해 `pip install`이 차단될 수 있습니다. 다음 두 가지 방법 중 하나를 선택하세요.

**방법 A: 가상 환경(venv) 사용 (권장)**
```bash
# 가상 환경 생성 및 활성화 (현제 폴더를 가상환경으로 만들기)
python3 -m venv .venv # 하위 폴더에 .venv 폴더 생성
source .venv/bin/activate # 가상환경 활성화

# 라이브러리 설치
pip install -r requirements.txt
```
> [!TIP]
> **매번 `source`를 하기 번거롭다면?**  
> 가상 환경을 활성화하지 않고도 가상 환경 내의 파이썬 실행 파일을 직접 지정하여 실행하면 됩니다.
> ```bash
> sudo ./.venv/bin/python3 main.py --text "Hello"
> ```
> 이렇게 실행하면 자동으로 가상 환경에 설치된 패키지들을 사용하게 됩니다.
*이후 실행 시에도 `source venv/bin/activate`를 먼저 입력하거나, `./venv/bin/python3 main.py` 형식으로 실행하세요.*

**방법 B: 시스템 패키지 매니저(`apt`) 사용**
가상 환경 없이 시스템 전체에 설치하고 싶을 때 사용합니다.
```bash
sudo apt install python3-scapy python3-pil python3-numpy
```

### 필수 시스템 패키지
`scapy`가 로우 패킷을 보내기 위해 추가 도구가 필요합니다.
```bash
sudo apt update
sudo apt install tcpdump libpcap-dev
```

### 폰트 경로
리눅스에는 기본적으로 macOS용 폰트가 없습니다. 코드는 일반적인 리눅스 폰트 경로(`liberation`, `dejavu` 등)를 확인하도록 업데이트되어 있습니다. 만약 폰트 관련 오류가 난다면 나눔 고딕 등을 설치해 보세요.
```bash
sudo apt install fonts-nanum
```

### 절대 경로로 실행하기 (어느 위치에서든 실행)
스크립트가 `/home/fpp/colorlight/` 폴더에 있고 가상 환경이 `.venv`라면, 터미널 어디에서든 다음과 같이 한 줄로 실행할 수 있습니다.
```bash
sudo /home/fpp/colorlight/.venv/bin/python3 /home/fpp/colorlight/main.py --text "Running" --loop
```

### 부팅 시 자동 실행 + 키보드 제어 가능하게 설정 (Systemd TTY)
`systemd`로 자동 실행하면서도 연결된 USB 키보드로 직접 조작(Ctrl+C 등)하거나 입력을 주고 싶다면, 서비스를 **가상 터미널(TTY)**에 할당해야 합니다.

1. **서비스 파일 수정 (`sudo nano /etc/systemd/system/colorlight.service`)**
   기존 내용에 아래 TTY 관련 설정을 추가합니다:
   ```ini
   [Unit]
   Description=Colorlight LED Controller with TTY
   After=network.target

   [Service]
   WorkingDirectory=/home/fpp/colorlight
   ExecStart=/home/fpp/colorlight/.venv/bin/python3 /home/fpp/colorlight/main.py --text "Interactive" --loop
   
   # TTY 연결 설정 (키보드 입력 가능하게 함)
   StandardInput=tty
   StandardOutput=tty
   TTYPath=/dev/tty1
   TTYReset=yes
   TTYVHangup=yes
   
   Restart=always
   User=root

   [Install]
   WantedBy=multi-user.target
   ```

2. **주의 사항**
   - 위 설정은 서비스를 `/dev/tty1`(기본 모니터 화면)에 강제로 할당합니다.
   - 부팅 시 해당 화면에 프로그램이 바로 나타나며, 연결된 키보드로 조작이 가능해집니다.
   - 일부 리눅스 배포판에서는 기본 로그인 프롬프트(getty)와 충돌할 수 있으므로 테스트가 필요합니다.

3. **서비스 활성화 및 시작**
   ```bash
   # 서비스 파일 새로고침
   sudo systemctl daemon-reload
   
   # 부팅 시 자동 시작 등록
   sudo systemctl enable colorlight.service
   
   # 지금 즉시 시작
   sudo systemctl start colorlight.service
   ```

4. **상태 확인 및 로그 보기**
   ```bash
   # 상태 확인
   sudo systemctl status colorlight.service
   
   # 실행 로그(에러 확인 등)
   sudo journalctl -u colorlight.service -f
   ```

### 서비스 vs 대화형 실행 (키보드 입력 관련)

`systemd`나 `crontab`으로 연동하면 프로그램이 **백그라운드(Background)**에서 돌아가므로, 직접 연결된 USB 키보드로 `Ctrl+C`를 누르거나 입력을 줄 수 없습니다.

- **키보드 제어가 필요한 경우**: 현장에서 직접 키보드로 텍스트를 입력하거나 프로그램을 껐다 켜야 한다면, 자동 실행보다는 사용자가 직접 로그인한 뒤 실행하거나 `.bashrc` 끝에 추가하는 방식이 유리합니다.
- **순수 자동화가 필요한 경우**: 사람이 개입하지 않고 부팅과 동시에 항상 켜져 있어야 한다면 `systemd`가 가장 안정적입니다.

> [!TIP]
> 만약 `systemd`로 실행 중인데 프로그램을 끄고 싶다면, 다른 PC에서 SSH로 접속하거나 키보드로 직접 제어할 수 없으므로 다음 명령어를 사용해야 합니다:
> ```bash
> sudo systemctl stop colorlight.service
> ```

#### 1) `.bashrc` 활용 (로그인 시 실행)
`.bashrc`는 엄밀히 말하면 '부팅' 시가 아니라, 사용자가 **터미널을 열거나 로그인할 때** 실행됩니다.
- **방법**: `nano ~/.bashrc` 맨 아랫줄에 명령어 추가
- **주의**: 터미널을 열 때마다 실행되므로, 여러 개의 프로그램이 중복 실행될 위험이 있습니다. 배경 서비스로 돌리기에는 부적합합니다.

#### 2) `crontab` 활용 (간편한 부팅 시 실행)
`systemd` 설정이 복잡하다면, 리눅스의 예약 작업 도구인 `cron`을 사용할 수 있습니다.
- **설정 방식**: `sudo crontab -e` 명령 실행 후 맨 아래에 추가:
  ```bash
  @reboot /home/fpp/colorlight/.venv/bin/python3 /home/fpp/colorlight/main.py --text "Booting..." --loop
  ```
- **특징**: 부팅 직후에 root 권한으로 딱 한 번 실행됩니다. 간단한 자동화에 아주 유용합니다.

#### 3) `systemd` (가장 권장됨)
- **특징**: 프로그램이 에러로 죽어도 자동으로 다시 실행해주며, 로그 관리가 쉽습니다. 실제 운영 환경에서는 이 방식을 가장 권장합니다.

## 9. 자주 묻는 질문 (FAQ)

**Q: `dquote>` 라는 메시지가 뜨면서 실행이 안 됩니다.**
A: 따옴표(`"`) 내부에 느낌표(`!`)가 들어가면 발생하는 셸 오류입니다. 텍스트를 홑따옴표(`'`)로 감싸서 실행하세요.

**Q: `Operation not permitted` 오류가 납니다.**
A: 이더넷 패킷을 직접 생성하려면 관리자 권한이 필요합니다. 명령어 앞에 `sudo`를 붙였는지 확인하세요.

**Q: 글자가 화면 중앙에 오지 않습니다.**
A: `--width`와 `--height`가 실제 LED 패널 구성과 일치하는지 확인하세요.
