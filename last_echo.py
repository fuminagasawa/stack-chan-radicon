
import threading

from app.fasterwhisper import transcribe_thread
from server.udpsender import gen_cmd_audio, send_udp_message,find_stackchan_ip
from server.voicevox_tts import synthesize_voice
#from server.stackchan_host import find_stackchan_ip, send_udp_message



stackchan_mac = "48-27-e2-7a-37-7c"
stackchan_port = 12345

stackchan_ip = find_stackchan_ip(stackchan_mac)


def transcribe_callback(segments, info):

    for segment in segments:
        print("[%.2fs -> %.2fs] %s" % (segment.start, segment.end, segment.text))
        # 音声合成を行う
        synthesize_voice(segment.text, speaker=1, filename="./server/public_http/output.wav", voicevox_host="localhost:50021")
        
        # UDPで音声データを送信する
        audio_command = gen_cmd_audio(stackchan_ip, "output.wav", port=8080)
        send_udp_message( audio_command, stackchan_ip, stackchan_port)
        

if __name__ == "__main__":


    # 音声認識スレッドを開始
    transcribe_thread = threading.Thread(target=transcribe_thread, args=(transcribe_callback,))
    transcribe_thread.start()


    transcribe_thread.join()

    print("終了")