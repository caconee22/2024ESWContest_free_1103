#include <SPI.h>
#include <DW1000.h>

// 앵커의 고정된 좌표 (삼각형 꼭지점)
float anchor1[2] = {0.0, 0.0};
float anchor2[2] = {1.5, 0.0};
float anchor3[2] = {0.75, 1.299}; // 1.299m는 1.5 * sqrt(3) / 2

// 지수 이동 평균 필터 변수들
float alpha = 0.2; // EMA 필터 계수, 0 < alpha <= 1
float filteredDistance1 = 0;
float filteredDistance2 = 0;
float filteredDistance3 = 0;

// 이전 필터링된 거리
float previousFilteredDistance1 = 0;
float previousFilteredDistance2 = 0;
float previousFilteredDistance3 = 0;

// 지수 이동 평균 필터 함수
float exponentialMovingAverage(float currentMeasurement, float previousFiltered) {
    return alpha * currentMeasurement + (1 - alpha) * previousFiltered;
}

void setup() {
    Serial.begin(9600);
    DW1000.begin();
    DW1000.setup();
    DW1000.enableDebounceClock();
    DW1000.enableLedBlink();
    DW1000.enableTransmitPower(3.3); // 3.3V에서 최적화된 전송 전력
}

void loop() {
    // 거리 측정 예제
    DW1000.newReceive();
    DW1000.receivePermanently();
    DW1000.startReceive();

    if (DW1000.receiveData()) {
        float distance1 = DW1000.getRange(); // 앵커 1의 거리 측정
        float distance2 = DW1000.getRange(); // 앵커 2의 거리 측정
        float distance3 = DW1000.getRange(); // 앵커 3의 거리 측정

        // 지수 이동 평균 필터 적용
        filteredDistance1 = exponentialMovingAverage(distance1, previousFilteredDistance1);
        filteredDistance2 = exponentialMovingAverage(distance2, previousFilteredDistance2);
        filteredDistance3 = exponentialMovingAverage(distance3, previousFilteredDistance3);

        // 이전 필터링된 거리 업데이트
        previousFilteredDistance1 = filteredDistance1;
        previousFilteredDistance2 = filteredDistance2;
        previousFilteredDistance3 = filteredDistance3;

        // 드론의 위치 계산
        float x_drone, y_drone;
        calculatePosition(filteredDistance1, filteredDistance2, filteredDistance3, &x_drone, &y_drone);

        // 중심에서 벗어난 거리 출력
        float offset = calculateOffset(x_drone, y_drone);
        Serial.print("Offset from center: ");
        Serial.println(offset);
    }

    delay(1000); // 1초마다 업데이트
}

// 드론의 현재 위치를 계산하는 함수
void calculatePosition(float d1, float d2, float d3, float* x, float* y) {
    *x = (d1 * d1 - d2 * d2 + 2.25) / 3.0;
    *y = (d1 * d1 - d3 * d3 + 1.6875) / 2.598;
}

// 중심에서 벗어난 거리를 계산하는 함수
float calculateOffset(float x, float y) {
    float x_center = 0.75;
    float y_center = 0.433;
    return sqrt(pow(x - x_center, 2) + pow(y - y_center, 2));
}