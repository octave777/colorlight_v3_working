# ColorLight v2 - sudo 없이 실행하는 방법

네트워크 소켓 제어 권한(Raw Socket)이 파이썬 실행 파일에 필요하기 때문에, 기본적으로는 `sudo`가 필요합니다.
하지만 아래 명령어를 통해 파이썬 실행 파일에 네트워크 제어 권한(capability)을 부여하면 `sudo` 없이 실행할 수 있습니다.

## 1. 권한 부여 명령어 실행
이 명령어는 1회만 실행하면 됩니다. (가상 환경을 새로 만들거나 파이썬 버전을 업데이트하면 다시 실행해야 합니다.)

```bash
sudo setcap 'cap_net_raw,cap_net_admin+eip' $(readlink -f .venv/bin/python3)
```

> **참고**: 관리자 비밀번호를 물어보면 `1`을 입력하세요.

## 2. 일반 실행
권한이 정상적으로 부여되었다면, 앞으로는 아래처럼 평소와 같이 실행하시면 됩니다.

```bash
# 기본 실행 (에러 없이 실행되어야 함)
python3 main.py

# 옵션을 주어 실행
python3 main.py --interface enp0s6 --text "Hello!" --text-color "blue"
```
