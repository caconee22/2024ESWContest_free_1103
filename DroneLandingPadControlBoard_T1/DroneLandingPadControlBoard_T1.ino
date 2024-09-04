// 핀 설정
const int bluetoothRxPin = 2;  // 블루투스 RX 핀
const int bluetoothTxPin = 3;  // 블루투스 TX 핀
const int pwmPinRelayControl = 4;  // 릴레이 제어를 위한 PWM 신호 핀
const int pwmPinOpenClose = 5;     // 개폐 여부를 위한 PWM 신호 핀
const int relayPin = 12;   // 릴레이 핀 (전자석 제어)

// 블루투스 및 디버깅 시리얼 속도
const long bluetoothBaudRate = 9600;
const long debugBaudRate = 115200;

// 상태 변수
bool relayState = false;
bool openCloseState = false;

// 타이머 변수
unsigned long previousMillis = 0;
const long interval = 1000;  // 1초 간격

void setup() {
  // 핀 모드 설정
  pinMode(pwmPinRelayControl, INPUT);
  pinMode(pwmPinOpenClose, INPUT);
  pinMode(relayPin, OUTPUT);
  
  // 시리얼 통신 시작
  Serial.begin(debugBaudRate);
  Serial1.begin(bluetoothBaudRate); // HC06 블루투스 모듈 통신

  // 초기 상태 설정
  digitalWrite(relayPin, LOW);  // 릴레이 초기 상태 (꺼짐)
}

void loop() {
  // PWM 신호 읽기
  int relayControlSignal = pulseIn(pwmPinRelayControl, HIGH);
  int openCloseSignal = pulseIn(pwmPinOpenClose, HIGH);

  // 릴레이 제어 로직
  if (relayControlSignal >= 1500) {
    relayState = true;
    digitalWrite(relayPin, HIGH);  // 릴레이 켜기
  } else {
    relayState = false;
    digitalWrite(relayPin, LOW);  // 릴레이 끄기
  }

  // 개폐 여부 판단 및 블루투스 전송
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    if (openCloseSignal >= 1500) {
      openCloseState = true;
      Serial1.println("O");  // 블루투스에 'O' 전송 (내리기 명령)
    } else {
      openCloseState = false;
      Serial1.println("C");  // 블루투스에 'C' 전송 (올리기 명령)
    }
  }

  // 디버깅 정보 출력
  Serial.print("Relay Control Signal: ");
  Serial.print(relayControlSignal);
  Serial.print(" | Relay State: ");
  Serial.println(relayState ? "ON" : "OFF");

  Serial.print("Open/Close Signal: ");
  Serial.print(openCloseSignal);
  Serial.print(" | Open/Close State: ");
  Serial.println(openCloseState ? "OPEN" : "CLOSED");
}