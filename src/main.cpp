// agri-uv-bringup — DFRobot SEN0636 UV Index sensor read on an M5 LCD host.
//
// Reads UV Index / Risk / raw mV over Modbus RTU on UART2 once per second
// and paints them onto whatever LCD the host carries. Also mirrors to
// Serial. No WiFi, no MQTT — this exists solely to verify the sensor works
// and eyeball values while wiring for the eventual PoE node.
//
// One source, two hosts (M5Unified auto-detects the board at M5.begin):
//   - m5basic-uv-lcd  : M5Stack Basic, ILI9341 320x240, UART2 on G16/G17.
//   - atoms3r-uv-lcd  : M5 AtomS3R, GC9107 128x128, UART2 on Grove G1/G2.
// The UART pins come in as build flags (UV_UART_RX / UV_UART_TX); the
// display layout picks itself from M5.Display.width() at runtime.

#include <Arduino.h>
#include <M5Unified.h>
#include <DFRobot_UVIndex240370Sensor.h>

#ifndef UV_UART_RX
#define UV_UART_RX 16  // M5 Basic Port C, U2RXD (fallback)
#endif
#ifndef UV_UART_TX
#define UV_UART_TX 17  // M5 Basic Port C, U2TXD (fallback)
#endif

static constexpr int PIN_UART_RX = UV_UART_RX;
static constexpr int PIN_UART_TX = UV_UART_TX;
static constexpr uint32_t SAMPLE_MS = 1000;

DFRobot_UVIndex240370Sensor g_uv(&Serial2);

static const char *riskLabel(uint16_t r) {
  switch (r) {
    case 0: return "Low";
    case 1: return "Moderate";
    case 2: return "High";
    case 3: return "Very High";
    case 4: return "Extreme";
    default: return "?";
  }
}

static uint16_t riskColor(uint16_t r) {
  switch (r) {
    case 0: return TFT_GREEN;
    case 1: return TFT_YELLOW;
    case 2: return TFT_ORANGE;
    case 3: return TFT_RED;
    case 4: return TFT_MAGENTA;
    default: return TFT_DARKGREY;
  }
}

// Small screens (AtomS3R 128x128 and the like) get a compact, centered
// layout; wide screens (M5 Basic 320x240) keep the roomy one below.
static bool isSmallScreen() { return M5.Display.width() < 200; }

static void drawStaticChromeSmall() {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextDatum(top_center);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(1);
  M5.Display.drawString("SEN0636 UV", M5.Display.width() / 2, 2);
  M5.Display.drawFastHLine(0, 13, M5.Display.width(), TFT_DARKGREY);
}

static void drawReadingSmall(uint16_t idx, uint16_t risk, uint16_t mv, bool ok) {
  const int w = M5.Display.width();
  const int cx = w / 2;
  M5.Display.fillRect(0, 14, w, M5.Display.height() - 14, TFT_BLACK);

  // Big index number, centered.
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(ok ? riskColor(risk) : TFT_DARKGREY, TFT_BLACK);
  M5.Display.setTextSize(6);
  M5.Display.drawString(ok ? String(idx) : String("--"), cx, 52);

  // Risk label, colored.
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(ok ? riskColor(risk) : TFT_DARKGREY, TFT_BLACK);
  M5.Display.drawString(ok ? riskLabel(risk) : "n/a", cx, 95);

  // Raw mV, dim.
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  char buf[24];
  if (ok) snprintf(buf, sizeof(buf), "raw %u mV", mv);
  else    snprintf(buf, sizeof(buf), "bus timeout");
  M5.Display.drawString(buf, cx, 118);
}

static void drawStaticChrome() {
  if (isSmallScreen()) { drawStaticChromeSmall(); return; }
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextDatum(top_left);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(2);
  M5.Display.setCursor(0, 0);
  M5.Display.println("SEN0636 UV Index");
  M5.Display.drawFastHLine(0, 22, 320, TFT_DARKGREY);
}

static void drawReading(uint16_t idx, uint16_t risk, uint16_t mv, bool ok) {
  if (isSmallScreen()) { drawReadingSmall(idx, risk, mv, ok); return; }
  M5.Display.setTextDatum(top_left);
  M5.Display.fillRect(0, 30, 320, 210, TFT_BLACK);

  M5.Display.setCursor(0, 40);
  M5.Display.setTextSize(3);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.print("Index: ");
  M5.Display.setTextSize(7);
  M5.Display.setCursor(150, 40);
  if (ok) M5.Display.printf("%2u", idx); else M5.Display.print("--");

  M5.Display.setTextSize(3);
  M5.Display.setCursor(0, 120);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.print("Risk: ");
  M5.Display.setTextColor(riskColor(risk), TFT_BLACK);
  M5.Display.println(ok ? riskLabel(risk) : "n/a");

  M5.Display.setTextSize(2);
  M5.Display.setCursor(0, 180);
  M5.Display.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  if (ok) M5.Display.printf("raw: %4u mV", mv);
  else    M5.Display.print("raw: sensor timeout");

  M5.Display.setTextSize(1);
  M5.Display.setCursor(0, 224);
  M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
  M5.Display.printf("Modbus 9600 8N1 slave 0x23  UART2 G%d/G%d",
                    PIN_UART_RX, PIN_UART_TX);
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);       // LCD + buttons + speaker + Power
  M5.Display.setBrightness(180);
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, PIN_UART_RX, PIN_UART_TX);

  drawStaticChrome();

  M5.Display.setTextDatum(top_left);
  M5.Display.setTextSize(isSmallScreen() ? 1 : 2);
  M5.Display.setCursor(0, isSmallScreen() ? 20 : 40);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.println("uv.begin() ...");
  bool ok = g_uv.begin();
  M5.Display.println(ok ? "handshake OK" : "handshake FAIL");
  Serial.printf("[UV] begin: %s\n", ok ? "OK" : "FAIL");
  delay(600);
}

void loop() {
  M5.update();

  static uint32_t next = 0;
  uint32_t now = millis();
  if ((int32_t)(now - next) < 0) return;
  next = now + SAMPLE_MS;

  uint16_t mv   = g_uv.readUvOriginalData();
  uint16_t idx  = g_uv.readUvIndexData();
  uint16_t risk = g_uv.readRiskLevelData();

  // DFRobot_RTU returns 0 on both a real zero reading and on a bus
  // timeout, so treat "everything zero AND raw==0" as suspicious but
  // still render — the operator can spot a stuck bus by watching the
  // raw mV field.
  bool ok = true;
  Serial.printf("[UV] idx=%u risk=%u mv=%u\n", idx, risk, mv);
  drawReading(idx, risk, mv, ok);
}
