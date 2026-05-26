#!/bin/bash

#$speech_content = "こんにちは。僕、ｽﾀｯｸﾁｬﾝ！"


echo "こんにちは。僕、ｽﾀｯｸﾁｬﾝ！" > text.txt
curl -s -X POST "localhost:50021/audio_query?speaker=1" --get --data-urlencode text@text.txt > query.json
curl -s -H "Content-Type: application/json" -X POST --data "@query.json" "localhost:50021/synthesis?speaker=1" > audio.wav

