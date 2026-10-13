#include <WiFiNINA.h>
#include <Servo.h>

// ===================== Forward declarations =====================
void catchCube();
void dropIntoRed();
void dropIntoBlack();
// ===================== CONFIG =====================
#define USE_HOTSPOT true   // true = create AP (hotspot), false = join WiFi

// ---- WiFi (STA) ----
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// ---- WiFi (AP) ----
// Must match the ssid/password configured on the XIAO ESP32-C3
const char* hotspotSSID     = "YOUR_HOTSPOT_SSID";
const char* hotspotPassword = "YOUR_HOTSPOT_PASSWORD"; // min 8 characters

WiFiServer server(80);

// ===================== Servo Setup =====================
/*
Order/index:
0: Waist
1: Shoulder
2: Elbow
3: WristRoll
4: WristPitch
5: Gripper
*/
Servo servos[6];
int   servoPins[6]      = {5, 6, 7, 8, 9, 10};
String servoNames[6]    = {"Waist","Shoulder","Elbow","WristRoll","WristPitch","Gripper"};
int   servoPosInit[6]   = {130, 82, 90, 0, 102, 180}; // power-on pose

struct ServoState { int current; int target; };
ServoState servoStates[6];

// Per-servo calibrated pulse ranges (us). Gripper narrower for micro servo.
int minPulseUS[6] = {544, 544, 544, 544, 544, 600};   // Gripper index=5
int maxPulseUS[6] = {2400,2400,2400,2400,2400,2300}; // Gripper index=5

// ---- Gripper calibration (SG92R or similar micro servo) ----
const int GRIPPER_INDEX   = 5;
const int GRIPPER_MIN_US  = 600;   // try 550..700 if needed
const int GRIPPER_MAX_US  = 2300; // try 2200..2400 if needed
const int GRIPPER_MIN_DEG = 0;     // clamp targets for gripper (degrees)
const int GRIPPER_MAX_DEG = 180;
// ===================== Timing =====================
unsigned long lastMoveTime = 0;
const int moveInterval = 15; // ms between incremental steps

// ===================== Non-blocking sequence engine =====================
enum SeqType { SEQ_NONE, SEQ_CATCH, SEQ_CATCH_XIAO, SEQ_RED, SEQ_BLACK, SEQ_BASE };
SeqType activeSeq = SEQ_NONE;
// ---- Simple FIFO queue so we can chain sequences from any state ----
const int QUEUE_CAP = 8;
SeqType seqQueue[QUEUE_CAP];
int qHead = 0, qTail = 0;

bool queueIsEmpty() { return qHead == qTail; }
bool queueIsFull()  { return ((qTail + 1) % QUEUE_CAP) == qHead; }

bool enqueueSeq(SeqType t) {
  if (queueIsFull()) return false;
  seqQueue[qTail] = t;
  qTail = (qTail + 1) % QUEUE_CAP;
  return true;
}
bool dequeueSeq(SeqType &t) {
  if (queueIsEmpty()) return false;
  t = seqQueue[qHead];
  qHead = (qHead + 1) % QUEUE_CAP;
  return true;
}

// ---- Step engine state ----
int seqStep = 0;
unsigned long seqStepStart = 0;
const unsigned long STEP_TIMEOUT_MS = 2500; // safety per row
const int REACHED_TOL = 0; // 0° tolerance (we move in 1° increments)

inline bool atTarget(int i) {
  return abs(servoStates[i].current - servoStates[i].target) <= REACHED_TOL;
}

// Apply only targets defined in this row (-1 = no change)
void applyStepTargets(const int stepTargets[6]) {
  for (int i = 0; i < 6; i++) {
    if (stepTargets[i] >= 0) {
      int val = stepTargets[i];
      if (i == GRIPPER_INDEX) val = constrain(val, GRIPPER_MIN_DEG, GRIPPER_MAX_DEG);
      servoStates[i].target = val;
    }
  }
}

// True if every servo that moved in this row reached its target
bool stepReached(const int stepTargets[6]) {
  for (int i = 0; i < 6; i++) {
    if (stepTargets[i] >= 0 && !atTarget(i)) return false;
  }
  return true;
}

/* Rows: {"Waist","Shoulder","Elbow","WristRoll","WristPitch","Gripper"} */

// ---- Catch sequence variant tuned for XIAO camera pipeline ----
const int CATCH_STEPS_XIAO[][6] = {
  { -1,  -1,  -1,   -1,   -1, 102 }, // open gripper
  {120,  -1,  -1,   -1,   -1,  -1 }, // waist 120
  { -1,  -1,  -1,    0,   -1,  -1 }, // wrist roll 0
  { -1,  -1,  -1,   -1,  102,  -1 }, // wrist pitch 102
  {122,  -1,  -1,   -1,   -1,  -1 }, // waist 122
  { -1,  -1, 155,   -1,   -1,  -1 }, // elbow 155
  { -1, 170,  -1,   -1,   -1,  -1 }, // shoulder 170
  { -1,  -1,  -1,   -1,   -1,   2 }  // close gripper
};
const int CATCH_STEPS_XIAO_LEN = sizeof(CATCH_STEPS_XIAO)/sizeof(CATCH_STEPS_XIAO[0]);

// ---- Catch sequence (manual test) ----
const int CATCH_STEPS[][6] = {
  { -1,  -1,  -1,   -1,   -1, 102 }, // open gripper
  { -1,  -1,  -1,    0,   -1,  -1 }, // wrist roll 0
  { -1,  -1,  -1,   -1,  102,  -1 }, // wrist pitch 102
  {124,  -1,  -1,   -1,   -1,  -1 }, // waist 124
  { -1,  -1, 150,   -1,   -1,  -1 }, // elbow 150
  { -1, 167,  -1,   -1,   -1,  -1 }, // shoulder 167
  { -1,  -1,  -1,   -1,   -1,  30 }  // close gripper
};
const int CATCH_STEPS_LEN = sizeof(CATCH_STEPS)/sizeof(CATCH_STEPS[0]);

// ---- Drop-only sequences ----
const int RED_STEPS[][6] = {
  { -1, 154,  -1,  -1,  -1,  -1 },   // shoulder 154
  { 75,  -1,  -1,  -1,  -1,  -1 },   // waist 75
  { -1,  -1, 168,  -1,  -1,  -1 },   // elbow 168
  { -1,  -1,  -1,  -1,  -1, 102 }    // open gripper
};
const int RED_STEPS_LEN = sizeof(RED_STEPS)/sizeof(RED_STEPS[0]);

const int BLACK_STEPS[][6] = {
  { -1, 145,  -1,  -1,  -1,  -1 },   // shoulder 145
  { 50,  -1,  -1,  -1,  -1,  -1 },   // waist 50
  { -1,  -1, 135,  -1,  -1,  -1 },   // elbow 135
  { -1,  -1,  -1,  -1,  -1, 102 }    // open gripper
};
const int BLACK_STEPS_LEN = sizeof(BLACK_STEPS)/sizeof(BLACK_STEPS[0]);

// ---- Base/home position ----
const int BASE_STEPS[][6] = { {124, 100, 120, 0, 102, 140} };
const int BASE_STEPS_LEN = sizeof(BASE_STEPS)/sizeof(BASE_STEPS[0]);

// ---- Sequence helpers ----
void beginSequence(SeqType t) {
  if (activeSeq != SEQ_NONE) {
    if (!enqueueSeq(t)) {
      Serial.println("Queue full, dropping request");
    } else {
      Serial.print("Queued seq: "); Serial.println((int)t);
    }
    return;
  }
  activeSeq = t;
  seqStep = 0;
  seqStepStart = millis();
  switch (t) {
    case SEQ_CATCH:      applyStepTargets(CATCH_STEPS[0]);      Serial.println("Sequence: catchCube (Manual)"); break;
    case SEQ_CATCH_XIAO: applyStepTargets(CATCH_STEPS_XIAO[0]); Serial.println("Sequence: catchCube (XIAO)");   break;
    case SEQ_RED:        applyStepTargets(RED_STEPS[0]);        Serial.println("Sequence: dropIntoRed");        break;
    case SEQ_BLACK:      applyStepTargets(BLACK_STEPS[0]);      Serial.println("Sequence: dropIntoBlack");      break;
    case SEQ_BASE:       applyStepTargets(BASE_STEPS[0]);       Serial.println("Sequence: BASE");               break;
    default: break;
  }
}
void startNextIfQueued() {
  SeqType nxt;
  if (dequeueSeq(nxt)) {
    activeSeq = SEQ_NONE;
    beginSequence(nxt);
  }
}

// Use existing sequences to build detection pipeline (catch -> drop(color) -> base)
void handleDetection(const String& color) {
  if (color == "Red") {
    beginSequence(SEQ_CATCH_XIAO);
    enqueueSeq(SEQ_RED);
    enqueueSeq(SEQ_BASE);
  } else if (color == "Black") {
    beginSequence(SEQ_CATCH_XIAO);
    enqueueSeq(SEQ_BLACK);
    enqueueSeq(SEQ_BASE);
  } else {
    Serial.println("handleDetection: unknown color");
  }
}

void updateSequence() {
  if (activeSeq == SEQ_NONE) return;
  const unsigned long now = millis();

  const int (*steps)[6] = nullptr;
  int len = 0;
  switch (activeSeq) {
    case SEQ_CATCH:      steps = CATCH_STEPS;      len = CATCH_STEPS_LEN;      break;
    case SEQ_CATCH_XIAO: steps = CATCH_STEPS_XIAO; len = CATCH_STEPS_XIAO_LEN; break;
    case SEQ_RED:        steps = RED_STEPS;        len = RED_STEPS_LEN;        break;
    case SEQ_BLACK:      steps = BLACK_STEPS;      len = BLACK_STEPS_LEN;      break;
    case SEQ_BASE:       steps = BASE_STEPS;       len = BASE_STEPS_LEN;       break;
    default: return;
  }

  if (stepReached(steps[seqStep]) || (now - seqStepStart >= STEP_TIMEOUT_MS)) {
    seqStep++;
    if (seqStep >= len) {
      Serial.println("Sequence done.");
      activeSeq = SEQ_NONE;
      startNextIfQueued();
      return;
    }
    applyStepTargets(steps[seqStep]);
    seqStepStart = now;
  }
}

// ===================== Prototypes =====================
void handleClients();
void handleSerial();
void updateServos();
// ===================== Setup =====================
void setup() {
  Serial.begin(9600);

  // Apply gripper pulse calibration
  minPulseUS[GRIPPER_INDEX] = GRIPPER_MIN_US;
  maxPulseUS[GRIPPER_INDEX] = GRIPPER_MAX_US;
  // Attach all servos with calibrated min/max, initialize pose/state
  for (int i = 0; i < 6; i++) {
    servos[i].attach(servoPins[i], minPulseUS[i], maxPulseUS[i]);
    servos[i].write(servoPosInit[i]);
    servoStates[i].current = servoPosInit[i];
    servoStates[i].target  = servoPosInit[i];
  }

  // WiFi
  if (!USE_HOTSPOT) {
    Serial.println("Connecting to WiFi (STA)...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    Serial.println();
    Serial.print("Connected! IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Starting hotspot (AP)...");
    if (WiFi.beginAP(hotspotSSID, hotspotPassword) != WL_AP_LISTENING) {
      Serial.println("Failed to start hotspot! (Continuing with Serial only)");
    } else {
      Serial.print("Hotspot started. IP: ");
      Serial.println(WiFi.localIP());
    }
  }
  server.begin();
  // Serial help
  Serial.println("\nSerial commands (9600 baud):");
  Serial.println("  c : catchCube   (just the catch sequence)");
  Serial.println("  r : dropIntoRed (just the drop-to-red)");
  Serial.println("  b : dropIntoBlack (just the drop-to-black)");
  Serial.println("  d : simulate Red detection (catch->red->base)");
  Serial.println("  k : simulate Black detection (catch->black->base)");
  Serial.println("  x : moveToBase");
  Serial.println("  h : help");
}

// ===================== Main Loop =====================
void loop() {
  handleClients(); // HTTP control and XIAO color callbacks
  handleSerial();    // Serial test control
  updateServos();    // Smooth incremental motion
  updateSequence();  // Progress active sequence
}

// ===================== Serial control =====================
void handleSerial() {
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (!line.length()) return;

  char cmd = tolower(line.charAt(0));
  switch (cmd) {
    case 'c': beginSequence(SEQ_CATCH);  break;
    case 'r': beginSequence(SEQ_RED);    break;
    case 'b': beginSequence(SEQ_BLACK);  break;
    case 'd': handleDetection("Red");    break;   // simulate detection
    case 'k': handleDetection("Black");  break; // simulate detection
    case 'x': beginSequence(SEQ_BASE);   break;
    case 'h':
    default:
      Serial.println("Commands:");
      Serial.println("  c : catchCube");
      Serial.println("  r : dropIntoRed");
      Serial.println("  b : dropIntoBlack");
      Serial.println("  d : detect Red  (catch->red->base)");
      Serial.println("  k : detect Black(catch->black->base)");
      Serial.println("  x : moveToBase");
      break;
  }
}

// ===================== HTTP =====================
void handleClients() {
  WiFiClient client = server.available();
  if (!client) return;
  // Read first request line (simple, fast)
  String request = "";
  unsigned long timeout = millis() + 80;
  while (client.connected() && millis() < timeout) {
    while (client.available()) {
      char c = client.read();
      request += c;
      if (c == '\n') break; // stop at end of request line
    }
    if (request.length() > 0) break;
  }
  if (request.length() == 0) { client.stop(); return; }

  // ---------- /servo?num=&angle= ----------
  if (request.indexOf("/servo?") >= 0) {
    int numIndex   = request.indexOf("num=");
    int angleIndex = request.indexOf("angle=");
    if (numIndex >= 0 && angleIndex >= 0) {
      int numEnd   = request.indexOf('&', numIndex);
      if (numEnd < 0) numEnd = request.indexOf(' ', numIndex);
      int servoNum = request.substring(numIndex + 4, numEnd).toInt();
      int angEnd = request.indexOf('&', angleIndex);
      if (angEnd < 0) angEnd = request.indexOf(' ', angleIndex);
      int angle  = request.substring(angleIndex + 6, angEnd).toInt();

      if (servoNum >= 1 && servoNum <= 6 && angle >= 0 && angle <= 180) {
        if (servoNum - 1 == GRIPPER_INDEX) {
          angle = constrain(angle, GRIPPER_MIN_DEG, GRIPPER_MAX_DEG);
        }
        servoStates[servoNum - 1].target = angle;
        Serial.print("Target for servo "); Serial.print(servoNum);
        Serial.print(": "); Serial.println(angle);
      } else {
        Serial.println("Bad /servo params");
      }
    }
  }

  // ---------- /color?value=Red|Black  (FROM XIAO) ----------
  if (request.indexOf("/color?value=") >= 0) {
    int valIndex = request.indexOf("value=");
    int end = request.indexOf(' ', valIndex);
    if (end < 0) end = request.length();
    String color = request.substring(valIndex + 6, end);
    color.trim();

    Serial.print("Color received: ");
    Serial.println(color);

    if (color == "Red" || color == "Black") {
      handleDetection(color);    // catch -> drop(color) -> base
    } else {
      Serial.println("Unknown color");
    }
  }

  // ---------- /action?cmd= ----------
  if (request.indexOf("/action?") >= 0) {
    int cmdIndex = request.indexOf("cmd=");
    if (cmdIndex >= 0) {
      int end = request.indexOf(' ', cmdIndex);
      if (end < 0) end = request.length();
      String cmd = request.substring(cmdIndex + 4, end);
      cmd.trim();
      if      (cmd == "catchCube")     beginSequence(SEQ_CATCH);
      else if (cmd == "dropIntoRed")   beginSequence(SEQ_RED);
      else if (cmd == "dropIntoBlack") beginSequence(SEQ_BLACK);
      else if (cmd == "base" || cmd == "moveToBase") beginSequence(SEQ_BASE);
      else Serial.println("Unknown action");
    }
  }

  // Default 200 OK JSON
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Connection: close");
  client.println();
  client.println("{\"status\":\"ok\"}");
  delay(1);
  client.stop();
}

// ===================== Smooth Servo Movement =====================
void updateServos() {
  unsigned long now = millis();
  if (now - lastMoveTime < moveInterval) return;
  lastMoveTime = now;

  static int slowCounter = 0;
  slowCounter++;
  for (int i = 0; i < 6; i++) {
    // Gripper: instant (we want snappy open/close)
    if (i == GRIPPER_INDEX) {
      servoStates[i].target = constrain(servoStates[i].target, GRIPPER_MIN_DEG, GRIPPER_MAX_DEG);
      if (servoStates[i].current != servoStates[i].target) {
        servoStates[i].current = servoStates[i].target;
        servos[i].write(servoStates[i].current);
      }
      continue;
    }

    // Slightly slow down WristPitch (index 4) for stability
    if (i == 4 && (slowCounter % 3 != 0)) continue;
    int step = 1; // 1° per tick for smoothness
    if (servoStates[i].current < servoStates[i].target) {
      servoStates[i].current += step;
      if (servoStates[i].current > servoStates[i].target)
        servoStates[i].current = servoStates[i].target;
      servos[i].write(servoStates[i].current);
    } else if (servoStates[i].current > servoStates[i].target) {
      servoStates[i].current -= step;
      if (servoStates[i].current < servoStates[i].target)
        servoStates[i].current = servoStates[i].target;
      servos[i].write(servoStates[i].current);
    }
  }
}

// ===================== Action starters (compat) =====================
void catchCube()     { beginSequence(SEQ_CATCH); }
void dropIntoRed()   { beginSequence(SEQ_RED);   }
void dropIntoBlack() { beginSequence(SEQ_BLACK); }
