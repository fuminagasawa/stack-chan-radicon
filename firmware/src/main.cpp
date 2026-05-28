/*
 * Stack-chan-tester
 * 
 * @author TakaoAkaki
 * Copyright (c) 2021-2024 Takao Akaki. All right reserved
 */

//#define ARDUINO_M5STACK_CORES3 (1)

// ------------------------
// ヘッダファイルのinclude
// 
#include <Arduino.h>                         // Arduinoフレームワークを使用する場合は必ず必要
#include <SD.h>                              // SDカードを使うためのライブラリです。
#include <Update.h>                          // 定義しないとエラーが出るため追加。
#include <Ticker.h>                          // 定義しないとエラーが出るため追加。
#include <M5StackUpdater.h>                  // M5Stack SDUpdaterライブラリ
#include <M5Unified.h>                       // M5Unifiedライブラリ
#include <Stackchan_system_config.h>         // stack-chanの初期設定ファイルを扱うライブラリ
#include <Stackchan_servo.h>                 // stack-chanのサーボを動かすためのライブラリ
#include <Wifi.h>                            // WiFiを使うためのライブラリ
#include <HTTPClient.h>                      // HTTPリクエストを送るためのライブラリ 
#include <ArduinoJson.h>                    // JSONを扱うためのライブラリ
#include <vector>                             // C++の標準ライブラリのvectorクラスを使うためのinclude
#include <queue>                              // C++の標準ライブラリのqueueクラスを使うためのinclude

#ifdef ARDUINO_M5STACK_CORES3
//#include <gob_unifiedButton.hpp>
//goblib::UnifiedButton unifiedButton;         // M5CoreS3のタッチパネルをボタンA,B,Cとして使うためのライブラリ。
#endif
#include <Avatar.h>                          // 顔を表示するためのライブラリ https://github.com/meganetaaan/m5stack-avatar
#include "formatString.hpp"                  // 文字列に変数の値を組み込むために使うライブラリ https://gist.github.com/GOB52/e158b689273569357b04736b78f050d6


#include "AudioFileSourceSD.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"

#include "network_controller.hpp" // ネットワーク関係の関数をまとめたファイル
#include "udp_commands.hpp"       // UDPコマンド関係の関数をまとめたファイル

// ヘッダファイルのinclude end 
// ================================== End

// ---------------------------------------------
// グローバル変数の定義エリア
// プログラム全体で利用する変数やクラスを決める
constexpr int servo_offset_x = 170;  // X軸サーボのオフセット（サーボの初期位置からの+-で設定）
constexpr int servo_offset_y =   5;  // Y軸サーボのオフセット（サーボの初期位置からの+-で設定）

constexpr int servo_start_x =  0;  // X軸サーボの初期位置
constexpr int servo_start_y =  0;  // Y軸サーボの初期位置



using namespace m5avatar;     // (Avatar.h)avatarのnamespaceを使う宣言（こうするとm5avatar::???と書かなくて済む。)
Avatar  avatar;                // (Avatar.h)avatarのクラスを定義
ColorPalette *cps[2];

//#define SDU_APP_PATH "/stackchan_tester.bin"  // (SDUpdater.h)SDUpdaterで使用する変数
//#define TFCARD_CS_PIN 4                       // SDカードスロットのCSPIN番号

StackchanSERVO servo;                         // (Stackchan_servo.h) サーボを扱うためのクラス
//StackchanSystemConfig system_config;          // (Stackchan_system_config.h) プログラム内で使用するパラメータをYAMLから読み込むクラスを定義

uint32_t mouth_wait = 2000;       // 通常時のセリフ入れ替え時間（msec）
uint32_t last_mouth_millis = 0;   // セリフを入れ替えた時間
bool core_port_a = false;         // Core1のPortAを使っているかどうか


// フキダシ関係
int32_t fukidashi_show_time_rest = 0; // フキダシを表示している残り時間（msec）


// ================================== End

// ---------------------------------------------
// SDカード 読み書き関係
void get_and_show_sd_data(){


    // ファイルオープン
    File f = SD.open("/hello.txt");

    if (f) {
        unsigned int auiSize = 0;
        unsigned int auiCnt = 0;

        M5.Log.printf("File open successful\n");

        // ファイルサイズ取得
        auiSize = f.size();
        // サイズ分ループ
        for( auiCnt = 0; auiCnt < auiSize; auiCnt++ ){

            // ファイルの中身を表示
            f.seek(auiCnt);
            M5.Log.printf("%c",f.read());
            //M5.Log.printf("File open error hello.txt\n");
        }
        M5.Log.printf("\n");
        // ファイルクローズ   
        f.close();
    } else {
        M5.Log.printf("File open error hello.txt\n");
    }

    File outfile = SD.open("/output.txt", FILE_WRITE);  //add "/"
    if (outfile) {
        outfile.println("Hello World");
        outfile.close();
        M5.Log.printf("OutFile open successful\n");
    } else {
        M5.Log.printf("OutFile open error hello.txt\n");
    }


    return;

}
// ================================== End


// ---------------------------------------------
// 音声再生系
#define BCLK_PIN 12
#define LRCK_PIN 0
#define SADTA_PIN 2
#define EXTERNAL_I2S 0
#define OUTPUT_GAIN 80


AudioGeneratorWAV *wav = nullptr;
void play_audio(const char* filepath) {


    AudioFileSourceSD *mfile;
    AudioOutputI2S *out;

    //mfile = new AudioFileSourceSD(filepath); // SDカードのファイルを指定

    M5.Log.printf("playAudio: %s\n", filepath);
    //M5.SD.play(filename, 1.0f, 0.0f, false);
    //SD.play(filename, 1.0f, 0.0f, true);
    

    /*
    out = new AudioOutputI2S(0, 1); // Output to builtInDAC
    out->SetOutputModeMono(true);
    out->SetGain(0.3); 
    wav = new AudioGeneratorWAV();
    wav->begin(mfile, out);*/


    mfile = new AudioFileSourceSD(filepath);
    out = new AudioOutputI2S(I2S_NUM_0, EXTERNAL_I2S); 
    out->SetPinout(BCLK_PIN, LRCK_PIN, SADTA_PIN);
    out->SetOutputModeMono(true);
    out->SetGain((float)OUTPUT_GAIN/100.0);
    wav = new AudioGeneratorWAV();
    wav->begin(mfile, out);

    delay(3000);

    while(wav->isRunning()) {
        if (!wav->loop()) {
            wav->stop();
        }
    }
    
    M5.Log.printf("playAudio end\n");
}


void play_wav(std::string filename) {


    // SDカードのファイルを指定
    //const char *filename = "/audio/hello_stack.wav";

    // wavファイルを開く
    File file = SD.open(filename.c_str());
    if (!file) {
        M5.Log.printf("Failed to open file: %s\n", filename.c_str());
        return;
    }
    // bufにファイルを
    size_t bufSize = file.size();
    uint8_t *buf = new uint8_t[bufSize];
    if (!buf) {
        M5.Log.printf("Failed to allocate buffer\n");
        file.close();
        return;
    }
    size_t bytesRead = file.read(buf, bufSize);
    if (bytesRead != bufSize) {
        M5.Log.printf("Failed to read file: %s\n", filename.c_str());
        delete[] buf;
        file.close();
        return;
    }

    file.close();
    M5.Log.printf("Read %zu bytes from file: %s\n", bytesRead, filename.c_str());

    auto config = M5.Speaker.config();
    config.sample_rate = 44100;
    M5.Speaker.config(config);
    M5.Speaker.setVolume(255);
    M5.Speaker.begin();


    while (M5.Speaker.isPlaying()) { vTaskDelay(1); }
    M5.Log.printf("play wav\n");

    //M5.Speaker.playRaw(buf, bufSize, 44100, false);    
    M5.Speaker.setVolume(100);
    M5.Speaker.playWav(buf, bufSize, 1);

    // 再生が終わるまで待機
    while (M5.Speaker.isPlaying()) { vTaskDelay(1); }
    M5.Log.printf("play wav end\n");

    // bufを解放
    delete[] buf;
    M5.Log.printf("wav buffer released\n");
}

#define AUDIO_STREAM_RECV_PORT 4423
#define AUDIO_STREAM_HOST_PORT 4423

void play_stream(std::string stream_url,std::string streaming_host, int recv_port = AUDIO_STREAM_RECV_PORT, int timeout_ms=500, unsigned int buffering_time_ms = 10) {


    M5.Log.printf("play_stream is not implemented yet.\n");
    return;

    // 受信用UDPポートを開く
    WiFiUDP stream_recv_udp;
    stream_recv_udp.begin(recv_port);

    M5.Log.printf("UDP port %d opened for receiving audio stream.\n", recv_port);


    //stream_recv_udp.stop();
    //return; 

    // ストリームサーバへのリクエストを送る
    HTTPClient http;
    http.begin(stream_url.c_str());
    //int httpCode = http.POST("{"+formatString("\"recv_port\": %d, \"buffersize\": %d", recv_port, buffering_time_ms)+"}");
    int httpCode = http.POST("{\"recv_port\": 4423}");
    if (httpCode != HTTP_CODE_OK) {
        M5.Log.printf("HTTP POST failed with code: %d\n", httpCode);
        http.end();
        return;
    }



    // 最後にデータを受信した時間
    int last_receive_time_ms = millis();
    bool stream_ended = false; // ストリームが終了したかどうかのフラグ

    // スピーカーをセットアップ
    auto config = M5.Speaker.config();
    config.sample_rate = 44100;
    M5.Speaker.config(config);
    M5.Speaker.setVolume(255);
    M5.Speaker.begin();

    // ダブルバッファリングのためのバッファ
    //const int buffer_size = 512; // バッファサイズは適宜調整してください
    //uint8_t buf1[buffer_size];
    //uint8_t buf2[buffer_size];
    //char playing_buf1 = false; // どちらのバッファを再生しているかのフラグ
    //bool bufferd_buf1 = false; // buf1にデータが入っているかどうかのフラグ
    //bool bufferd_buf2 = false; // buf2にデータが入っているかどうかのフラグ

    M5.Log.printf("Start receiving audio stream.\n");

    // ストリームバッファリング
    const int buffer_size = 1024; // バッファサイズは適宜調整. 再生が終わるまでのデータを十分ためて置けるものが良い。とりあえず200KBにしている。
    uint8_t buf_streaming[buffer_size];
    
    
    // End-of-streamを検知するか、タイムアウトするまで、ストリームを受信して、受信したデータを再生する 
    while( !stream_ended){

        // タイムアウトの判定
        int time_since_last_receive = millis() - last_receive_time_ms;
        M5.Log.printf("Time since last receive: %d ms\n", time_since_last_receive);
        if ( time_since_last_receive > timeout_ms) {
            M5.Log.printf("Stream receive timeout\n");
            break;
        }

        // ストリームからデータを受信して、バッファの後ろに書き込む 
        int packetSize = stream_recv_udp.parsePacket();
        if (packetSize) {

            last_receive_time_ms = millis(); // データを受信した時間を更新する

            //M5.Log.printf("Received stream packet: %d bytes\n", packetSize);

            int len = stream_recv_udp.read(buf_streaming, buffer_size); // バッファの空きサイズに合わせてデータを受信する
            if (len > 0) {

                // [EOS]だったらループを抜ける
                if (len == 3 && buf_streaming[0] == 'E' && buf_streaming[1] == 'O' && buf_streaming[2] == 'S') {
                    M5.Log.printf("End of stream detected\n");
                    stream_ended = true;
                    break;
                }


                M5.Log.printf("Received stream packet: %d bytes\n", len);
                //buf_writing_pos += len; // 書き込んだ分だけ書き込み位置を進める
                // 受信データを再生する
                M5.Speaker.playRaw(buf_streaming, buffer_size, config.sample_rate, false); // バッファの先頭から書き込み位置までのデータを再生する
                
                // 次のセグメントをリクエストする
                //stream_recv_udp.sendTo( streaming_host, AUDIO_STREAM_HOST_PORT, "NEXT"); // ここでサーバのIPアドレスとポート番号を指定して、次のセグメントのリクエストを送る。サーバはこのリクエストを受け取ったら、次のセグメントを送る。

                // 現在の再生が終わるまで待機する
                while (M5.Speaker.isPlaying()) { vTaskDelay(1); }

                    

                // バッファの先頭から書き込み位置までのデータを再生した後、バッファの内容を先頭から書き込み位置までシフトする
                //int remaining_data_size = buf_writing_pos - buf_streaming; // バッファの先頭から書き込み位置までのデータサイズ
                //memmove(buf_streaming, buf_writing_pos, remaining_data_size); // バッファの内容を先頭から書き込み位置までシフトする
                //buf_writing_pos = buf_streaming + remaining_data_size; // 書き込み位置をシフトした分だけ前に移動する
            }
        }
    }
    stream_recv_udp.stop(); // 受信用UDPポートを閉じる
}



void say_hello() {

    M5.Log.printf("/audio/hello_stack.wav\n");
    play_wav("/audio/hello_stack.wav");
    //play_audio("/audio/hello_stack.wav");

}
// ================================== End

// ------------------------------------------------------------------
// サーボモーター制御系

void set_servo_position(int angle_x, int angle_y, int time_to_move, bool wait_for_move = false){

    constexpr int max_x =  180; // X軸の最大角度
    constexpr int min_x = -180; // X軸の最大角度
    constexpr int max_y =   25;  // Y軸の最大角度
    constexpr int min_y =   -5;  // Y軸の最小角度
 
    M5.Log.printf("Servo moveTo (src) x: %3d, y: %3d\n", angle_x, angle_y);

    int angleTo_x = constrain(angle_x, min_x, max_x); // 目標のX軸の角度にオフセットを加えて、最大最小の範囲に収める
    int angleTo_y = constrain(angle_y, min_y, max_y); // 目標のY軸の角度にオフセットを加えて、最大最小の範囲に収める

    int duration_to_move = max(time_to_move, 0) ; // サーボが動く時間（msec）

    M5.Log.printf("Servo moveTo (limited) x: %3d, y: %3d\n", angleTo_x, angleTo_y);

    servo.moveXY( angleTo_x, angleTo_y, duration_to_move); // サーボを動かす。

    if (wait_for_move) {
        delay(duration_to_move); // サーボが動くまで待つ。
    }
}


void set_servo_position_area_9(int area_9){

  
    int angleTo_y = 0;


    if(       7 <= area_9 ){
      angleTo_y = -5;  // 上のエリアは上を向く
    }else if( area_9 <= 3 ){
      angleTo_y = 25; // 上のエリアは上を向く
    }else{
      angleTo_y = 5; // 上のエリアは上を向く
    }

    int angleTo_x = 0;



    int x_of_area = area_9 % 3;
    if ( x_of_area == 0){
      angleTo_x = 35 ; // 右のエリアは右を向く
    }else if(x_of_area == 1){
      angleTo_x = -40 ; // 左のエリアは左を向く
    }else{
      angleTo_x = -5;
    }


    avatar.setMouthOpenRatio(0.5);                               // アバターの口を70%開きます。(70%=0.7)

    //M5.Log.printf("ServoType: %d\n", servo.getServoType());      // サーボのタイプをログに出力
    M5.Log.printf("servo moveTo x: %3d, y: %3d\n", angleTo_x, angleTo_y);
    //servo.moveXY( angleTo_x, angleTo_y, 1500); // サーボを動かす。
    set_servo_position(angleTo_x, angleTo_y, 1500, true);
    
    delay(500); // サーボ動作完了後、顔をキープする時間を少し置く

    avatar.setMouthOpenRatio(0.0); // アバターの口を閉じる。


}

// ================================== End

// 吹き出しでメッセージを表示
void show_fukidashi_message(std::string message, int duration_ms) {
 

    avatar.setSpeechText(message.c_str()); // アバターにメッセージを表示させる。duration_msで表示時間を指定する。duration_msが過ぎると自動的に消える。

    fukidashi_show_time_rest = duration_ms; // フキダシを表示している残り時間を更新する

    //avatar.setMessage(message.c_str(), duration_ms);
}


void show_message(std::string message, int duration_ms) {

    M5.Log.printf("Show fukidashi message: %s duration_ms=%d\n", message.c_str(), duration_ms);
    show_fukidashi_message(message, duration_ms);
    
}





// コマンド実行

void execute_udp_command(UDPCommand cmd) {
 
    M5.Log.printf("Executing UDP command:\n");
    cmd.print();

    if(cmd.command_name == "ECHO"){
        if (cmd.args.size() >= 1){
            std::string msg = cmd.args[0];
            M5.Log.printf("ECHO: %s\n", msg.c_str());
        }
    }else if(cmd.command_name == "PRINT"){
        if (cmd.args.size() >= 2){
            std::string msg = cmd.args[0];
            int duration_ms = std::stoi(cmd.args[1]);
            M5.Log.printf("PRINT: %s, duration_ms=%d\n", msg.c_str(), duration_ms);
            show_message(msg, duration_ms);
        }
    
    }else if(cmd.command_name == "PLAY_HTTP_AUDIO"){
        if (cmd.args.size() >= 1){
        
            std::string url = cmd.args[0];
            M5.Log.printf("PLAY_HTTP_AUDIO: %s\n", url.c_str());
            const char* save_path = "/temp_audio.wav";
            if (download_file(url.c_str(), save_path)){

                // ダウンロードしたファイルを再生する
                play_wav(save_path);
            }else{
                M5.Log.printf("Failed to download audio file from URL: %s\n", url.c_str());
            }
        }

    }else if(cmd.command_name == "PLAY_HTTP_AUDIO_RAW"){
        if (cmd.args.size() >= 1){
        
            std::string url = cmd.args[0];
            M5.Log.printf("PLAY_HTTP_AUDIO_RAW: %s\n", url.c_str());
            const char* save_path = "/temp_audio.wav";
            if (download_file(url.c_str(), save_path)){

                // ダウンロードしたファイルを再生する
                play_wav(save_path);
            }else{
                M5.Log.printf("Failed to download audio file from URL: %s\n", url.c_str());
            }
        }

    }else if(cmd.command_name == "PLAY_STREAM_AUDIO"){
        
        if (cmd.args.size() >= 2){
        
            std::string stream_url = cmd.args[0];
            std::string stream_udp_host = cmd.args[1];
            //int recv_port = std::stoi(cmd.args[1]);
            M5.Log.printf("PLAY_STREAM_AUDIO: url=%s\n", stream_url.c_str());

            play_stream(stream_url, stream_udp_host);
        }

    }else if(cmd.command_name == "SET_SERVO"){

        if (cmd.args.size() >= 3){
            int angle_x = std::stoi(cmd.args[0]);
            int angle_y = std::stoi(cmd.args[1]);
            int time_to_move = stoi(cmd.args[2]);
            M5.Log.printf("SET_SERVO: x=%d, y=%d, time_to=%d\n", angle_x, angle_y, time_to_move);

            avatar.setMouthOpenRatio(0.5);                               // アバターの口を70%開きます。(70%=0.7)
            servo.moveXY( angle_x, angle_y, time_to_move); // サーボを動かす。
            delay(time_to_move); // サーボが動くまで待つ。
            avatar.setMouthOpenRatio(0.0);        // アバターの口を閉じる)
        }

    }else if(cmd.command_name == "SAY_HELLO"){
        say_hello();
    }else if(cmd.command_name == "SHOW_IP"){
        show_ip_address();
        show_message(get_my_ip_address(), 3000);
    }
}




// ================================== End


// ================================== End
// スイッチ入力系

int x_split9_left   = 106; // 
int x_split9_right  = 214; // 
int y_split9_top    =  80; // 
int y_split9_bottom = 160; // 


int get_touchpad_area_9(int x, int y){

    int pressed_area_of_9 = 0; // 画面の9分割したときの押されたエリアを記録する変数

    if(      x < x_split9_left  ){  pressed_area_of_9 = 1;}
    else if( x < x_split9_right ){  pressed_area_of_9 = 2;}
    else{                             pressed_area_of_9 = 3;}

    if(      y < y_split9_top   ){  pressed_area_of_9 +=0;}
    else if( y < y_split9_bottom){  pressed_area_of_9 +=3;}
    else{                             pressed_area_of_9 +=6;}

    return pressed_area_of_9;
}



// ================================== End




// ------------------------------------------------------------------
// Arduinoフレームワークで一番最初に実行される関数の定義
// void setup()とvoid loop()は必ず必要です。
// void setup()は、最初に1回だけ実行します。
void setup() {

    auto cfg = M5.config();       // 設定用の情報を抽出
    cfg.serial_baudrate = 115200;
    M5.begin(cfg);                // M5Stackをcfgの設定で初期化

    M5.Lcd.setCursor(0, 30);
    M5.Lcd.print("Starting up...\n");

    M5.Log.setLogLevel(m5::log_target_display, ESP_LOG_NONE);    // M5Unifiedのログ初期化（画面には表示しない。)
    M5.Log.setLogLevel(m5::log_target_serial, ESP_LOG_INFO);     // M5Unifiedのログ初期化（シリアルモニターにESP_LOG_INFOのレベルのみ表示する)
    M5.Log.setEnableColor(m5::log_target_serial, false);         // M5Unifiedのログ初期化（ログをカラー化しない。）
    M5_LOGI("Hello World");                                      // logにHello Worldと表示
    
    
    // SDカードの初期化
    SD.begin(GPIO_NUM_4, SPI, 25000000); 
    // SDカードの初期化を少し待ちます。
    delay(2000);                         
    M5.Lcd.print("SD Card initialized.\n");

    // SDカードからWiFiのSSIDとパスワードを読み込みます。
    load_wifi_config_from_sd("/config/wifi.txt"); 

    // WiFiに接続します。
    connect_wifi(wifi_ssid, wifi_pass);                 
    M5.Lcd.print("Wifi connected.\n");

    // UDPポートを開きます。
    open_udp_port();



    // アールティverに合わせてサーボモータを初期化
    #if 1
    servo.begin( 6,  servo_start_x, servo_offset_x,
                 7,  servo_start_y, servo_offset_y,
                 ServoType::RT_DYN_XL330);
    M5.Lcd.print("Servo initialized.\n");
    #endif

    #if 1
    avatar.init();                   // avatarを初期化して実行開始します。(このときに顔が表示されます。)
    cps[0] = new ColorPalette();
    cps[0]->set(COLOR_PRIMARY, TFT_WHITE);
    cps[0]->set(COLOR_BACKGROUND, TFT_BLACK);
    avatar.setColorPalette(*cps[0]);
    #endif

    last_mouth_millis = millis();    // loop内で使用するのですが、処理を止めずにタイマーを実行するための変数です。一定時間で口を開くのとセリフを切り替えるのに利用します。
    //moveRandom();
    //testServo();
}


bool hertbeat_enabled = false; // ハートビートを有効にするかどうかのフラグ
uint32_t heartbeat_interval = 1000; // ハートビートの間隔（msec）
uint32_t last_hb_millis = 0;   // ハートビートを実行した時間

bool button_pressed_A = false;  // ボタンAが押されたかどうかのフラグ
bool button_pressed_B = false;  // ボタンBが押されたかどうかのフラグ
bool button_pressed_C = false;  // ボタンCが押されたかどうかのフラグ

int x_BtnA_Right = 106 ; // ボタンAの右端のX座標
int x_BtnB_Right = 214 ; // ボタンBの右端のX座標




int pressed_area_of_9 = 0; // 画面の9分割したときの押されたエリアを記録する変数



bool now_touched = false; // タッチされたかどうかのフラグ



// メインのloop処理。（必ず定義が必要。）
// 基本的にはずっと繰り返し実行されます。
// stack-chan-testerの場合は、ボタンを押して、各モードの関数が実行されると一時停止します。
void loop() {

    M5.update();  // M5Stackのボタン状態を更新します。

    //M5.Display.startWrite();
    if (M5.Touch.isEnabled()) {
      
      auto t = M5.Touch.getDetail(); // タッチパネルの状態を取得します。
      now_touched = t.isPressed(); // タッチされたかどうかのフラグ
      

      check_wifi_connection(); // WiFiの接続状態を確認して、切れていたら再接続します。

      
      // UDPで受信したコマンドを解析して、コマンドリストに追加します。
      read_udp_commands(); // UDPで受信したコマンドを解析して、コマンドリストに追加します。
      
      //コマンドリストにコマンドがあれば、先頭のコマンドを取り出して実行し、コマンドリストから削除します。
      while (!stored_udp_commands.empty()) {
          UDPCommand cmd = stored_udp_commands.front(); // コマンドリストから先頭のコマンドを削除する
          stored_udp_commands.pop();
          execute_udp_command(cmd); // 取り出したコマンドを実行する
      }


      // UDPで受信したデータがあれば、udp_bufferに格納して、内容をログに出力。
      //int udpsize = read_udp();
      //if (udpsize > 0){
      //    M5.Log.printf("UDP packet size: %d\n", udpsize);
      //}



      int touched_x = t.x; // タッチされたX座標   

      button_pressed_A = false;
      button_pressed_B = false;
      button_pressed_C = false;
      
      if(t.isPressed()){
        
          if( touched_x < x_BtnA_Right){
              button_pressed_A = true;
          }else if(touched_x < x_BtnB_Right){
              button_pressed_B = true;
          }else{
              button_pressed_C = true;
          }
          M5.Log.printf("button A:%1d B:%1d C:%1d\n", button_pressed_A, button_pressed_B, button_pressed_C);
      }

      
        // 画面を9分割して、どのエリアが押されたかを調べる。テンキーのように割り当てられる。
        pressed_area_of_9 = 0; // 9分割したエリアの初期化
        if (t.wasReleased()) {
            
        
            pressed_area_of_9 = get_touchpad_area_9(t.x, t.y); // タッチされたエリアを取得        
            M5.Log.printf("touch released  area:%d x: %4d, y: %4d\n", pressed_area_of_9, t.x, t.y);
            set_servo_position_area_9( pressed_area_of_9);

            get_and_show_sd_data();


            if (pressed_area_of_9 ==2){
                load_wifi_config_from_sd();
                open_udp_port();

                show_ip_address();
                show_message(get_my_ip_address(), 3000);
            }
            if (pressed_area_of_9 ==5){
                say_hello();
            }
        }


      // タッチされた場所を見つめる
      if(t.isPressed()){

          float gaze_target_h = (t.x - 160.0)/10.0f; 
          float gaze_target_v = (t.y - 120.0)/10.0f; 

          avatar.setLeftGaze(  gaze_target_v, gaze_target_h); // 目の動き（左目）をタッチパネルのX座標に合わせる。
          avatar.setRightGaze( gaze_target_v, gaze_target_h); // 目の動き（右目）をタッチパネルのX座標に合わせる。
      }else{
          avatar.setLeftGaze(  0, 0); // 目の動き（左目）をタッチパネルのX座標に合わせる。
          avatar.setRightGaze( 0, 0); // 目の動き（右目）をタッチパネルのX座標に合わせる。

      }


          avatar.setIsAutoBlink(!t.isPressed()); // タッチパネルが押されていないときは、まばたきをする。

      }


    float gaze_left_v, gaze_left_h;
    avatar.getLeftGaze(&gaze_left_v, &gaze_left_h); // 目の動きを取得(左目)
    float gaze_right_v, gaze_right_h;
    avatar.getLeftGaze(&gaze_right_v, &gaze_right_h); // 目の動きを取得(右目)



      

    //M5.Log.printf("Avatar gaze: L(v:%f, h:%f) R(v:%f, h:%f)\n", gaze_left_v, gaze_left_h,  gaze_right_v, gaze_right_h); // 目の動きをログに出力

    //M5.Display.endWrite();
    delay(1);
    

    // オーディオ
    /*
    if (wav != nullptr){
        
        if (wav->isRunning()) {
            
            // ループ再生しないなら自動停止
            if (!wav->loop()){
                wav->stop(); 
                M5.Log.printf("Audio end\n");
            }
        }
    }
    */
    // 吹き出し関連
    fukidashi_show_time_rest = max(0, fukidashi_show_time_rest - 1); // フキダシを表示している残り時間を減らす
    if (fukidashi_show_time_rest == 0) {
        avatar.setSpeechText(""); // フキダシを消す
    }
    

    if(hertbeat_enabled){
        if ((millis() - last_hb_millis) > heartbeat_interval) {       // 口を開けるタイミングを待ちます。時間を図ってmouth_waitで設定した時間が経つと繰り返します。
            M5_LOGI("Heartbeat.");
            last_hb_millis = millis();                                // 実行時間を更新。（この時点からまたmouth_wait分待ちます。）
        }  
    }


}
