/*
 * ============================================================
 *  MOUSLIDE — Mouse adaptativo por Bluetooth HID
 *  Dispositivo: ESP32 DevKit V1
 *  Comunicación: Bluetooth Classic HID (funciona como mouse)
 *  Hardware: MPU-6050 (movimiento) + 6x TTP223B (botones)
 *  IDE: Arduino IDE 2.x
 * ============================================================
 *
 *  PINES UTILIZADOS:
 *    MPU-6050  → SDA: GPIO21 | SCL: GPIO22 | VCC: 3V3 | GND: GND
 *    TTP223B ↑ → GPIO13
 *    TTP223B ↓ → GPIO12
 *    TTP223B ← → GPIO14
 *    TTP223B → → GPIO27
 *    TTP223B Enter (clic izq) → GPIO26
 *    TTP223B Space (clic der) → GPIO25
 *    LED estado BT → GPIO2 (LED interno del DevKit)
 *
 *  LIBRERÍA REQUERIDA:
 *    ESP32-BLE-Mouse por T-vK
 *    Instalar manualmente desde:
 *    https://github.com/T-vK/ESP32-BLE-Mouse
 *    (descargar ZIP → Sketch → Incluir librería → Añadir .ZIP)
 *
 *    Adafruit MPU6050     (gestor de librerías Arduino)
 *    Adafruit Unified Sensor (gestor de librerías Arduino)
 *
 *  BOARD SETTINGS en Arduino IDE:
 *    Board: "ESP32 Dev Module"
 *    Upload Speed: 921600
 *    CPU Frequency: 240MHz
 *    Flash Size: 4MB
 *    Partition Scheme: Default 4MB with spiffs
 * ============================================================
 */

// ── Librerías ─────────────────────────────────────────────
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <BleMouse.h>

// ── Instancias ────────────────────────────────────────────
Adafruit_MPU6050 mpu;

// BleMouse(nombre_visible, fabricante, bateria_%)
BleMouse bleMouse("Mouslide", "ITP Pachuca", 100);

// ── Pines de botones TTP223B ──────────────────────────────
const int PIN_UP    = 32;   // ↑ Arriba
const int PIN_DOWN  = 33;   // ↓ Abajo
const int PIN_LEFT  = 14;   // ← Izquierda
const int PIN_RIGHT = 27;   // → Derecha
const int PIN_ENTER = 26;   // Enter = clic izquierdo
const int PIN_SPACE = 25;   // Space = clic derecho
const int PIN_LED   = 2;    // LED indicador (LED azul interno del DevKit)

// ── Parámetros de movimiento ──────────────────────────────
/*
 *  SENSITIVITY: multiplica la aceleración para obtener el movimiento del cursor.
 *    - Aumenta este valor si el cursor se mueve muy poco.
 *    - Disminuye si se mueve demasiado rápido.
 *    Valor recomendado inicial: 8.0 — ajustar según el usuario.
 */
const float SENSITIVITY   = 5.0;

/*
 *  DEAD_ZONE: zona muerta en m/s². Movimientos menores a este valor
 *  son ignorados para evitar que el cursor derive cuando el dispositivo
 *  está en reposo. Aumentar si hay drift; disminuir si no responde bien.
 */
const float DEAD_ZONE     = 1.5;

// Intervalo de envío de posición del cursor (~60 Hz)
const int MOVE_DELAY_MS   = 16;

/*
 *  USE_MPU = true  → el MPU-6050 controla el movimiento del cursor
 *                     (inclinar el dispositivo = mover el cursor)
 *                     Los 4 botones de dirección hacen: scroll, doble clic, clic central
 *
 *  USE_MPU = false → los 4 botones TTP223B mueven el cursor paso a paso
 *                     (útil si no tienes MPU-6050 o como modo alternativo)
 */
const bool USE_MPU = false;

// ── Variables de estado (detección de borde para botones) ─
bool lastUp    = false;
bool lastDown  = false;
bool lastLeft  = false;
bool lastRight = false;
bool lastEnter = false;
bool lastSpace = false;

// ── Variables de tiempo ───────────────────────────────────
unsigned long lastMoveTime = 0;
unsigned long lastLedBlink = 0;
bool ledState = false;


// ═════════════════════════════════════════════════════════
//  SETUP
// ═════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== MOUSLIDE v1.0 ===");
  Serial.println("Iniciando hardware...");

  // ── LED ─────────────────────────────────────────────────
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  // ── Botones TTP223B (el sensor ya provee nivel alto al tocarlo) ──
  // INPUT_PULLDOWN asegura nivel bajo cuando no se toca
  pinMode(PIN_UP,    INPUT_PULLDOWN);
  pinMode(PIN_DOWN,  INPUT_PULLDOWN);
  pinMode(PIN_LEFT,  INPUT_PULLDOWN);
  pinMode(PIN_RIGHT, INPUT_PULLDOWN);
  pinMode(PIN_ENTER, INPUT_PULLDOWN);
  pinMode(PIN_SPACE, INPUT_PULLDOWN);

  // ── MPU-6050 ─────────────────────────────────────────────
  if (USE_MPU) {
    Wire.begin(21, 22);  // SDA=GPIO21, SCL=GPIO22

    if (!mpu.begin()) {
      Serial.println("[ERROR] MPU-6050 no detectado.");
      Serial.println("Verifica: VCC->3V3, GND->GND, SDA->GPIO21, SCL->GPIO22");
      // Parpadeo rápido de error hasta que se resuelva
      while (true) {
        digitalWrite(PIN_LED, HIGH); delay(100);
        digitalWrite(PIN_LED, LOW);  delay(100);
      }
    }
    Serial.println("[OK] MPU-6050 detectado.");

    // Rango del acelerómetro: ±2g
    // (suficiente para movimientos lentos de un miembro)
    mpu.setAccelerometerRange(MPU6050_RANGE_2_G);

    // Rango del giroscopio: ±250 grados/segundo
    mpu.setGyroRange(MPU6050_RANGE_250_DEG);

    // Filtro de paso bajo: 21 Hz — suaviza vibraciones sin añadir mucha latencia
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    Serial.println("[OK] MPU-6050 configurado.");
  }

  // ── Bluetooth HID ────────────────────────────────────────
  Serial.println("Iniciando Bluetooth...");
  bleMouse.begin();
  Serial.println("[OK] Bluetooth activo.");
  Serial.println("Busca el dispositivo 'Mouslide' en tu laptop para emparejar.");
  Serial.println("LED parpadeando lento = buscando conexion.");
  Serial.println("LED fijo = conectado y listo.");
}


// ═════════════════════════════════════════════════════════
//  LOOP PRINCIPAL
// ═════════════════════════════════════════════════════════
void loop() {

  // ── Sin conexión BT: parpadear y esperar ────────────────
  if (!bleMouse.isConnected()) {
    blinkLed(700);
    delay(10);
    return;
  }

  // ── Conectado: LED fijo ──────────────────────────────────
  digitalWrite(PIN_LED, HIGH);

  // ── Movimiento del cursor a ~60 Hz ──────────────────────
  unsigned long now = millis();
  if (now - lastMoveTime >= MOVE_DELAY_MS) {
    lastMoveTime = now;

    if (USE_MPU) {
      processMPUMovement();
    } else {
      processButtonMovement();
    }
  }

  // ── Botones de acción (siempre se leen) ─────────────────
  processActionButtons();
}


// ═════════════════════════════════════════════════════════
//  MOVIMIENTO POR MPU-6050
//
//  El MPU-6050 mide la aceleración en 3 ejes.
//  Con el dispositivo sujeto al miembro del usuario:
//    Eje X (accel.x) → inclinación lateral → cursor izquierda/derecha
//    Eje Y (accel.y) → inclinación frontal → cursor arriba/abajo
//
//  Si la orientación de tu montaje es diferente, intercambia
//  los ejes o cambia el signo según sea necesario.
// ═════════════════════════════════════════════════════════
void processMPUMovement() {
  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);

  float rawX = accel.acceleration.x;  // Rango aprox: -9.8 a +9.8 m/s²
  float rawY = accel.acceleration.y;

  // Aplicar zona muerta
  if (abs(rawX) < DEAD_ZONE) rawX = 0;
  if (abs(rawY) < DEAD_ZONE) rawY = 0;

  // Escalar a movimiento de cursor (rango int8: -127 a 127)
  int8_t moveX = (int8_t)constrain(rawX * SENSITIVITY, -127, 127);
  int8_t moveY = (int8_t)constrain(rawY * SENSITIVITY, -127, 127);

  if (moveX != 0 || moveY != 0) {
    // move(x, y, scroll_wheel)
    // Negamos Y porque en pantalla "abajo" es positivo pero
    // inclinar hacia adelante da Y positivo en el acelerómetro
    bleMouse.move(moveX, -moveY, 0);
  }

  // ── Debug (descomenta para calibrar la orientación) ─────
  Serial.printf("aX:%.2f aY:%.2f | mX:%d mY:%d\n", rawX, rawY, moveX, moveY);
}


// ═════════════════════════════════════════════════════════
//  MOVIMIENTO POR BOTONES (USE_MPU = false)
//  Cada TTP223B mueve el cursor en su dirección
// ═════════════════════════════════════════════════════════
void processButtonMovement() {
  // Paso en píxeles por ciclo (ajustar a gusto del usuario)
  const int8_t STEP = 6;

  int8_t moveX = 0;
  int8_t moveY = 0;

  if (digitalRead(PIN_UP))    moveY -= STEP;
  if (digitalRead(PIN_DOWN))  moveY += STEP;
  if (digitalRead(PIN_LEFT))  moveX -= STEP;
  if (digitalRead(PIN_RIGHT)) moveX += STEP;

  if (moveX != 0 || moveY != 0) {
    bleMouse.move(moveX, moveY, 0);
  }
}


// ═════════════════════════════════════════════════════════
//  BOTONES DE ACCIÓN
//
//  Cuando USE_MPU = true:
//    PIN_ENTER → clic izquierdo
//    PIN_SPACE → clic derecho
//    PIN_UP    → scroll arriba (continuo mientras se mantiene)
//    PIN_DOWN  → scroll abajo
//    PIN_LEFT  → doble clic izquierdo (al presionar)
//    PIN_RIGHT → clic central / rueda (al presionar)
//
//  Cuando USE_MPU = false:
//    PIN_ENTER → clic izquierdo
//    PIN_SPACE → clic derecho
//    (los 4 de dirección están ocupados moviendo el cursor)
// ═════════════════════════════════════════════════════════
void processActionButtons() {

  bool curEnter = digitalRead(PIN_ENTER);
  bool curSpace = digitalRead(PIN_SPACE);

  // ── Clic izquierdo ────────────────────────────────────
  if (curEnter && !lastEnter) {
    bleMouse.click(MOUSE_LEFT);
    Serial.println("[BTN] Clic izquierdo");
  }
  lastEnter = curEnter;

  // ── Clic derecho ──────────────────────────────────────
  if (curSpace && !lastSpace) {
    bleMouse.click(MOUSE_RIGHT);
    Serial.println("[BTN] Clic derecho");
  }
  lastSpace = curSpace;

  // ── Funciones extra cuando el MPU controla el movimiento ──
  if (USE_MPU) {
    bool curUp    = digitalRead(PIN_UP);
    bool curDown  = digitalRead(PIN_DOWN);
    bool curLeft  = digitalRead(PIN_LEFT);
    bool curRight = digitalRead(PIN_RIGHT);

    // Scroll continuo (se repite mientras el botón esté pulsado)
    if (curUp)   { bleMouse.move(0, 0,  1); delay(30); }
    if (curDown) { bleMouse.move(0, 0, -1); delay(30); }

    // Doble clic (detecta borde de subida)
    if (curLeft && !lastLeft) {
      bleMouse.click(MOUSE_LEFT);
      delay(50);
      bleMouse.click(MOUSE_LEFT);
      Serial.println("[BTN] Doble clic");
    }

    // Clic central (abrir enlace en nueva pestaña, etc.)
    if (curRight && !lastRight) {
      bleMouse.click(MOUSE_MIDDLE);
      Serial.println("[BTN] Clic central");
    }

    lastUp    = curUp;
    lastDown  = curDown;
    lastLeft  = curLeft;
    lastRight = curRight;
  }
}


// ═════════════════════════════════════════════════════════
//  HELPER: Parpadeo no bloqueante del LED
// ═════════════════════════════════════════════════════════
void blinkLed(unsigned long interval) {
  unsigned long now = millis();
  if (now - lastLedBlink >= interval) {
    lastLedBlink = now;
    ledState = !ledState;
    digitalWrite(PIN_LED, ledState);
  }
}
