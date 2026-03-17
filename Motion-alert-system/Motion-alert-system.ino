#include <Wire.h>

#define LIS3DH_ADDR 0x18   // Change to 0x19 if needed

// Registers
#define CTRL_REG1 0x20
#define CTRL_REG3 0x22
#define INT1_CFG  0x30
#define INT1_THS  0x32
#define INT1_DURATION 0x33
#define OUT_X_L 0x28

#define INTERRUPT_PIN 2
#define LED_PIN 13

volatile bool motionFlag = false;

// ---------------- I2C FUNCTIONS ----------------

void writeRegister(byte reg, byte value) {
  Wire.beginTransmission(LIS3DH_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

bool readMulti(byte reg, byte *data, int len) {
  Wire.beginTransmission(LIS3DH_ADDR);
  Wire.write(reg | 0x80); // auto increment
  if (Wire.endTransmission(false) != 0) return false;

  Wire.requestFrom(LIS3DH_ADDR, len);

  for (int i = 0; i < len; i++) {
    if (Wire.available()) {
      data[i] = Wire.read();
    } else {
      return false;
    }
  }
  return true;
}

// ---------------- SENSOR INIT ----------------

void initLIS3DH() {
  writeRegister(CTRL_REG1, 0x57); // 100Hz, enable XYZ
}

// ---------------- INTERRUPT SETUP ----------------

void setupInterrupt() {
  writeRegister(CTRL_REG3, 0x40); // INT1 enable
  writeRegister(INT1_THS, 0x10);  // Threshold
  writeRegister(INT1_DURATION, 0x01);
  writeRegister(INT1_CFG, 0x2A);  // X,Y,Z high events
}

// ---------------- ISR ----------------

void motionISR() {
  motionFlag = true;   // Keep ISR short
}

// ---------------- READ ACCEL ----------------

bool readAccel(int16_t *x, int16_t *y, int16_t *z) {
  byte data[6];

  if (!readMulti(OUT_X_L, data, 6)) return false;

  *x = (int16_t)(data[1] << 8 | data[0]);
  *y = (int16_t)(data[3] << 8 | data[2]);
  *z = (int16_t)(data[5] << 8 | data[4]);

  return true;
}

// ---------------- SETUP ----------------

void setup() {
  Serial.begin(115200);
  Wire.begin();

  pinMode(LED_PIN, OUTPUT);
  pinMode(INTERRUPT_PIN, INPUT);

  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), motionISR, RISING);

  initLIS3DH();
  setupInterrupt();

  Serial.println("System Ready...");
}

// ---------------- LOOP ----------------

void loop() {
  if (motionFlag) {
    motionFlag = false;

    int16_t x, y, z;

    if (readAccel(&x, &y, &z)) {

      int magnitude = abs(x) + abs(y) + abs(z);

      Serial.print("X: "); Serial.print(x);
      Serial.print(" Y: "); Serial.print(y);
      Serial.print(" Z: "); Serial.println(z);

      // Threshold check
      if (magnitude > 1000) {
        Serial.println(" ALERT: Motion Detected!");

        digitalWrite(LED_PIN, HIGH);
        delay(200);
        digitalWrite(LED_PIN, LOW);
      }
    } else {
      Serial.println("Read Error!");
    }
  }
}