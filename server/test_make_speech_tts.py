

import requests
import json
import sys

from udpsender import gen_cmd_audio, send_udp_message


# 音声合成を行う関数
def synthesize_voice(text, speaker=1, filename="output.wav"):
    # 1. テキストから音声合成のためのクエリを作成
    query_payload = {'text': text, 'speaker': speaker}
    query_response = requests.post(f'http://localhost:50021/audio_query', params=query_payload)

    if query_response.status_code != 200:
        print(f"Error in audio_query: {query_response.text}")
        return

    query = query_response.json()
    print(f"Audio query created: {json.dumps(query, ensure_ascii=False)}")

    # 2. クエリを元に音声データを生成
    synthesis_payload = {'speaker': speaker}
    synthesis_response = requests.post(f'http://localhost:50021/synthesis', params=synthesis_payload, json=query)

    if synthesis_response.status_code == 200:
        # 音声ファイルとして保存
        with open(filename, 'wb') as f:
            f.write(synthesis_response.content)
        print(f"音声が {filename} に保存されました。")
    else:
        print(f"Error in synthesis: {synthesis_response.text}")

if __name__ == "__main__":
    
    
    stackchan_ip = "192.168.137.15"
    stackchan_port = 12345
    host_port = 8080
        
    # 読み上げたいテキスト
    text = "ほげほげふがふが"


    if 2 < len(sys.argv):
        text = sys.argv[1]

    output_wavfilename = f"voicevox_output.wav"

    # 音声合成の実行
    synthesize_voice(text, speaker=3, filename=f"./public_http/{output_wavfilename}")

    # UDPでスタックちゃんに再生コマンドを送信
    message = gen_cmd_audio(stackchan_ip, output_wavfilename, port=host_port)
    send_udp_message(message, stackchan_ip, stackchan_port)





    



