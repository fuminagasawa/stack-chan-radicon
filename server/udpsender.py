

import socket


def send_udp_message(message, host, port):
    
    # Create a UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    try:
        # Send the message to the specified host and port
        sock.sendto(message.encode(), (host, port))
        print(f"Message sent to {host}:{port}")
    except Exception as e:
        print(f"Error sending message: {e}")
    finally:
        # Close the socket
        sock.close()


# 目標のマシンと同じネットワークに接続されている自分のIPアドレスを取得する関数
def get_shared_self_ip(target_ip=""):

    hostname = socket.gethostname()
    hostname, alias_list, ipaddr_list = socket.gethostbyname_ex(hostname)
    for ip in ipaddr_list:
        if target_ip and ip.startswith(target_ip.rsplit('.', 1)[0]):
            return ip
        elif not target_ip:
            return ip


def gen_cmd_servo(x,y,time=1000):

    return f"CMD;SET_SERVO;{x};{y};{time}"

# 目を動かすコマンドを生成する
def gen_cmd_eye_gaze(x,y):

    return f"CMD;EYE_GAZE;{x};{y}"

# 目の自動制御のON/OFFコマンドを生成する
def gen_cmd_eye_auto(enable):

    return f"CMD;EYE_AUTO;{1 if enable else 0}"

# 口の開き具合のコマンドを生成する
def gen_cmd_mouth_open(open_value):

    return f"CMD;MOUTH_OPEN;{open_value}"

# HTTPで再生する音声ファイルのコマンドを生成する
def gen_cmd_audio(target_host,file_path,port=12345):

    self_ip = get_shared_self_ip(target_host)


    self_addr = f"http://{self_ip}:{port}"

    print(f"Generated audio command with self address: {self_addr}")

    file_addr = f"{self_addr}/files/{file_path}"

    return f"CMD;PLAY_HTTP_AUDIO;{file_addr}"


def gen_cmd_stream(target_host,file_path, request_port=8080, continue_port=4423):

    self_ip = get_shared_self_ip(target_host)


    self_addr = f"http://{self_ip}:{request_port}"

    print(f"Generated audio command with self address: {self_addr}")

    file_addr = f"{self_addr}/stream_audio/{file_path}"

    return f"CMD;PLAY_STREAM_AUDIO;{file_addr};{self_ip};{continue_port}"



def find_stackchan_ip( target_mac="00:11:22:33:44:55"):

    # 目標のMACアドレスに対応するIPアドレスをARPテーブルから検索する関数
    import subprocess
    import re


    # ARPテーブルを取得
    arp_output = subprocess.check_output("arp -a", shell=True).decode("shift-jis")




    # 正規表現でMACアドレスとIPアドレスを抽出
    pattern = re.compile(r"(\d+\.\d+\.\d+\.\d+)\s+.*\s+" + re.escape(target_mac))
    match = pattern.search(arp_output)

    if match:
        return match.group(1)
    else:
        print(f"MACアドレス {target_mac} に対応するIPアドレスが見つかりませんでした。")
        return None
    



if __name__ == "__main__":



    # Example usage
    #message = r"Hello, UDP!"
    stackchan_mac = "48-27-e2-7a-37-7c"

    print("Finding Stackchan IP...")
    stackchan_ip = find_stackchan_ip(stackchan_mac)
    print(f"Stackchan IP: {stackchan_ip}")


    stackchan_ip   = stackchan_ip#"192.168.137.196"
    stackchan_port = 12345
    host_port      = 8080

    messages = [
        r"CMD;ECHO;Hay stack!",
        r"CMD;PRINT;Hay stack!;100",
        r"CMD;MOUTH_OPEN;0",
        r"CMD;EYE_AUTO;0",
        r"CMD;EYE_GAZE;0;0",
        gen_cmd_audio(stackchan_ip, "hello_stack.wav", port=host_port),
        #gen_cmd_servo(stackchan_ip, 0, 0, time=1000),
        #gen_cmd_servo(stackchan_ip, 30, 0, time=3000),
        #gen_cmd_servo(stackchan_ip, -30, 0, time=3000),
        #gen_cmd_servo(stackchan_ip, 0, 0, time=3000),
        #g#en_cmd_servo(stackchan_ip, 180, 0, time=3000),
        #gen_cmd_servo(stackchan_ip, -180, 0, time=3000),
        #gen_cmd_servo(stackchan_ip, 0, 0, time=3000),
        
        #gen_cmd_servo(stackchan_ip, 0, 0, time=1000),
        #gen_cmd_servo(stackchan_ip, -180, 0, time=1000),
        #gen_cmd_servo(stackchan_ip, 0, 5, time=1000)
        #gen_cmd_stream(stackchan_ip, "hello_stack.wav", request_port=host_port, continue_port=4423)
    ]

    message = ""
    for msg in messages:
        message += msg + "\n"




    send_udp_message(message, stackchan_ip, stackchan_port)


