

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



def gen_cmd_audio(target_host,file_path,port=12345):



    self_ip = get_shared_self_ip(target_host)


    self_addr = f"http://{self_ip}:{port}"

    print(f"Generated audio command with self address: {self_addr}")

    file_addr = f"{self_addr}/files/{file_path}"

    return f"CMD;PLAY_HTTP_AUDIO;{file_addr}"



if __name__ == "__main__":



    # Example usage
    #message = r"Hello, UDP!"


    stackchan_ip   = "192.168.137.152"
    stackchan_port = 12345
    host_port      = 8080

    messages = [
        r"CMD;ECHO;Hay stack!",
        gen_cmd_audio(stackchan_ip, "hello_stack.wav", port=host_port)
    ]

    message = ""
    for msg in messages:
        message += msg + "\n"




    send_udp_message(message, stackchan_ip, stackchan_port)


