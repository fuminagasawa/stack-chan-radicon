

import requests
import json


# 音声合成を行う関数
def synthesize_voice(text, speaker=1, filename="output.wav",voicevox_host="localhost:50021"):


    # 1. テキストから音声合成のためのクエリを作成
    query_payload = {'text': text, 'speaker': speaker}
    
    query_response = requests.post(f'http://{voicevox_host}/audio_query', params=query_payload)


    if query_response.status_code != 200:
        print(f"Error in audio_query: {query_response.text}")
        return

    query = query_response.json()
    print(f"Audio query created: {json.dumps(query, ensure_ascii=False)}")

    # 2. クエリを元に音声データを生成
    synthesis_payload = {'speaker': speaker}
    synthesis_response = requests.post(f'http://{voicevox_host}/synthesis', params=synthesis_payload, json=query)


    if synthesis_response.status_code == 200:
        # 音声ファイルとして保存
        with open(filename, 'wb') as f:
            f.write(synthesis_response.content)
        print(f"音声が {filename} に保存されました。")
    else:
        print(f"Error in synthesis: {synthesis_response.text}")
