import requests
import json
import sys

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
    # 読み上げたいテキスト
    text = "こんにちは、VOICEVOXでテキストを音声に変換しています。"

    if 2 < len(sys.argv):
        text = sys.argv[1]

    # 音声合成の実行
    synthesize_voice(text, speaker=3, filename="voicevox_output.wav")
