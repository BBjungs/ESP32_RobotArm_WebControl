#include <Arduino.h>
#include <ESP32Servo.h>
#include <WebServer.h>
#include <WiFi.h>

/*
  ESP32 Robot Arm Web Control - Servo 6 แกน ไม่ใช้ PCA9685

  คำเตือนด้านไฟเลี้ยง:
  - ห้ามจ่ายไฟ Servo จากขา 5V หรือ 3.3V ของ ESP32 โดยตรง เพราะกระแสไม่พอและอาจทำให้บอร์ดเสียหาย
  - ควรใช้แหล่งจ่าย Servo แยก 5V/6V กระแสอย่างน้อย 10A สำหรับ Servo 6 ตัว
  - ต้องต่อ GND ของแหล่งจ่าย Servo ร่วมกับ GND ของ ESP32 เสมอ
  - แนะนำให้ใส่ Capacitor 470uF ถึง 2200uF คร่อม V+ และ GND ของไฟเลี้ยง Servo เพื่อลดไฟตกและ Servo กระตุก
*/

// -------------------- WiFi Access Point --------------------
const char *AP_SSID = "ESP32_ROBOT_ARM";
const char *AP_PASSWORD = "12345678";

IPAddress localIP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);

// -------------------- Servo Config --------------------
const uint8_t SERVO_COUNT = 6;

enum ServoIndex : uint8_t {
  BASE = 0,
  SHOULDER,
  ELBOW,
  WRIST_PITCH,
  WRIST_ROTATE,
  GRIPPER
};

const char *servoNames[SERVO_COUNT] = {
    "BASE",
    "SHOULDER",
    "ELBOW",
    "WRIST_PITCH",
    "WRIST_ROTATE",
    "GRIPPER"};

const uint8_t servoPins[SERVO_COUNT] = {13, 14, 16, 17, 18, 19};
const int minAngles[SERVO_COUNT] = {0, 30, 20, 20, 0, 30};
const int maxAngles[SERVO_COUNT] = {180, 150, 160, 160, 180, 120};
const int homeAngles[SERVO_COUNT] = {90, 90, 90, 90, 90, 80};

const int GRIPPER_OPEN_ANGLE = 120;
const int GRIPPER_CLOSE_ANGLE = 35;

const int SERVO_MIN_PULSE_US = 500;
const int SERVO_MAX_PULSE_US = 2500;
const uint16_t MANUAL_STEP_INTERVAL_MS = 12;

Servo servos[SERVO_COUNT];
int currentAngles[SERVO_COUNT];
int targetAngles[SERVO_COUNT];

enum Command : uint8_t {
  CMD_NONE = 0,
  CMD_HOME,
  CMD_OPEN_GRIPPER,
  CMD_CLOSE_GRIPPER,
  CMD_PICK_DEMO,
  CMD_PLACE_DEMO
};

Command pendingCommand = CMD_NONE;
bool demoRunning = false;

// -------------------- Web Page --------------------
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="th">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 Robot Arm</title>
  <style>
    :root {
      color-scheme: light;
      --bg: #f4f7fb;
      --panel: #ffffff;
      --text: #17202a;
      --muted: #667085;
      --line: #d7dee8;
      --brand: #0969da;
      --brand-dark: #0757b8;
      --ok: #157347;
      --warn: #b54708;
      --shadow: 0 14px 35px rgba(29, 45, 68, 0.12);
    }

    * { box-sizing: border-box; }

    body {
      margin: 0;
      min-height: 100vh;
      font-family: Arial, Helvetica, sans-serif;
      color: var(--text);
      background: var(--bg);
    }

    .app {
      width: min(980px, 100%);
      margin: 0 auto;
      padding: 18px;
    }

    header {
      display: flex;
      justify-content: space-between;
      gap: 14px;
      align-items: flex-start;
      margin-bottom: 14px;
    }

    h1 {
      margin: 0 0 4px;
      font-size: 28px;
      line-height: 1.2;
    }

    .subtitle {
      color: var(--muted);
      font-size: 14px;
      line-height: 1.5;
    }

    .status {
      padding: 10px 12px;
      border: 1px solid var(--line);
      border-radius: 8px;
      background: var(--panel);
      min-width: 150px;
      text-align: right;
      box-shadow: var(--shadow);
      font-size: 13px;
      color: var(--muted);
    }

    .status strong {
      display: block;
      color: var(--ok);
      font-size: 15px;
      margin-top: 2px;
    }

    .controls {
      display: grid;
      grid-template-columns: repeat(5, minmax(0, 1fr));
      gap: 10px;
      margin: 14px 0;
    }

    button {
      min-height: 44px;
      border: 0;
      border-radius: 8px;
      background: var(--brand);
      color: #ffffff;
      font-weight: 700;
      font-size: 14px;
      cursor: pointer;
      transition: background 0.16s ease, transform 0.16s ease;
      touch-action: manipulation;
    }

    button:hover { background: var(--brand-dark); }
    button:active { transform: translateY(1px); }
    button.secondary { background: #344054; }
    button.secondary:hover { background: #1d2939; }
    button.grip { background: var(--ok); }
    button.grip:hover { background: #0f5c38; }
    button.demo { background: var(--warn); }
    button.demo:hover { background: #93370d; }

    .panel {
      background: var(--panel);
      border: 1px solid var(--line);
      border-radius: 8px;
      box-shadow: var(--shadow);
      overflow: hidden;
    }

    .servo-row {
      display: grid;
      grid-template-columns: 190px minmax(0, 1fr) 72px;
      gap: 14px;
      align-items: center;
      padding: 16px;
      border-bottom: 1px solid var(--line);
    }

    .servo-row:last-child { border-bottom: 0; }

    .servo-name {
      font-weight: 700;
      line-height: 1.35;
    }

    .servo-range {
      color: var(--muted);
      font-size: 12px;
      margin-top: 2px;
    }

    input[type="range"] {
      width: 100%;
      accent-color: var(--brand);
    }

    .angle {
      font-variant-numeric: tabular-nums;
      text-align: right;
      font-weight: 700;
      color: var(--brand);
      white-space: nowrap;
    }

    .note {
      color: var(--muted);
      font-size: 13px;
      line-height: 1.55;
      margin-top: 12px;
    }

    @media (max-width: 720px) {
      .app { padding: 14px; }
      header {
        display: block;
      }
      h1 { font-size: 23px; }
      .status {
        text-align: left;
        margin-top: 12px;
      }
      .controls {
        grid-template-columns: repeat(2, minmax(0, 1fr));
      }
      .controls button:first-child {
        grid-column: 1 / -1;
      }
      .servo-row {
        grid-template-columns: 1fr 64px;
        gap: 10px;
      }
      .servo-row input[type="range"] {
        grid-column: 1 / -1;
      }
      .angle {
        align-self: start;
      }
    }
  </style>
</head>
<body>
  <main class="app">
    <header>
      <div>
        <h1>ESP32 Robot Arm</h1>
        <div class="subtitle">Access Point: ESP32_ROBOT_ARM · IP: 192.168.4.1</div>
      </div>
      <div class="status">
        สถานะ
        <strong id="statusText">พร้อมใช้งาน</strong>
      </div>
    </header>

    <section class="controls" aria-label="Robot commands">
      <button class="secondary" onclick="runCommand('home')">HOME</button>
      <button class="grip" onclick="runCommand('open')">OPEN GRIPPER</button>
      <button class="grip" onclick="runCommand('close')">CLOSE GRIPPER</button>
      <button class="demo" onclick="runCommand('pick')">PICK DEMO</button>
      <button class="demo" onclick="runCommand('place')">PLACE DEMO</button>
    </section>

    <section id="servoPanel" class="panel"></section>

    <p class="note">
      ปรับ Slider เพื่อส่งค่ามุมไปยัง ESP32 แบบ real-time ระบบจะจำกัดมุมตามค่าปลอดภัยของแต่ละแกนอัตโนมัติ
    </p>
  </main>

  <script>
    const servos = [
      { id: 0, name: 'BASE', label: 'ฐานหมุนซ้าย/ขวา', min: 0, max: 180, value: 90 },
      { id: 1, name: 'SHOULDER', label: 'ไหล่ยกขึ้น/ลง', min: 30, max: 150, value: 90 },
      { id: 2, name: 'ELBOW', label: 'ศอกพับเข้า/ออก', min: 20, max: 160, value: 90 },
      { id: 3, name: 'WRIST_PITCH', label: 'ข้อมือก้ม/เงย', min: 20, max: 160, value: 90 },
      { id: 4, name: 'WRIST_ROTATE', label: 'หมุนข้อมือ', min: 0, max: 180, value: 90 },
      { id: 5, name: 'GRIPPER', label: 'ก้ามปูเปิด/ปิด', min: 30, max: 120, value: 80 }
    ];

    const panel = document.getElementById('servoPanel');
    const statusText = document.getElementById('statusText');
    const sendTimers = {};

    function render() {
      panel.innerHTML = servos.map((servo) => `
        <div class="servo-row">
          <div>
            <div class="servo-name">${servo.name}</div>
            <div class="servo-range">${servo.label} · ${servo.min}°-${servo.max}°</div>
          </div>
          <input id="slider-${servo.id}" type="range" min="${servo.min}" max="${servo.max}" value="${servo.value}" oninput="sliderChanged(${servo.id}, this.value)">
          <div id="angle-${servo.id}" class="angle">${servo.value}°</div>
        </div>
      `).join('');
    }

    function setStatus(text) {
      statusText.textContent = text;
    }

    function updateServoUi(id, angle) {
      const slider = document.getElementById(`slider-${id}`);
      const label = document.getElementById(`angle-${id}`);
      if (slider) slider.value = angle;
      if (label) label.textContent = `${angle}°`;
    }

    function sliderChanged(id, value) {
      const angle = Number(value);
      updateServoUi(id, angle);
      clearTimeout(sendTimers[id]);
      sendTimers[id] = setTimeout(() => sendServo(id, angle), 35);
    }

    async function sendServo(id, angle) {
      try {
        const response = await fetch(`/set?servo=${id}&angle=${angle}`, { cache: 'no-store' });
        const data = await response.json();
        if (data.ok) {
          updateServoUi(data.servo, data.target);
          setStatus('กำลังควบคุม');
        } else {
          setStatus('คำสั่งไม่ถูกต้อง');
        }
      } catch (error) {
        setStatus('เชื่อมต่อไม่ได้');
      }
    }

    async function runCommand(name) {
      setStatus('กำลังส่งคำสั่ง');
      try {
        const response = await fetch(`/cmd?name=${name}`, { cache: 'no-store' });
        const data = await response.json();
        setStatus(data.ok ? 'กำลังทำงาน' : 'คำสั่งไม่ถูกต้อง');
        setTimeout(loadState, 700);
      } catch (error) {
        setStatus('เชื่อมต่อไม่ได้');
      }
    }

    async function loadState() {
      try {
        const response = await fetch('/state', { cache: 'no-store' });
        const data = await response.json();
        if (data.ok && Array.isArray(data.angles)) {
          data.angles.forEach((angle, id) => updateServoUi(id, angle));
          setStatus(data.demoRunning ? 'Demo กำลังทำงาน' : 'พร้อมใช้งาน');
        }
      } catch (error) {
        setStatus('เชื่อมต่อไม่ได้');
      }
    }

    render();
    loadState();
    setInterval(loadState, 2000);
  </script>
</body>
</html>
)rawliteral";

// -------------------- Servo Helpers --------------------
int safeAngle(uint8_t servoIndex, int angle) {
  if (servoIndex >= SERVO_COUNT) {
    return 0;
  }
  return constrain(angle, minAngles[servoIndex], maxAngles[servoIndex]);
}

void writeServoNow(uint8_t servoIndex, int angle) {
  if (servoIndex >= SERVO_COUNT) {
    return;
  }

  int limitedAngle = safeAngle(servoIndex, angle);
  servos[servoIndex].write(limitedAngle);
  currentAngles[servoIndex] = limitedAngle;
}

// ตั้งเป้าหมายมุม Servo โดยจำกัดมุมให้อยู่ในช่วงปลอดภัย
void setServo(uint8_t servoIndex, int angle) {
  if (servoIndex >= SERVO_COUNT) {
    Serial.println("setServo: servo index ไม่ถูกต้อง");
    return;
  }

  int limitedAngle = safeAngle(servoIndex, angle);
  if (targetAngles[servoIndex] != limitedAngle) {
    Serial.printf("Set %s target = %d deg\n", servoNames[servoIndex], limitedAngle);
  }
  targetAngles[servoIndex] = limitedAngle;
}

bool allServosAtTarget() {
  for (uint8_t i = 0; i < SERVO_COUNT; i++) {
    if (currentAngles[i] != targetAngles[i]) {
      return false;
    }
  }
  return true;
}

// ขยับ Servo ทั้งหมดทีละ 1 องศาแบบ non-blocking ใช้สำหรับ Slider
void updateServoMotion() {
  static unsigned long lastStepMs = 0;
  unsigned long now = millis();

  if (now - lastStepMs < MANUAL_STEP_INTERVAL_MS) {
    return;
  }
  lastStepMs = now;

  for (uint8_t i = 0; i < SERVO_COUNT; i++) {
    if (currentAngles[i] < targetAngles[i]) {
      writeServoNow(i, currentAngles[i] + 1);
    } else if (currentAngles[i] > targetAngles[i]) {
      writeServoNow(i, currentAngles[i] - 1);
    }
  }
}

// ฟังก์ชันบังคับให้ Servo ขยับแบบนุ่มนวลจนถึงเป้าหมาย ใช้ใน HOME และ Demo
void moveServoSmooth(uint8_t servoIndex, int angle, uint16_t stepDelayMs = 10) {
  if (servoIndex >= SERVO_COUNT) {
    return;
  }

  int limitedAngle = safeAngle(servoIndex, angle);
  targetAngles[servoIndex] = limitedAngle;

  Serial.printf("Move %s smooth to %d deg\n", servoNames[servoIndex], limitedAngle);

  while (currentAngles[servoIndex] != limitedAngle) {
    if (currentAngles[servoIndex] < limitedAngle) {
      writeServoNow(servoIndex, currentAngles[servoIndex] + 1);
    } else {
      writeServoNow(servoIndex, currentAngles[servoIndex] - 1);
    }

    unsigned long startedMs = millis();
    while (millis() - startedMs < stepDelayMs) {
      server.handleClient();
      delay(1);
      yield();
    }
  }

  targetAngles[servoIndex] = currentAngles[servoIndex];
}

void waitWithWebServer(uint16_t waitMs) {
  unsigned long startedMs = millis();
  while (millis() - startedMs < waitMs) {
    server.handleClient();
    updateServoMotion();
    delay(2);
    yield();
  }
}

void movePoseSmooth(const int pose[SERVO_COUNT], uint16_t stepDelayMs = 10) {
  for (uint8_t i = 0; i < SERVO_COUNT; i++) {
    targetAngles[i] = safeAngle(i, pose[i]);
  }

  Serial.println("Move pose smooth");

  while (!allServosAtTarget()) {
    for (uint8_t i = 0; i < SERVO_COUNT; i++) {
      if (currentAngles[i] < targetAngles[i]) {
        writeServoNow(i, currentAngles[i] + 1);
      } else if (currentAngles[i] > targetAngles[i]) {
        writeServoNow(i, currentAngles[i] - 1);
      }
    }

    unsigned long startedMs = millis();
    while (millis() - startedMs < stepDelayMs) {
      server.handleClient();
      delay(1);
      yield();
    }
  }
}

// -------------------- Robot Commands --------------------
void homePosition() {
  Serial.println("Command: HOME");
  movePoseSmooth(homeAngles, 9);
}

void openGripper() {
  Serial.println("Command: OPEN GRIPPER");
  moveServoSmooth(GRIPPER, GRIPPER_OPEN_ANGLE, 8);
}

void closeGripper() {
  Serial.println("Command: CLOSE GRIPPER");
  moveServoSmooth(GRIPPER, GRIPPER_CLOSE_ANGLE, 8);
}

void pickDemo() {
  Serial.println("Command: PICK DEMO start");

  const int prePick[SERVO_COUNT] = {75, 95, 95, 90, 90, GRIPPER_OPEN_ANGLE};
  const int lowerToPick[SERVO_COUNT] = {75, 70, 130, 80, 90, GRIPPER_OPEN_ANGLE};
  const int liftObject[SERVO_COUNT] = {75, 105, 95, 90, 90, GRIPPER_CLOSE_ANGLE};

  movePoseSmooth(prePick, 9);
  waitWithWebServer(250);
  movePoseSmooth(lowerToPick, 9);
  waitWithWebServer(250);
  closeGripper();
  waitWithWebServer(350);
  movePoseSmooth(liftObject, 9);
  waitWithWebServer(300);
  homePosition();

  Serial.println("Command: PICK DEMO done");
}

void placeDemo() {
  Serial.println("Command: PLACE DEMO start");

  const int carryPose[SERVO_COUNT] = {100, 105, 95, 90, 90, GRIPPER_CLOSE_ANGLE};
  const int placePose[SERVO_COUNT] = {130, 75, 125, 95, 90, GRIPPER_CLOSE_ANGLE};
  const int retreatPose[SERVO_COUNT] = {130, 105, 95, 90, 90, GRIPPER_OPEN_ANGLE};

  movePoseSmooth(carryPose, 9);
  waitWithWebServer(250);
  movePoseSmooth(placePose, 9);
  waitWithWebServer(250);
  openGripper();
  waitWithWebServer(350);
  movePoseSmooth(retreatPose, 9);
  waitWithWebServer(300);
  homePosition();

  Serial.println("Command: PLACE DEMO done");
}

void runPendingCommand() {
  if (pendingCommand == CMD_NONE || demoRunning) {
    return;
  }

  Command command = pendingCommand;
  pendingCommand = CMD_NONE;
  demoRunning = true;

  switch (command) {
    case CMD_HOME:
      homePosition();
      break;
    case CMD_OPEN_GRIPPER:
      openGripper();
      break;
    case CMD_CLOSE_GRIPPER:
      closeGripper();
      break;
    case CMD_PICK_DEMO:
      pickDemo();
      break;
    case CMD_PLACE_DEMO:
      placeDemo();
      break;
    default:
      break;
  }

  demoRunning = false;
}

// -------------------- Web Handlers --------------------
void handleRoot() {
  server.send(200, "text/html; charset=utf-8", INDEX_HTML);
}

String buildStateJson() {
  String json = "{\"ok\":true,\"demoRunning\":";
  json += demoRunning ? "true" : "false";
  json += ",\"angles\":[";
  for (uint8_t i = 0; i < SERVO_COUNT; i++) {
    if (i > 0) {
      json += ",";
    }
    json += String(currentAngles[i]);
  }
  json += "],\"targets\":[";
  for (uint8_t i = 0; i < SERVO_COUNT; i++) {
    if (i > 0) {
      json += ",";
    }
    json += String(targetAngles[i]);
  }
  json += "]}";
  return json;
}

void handleState() {
  server.send(200, "application/json", buildStateJson());
}

void handleSetServo() {
  if (!server.hasArg("servo") || !server.hasArg("angle")) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing servo or angle\"}");
    return;
  }

  int servoIndex = server.arg("servo").toInt();
  int angle = server.arg("angle").toInt();

  if (servoIndex < 0 || servoIndex >= SERVO_COUNT) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid servo index\"}");
    return;
  }

  setServo((uint8_t)servoIndex, angle);

  String json = "{\"ok\":true,\"servo\":";
  json += String(servoIndex);
  json += ",\"target\":";
  json += String(targetAngles[servoIndex]);
  json += "}";
  server.send(200, "application/json", json);
}

void handleCommand() {
  if (!server.hasArg("name")) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing command name\"}");
    return;
  }

  String name = server.arg("name");

  if (name == "home") {
    pendingCommand = CMD_HOME;
  } else if (name == "open") {
    pendingCommand = CMD_OPEN_GRIPPER;
  } else if (name == "close") {
    pendingCommand = CMD_CLOSE_GRIPPER;
  } else if (name == "pick") {
    pendingCommand = CMD_PICK_DEMO;
  } else if (name == "place") {
    pendingCommand = CMD_PLACE_DEMO;
  } else {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"unknown command\"}");
    return;
  }

  Serial.printf("Web command queued: %s\n", name.c_str());
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleNotFound() {
  server.send(404, "application/json", "{\"ok\":false,\"error\":\"not found\"}");
}

// -------------------- Setup / Loop --------------------
void attachServos() {
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  for (uint8_t i = 0; i < SERVO_COUNT; i++) {
    servos[i].setPeriodHertz(50);
    servos[i].attach(servoPins[i], SERVO_MIN_PULSE_US, SERVO_MAX_PULSE_US);
    currentAngles[i] = homeAngles[i];
    targetAngles[i] = homeAngles[i];
    servos[i].write(homeAngles[i]);

    Serial.printf("Servo %u %-13s GPIO%u HOME=%d range=%d-%d\n",
                  i + 1,
                  servoNames[i],
                  servoPins[i],
                  homeAngles[i],
                  minAngles[i],
                  maxAngles[i]);
  }
}

void setupWiFiAP() {
  WiFi.mode(WIFI_AP);

  if (!WiFi.softAPConfig(localIP, gateway, subnet)) {
    Serial.println("WiFi AP config failed");
  }

  if (WiFi.softAP(AP_SSID, AP_PASSWORD)) {
    Serial.println("WiFi Access Point started");
    Serial.printf("SSID: %s\n", AP_SSID);
    Serial.printf("Password: %s\n", AP_PASSWORD);
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("WiFi Access Point failed");
  }
}

void setupWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/state", HTTP_GET, handleState);
  server.on("/set", HTTP_GET, handleSetServo);
  server.on("/cmd", HTTP_GET, handleCommand);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Web Server started on port 80");
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("ESP32 Robot Arm Web Control");
  Serial.println("Servo PWM direct from ESP32 GPIO, no PCA9685");

  attachServos();
  setupWiFiAP();
  setupWebServer();

  Serial.println("Ready. Connect WiFi and open http://192.168.4.1");
}

void loop() {
  server.handleClient();
  runPendingCommand();
  updateServoMotion();
}
