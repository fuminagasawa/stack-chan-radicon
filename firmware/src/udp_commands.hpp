



struct UDPCommand {
    std::string  command_name;
    std::vector<std::string> args;

    void print() {
        M5.Log.printf("Command: %s\n", command_name.c_str());
        for (size_t i = 0; i < args.size(); i++) {
            M5.Log.printf("  Arg%d: %s\n", (int)i, args[i].c_str());
        }
    }

};


#define UDP_COMMAND_SEPARATOR ';'
#define UDP_COMMAND_HEADER "CMD;"


bool is_valid_udp_command(std::string command_str) {
    return command_str.rfind( UDP_COMMAND_HEADER, 0) == 0; // "CMD:"で始まるかどうかを確認
}


// UDPコマンド文字列を解析してリストに格納する関数
// コマンドは以下のような書式
// CMD:COMMAND_NAME:ARG1:ARG2:...
// CMDから始まらない文字列は無視される
UDPCommand parse_udp_commandline(std::string command_str) {

    UDPCommand cmd;
    if (is_valid_udp_command(command_str)) { // "CMD:"で始まるかどうかを確認
        size_t pos = 4; // "CMD:"の次の位置から解析を開始
        size_t next_pos = command_str.find( UDP_COMMAND_SEPARATOR, pos);
        if (next_pos != std::string::npos) {
            cmd.command_name = command_str.substr(pos, next_pos - pos); // コマンド名を抽出
            pos = next_pos + 1; // 次の引数の位置に移動
            while ((next_pos = command_str.find( UDP_COMMAND_SEPARATOR, pos)) != std::string::npos) {
                cmd.args.push_back(command_str.substr(pos, next_pos - pos)); // 引数を抽出してリストに追加
                pos = next_pos + 1; // 次の引数の位置に移動
            }
            if (pos < command_str.length()) {
                cmd.args.push_back(command_str.substr(pos)); // 最後の引数を追加
            }
        } else {
            cmd.command_name = command_str.substr(pos); // コマンド名だけの場合
        }
    }
    return cmd;
}

// 複数行のUDPコマンドを解析してリストに格納する関数
std::vector<UDPCommand> parse_multiple_udp_commands(std::string commands_str) {

    std::vector<UDPCommand> commands;
    size_t pos = 0;
    size_t next_pos;
    while ((next_pos = commands_str.find('\n', pos)) != std::string::npos) {
        std::string command_line = commands_str.substr(pos, next_pos - pos); // コマンド行を抽出
        if (is_valid_udp_command(command_line)) { // "CMD:"で始まるかどうかを確認
            commands.push_back(parse_udp_commandline(command_line)); // コマンド行を解析してリストに追加
        }
        pos = next_pos + 1; // 次のコマンド行の位置に移動
    }
    if (pos < commands_str.length()) {
        std::string command_line = commands_str.substr(pos); // 最後のコマンド行を抽出
        if (is_valid_udp_command(command_line)) { // "CMD:"で始まるかどうかを確認
            commands.push_back(parse_udp_commandline(command_line)); // コマンド行を解析してリストに追加
        }
    }
    return commands;

}


// UDPコマンドリスト
std::queue<UDPCommand> stored_udp_commands; // 受信したUDPコマンドを格納するリスト

// UDPコマンドを収集する
// UDPで受信したコマンドを解析して、コマンドリストに追加する関数
void read_udp_commands() {

    //std::queue<UDPCommand> commands;
    int udpsize = read_udp();
    if (0 < udpsize){
        std::string udp_data(udp_buffer); // 受信したUDPデータを文字列に変換
        std::vector<UDPCommand>commands = parse_multiple_udp_commands(udp_data); // 複数のコマンドを解析してリストに格納


        // stored_udp_commandsにcommandsの内容を追加する
        for (size_t i = 0; i < commands.size(); i++) {
            stored_udp_commands.push(commands[i]); // コマンドをリストに追加
        }
    }

    return;
}


    

