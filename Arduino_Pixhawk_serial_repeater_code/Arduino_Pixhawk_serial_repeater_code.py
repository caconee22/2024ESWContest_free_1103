import serial
import serial.tools.list_ports
import threading
import time

class SerialDevice:
    """
    시리얼 장치를 나타내는 클래스
    - port: 시리얼 장치의 포트 (예: '/dev/ttyUSB0')
    - baudrate: 통신 속도 (기본 9600bps)
    - timeout: 통신 타임아웃 (기본 1초)
    """
    def __init__(self, port, baudrate=9600, timeout=1):
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.ser = None  # 시리얼 객체 초기화
        self._stop_flag = threading.Event()  # 통신 종료 플래그

    def open(self):
        """
        시리얼 포트 열기
        """
        try:
            # 지정된 포트로 시리얼 연결
            self.ser = serial.Serial(self.port, self.baudrate, timeout=self.timeout)
            print(f"Serial device {self.port} connected successfully.")
        except serial.SerialException as e:
            # 시리얼 연결에 실패하면 에러 출력
            print(f"Error connecting to {self.port}: {e}")
            self.ser = None

    def close(self):
        """
        시리얼 포트 닫기
        """
        if self.ser and self.ser.is_open:
            self.ser.close()
            print(f"Serial device {self.port} closed.")

    def read_data(self):
        """
        시리얼 포트에서 데이터 읽기
        - 데이터를 읽어와서 문자열로 반환
        """
        if self.ser and self.ser.is_open:
            try:
                return self.ser.readline().decode().strip()  # 시리얼 데이터 읽어서 문자열로 디코딩
            except serial.SerialException as e:
                print(f"Error reading from {self.port}: {e}")
        return None

    def write_data(self, data):
        """
        시리얼 포트로 데이터 전송
        - 데이터를 받아서 시리얼로 출력
        """
        if self.ser and self.ser.is_open:
            try:
                self.ser.write(data.encode())  # 데이터를 인코딩하여 전송
                print(f"Sent data to {self.port}: {data}")
            except serial.SerialException as e:
                print(f"Error writing to {self.port}: {e}")

    def stop(self):
        """
        통신을 종료할 때 플래그 설정
        """
        self._stop_flag.set()

    def is_stopped(self):
        """
        통신 종료 여부를 확인하는 함수
        - 플래그가 설정되었는지 여부 반환
        """
        return self._stop_flag.is_set()


class SerialManager:
    """
    두 개의 시리얼 장치를 관리하는 클래스
    - 하나는 입력용, 하나는 출력용
    """
    def __init__(self):
        self.input_device = None  # 입력용 시리얼 장치
        self.output_device = None  # 출력용 시리얼 장치

    def find_ports(self):
        """
        연결된 시리얼 포트를 검색하는 함수
        - 현재 사용 가능한 모든 포트를 리스트로 반환
        """
        ports = list(serial.tools.list_ports.comports())
        return ports

    def select_ports(self):
        """
        사용할 두 개의 시리얼 포트를 자동으로 선택
        - 포트가 두 개 이상 연결된 경우 첫 번째는 입력용, 두 번째는 출력용으로 설정
        """
        ports = self.find_ports()
        if len(ports) < 2:
            print("Not enough serial devices found!")
            return

        # 포트가 두 개 이상 있으면 첫 번째 장치는 입력, 두 번째 장치는 출력으로 설정
        self.input_device = SerialDevice(ports[0].device)  # 첫 번째 아두이노
        self.output_device = SerialDevice(ports[1].device)  # 두 번째 아두이노

    def start_communication(self):
        """
        시리얼 통신을 시작하는 함수
        - 두 시리얼 장치를 열고 비동기적으로 데이터를 중계
        """
        if not self.input_device or not self.output_device:
            print("Serial devices not set properly!")
            return

        # 입력 장치와 출력 장치의 시리얼 포트 열기
        self.input_device.open()
        self.output_device.open()

        # 시리얼 포트 열기에 실패했는지 확인
        if self.input_device.ser is None or self.output_device.ser is None:
            print("Error in opening serial devices!")
            return

        # 비동기 통신을 시작
        self._start_async_communication()

    def _start_async_communication(self):
        """
        비동기 통신을 시작하는 내부 함수
        - 스레드를 통해 데이터를 읽고 전달하는 작업을 수행
        """
        def read_and_forward():
            # 입력 장치에서 데이터를 읽고 출력 장치로 전달
            while not self.input_device.is_stopped():
                data = self.input_device.read_data()  # 입력 장치에서 데이터 읽기
                if data:
                    print(f"Received data from input device: {data}")
                    self.output_device.write_data(data)  # 읽은 데이터를 출력 장치로 전달
                time.sleep(0.1)  # 루프가 너무 빨리 도는 것을 방지하기 위한 짧은 대기

        # 스레드를 사용하여 read_and_forward 함수를 실행
        communication_thread = threading.Thread(target=read_and_forward)
        communication_thread.start()

        try:
            # 메인 스레드에서 프로그램이 종료되지 않도록 유지
            while not self.input_device.is_stopped():
                time.sleep(1)
        except KeyboardInterrupt:
            # Ctrl+C로 프로그램 중단 시 처리
            print("Stopping communication...")
            self.input_device.stop()
            communication_thread.join()  # 스레드가 종료될 때까지 대기

        # 통신 종료 후 시리얼 포트를 닫음
        self.input_device.close()
        self.output_device.close()


if __name__ == "__main__":
    # 프로그램이 실행되면 SerialManager 인스턴스 생성
    serial_manager = SerialManager()
    
    # 연결된 포트를 자동으로 선택
    serial_manager.select_ports()
    
    # 시리얼 통신 시작
    serial_manager.start_communication()
