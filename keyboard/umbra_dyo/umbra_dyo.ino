/*
  My home-brew MIDI Keyboard firmware「Aqua Arta.」
*/

#include <U8g2lib.h>
#include <MsTimer2.h>
#include <MIDIUSB.h>

U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

const int rowNum =  4;//キーボードの縦軸
const int colNum = 10;//キーボードの横軸
const byte key_in_buffer = 10;


// ピンの割当 for nomu30
const int rowPin[rowNum] = { A1,A0,15,14 };
//const int colPin[colNum] = { A3, A2, A1, A0, 15, 14,   16, 10, 8, 7, 6, 5 };
const int colPin[colNum] = {16,10,4,5, 6,7,8,9,A3,A2 };

const byte ctrlValue[12] = {0,16,32,48,64, 80,96,100,112,127};

const byte keyFunctionMap[rowNum][colNum] = {
{20,21,22,29,30,31,32,33,34,0},
{17,18,19,0,2,4,0,7, 9,11},
{14,15,16,1,3,5,6,8,10,12},
{0, 13, 0, 23,24,0,0,35,0,0}
};
/*
{0,2,4,0,7, 9,11,29,30,31},
{1,3,5,6,8,10,12,32,33,34},
{13,14,15,16,17,18,19,20,21,22},
{0, 23, 0, 24,25,26,27,28,0,0}
};

1-12：鍵盤
13-22：パラメータスイッチ

23：Octaveスイッチ
24: Velocityスイッチ
25: 
26:
27:
28: CC番号入力

29-34: CCストック（バッファ）切替


基本的に23-28を押してCCを切り替える

表示は16*3

"Oct:" "Vel:" "Rot:"
"CC :" "Val:" "Scl:"

*/




//最初のアドレスが違うのはOLEDを接続しようと動かしたため

// 押されたキーの情報を管理するための配列
bool currentState[rowNum][colNum];
bool beforeState[rowNum][colNum];
byte currentNote[rowNum][colNum];

byte keyCount[rowNum][colNum];

//CC番号の初期定義（書き換え可能）
byte ctrlCc[6] = {64,//sustain
                  7,//MainVolume
                  11,//Expression
                  10,//PanPot
                  1,//Modulation
                  2
};

//利用変数。
int row, col;
byte note = 0;
byte oct_shift = 5;
byte vel_shift=10;
int velocity=100;
byte vel_temp = 100;
int cnt=0;

int note_r;
int note_c;
bool on_flag = false;

//送信CC番号
byte send_cc_no= 64;

bool oct_flag = false;
bool vel_flag = false;

byte seq_count = 0;//const int seq_lim = 16;//シーケンスのリミット
                           
unsigned long prev=0,interval=0;

void setup() {
  // Serial.begin(115200);
  
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
      currentNote[rowNum][colNum] = 0;
      keyCount[row][col]=0;
    }
  }

  interval = 100;   // 実行周期を設定

  //u8g2.setFlipMode(0);

  //タイマー起動(1秒に100/4=250回くらいのループ想定)
  MsTimer2::set(2, myLoop); // 500ms period
  MsTimer2::start();

  u8g2.begin();
  u8g2.setFont(u8g2_font_8x13_tf);

  //画面出力
  dispLoop();
}

void dispLoop(){
  u8g2.clearBuffer();

/*
  可視範囲が
  x:0-120
  y:10-40（くらい）
*/


//参考
//https://qiita.com/CoTechWorks/items/3b6ada2d0b5951b2e33c
//http://zattouka.net/GarageHouse/micon/Arduino/XIAO/ExpansionBoard/DevelopedArduino1.htm
//https://iot.edu2web.com/2018/06/22/wemos-11-i2c-oled-sh1106-u8g2/

  //タイトル
  u8g2.drawStr(0, 9, "Midi Controller"); //15
  u8g2.drawStr(0, 21, " UMBRA - dyo");
  u8g2.drawStr(0, 32, "  BY MachiaWorks");
  u8g2.sendBuffer();
}

void keyDisp(int *x, int *y){
  char str[3];
  u8g2.clearBuffer();
  //u8g2.setFont(u8g2_font_8x13_tf);
  
  u8g2.drawStr(0, 10, "push_key");
  u8g2.drawStr(0, 21, "row:");

  dtostrf(*x, 4, 0, str);
  u8g2.drawStr(32,21,str);
  u8g2.drawStr(0, 32, "col:");

  dtostrf(*y, 4, 0, str);
  u8g2.drawStr(32,32,str);
  u8g2.sendBuffer();
}

void normalDisp(){
  char str[3];
  u8g2.clearBuffer();
  //u8g2.setFont(u8g2_font_8x13_tf);

/*
"Oct:" "Vel:" "Rot:"
"CC :" "Val:" "Scl:"
*/
  
  u8g2.drawStr(0, 10, "Oct:");
  dtostrf(oct_shift, 3, 0, str);
  u8g2.drawStr(32,10,str);
  
  u8g2.drawStr(0, 21, "Vel:");

  dtostrf(vel_temp, 3, 0, str);
  u8g2.drawStr(32,21,str);
  
  u8g2.drawStr(64, 10, "CC :");
  dtostrf(send_cc_no, 3, 0, str);
  u8g2.drawStr(96,10,str);
  
  u8g2.sendBuffer();
}

void myLoop(){
  // コントロールキーの読み取り
  row = rowNum;

  // 音階キーの読取りと発音
  for (row = 0; row < rowNum; row++) {

    //一度ピンを全部ローにする（差分を確認するのが目的じゃないかな）
    digitalWrite( rowPin[row], LOW );
    for ( col = 0; col < colNum; col++) {
      currentState[row][col] = digitalRead(colPin[col]);
      if ( currentState[row][col] != beforeState[row][col] ) {
        
        Serial.print("key(");
        Serial.print(row);
        Serial.print(",");
        Serial.print(col);
        Serial.print(")");
        
        //high→Lowになった時のみ処理
        if ( currentState[row][col] == LOW && keyCount[row][col]>key_in_buffer) {
          
          //今となっては不要かもの記述。
          //MIDI命令を送信する
          keyCount[row][col]=0;

          if( keyFunctionMap[row][col] != 0 ){
            //鍵盤演奏(func =1～12)
            if(keyFunctionMap[row][col] <=12){
              //鍵盤演奏
              NoteCalc();
              //note = keyFunctionMap[row][col]+oct_shift*12;
              note = (note>128)?127:note;
              
              velocity = vel_temp;
              velocity = (velocity>128)?127:
                        velocity;
                        
              //ノートのコントロールを行う。（再生）
              NoteControl(0,note, velocity);
            }else if( keyFunctionMap[row][col] <=22){

              //Velocity変更のみ
              if(vel_flag == true){
                vel_temp = ctrlValue[keyFunctionMap[row][col] - 13];
              }
              else if(oct_flag == true){
                //オクターブ変更
                OctaveAbs(keyFunctionMap[row][col]-13);
              }
              else{
                //CC送信する
                //send_cc_no = ctrlCc[0];
                byte dat = ctrlValue[keyFunctionMap[row][col] - 13];
                controlChange(0,send_cc_no,dat);
              }
            }
            else if(keyFunctionMap[row][col] == 23 ){
              oct_flag = true;
            }
            else if(keyFunctionMap[row][col] == 24 ){
              vel_flag = true;
            }
            else if( keyFunctionMap[row][col] <=28){
            
            }else if( keyFunctionMap[row][col] <=34){
              //送信するCC番号の変更
              send_cc_no = ctrlCc[keyFunctionMap[row][col]-29];
            }
          }
          //今となっては不要かもの記述。
          currentNote[row][col] = note;

          //MIDI命令を送信する
          MidiUSB.flush();
          Serial.println(" Push!");

          //keyDisp(&row, &col);
          //normalDisp();

          //  Keyboard.press( keyMap[row][col] );
        } else {
          //鍵盤を離したときの処理
          if( keyFunctionMap[row][col] != 0){// && keyCount[row][col]>key_in_buffer ){
            keyCount[row][col]=0;
            
            if(keyFunctionMap[row][col] <=12){
              //鍵盤については離すといずれのケースも音を止めるようにした。
              //if(shift_on==false)
              NoteCalc();
              //note = keyFunctionMap[row][col]+oct_shift*12;
              note = (note>128)?127:note;

              velocity = vel_temp;
              velocity = (velocity>128)?127:velocity;
              
              NoteControl(1,note, velocity);
              
            }else if(keyFunctionMap[row][col] == 23 ){
              oct_flag = false;
            }
            else if(keyFunctionMap[row][col] == 24 ){
              vel_flag = false;
            }
          }
          MidiUSB.flush();
          Serial.println(" Release!");
        }
        beforeState[row][col] = currentState[row][col];
      }
      keyCount[row][col]= (keyCount[row][col]>=254)?254:keyCount[row][col]+1;
    }
    digitalWrite( rowPin[row], HIGH );
  }
}

void loop() {
  if( millis()>2000){
    normalDisp();
  }
}

void NoteCalc()
{
  note = keyFunctionMap[row][col]+oct_shift*12-1;
}

void NoteControl( byte flag, byte note, byte velocity ){
  if( flag == 0){
      noteOn(0, note, velocity);   // Channel 0, note, normal velocity
  }
  else if( flag ==1 ){
      noteOff(0, note, velocity);   // Channel 0, note, normal velocity
  }
}

void OctaveShift( byte flag ){
  byte tmp_oct = oct_shift;
  tmp_oct = (flag ==1)?tmp_oct+1:
              (flag ==0)?tmp_oct-1:
              tmp_oct;

  oct_shift = (tmp_oct<1)?oct_shift:
              (tmp_oct>8)?oct_shift:
              tmp_oct;
}

void OctaveAbs( byte num ){
  byte tmp_oct = num;

  oct_shift = (tmp_oct<1)?oct_shift:
              (tmp_oct>8)?oct_shift:
              tmp_oct;
}

void VelocityChange( byte flag ){
  byte tmp_vel = vel_shift;
  tmp_vel = (flag ==1)?tmp_vel+1:
              (flag ==0)?tmp_vel-1:
              tmp_vel;

//境界値条件
  vel_shift = (tmp_vel<1 )?vel_shift:
              (tmp_vel>11)?vel_shift:
              tmp_vel;
}


void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}
// First parameter is the event type (0x09 = note on, 0x08 = note off).
// Second parameter is note-on/note-off, combined with the channel.
// Channel can be anything between 0-15. Typically reported to the user as 1-16.
// Third parameter is the note number (48 = middle C).
// Fourth parameter is the velocity (64 = normal, 127 = fastest).
void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

// First parameter is the event type (0x0B = control change).
// Second parameter is the event type, combined with the channel.
// Third parameter is the control number number (0-119).
// Fourth parameter is the control value (0-127).

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}
