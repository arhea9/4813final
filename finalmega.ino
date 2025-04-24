#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7796S.h>
#include <Fonts/FreeSansBold18pt7b.h> // A custom font
#include <SPI.h>

#define I2C_SDA A4
#define I2C_SCL A5
#define RST_N_PIN 5


#define TFT_CS 10
#define TFT_RST 9
#define TFT_DC 8
Adafruit_ST7796S display(TFT_CS, TFT_DC, TFT_RST);
uint8_t rotate = 0;
#define CORNER_RADIUS 0
#define PAUSE 1000
#define BLUE 0x001F
 
#define RE 3
#define DE 2

#define sensorPower 7
#define sensorPin A3

int joyPin1 = A0;
int joyPin2 = A1;
const int joyButton = 2;
int idealMin[] = {35, 20, 40, 32};
int idealMax[] = {45, 26, 47, 43};


byte values[11];
SoftwareSerial modbus(2,3);
 
int selectedLabel = 0;
unsigned long lastMoveTime = 0;
const int moveDelay = 300; // ms between joystick moves

void setup() {
  Serial.begin(9600);
  Wire.begin();
  display.init(320, 480, 0, 0, ST7796S_RGB);
  display.setRotation(1);
  display.fillScreen(0);
  SPI.begin();
  modbus.begin(9600);
  pinMode(RE, OUTPUT);
  pinMode(DE, OUTPUT);
  pinMode(sensorPower, OUTPUT);
  digitalWrite(sensorPower, LOW);
  delay(500);
  drawStaticMoistureBox(selectedLabel);
}
 
void loop() {
  updateMoistureBar(selectedLabel);
  delay(500);
  
  
  Serial.print("Analog output: ");
  Serial.println(readSensor());
  delay(500);
  
  int joyX = analogRead(joyPin1); // Horizontal axis

  unsigned long now = millis();
  if (now - lastMoveTime > moveDelay) {
    if (joyX < 400) { // Move left
      selectedLabel = (selectedLabel + 3) % 4; // wrap around left
      lastMoveTime = now;
      drawStaticMoistureBox(selectedLabel);
    } else if (joyX > 600) { // Move right
      selectedLabel = (selectedLabel + 1) % 4; // wrap around right
      lastMoveTime = now;
      Serial.println(selectedLabel);
      drawStaticMoistureBox(selectedLabel);
    }
  }

  delay(100);
}
 
int readSensor() {
  digitalWrite(sensorPower, HIGH);
  delay(10);
  int val = analogRead(sensorPin);
  digitalWrite(sensorPower, LOW);
  return val;
  }


void drawStaticMoistureBox(int selectedLabel) {
  display.setFont();
  display.setTextSize(3);
  display.setTextColor(0xFFFF);

  // Draw the large "M" moisture box
  int boxW = 220;
  int boxH = 80;
  int x = (display.width() - boxW) / 2;
  int y = (display.height() / 2) - boxH - 20;

  for (int i = 0; i < 3; i++) {
  display.drawRect(x - i, y - i, boxW + 2 * i, boxH + 2 * i, 0xFFFF); // White outline
}
  display.setCursor(x - 45, y + boxH / 2 - 12);
  display.print("M");

  // Set font for plant labels
  display.setTextSize(2);
  int labelBoxW = 150;
  int labelBoxH = 40;
  int padding = 10;

  // Coordinates for the 2×2 layout
  int col1X = (display.width() / 2) - labelBoxW - padding;
  int col2X = (display.width() / 2) + padding;
  int row1Y = display.height() - 150;
  int row2Y = display.height() - 105;

  struct Label {
    const char* text;
    int x;
    int y;
  } labels[] = {
    {"TOMATO",    col1X, row1Y},
    {"JALAPENO",  col2X, row1Y},
    {"CILANTRO",  col1X, row2Y},
    {"BELL PEPPER", col2X, row2Y}
  };

  // Clear label area first
  display.fillRect(0, row1Y - 4, display.width(), labelBoxH * 2 + 20, 0x0000);

  // Draw label boxes and centered text
  for (int i = 0; i < 4; i++) {
    // Draw selection outline if current label is selected
    if (i == selectedLabel) {
      for (int t = 0; t < 3; t++) { // 3-pixel thickness
        display.drawRect(
          labels[i].x - 4 - t,
          labels[i].y - t,
          labelBoxW + 8 + t * 2,
          labelBoxH + 8 + t * 2,
          0x07C0
        );
      }

      //display.drawRect(labels[i].x - 4, labels[i].y - 2, labelBoxW + 8, labelBoxH + 8, 0x07C0); // Red outline
    }

    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(labels[i].text, labels[i].x, labels[i].y, &x1, &y1, &w, &h);

    int centerX = labels[i].x + (labelBoxW - w) / 2;
    int centerY = labels[i].y + (labelBoxH + h) / 2 - 4;

    display.setCursor(centerX, centerY);
    display.setTextColor(0xFFFF);
    display.print(labels[i].text);
  }
}






void updateMoistureBar(int selectedLabel) {
  int boxW = 220;
  int boxH = 80;

  int x = (display.width() - boxW) / 2;
  int y = (display.height() / 2) - boxH - 20;

  // Erase previous fill (interior only)
  display.fillRect(x + 2, y + 2, boxW - 4, boxH - 4, 0x0000); // background color

  // Read sensor and calculate moisture percentage
  int raw = readSensor();
  int moisturePercent = ((260 - raw) * 100) / 260 + 1;
  if (moisturePercent < 0) {
    moisturePercent = 0;
  }
  int fillWidth = (boxW * moisturePercent) / 100;



  // Pick color based on moisture range
  uint16_t fillColor = 0x07C0; // Default bluish
  if (moisturePercent >= 20 && moisturePercent <= 50) {
    fillColor = 0x07E0; // Green
  }

  // Draw new fill
  display.fillRect(x + 2, y + 2, fillWidth - 4, boxH - 4, fillColor);

  // 🌱 Use selected label’s ideal range
  int outlineStartPercent = idealMin[selectedLabel];
  int outlineEndPercent = idealMax[selectedLabel];

  int outlineX = x + 2 + (boxW * outlineStartPercent) / 100;
  int outlineW = ((boxW * outlineEndPercent) / 100) - ((boxW * outlineStartPercent) / 100);

  // Draw the 3-pixel-thick outline
  for (int i = 0; i < 3; i++) {
    display.drawRect(outlineX - i, y + 2 - i, outlineW + 2 * i, boxH - 4 + 2 * i, 0xFFFF); // White
  }
}




