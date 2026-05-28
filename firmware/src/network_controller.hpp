

// ネットワーク関係
#define WIFI_SSID_MAX_LENGTH (33)
#define WIFI_PASS_MAX_LENGTH (40)
char wifi_ssid[WIFI_SSID_MAX_LENGTH] = "NONE";   // SSIDは32文字+終端のNULL文字で33文字必要
char wifi_pass[WIFI_PASS_MAX_LENGTH] = "NONE"; // パスワードは128ビットAESの場合は16文字、192ビットAESの場合は24文字、256ビットAESの場合は32文字+終端のNULL文字で33文字必要
#define WIFI_CHECK_INTERVAL 5000 // WiFiの接続状態を確認する間隔（msec）
time_t last_wifi_check_time = 0; // 最後にWiFiの接続状態を確認した時間

// WiFiに接続されているかどうかを返す
bool wifi_connected() {
    return WiFi.status() == WL_CONNECTED;
}

// WiFiに接続する
bool connect_wifi(char* _ssid, char* _password) {

    WiFi.begin(_ssid, _password);
    M5.Log.printf("Connecting to WiFi: %s\n", _ssid);
 
    if (WiFi.waitForConnectResult() == WL_CONNECTED) {
        M5.Log.printf("\nConnected to WiFi. IP address: %s\n", WiFi.localIP().toString().c_str());
        return true;
    }
    return false;
}
// SDカードからWifiのSSIDとパスワードを読み込む
void load_wifi_config_from_sd(const char* filepath = "/config/wifi.txt") {
    File f = SD.open(filepath);
    if (f) {
        f.readBytesUntil('\n', wifi_ssid, WIFI_SSID_MAX_LENGTH);
        f.readBytesUntil('\n', wifi_pass, WIFI_PASS_MAX_LENGTH);
        f.close();
        M5.Log.printf("Loaded WiFi config from SD: SSID=%s, PASS=%s\n", wifi_ssid, wifi_pass);
    } else {
        M5.Log.printf("Failed to open WiFi config file on SD.\n");
    }
}


std::string get_my_ip_address() {
    if (wifi_connected()) {
        return WiFi.localIP().toString().c_str();
    } else {
        return "";
    }
}


// IPアドレスを表示する関数
void show_ip_address() {
    if (wifi_connected()) {
        M5.Log.printf("\nMy IP address: %s\n", get_my_ip_address().c_str());
    } else {
        M5.Log.printf("\nNot connected to WiFi.\n");
    }
}

void check_wifi_connection() {

    if (millis() - last_wifi_check_time > WIFI_CHECK_INTERVAL) {
        if (!wifi_connected()) {
            M5.Log.printf("\nWiFi connection lost. Reconnecting...\n");
            connect_wifi(wifi_ssid, wifi_pass);
        }
        last_wifi_check_time = millis();
    }
}



#define NETWORK_UDP_PORT 12345
#define NETWORK_UDP_BUFFER_SIZE 1024

char udp_buffer[NETWORK_UDP_BUFFER_SIZE]; // UDP受信用のバッファ

// UDP通信のためのWiFiUDPクラスのインスタンス
WiFiUDP udp;

// UDPポートを開く
void open_udp_port() {
    udp.begin(NETWORK_UDP_PORT);
    M5.Log.printf("UDP port %d opened for listening.\n", NETWORK_UDP_PORT);
}

// UDPでデータを受信する関数. 受信したデータはudp_bufferに格納されます。受信したデータの長さが返されます。
int read_udp(){

    if(wifi_connected()){
        int packetSize = udp.parsePacket();
        if (packetSize) {
            int len = udp.read(udp_buffer, NETWORK_UDP_BUFFER_SIZE-1); // バッファサイズ-1にして、最後にNULL文字を追加できるようにする
            if (len > 0) {
                udp_buffer[len] = 0; // 受信したデータの最後にNULL文字を追加して文字列として扱えるようにする
                M5.Log.printf("Received UDP packet: %s\n", udp_buffer);
                return len;
            }   
        }
    }
    return 0;
}



// HTTPサーバからファイルをダウンロードする
bool download_file(const char* url, const char* save_path) {

    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        File file = SD.open(save_path, FILE_WRITE);
        if (!file) {
            M5.Log.printf("Failed to open file for writing: %s\n", save_path);
            http.end();
            return false;
        }

        // こちらの実装ではファイルの受信が最後までできる
        WiFiClient *stream = http.getStreamPtr();
        uint8_t buffer[256];
        M5.Log.printf("Downloading file from URL: %s\n", url);
        M5.Log.printf("streamsize: %d bytes\n", stream->available());
        
         while (http.connected() && (stream->available() > 0 || stream->peek() != EOF)) {
          size_t size = stream->available();
          if (size > 0) {
            int c = stream->readBytes(buffer, ((size > sizeof(buffer)) ? sizeof(buffer) : size));
            file.write(buffer, c);
            //Serial.printf("%d bytes written\n", c);
          }
          delay(1);
        }
        file.close();
        
        return true;
    } else {
        M5.Log.printf("HTTP GET failed with code: %d\n", httpCode);
        http.end();
        return false;
    }
}

