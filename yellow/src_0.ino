// =====================================================インポート=====================================================
#include <Bluepad32.h>
#include <void.h>


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
const int right_front2 =  2; // C

// モータードライバ
const int STBY = 15;

// =====================================================コントローラー=====================================================
ControllerPtr myController;

int lx, ly, rx, ry, L1, L2, R1, R2, Cross, Circle, Square, Triangle;

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
class Motor {
  private:
    int pin1, pin2;
    bool rev;  // reverse フラグを保持

  public:
    // コンストラクタ
    Motor(int p1, int p2, bool reverse = false)
      : pin1(p1), pin2(p2), rev(reverse) {
      pinMode(pin1, OUTPUT);
      pinMode(pin2, OUTPUT);
    }

    // forward（正回転）
    void forward() {
      if (rev) {
        digitalWrite(pin1, LOW);
        digitalWrite(pin2, HIGH);
      } else {
        digitalWrite(pin1, HIGH);
        digitalWrite(pin2, LOW);
      }
    }

    // backward（逆回転）
    void backward() {
      if (rev) {
        digitalWrite(pin1, HIGH);
        digitalWrite(pin2, LOW);
      } else {
        digitalWrite(pin1, LOW);
        digitalWrite(pin2, HIGH);
      }
    }

    // 停止（ブレーキOFF相当）
    void stop() {
      digitalWrite(pin1, LOW);
      digitalWrite(pin2, LOW);
    }
};

// モーター定義（左右で正転方向が合うようにrevを調整）
Motor leftFront  (left_front1,  left_front2,  true);
Motor leftBack   (left_back1,   left_back2,   true);
Motor rightFront (right_front1, right_front2, false);
Motor rightBack  (right_back1,  right_back2,  true);

// =====================================================セットアップ=====================================================
void setup() {
  Serial.begin(115200);
  BP32.setup(&onConnectedController, &onDisconnectedController);
  Serial.println(":反時計回り矢印: Bluepad32 ready. Waiting for DUALSHOCK 4...");
  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, HIGH);
}

// =====================================================メインループ=====================================================
void loop() {
  BP32.update();
  if (myController && myController->isConnected()) {
    // 入力読み取り
    lx = myController->axisX();    // 左スティック 横
    ly = myController->axisY();    // 左スティック 縦（多くのパッドで手前=+ / 前=-）
    rx = myController->axisRX();   // 右スティック 横（旋回）
    ry = myController->axisRY();

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
void allStop() {
  leftFront.stop();
  leftBack.stop();
  rightFront.stop();
  rightBack.stop();
}

void controller_operation(void) {
  // デッドゾーン処理
  bool fwd   = (ly < -DEAD_ZONE);
  bool back  = (ly >  DEAD_ZONE);
  bool right = (lx >  DEAD_ZONE);
  bool left  = (lx < -DEAD_ZONE);
  bool rotR  = (rx >  DEAD_ZONE);
  bool rotL  = (rx < -DEAD_ZONE);

  // --- まずボタン系のテスト（任意） ---
  if (Cross) {          // 単発テスト：LFだけ前
    leftFront.forward();
    leftBack.stop();
    rightFront.stop();
    rightBack.stop();
    return;
  } else if (Circle) {  // LBだけ前
    leftFront.stop();
    leftBack.forward();
    rightFront.stop();
    rightBack.stop();
    return;
  } else if (Square) {  // RFだけ前
    leftFront.stop();
    leftBack.stop();
    rightFront.forward();
    rightBack.stop();
    return;
  } else if (Triangle) {// RBだけ前
    leftFront.stop();
    leftBack.stop();
    rightFront.stop();
    rightBack.forward();
    return;
  }

  // --- その場回転を優先 ---
  if (rotR) {
    // 右回転：左輪 前進 / 右輪 後退
    leftFront.forward();
    leftBack.forward();
    rightFront.backward();
    rightBack.backward();
    return;
  } else if (rotL) {
    // 左回転：左輪 後退 / 右輪 前進
    leftFront.backward();
    leftBack.backward();
    rightFront.forward();
    rightBack.forward();
    return;
  }

  // --- 平行移動（前後＋左右） ---
  // 斜めを使う場合は「対角2輪のみ駆動」でOK（メカナム特性上、低摩擦で素直に動く）
  if (fwd) {
    if (right) {
      // 斜め前右：LF & RB
      leftFront.forward();
      leftBack.stop();
      rightFront.stop();
      rightBack.forward();
    } else if (left) {
      // 斜め前左：RF & LB
      leftFront.stop();
      leftBack.forward();
      rightFront.forward();
      rightBack.stop();
    } else {
      // 前進：4輪前
      leftFront.forward();
      leftBack.forward();
      rightFront.forward();
      rightBack.forward();
    }
    return;
  }

  if (back) {
    if (right) {
      // 斜め後右：RF & LB 後退
      leftFront.stop();
      leftBack.backward();
      rightFront.backward();
      rightBack.stop();
    } else if (left) {
      // 斜め後左：LF & RB 後退
      leftFront.backward();
      leftBack.stop();
      rightFront.stop();
      rightBack.backward();
    } else {
      // 後退：4輪後
      leftFront.backward();
      leftBack.backward();
      rightFront.backward();
      rightBack.backward();
    }
    return;
  }

  // 水平のみ（ストレイフ）
  if (right) {
    // 右ストレイフ：LF前 / LB後 / RF後 / RB前
    leftFront.forward();
    leftBack.backward();
    rightFront.backward();
    rightBack.forward();
    return;
  } else if (left) {
    // 左ストレイフ：LF後 / LB前 / RF前 / RB後
    leftFront.backward();
    leftBack.forward();
    rightFront.forward();
    rightBack.backward();
    return;
  }

  // L/Rボタンの個別回し（必要なら）
  if (L1) { // 左前くるくる（左側だけ後退、右は停止）
    leftFront.backward();
    leftBack.backward();
    rightFront.stop();
    rightBack.stop();
    return;
  } else if (L2) { // 左後ろくるくる（左側だけ前進）
    leftFront.forward();
    leftBack.forward();
    rightFront.stop();
    rightBack.stop();
    return;
  } else if (R1) { // 右前くるくる（右側だけ後退）
    leftFront.stop();
    leftBack.stop();
    rightFront.backward();
    rightBack.backward();
    return;
  } else if (R2) { // 右後ろくるくる（右側だけ前進）
    leftFront.stop();
    leftBack.stop();
    rightFront.forward();
    rightBack.forward();
    return;
  }

  // 何もしないときは停止
  allStop();
}