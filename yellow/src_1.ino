// =====================================================インポート=====================================================
#include <Bluepad32.h>

// =====================================================ポート定義=====================================================
// 左サイド
const int left_back1  = 33; // A
const int left_back2  = 32; // A
const int left_front1 = 16; // B
const int left_front2 = 17; // B

// 右サイド
const int right_back1  = 27; // D
const int right_back2  = 13; // D
const int right_front1 = 18; // C
const int right_front2 = 19; // C

// モータードライバ
const int STBY = 15;

// =====================================================コントローラー=====================================================
ControllerPtr myController;
float lx, ly, rx, ry;
int   L1, L2, R1, R2, Cross, Circle, Square, Triangle;

// コントローラー接続時に呼ばれる
void onConnectedController(ControllerPtr ctl) {
  Serial.println(":チェックマーク_緑: Controller connected!");
  myController = ctl;
}
// コントローラー切断時に呼ばれる
void onDisconnectedController(ControllerPtr ctl) {
  Serial.println(":x: Controller disconnected!");
  if (ctl == myController) {
    myController = nullptr;
  }
}

// =====================================================コントロール=====================================================
int DEAD_ZONE = 200;

// =====================================================モーター定義=====================================================
// class Motor {
//   private:
//     int pin1, pin2;
//     bool rev;  // reverse フラグを保持

//   public:
//     // コンストラクタ
//     Motor(int p1, int p2, bool reverse = false)
//       : pin1(p1), pin2(p2), rev(reverse) {
//       pinMode(pin1, OUTPUT);
//       pinMode(pin2, OUTPUT);
//     }

//     // forward（正回転）
//     void forward() {
//       if (rev) {
//         digitalWrite(pin1, LOW);
//         digitalWrite(pin2, HIGH);
//       } else {
//         digitalWrite(pin1, HIGH);
//         digitalWrite(pin2, LOW);
//       }
//     }

//     // backward（逆回転）
//     void backward() {
//       if (rev) {
//         digitalWrite(pin1, HIGH);
//         digitalWrite(pin2, LOW);
//       } else {
//         digitalWrite(pin1, LOW);
//         digitalWrite(pin2, HIGH);
//       }
//     }

//     // 停止（ブレーキOFF相当）
//     void stop() {
//       digitalWrite(pin1, LOW);
//       digitalWrite(pin2, LOW);
//     }
// };

// // モーター定義（左右で正転方向が合うようにrevを調整）
// Motor leftFront  (left_front1,  left_front2,  true);
// Motor leftBack   (left_back1,   left_back2,   true);
// Motor rightFront (right_front1, right_front2, false);
// Motor rightBack  (right_back1,  right_back2,  true);

// =====================================================モーター定義=====================================================
class MotorSM {
  int in1, in2, ch1, ch2;  // in1/in2にそれぞれ別LEDCチャンネル
  bool rev;
  uint8_t duty = 0;        // 0..255
  float lastSpeed = 0;     // 前回のspeed記録（デバッグ用）

public:
  MotorSM(int _in1, int _in2, int _ch1, int _ch2, bool _rev=false)
    : in1(_in1), in2(_in2), ch1(_ch1), ch2(_ch2), rev(_rev) {}

  void begin(uint32_t freq=2000, uint8_t res=8) {
    pinMode(in1, OUTPUT);
    pinMode(in2, OUTPUT);
    ledcSetup(ch1, freq, res);
    ledcSetup(ch2, freq, res);
    ledcAttachPin(in1, ch1);
    ledcAttachPin(in2, ch2);
    ledcWrite(ch1, 0);
    ledcWrite(ch2, 0);
  }

  // dutyを直接設定する関数（0~255）
  void setDuty(uint8_t d) { duty = d; }

  // ★ 新関数：速度を -1.0 ~ +1.0 で設定
  void setSpeed(float speed) {
    // 範囲をクリップ
    if (speed > 1.0)  speed = 1.0;
    if (speed < -1.0) speed = -1.0;
    lastSpeed = speed;

    // duty変換（絶対値を0~255にマップ）
    duty = (uint8_t)(fabs(speed) * 255);

    if (speed > 0.05) {
      forward();
    } else if (speed < -0.05) {
      backward();
    } else {
      stopCoast();
    }
  }

  void forward() {
    if (!rev) {
      ledcWrite(ch1, duty); // IN1にPWM
      digitalWrite(in2, LOW);
    } else {
      ledcWrite(ch2, duty); // IN2にPWM
      digitalWrite(in1, LOW);
    }
  }

  void backward() {
    if (!rev) {
      ledcWrite(ch2, duty); // IN2にPWM
      digitalWrite(in1, LOW);
    } else {
      ledcWrite(ch1, duty); // IN1にPWM
      digitalWrite(in2, LOW);
    }
  }

  void stopCoast() {  // 惰性停止
    ledcWrite(ch1, 0);
    ledcWrite(ch2, 0);
  }

  void stopBrake() {  // ブレーキ停止
    ledcWrite(ch1, 255);
    ledcWrite(ch2, 255);
  }

  // ★ デバッグ用：現在の速度を返す
  float getSpeed() const { return lastSpeed; }
};

// モーター定義（左右で正転方向が合うようにrevを調整）
MotorSM leftFront (left_front1, left_front2,  0, 1, true);
MotorSM leftBack  (left_back1,  left_back2,   2, 3, true);
MotorSM rightFront(right_front1,right_front2, 4, 5, false);
MotorSM rightBack (right_back1, right_back2,  6, 7, false);

// =====================================================セットアップ ledc =====================================================
void setup() {
  Serial.begin(115200);
  BP32.setup(&onConnectedController, &onDisconnectedController);
  Serial.println(":反時計回り矢印: Bluepad32 ready. Waiting for DUALSHOCK 4...");
  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, HIGH);

  leftFront.begin();
  leftBack.begin();
  rightFront.begin();
  rightBack.begin();
}

// =====================================================セットアップ=====================================================
// void setup() {
//   Serial.begin(115200);
//   BP32.setup(&onConnectedController, &onDisconnectedController);
//   Serial.println(":反時計回り矢印: Bluepad32 ready. Waiting for DUALSHOCK 4...");
//   pinMode(STBY, OUTPUT);
//   digitalWrite(STBY, HIGH);
// }

// =====================================================メインループ=====================================================
void loop() {
  BP32.update();
  if (myController && myController->isConnected()) {
    // 入力読み取り
    lx = myController->axisX() / 512.0f;    // 左スティック 横
    ly = myController->axisY() / 512.0f;    // 左スティック 縦（多くのパッドで手前=+ / 前=-）
    rx = myController->axisRX()/ 512.0f;   // 右スティック 横（旋回）
    ry = myController->axisRY()/ 512.0f;

    L1 = myController->l1();
    L2 = myController->l2();
    R1 = myController->r1();
    R2 = myController->r2();

    Cross    = myController->a();
    Circle   = myController->b();
    Square   = myController->x();
    Triangle = myController->y();

    controller_operation();
    delay(10);  // 応答性確保。必要なら調整
  }
}

// =====================================================動作定義=====================================================

void controller_operation(void) {

  // --- まずボタン系のテスト（任意） ---
  if (Cross) {          // 単発テスト：LFだけ前
    leftFront.setSpeed(0.5);
    Serial.println(leftFront.getSpeed());
    Serial.println(lx);

    return;
  } else if (Circle) {  // LBだけ前
    leftBack.setSpeed(0.5);
    Serial.println(leftBack.getSpeed());
    return;
  } else if (Square) {  // RFだけ前
    rightFront.setSpeed(0.5);
    Serial.println(rightFront.getSpeed());
    return;
  } else if (Triangle) {// RBだけ前
    rightBack.setSpeed(0.5);
    Serial.println(rightBack.getSpeed());
    return;
  } else {
    leftFront.setSpeed(0);
    leftBack.setSpeed(0);
    rightFront.setSpeed(0);
    rightBack.setSpeed(0);
  }
}