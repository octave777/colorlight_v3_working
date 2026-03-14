import socket
import time
import math
import numpy as np
from scapy.all import Ether, sendp, conf, sniff, Raw

class ColorLight5a75Controller:
    """
    ColorLight-5a-75.cpp/h의 기능을 파이선으로 구현한 클래스입니다.
    이더넷 Raw 소켓을 사용하여 Colorlight 5A-75 리시빙 카드에 직접 데이터를 보냅니다.
    """
    
    # 패킷 타입 상수 (ColorLight-5a-75.h 참고)
    CL_SYNC_PACKET_TYPE = 0x01 # 화면 출력 동기화 패킷
    CL_BRIG_PACKET_TYPE = 0x0A # 밝기 제어 패킷
    CL_PIXL_PACKET_TYPE = 0x55 # 픽셀 데이터(행 단위) 패킷
    
    # 설정 상수
    # 하드웨어 특성에 따라 한 패킷(이더넷 프레임)에 들어갈 최대 픽셀 수
    # MTU 1500 기준, 400 픽셀(1200 바이트)까지 넉넉하게 하나의 패킷으로 전송 가능.
    # LEDVision 덤프 상 320픽셀 전체를 한 패킷에 담으므로 400으로 여유 있게 설정.
    CL_MAX_PIXL_PER_PACKET = 400 
    CL_DEST_MAC = "11:22:33:44:55:66" # 목적지 MAC 주소 (하드코딩된 값)
    CL_SRC_MAC = "21:22:33:44:55:66"  # 출발지 MAC 주소 (Destination과 동일하게 설정)
    
    def __init__(self, interface, width=320, height=240, color_order="BGR"):
        """
        초기화 함수
        :param interface: 네트워크 인터페이스 이름 (예: en5)
        :param width: 패널 가로 해상도 (기본: 320)
        :param height: 패널 세로 해상도 (기본: 240)
        :param color_order: 색상 순서 (RGB, BGR, GBR 등)
        """
        self.interface = interface
        self.width = width
        self.height = height
        self.color_order = color_order.upper()
        self.brightness = 100
        self.gamma = 2.2
        self.gamma_table = np.zeros(256, dtype=np.uint8)
        self.firmware_version = 0 # 0이면 일반, 13이상이면 패킷 중복 전송 등 특수 동작
        
        # 소켓 캐싱 (매 패킷마다 sendp() 사용 시 발생하는 지연 시간 제거)
        self.sock = conf.L2socket(iface=self.interface)
        
        # 마지막으로 전송한 밝기 상태 저장 (매 프레임 밝기 패킷 전송 시 깜빡임 방지용)
        self.last_brightness = -1
        
        # 감마 곡선 생성 (ColorLight-5a-75.cpp:396-406 참고)
        self._update_gamma_table()
        
    def _update_gamma_table(self):
        """
        입력된 감마값에 따라 256단계의 변환 테이블을 생성합니다.
        """
        for x in range(256):
            f = 255.0 * math.pow(x / 255.0, self.gamma)
            self.gamma_table[x] = max(0, min(255, round(f)))

    def set_brightness(self, brightness):
        """
        밝기를 설정합니다 (0-100).
        """
        self.brightness = max(0, min(100, brightness))

    def set_gamma(self, gamma):
        """
        감마값을 설정하고 테이블을 갱신합니다.
        """
        self.gamma = gamma
        self._update_gamma_table()

    def _convert_color_order(self, r, g, b):
        """
        지정된 색상 순서에 맞게 바이트 배열을 반환합니다.
        """
        if self.color_order == "BGR":
            return [b, g, r]
        elif self.color_order == "RGB":
            return [r, g, b]
        elif self.color_order == "GRB":
            return [g, r, b]
        elif self.color_order == "GBR":
            return [g, b, r]
        elif self.color_order == "RBG":
            return [r, b, g]
        elif self.color_order == "BRG":
            return [b, r, g]
        return [r, g, b]

    def _build_ether_header(self, packet_type, first_byte):
        """
        ColorLight 특수 EtherType 헤더를 구성합니다. (55 XX 형식)
        """
        # Scapy의 Ether 객체는 type 인자에 2바이트 정수를 받습니다.
        # ColorLight 프로토콜은 EtherType 필드의 첫 바이트가 packet_type, 
        # 두 번째 바이트가 데이터의 첫 바이트입니다.
        eth_type = (packet_type << 8) | first_byte
        return Ether(dst=self.CL_DEST_MAC, src=self.CL_SRC_MAC, type=eth_type)

    def send_brightness(self):
        """
        밝기 제어 패킷을 보냅니다 (Type 0x0A).
        """
        # fpp 원본: data[0,1,2] = brightness, data[3] = 0xff
        b_val = int(2.55 * self.brightness) # 100 -> 255
        
        # 첫 바이트는 밝기 값
        pkt = self._build_ether_header(self.CL_BRIG_PACKET_TYPE, b_val)
        
        # 나머지 데이터 부분 (C++ 원본 CL_BRIG_PACKET_SIZE = 77 - 헤더 13 = 64바이트)
        payload = bytearray(64)
        payload[0] = b_val # 0번 인덱스는 이미 EtherType에 들어갔으므로 1번부터 채워야 할 수도 있지만
                           # fpp의 SendMessagesHelper 로직을 보면 EtherType 이후의 데이터를 보냅니다.
                           # 파이선 Scapy에서는 Ether(type=...) 가 12,13바이트를 차지하므로 Payload는 14바이트부터입니다.
        payload[0] = b_val # 실제로는 EtherType에 포함된 값이 데이터의 [0]번째이므로, 
                           # Payload 문자열은 [1]번째 데이터부터 시작해야 합니다.
        payload[1] = b_val
        payload[2] = 0xFF
        # ... 나머지는 0
        
        self.sock.send(pkt / bytes(payload))
        
        # 펌웨어가 최신일 경우(13 이상) 한 번 더 보냄
        if self.firmware_version >= 13:
            self.sock.send(pkt / bytes(payload))

    def send_pixel_data(self, image_data):
        """
        이미지 데이터를 행 단위 패킷으로 쪼개서 전송합니다 (Type 0x55).
        실제 이미지 높이가 하드웨어 높이보다 작을 경우 검은색으로 패딩합니다.
        :param image_data: Pillow Image 객체
        """
        # 이미지 크기 가져오기
        img_w, img_h = image_data.size
        pixels = list(image_data.getdata())
        
        # 하드웨어의 모든 행(height)에 대해 루프
        for y in range(self.height):
            if y < img_h:
                # 이미지 범위 내: 실제 픽셀 데이터 사용
                row_pixels = pixels[y * img_w : (y + 1) * img_w]
                # 가로 크기가 부족할 경우 검은색으로 채움
                if len(row_pixels) < self.width:
                    row_pixels += [(0, 0, 0)] * (self.width - len(row_pixels))
            else:
                # 이미지 범위 밖: 검은색 행 전송 (패딩)
                row_pixels = [(0, 0, 0)] * self.width
            
            # 한 패킷에 담을 수 있는 크기로 쪼개기 (현재 320픽셀이면 한 패킷에 다 들어감)
            for x_offset in range(0, self.width, self.CL_MAX_PIXL_PER_PACKET):
                chunk = row_pixels[x_offset : x_offset + self.CL_MAX_PIXL_PER_PACKET]
                pixel_count = len(chunk)
                
                # 헤더 구성 (Row LSB, Offset, Count 등)
                # Data[0] (Row MSB)는 EtherType의 하위 바이트로 들어갑니다.
                row_msb = (y >> 8) & 0xFF
                row_lsb = y & 0xFF
                off_msb = (x_offset >> 8) & 0xFF
                off_lsb = x_offset & 0xFF
                cnt_msb = (pixel_count >> 8) & 0xFF
                cnt_lsb = pixel_count & 0xFF
                
                # Data[1...7]
                # LEDVision 덤프 분석 결과: Data[1] = Row LSB, Data[2,3] = Offset, Data[4,5] = Count, 이후 0x08 0x00 고정
                header_data = [row_lsb, off_msb, off_lsb, cnt_msb, cnt_lsb, 0x08, 0x00]
                
                # 픽셀 데이터 추가 및 감마/색상 변환
                pixel_payload = []
                for p in chunk:
                    r = self.gamma_table[p[0]]
                    g = self.gamma_table[p[1]]
                    b = self.gamma_table[p[2]]
                    pixel_payload.extend(self._convert_color_order(r, g, b))
                
                # Type (0x55) + Data[0] (Row MSB)
                pkt = self._build_ether_header(self.CL_PIXL_PACKET_TYPE, row_msb)
                self.sock.send(pkt / bytes(header_data + pixel_payload))
                
                # 펌웨어가 최신일 경우(13 이상) 각 행을 두 번씩 전송 (코스트/고스트 방지용)
                if self.firmware_version >= 13:
                    self.sock.send(pkt / bytes(header_data + pixel_payload))

    def send_sync(self):
        """
        화면 갱신 동기화 신호를 보냅니다 (Type 0x01).
        """
        # 사용자가 제공한 LEDVision/FPP 패킷 덤프 기반의 Sync 패킷 (Type 0x01, data[0] = 0xFF)
        # 덤프 분석: 01 ff 00 ff ff ff ff 00 00 ... 01 ... 01 ... ff ff fd ... (밝기나 프레임 번호로 추정되는 곳)
        pkt = self._build_ether_header(self.CL_SYNC_PACKET_TYPE, 0xFF)
        
        # 98바이트 페이로드 (Type과 Data[0]은 EtherType으로 처리되었으므로 나머지 98바이트)
        payload = bytearray(98)
        # 덤프의 고정된 패턴 적용
        payload[0] = 0x00
        payload[1] = 0xFF; payload[2] = 0xFF; payload[3] = 0xFF; payload[4] = 0xFF
        payload[12] = 0x01
        payload[17] = 0x01
        payload[24] = 0xFF; payload[25] = 0xFF; payload[26] = 0xFD
        
        # 밝기(brightness) 또는 기타 제어값으로 FPP의 위치(21, 24, 25, 26)나 덤프의 특정 위치 적용
        b_val = int(2.55 * self.brightness)
        # FPP 방식의 밝기 설정을 유지하면서 새로운 덤프 템플릿 사용
        # 덤프에서는 31번 위치가 0x80 또는 0xd0 등으로 변함
        payload[31] = b_val 
        
        self.sock.send(pkt / bytes(payload))
        
        # 펌웨어가 최신일 경우(13 이상) 한 번 더 보냄 (ColorLight-5a-75.cpp:674 참고)
        if self.firmware_version >= 13:
            self.sock.send(pkt / bytes(payload))

    def output_frame(self, image):
        """
        주어진 이미지를 LED 패널에 출력하는 1프레임 사이클입니다.
        1. 밝기 설정 (밝기 값이 변경되었을 때만 전송)
        2. 이미지 픽셀 데이터 전송
        3. 화면 갱신(Sync) 통지
        """
        if self.brightness != self.last_brightness:
            self.send_brightness()
            self.last_brightness = self.brightness
            
        self.send_pixel_data(image)
        self.send_sync()

    def detect_and_print_config(self):
        """
        리시빙 카드의 설정을 감지하고 출력합니다 (detect.py 로직 기반).
        """
        print(f"--- Colorlight 카드 감지 중 (인터페이스: {self.interface}) ---")
        
        # 탐색용 패킷 페이로드 준비
        payload1 = b'\x00' * 270
        payload2 = b'\x00\x00\x01' + b'\x00' * 267
        
        # 1. 첫 번째 패킷 전송 (카드 탐색 요청)
        # detect.py에서는 eth_type=0x0700을 사용함
        eth_type_detect = 0x0700
        pkt1 = Ether(dst=self.CL_DEST_MAC, src=self.CL_SRC_MAC, type=eth_type_detect) / Raw(load=payload1)
        
        try:
            # 패킷 전송
            self.sock.send(pkt1)
            
            # 2. 패킷 수신 및 분석 (sniff 사용, 2초 타임아웃)
            # count=10: 여러 패킷 중 카드의 응답을 찾기 위함
            packets = sniff(iface=self.interface, count=10, timeout=2)
            
            detected = False
            if packets:
                for p in packets:
                    data = bytes(p)
                    # 리눅스/맥 이더넷 헤더(14바이트) 포함 상태의 인덱스 로직
                    # data[12], data[13]은 EtherType (0x0805 등 응답 패턴 확인)
                    if len(data) > 13 and data[12] == 8 and data[13] == 5:
                        if data[14] == 4: # Colorlight 응답 식별자
                            version = f"{data[15]}.{data[16]}"
                            res_x = data[34] * 256 + data[35]
                            res_y = data[36] * 256 + data[37]
                            print(f"[감마] Colorlight 5A {version} 감지됨")
                            print(f"[해상도] 카드 설정 값: 가로 {res_x}, 세로 {res_y}")
                            
                            # 실제 설정된 값과 비교 안내
                            if res_x != self.width or res_y != self.height:
                                print(f"  * 알림: 현재 프로그램 설정({self.width}x{self.height})과 카드 설정이 다를 수 있습니다.")
                            
                            detected = True
                            break
            
            if not detected:
                print("Colorlight 카드의 응답을 받지 못했습니다. 인터페이스나 연결을 확인하세요.")

            # 3. 종료 혹은 다음 단계를 위한 패킷 전송 (detect.py 로직 유지)
            pkt2 = Ether(dst=self.CL_DEST_MAC, src=self.CL_SRC_MAC, type=eth_type_detect) / Raw(load=payload2)
            self.sock.send(pkt2)
            
        except Exception as e:
            print(f"카드 감지 중 오류 발생: {e}")
            if "Permission denied" in str(e) or "not permitted" in str(e):
                print("힌트: 패킷 수신(sniff)을 위해 sudo 권한이 필요합니다.")
        
        print(f"----------------------------------------------")
