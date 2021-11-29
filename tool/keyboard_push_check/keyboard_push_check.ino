/*
  Keyboard push check program.
  by MachiaWorks

  ■このファイルの利用目的
  キーマトリクススイッチの動作確認を行うこと。

  電気的に導通していればOKのはずであるが、
  実際にピンを挿してみたら動かない、なんてこともたまにあるので、
  （キースイッチを差し込んだときうまくハマらずピンが曲がる等）
  キー入力チェック用のプログラムを書いた。

  ■利用方法
  ・MsTimer2ライブラリを落としてくる（ツール→ライブラリを管理→「MsTimer2」で検索→Install）
  ・OLEDを確認してくる場合、u8g2ライブラリを落としてくる（ツール→ライブラリを管理→「MsTimer2」で検索→Install）
  ・ArduinoIDE上でソースコードの変更領域のみ変更（【書き換え部分】で検索すると引っかかるソース。Ctrl+Fで検索可能）
  ・ArduinoからFlashする（マイコンボードに書き込む）
  ・COMポートを選択し、シリアル通信の内容を確認する
  （ArduinoIDEなら、ポート選択→シリアルモニタで表示可能）
*/

#include <MsTimer2.h>

/*
  【書き換え部分】
  OLEDの実装を行わない場合、#define の部分をコメントアウトする
*/
#define OLED_CHECK

#ifdef OLED_CHECK
#include <U8g2lib.h>
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
#endif

/*
  【書き換え部分】
  rowNum/colNumの数値を変更する
*/
const int rowNum =  4;//キーボードの縦軸
const int colNum = 10;//キーボードの横軸

/*
  【書き換え部分】
  rowPin/colPinの利用ピンを変更する

  どのピンを使ってるかは、QMK firmware上の記載か
  基板の現物を確認して疎通を確認するのが早い。
*/
const int rowPin[rowNum] = { A1,A0,15,14 }; //縦並びの番号。Y方向
const int colPin[colNum] = {16,10,4,5, 6,7,8,9,A3,A2 };//横並びの番号。X方向

/*
 * 以下、チェック目的なら変更不要
 * 他にチェック項目があれば適時追加でいいと思う。
===================================================================
*/

// 押されたキーの情報を管理するための配列
bool currentState[rowNum][colNum];
bool beforeState[rowNum][colNum];

//row/colのカウント変数。
int row, col;
                           
void setup() {
  //デバッグ用のシリアル
  Serial.begin(31250);
  
  // 行単位の制御ピンの初期化
  for ( row = 0; row < rowNum; row++) {
    pinMode(rowPin[row], OUTPUT);
    delay(10);
    digitalWrite(rowPin[row], HIGH);
  }

  // 列情報読込みピンの初期化
  for ( col = 0; col < colNum; col++) {
    pinMode(colPin[col], INPUT_PULLUP);
  }
  
  // 読み取ったキーの情報を記録しておく配列を初期化する。
  for (row = 0; row < rowNum; row++) {
    for ( col = 0; col < colNum; col++) {
      currentState[row][col] = HIGH;
      beforeState[row][col] = HIGH;
    }
  }

  //タイマー起動(1秒に100/4=250回くらいのループ想定)
  MsTimer2::set(4, keyLoop); // 4ms period
  MsTimer2::start();
}

//キーボードの押下チェック。
//チャタリングはソフトウェア的に処理してないので、必要であれば適時追加
void keyLoop(){
  row = rowNum;

  // 音階キーの読取りと発音
  for (row = 0; row < rowNum; row++) {

    //一度ピンを全部ローにする
    digitalWrite( rowPin[row], LOW );
    for ( col = 0; col < colNum; col++) {
      currentState[row][col] = digitalRead(colPin[col]);
      if ( currentState[row][col] != beforeState[row][col] ) {
        
        Serial.print("key(");
        Serial.print(row);
        Serial.print(",");
        Serial.print(col);
        Serial.print(")");
        
        //high→Lowになった時のみ押したとして処理する
        if ( currentState[row][col] == LOW ) {
          Serial.println(" Push!");
        } else {
          //キーを離したときの処理
          Serial.println(" Release!");
        }
        beforeState[row][col] = currentState[row][col];
      }
    }
    //デジタルピンをHIGHにする
    digitalWrite( rowPin[row], HIGH );
  }
}

#ifdef OLED_CHECK
void dispLoop(){
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_8x13_tf);
  u8g2.drawStr(0, 10, "Hello world!");
  u8g2.drawStr(0, 21, "OLED screen TEST");
  u8g2.drawStr(0, 32, "with U8g2lib");
  u8g2.sendBuffer();
}
#endif

void loop() {
  // コントロールキーの読み取り
#ifdef OLED_CHECK
  dispLoop();
#endif
}
