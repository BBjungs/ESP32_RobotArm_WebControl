# ESP32 Robot Arm Web Control

โปรเจกต์นี้เป็นโปรแกรมควบคุมแขนกล Robot Arm แบบ Servo Motor จำนวน 6 ตัว ด้วยบอร์ด ESP32 DevKit โดย ESP32 ส่งสัญญาณ PWM ไปยัง Servo โดยตรงผ่าน GPIO ไม่ใช้บอร์ด PCA9685 และควบคุมผ่านหน้าเว็บที่ ESP32 สร้างขึ้นเองในโหมด WiFi Access Point

เมื่ออัปโหลดโปรแกรมแล้ว ผู้ใช้สามารถเชื่อมต่อ WiFi ของ ESP32 แล้วเปิดเว็บ `http://192.168.4.1` เพื่อควบคุม Servo ทั้ง 6 แกนด้วย Slider และปุ่มคำสั่งสำเร็จรูป

## คุณสมบัติหลัก

- ใช้ PlatformIO Framework Arduino สำหรับ ESP32
- ใช้ Library `ESP32Servo`
- ใช้ `WiFi.h` เพื่อเปิด ESP32 เป็น WiFi Access Point
- ใช้ `WebServer` ของ Arduino ESP32 เพื่อสร้างหน้าเว็บควบคุม
- ควบคุม Servo 6 ตัวผ่าน GPIO โดยตรง
- มี Slider ปรับมุม Servo แบบ real-time
- มีปุ่ม `HOME`, `OPEN GRIPPER`, `CLOSE GRIPPER`, `PICK DEMO`, `PLACE DEMO`
- มีระบบจำกัดช่วงมุมของ Servo แต่ละแกนด้วย `constrain`
- Servo ขยับแบบนุ่มนวลด้วย `moveServoSmooth()` และระบบขยับทีละองศา
- แสดงสถานะการทำงานผ่าน Serial Monitor
- หน้าเว็บรองรับมือถือ

## อุปกรณ์ที่ใช้

- ESP32 DevKit
- Servo Motor จำนวน 6 ตัว
- แหล่งจ่ายไฟ Servo แยก 5V หรือ 6V กระแสสูง
- สาย Jumper
- Capacitor 470uF ถึง 2200uF สำหรับคร่อมไฟเลี้ยง Servo
- คอมพิวเตอร์ที่ติดตั้ง PlatformIO

## คำเตือนด้านไฟเลี้ยง

ห้ามจ่ายไฟ Servo จากขา `5V` หรือ `3.3V` ของ ESP32 โดยตรง เพราะ Servo หลายตัวใช้กระแสสูงมาก โดยเฉพาะตอนเริ่มหมุนหรือตอนมีโหลด อาจทำให้ ESP32 รีเซ็ต ทำงานผิดพลาด หรือเสียหายได้

คำแนะนำ:

- ใช้แหล่งจ่าย Servo แยก 5V หรือ 6V อย่างน้อย 10A
- ต่อ `GND` ของแหล่งจ่าย Servo ร่วมกับ `GND` ของ ESP32 เสมอ
- ต่อสายสัญญาณ Servo จาก ESP32 ไปยังขา Signal ของ Servo
- ใส่ Capacitor 470uF ถึง 2200uF คร่อม `V+` และ `GND` ของไฟเลี้ยง Servo เพื่อลดอาการไฟตกและ Servo กระตุก

## การต่อสาย

| Servo | แกน | GPIO สัญญาณ | ไฟเลี้ยง Servo | GND |
|---|---|---:|---|---|
| Servo 1 | BASE ฐานหมุนซ้ายขวา | GPIO13 | 5V/6V จากแหล่งจ่ายแยก | ต่อร่วมกับ GND ESP32 |
| Servo 2 | SHOULDER ไหล่ยกขึ้นลง | GPIO14 | 5V/6V จากแหล่งจ่ายแยก | ต่อร่วมกับ GND ESP32 |
| Servo 3 | ELBOW ศอกพับเข้าออก | GPIO16 | 5V/6V จากแหล่งจ่ายแยก | ต่อร่วมกับ GND ESP32 |
| Servo 4 | WRIST_PITCH ข้อมือก้มเงย | GPIO17 | 5V/6V จากแหล่งจ่ายแยก | ต่อร่วมกับ GND ESP32 |
| Servo 5 | WRIST_ROTATE หมุนข้อมือ | GPIO18 | 5V/6V จากแหล่งจ่ายแยก | ต่อร่วมกับ GND ESP32 |
| Servo 6 | GRIPPER ก้ามปูเปิดปิด | GPIO19 | 5V/6V จากแหล่งจ่ายแยก | ต่อร่วมกับ GND ESP32 |

โครงสร้างไฟโดยรวม:

```text
ESP32 GPIO13  -> BASE Signal
ESP32 GPIO14  -> SHOULDER Signal
ESP32 GPIO16  -> ELBOW Signal
ESP32 GPIO17  -> WRIST_PITCH Signal
ESP32 GPIO18  -> WRIST_ROTATE Signal
ESP32 GPIO19  -> GRIPPER Signal

Servo V+      -> 5V/6V จากแหล่งจ่าย Servo แยก
Servo GND     -> GND แหล่งจ่าย Servo
ESP32 GND     -> GND แหล่งจ่าย Servo
```

## ช่วงมุม Servo

โปรแกรมจำกัดช่วงมุมเพื่อช่วยลดโอกาสแขนกลชนกันหรือฝืนกลไก

| แกน | ช่วงมุม |
|---|---:|
| BASE | 0 ถึง 180 องศา |
| SHOULDER | 30 ถึง 150 องศา |
| ELBOW | 20 ถึง 160 องศา |
| WRIST_PITCH | 20 ถึง 160 องศา |
| WRIST_ROTATE | 0 ถึง 180 องศา |
| GRIPPER | 30 ถึง 120 องศา |

## ตำแหน่ง HOME

เมื่อเริ่มระบบหรือกดปุ่ม `HOME` Servo จะกลับไปยังตำแหน่งเริ่มต้นนี้

| แกน | มุม HOME |
|---|---:|
| BASE | 90 |
| SHOULDER | 90 |
| ELBOW | 90 |
| WRIST_PITCH | 90 |
| WRIST_ROTATE | 90 |
| GRIPPER | 80 |

## โครงสร้างโปรเจกต์

```text
ESP32_RobotArm_WebControl/
├── platformio.ini
├── README.md
└── src/
    └── main.cpp
```

ไฟล์สำคัญ:

- `platformio.ini` กำหนดบอร์ด ESP32, Framework Arduino และ Library `ESP32Servo`
- `src/main.cpp` เป็นโปรแกรมหลักทั้งหมด เช่น WiFi AP, Web Server, หน้าเว็บ, การควบคุม Servo และฟังก์ชัน Demo

## การอัปโหลดโปรแกรม

เปิด Terminal ที่โฟลเดอร์โปรเจกต์ แล้วรัน:

```powershell
pio run -t upload
```

เปิด Serial Monitor:

```powershell
pio device monitor
```

Serial Monitor ใช้ความเร็ว `115200`

ถ้าอัปโหลดไม่ได้ในบางบอร์ด ให้กดปุ่ม `BOOT` บน ESP32 ค้างไว้ตอนเริ่มอัปโหลด แล้วปล่อยเมื่อ PlatformIO เริ่มเขียนแฟลช

## การเชื่อมต่อ WiFi และเข้าเว็บ

หลังอัปโหลดเสร็จ ESP32 จะเปิด WiFi Access Point ด้วยค่าต่อไปนี้

| รายการ | ค่า |
|---|---|
| SSID | ESP32_ROBOT_ARM |
| Password | 12345678 |
| IP Address | 192.168.4.1 |

ขั้นตอนใช้งาน:

1. เปิด WiFi บนมือถือหรือคอมพิวเตอร์
2. เชื่อมต่อ WiFi ชื่อ `ESP32_ROBOT_ARM`
3. ใส่รหัสผ่าน `12345678`
4. เปิด Browser
5. เข้าเว็บ `http://192.168.4.1`
6. ใช้ Slider และปุ่มบนหน้าเว็บเพื่อควบคุมแขนกล

## วิธีใช้งานหน้าเว็บ

หน้าเว็บมี Slider สำหรับ Servo ทั้ง 6 แกน

- เลื่อน Slider เพื่อปรับมุม Servo
- ค่ามุมปัจจุบันจะแสดงเป็นองศาข้าง Slider
- เมื่อเลื่อน Slider หน้าเว็บจะส่งค่าไปที่ ESP32 แบบ real-time
- ESP32 จะจำกัดมุมให้อยู่ในช่วงปลอดภัยก่อนสั่ง Servo

ปุ่มคำสั่ง:

| ปุ่ม | การทำงาน |
|---|---|
| HOME | กลับตำแหน่งเริ่มต้นของทุกแกน |
| OPEN GRIPPER | เปิดก้ามปู |
| CLOSE GRIPPER | ปิดก้ามปู |
| PICK DEMO | สาธิตท่าหยิบของ |
| PLACE DEMO | สาธิตท่าวางของ |

## หลักการทำงานของโปรแกรม

เมื่อ ESP32 เริ่มทำงาน โปรแกรมจะทำงานตามลำดับนี้

1. เปิด Serial Monitor ที่ความเร็ว `115200`
2. Attach Servo ทั้ง 6 ตัวเข้ากับ GPIO ที่กำหนด
3. ตั้งตำแหน่งเริ่มต้นของ Servo เป็นค่า HOME
4. เปิด WiFi แบบ Access Point ชื่อ `ESP32_ROBOT_ARM`
5. เริ่ม Web Server ที่ port `80`
6. รอรับคำสั่งจากหน้าเว็บ

ใน `loop()` โปรแกรมจะทำงานวนซ้ำดังนี้

```cpp
server.handleClient();
runPendingCommand();
updateServoMotion();
```

ความหมาย:

- `server.handleClient()` รับ request จากหน้าเว็บ
- `runPendingCommand()` ตรวจว่ามีคำสั่งจากปุ่ม HOME หรือ Demo รอทำงานหรือไม่
- `updateServoMotion()` ขยับ Servo แบบนุ่มนวลทีละ 1 องศาไปยังมุมเป้าหมาย

## ฟังก์ชันสำคัญ

| ฟังก์ชัน | หน้าที่ |
|---|---|
| `setServo()` | รับค่ามุมจากเว็บ จำกัดมุม และตั้งเป็นเป้าหมายของ Servo |
| `moveServoSmooth()` | ขยับ Servo ทีละองศาแบบนุ่มนวล |
| `homePosition()` | สั่งทุกแกนกลับตำแหน่ง HOME |
| `openGripper()` | เปิดก้ามปู |
| `closeGripper()` | ปิดก้ามปู |
| `pickDemo()` | ทำลำดับท่าทางสาธิตการหยิบของ |
| `placeDemo()` | ทำลำดับท่าทางสาธิตการวางของ |

## Web API ภายใน

หน้าเว็บเรียก API ภายใน ESP32 ด้วย HTTP GET

| Endpoint | ตัวอย่าง | หน้าที่ |
|---|---|---|
| `/` | `http://192.168.4.1/` | หน้าเว็บควบคุม |
| `/state` | `http://192.168.4.1/state` | อ่านมุมปัจจุบันของ Servo |
| `/set` | `http://192.168.4.1/set?servo=0&angle=90` | ตั้งมุม Servo |
| `/cmd` | `http://192.168.4.1/cmd?name=home` | ส่งคำสั่งปุ่ม |

หมายเลข Servo ใน API:

| หมายเลข | แกน |
|---:|---|
| 0 | BASE |
| 1 | SHOULDER |
| 2 | ELBOW |
| 3 | WRIST_PITCH |
| 4 | WRIST_ROTATE |
| 5 | GRIPPER |

คำสั่งที่ใช้กับ `/cmd`:

| name | การทำงาน |
|---|---|
| `home` | กลับตำแหน่ง HOME |
| `open` | เปิดก้ามปู |
| `close` | ปิดก้ามปู |
| `pick` | สาธิตหยิบของ |
| `place` | สาธิตวางของ |

## การปรับมุม Servo แต่ละแกน

ค่าช่วงมุมและค่า HOME อยู่ในไฟล์ `src/main.cpp`

```cpp
const int minAngles[SERVO_COUNT] = {0, 30, 20, 20, 0, 30};
const int maxAngles[SERVO_COUNT] = {180, 150, 160, 160, 180, 120};
const int homeAngles[SERVO_COUNT] = {90, 90, 90, 90, 90, 80};
```

ถ้าแขนกลชนโครงสร้างหรือ Servo ฝืน ให้ลดช่วงมุมใน `minAngles` และ `maxAngles`

ตัวอย่าง: ต้องการจำกัด `SHOULDER` ให้แคบลงจาก 30 ถึง 150 เป็น 45 ถึง 135

```cpp
const int minAngles[SERVO_COUNT] = {0, 45, 20, 20, 0, 30};
const int maxAngles[SERVO_COUNT] = {180, 135, 160, 160, 180, 120};
```

## ถ้า Servo หมุนกลับทิศ

Servo แต่ละรุ่นหรือการติดตั้งกลไกอาจทำให้ทิศทางการหมุนกลับกันได้ วิธีแก้คือกลับค่ามุมเฉพาะแกนที่ต้องการ

แก้ในฟังก์ชัน `writeServoNow()` หรือเพิ่ม logic ใน `safeAngle()`

ตัวอย่างกลับทิศเฉพาะ `BASE`:

```cpp
int safeAngle(uint8_t servoIndex, int angle) {
  if (servoIndex >= SERVO_COUNT) {
    return 0;
  }

  int limited = constrain(angle, minAngles[servoIndex], maxAngles[servoIndex]);

  if (servoIndex == BASE) {
    limited = minAngles[servoIndex] + maxAngles[servoIndex] - limited;
  }

  return limited;
}
```

ถ้าก้ามปูเปิดและปิดสลับกัน ให้สลับค่าตรงนี้:

```cpp
const int GRIPPER_OPEN_ANGLE = 120;
const int GRIPPER_CLOSE_ANGLE = 35;
```

เช่นเปลี่ยนเป็น:

```cpp
const int GRIPPER_OPEN_ANGLE = 35;
const int GRIPPER_CLOSE_ANGLE = 120;
```

## การปรับท่า Demo

ท่า Demo อยู่ใน `pickDemo()` และ `placeDemo()`

ตัวอย่างใน `pickDemo()`:

```cpp
const int prePick[SERVO_COUNT] = {75, 95, 95, 90, 90, GRIPPER_OPEN_ANGLE};
const int lowerToPick[SERVO_COUNT] = {75, 70, 130, 80, 90, GRIPPER_OPEN_ANGLE};
const int liftObject[SERVO_COUNT] = {75, 105, 95, 90, 90, GRIPPER_CLOSE_ANGLE};
```

ลำดับตัวเลขใน array คือ:

```text
{BASE, SHOULDER, ELBOW, WRIST_PITCH, WRIST_ROTATE, GRIPPER}
```

ถ้าต้องการปรับท่าหยิบหรือวาง ให้แก้ค่ามุมใน array เหล่านี้ตามกลไกแขนจริง

## การแก้ปัญหาเบื้องต้น

| อาการ | แนวทางตรวจสอบ |
|---|---|
| ESP32 รีเซ็ตตอน Servo ขยับ | แหล่งจ่าย Servo กระแสไม่พอ, ลืมต่อ GND ร่วม, ไม่มี capacitor |
| Servo กระตุก | ไฟตก, สายไฟเล็กเกินไป, ต่อ capacitor เพิ่ม |
| Servo ไม่ขยับ | ตรวจ GPIO, ตรวจสาย Signal, ตรวจไฟเลี้ยง Servo, ตรวจ GND ร่วม |
| เข้าเว็บไม่ได้ | ตรวจว่าเชื่อม WiFi `ESP32_ROBOT_ARM`, เปิด `http://192.168.4.1` |
| Slider ขยับแต่ Servo ไม่ตอบสนอง | ดู Serial Monitor, ตรวจว่า Servo ต่อกับ GPIO ถูกต้อง |
| ก้ามปูเปิดปิดกลับกัน | สลับค่า `GRIPPER_OPEN_ANGLE` และ `GRIPPER_CLOSE_ANGLE` |
| แขนชนโครงสร้าง | ลดช่วงมุมใน `minAngles` และ `maxAngles` |

## คำสั่ง PlatformIO ที่ใช้บ่อย

Build:

```powershell
pio run
```

Upload:

```powershell
pio run -t upload
```

Serial Monitor:

```powershell
pio device monitor
```

Clean build:

```powershell
pio run -t clean
```
