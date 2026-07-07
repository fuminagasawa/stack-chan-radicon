from faster_whisper import WhisperModel
import sounddevice as sd
import queue
import cv2
import copy
import threading
import numpy as np

# ------------------------------------------------

model_size = "small"#"large-v3"
compute_type = "int8"#_float16"  # or "int8_float16" for INT8 quantization
whisper_device = "cpu"  # or "cpu" for CPU inference



# ----- 録音設定 -----
SAMPLE_RATE     = 16000        # サンプリングレート
CHANNELS        = 1            # モノラル
CHUNK_DURATION  = 0.1          # チャンク長（秒）
CHUNK_SIZE      = int(SAMPLE_RATE * CHUNK_DURATION)

AUDIO_ACTIVATE_AMPLITUDE_THRESH = 0.02    # 録音開始判定用振幅閾値

BEGIN_SEGMENT_TIME_LIMIT = 0.5    # (区間開始判定)無音とみなす連続時間（秒）
BEGIN_SEGMENT_THRESH     = 0.2    # (区間開始判定)無音とみなす振幅閾値

END_SEGMENT_TIME_LIMIT   = 2.0      # (区間終了判定)無音とみなす連続時間（秒）
END_SEGMENT_THRESH       = 0.1      # (区間終了判定)無音とみなす振幅閾値

AUDIO_GAIN = 1.0 # 録音音声のゲイン調整用

recording_device_id = 0

record_dir = "./rec"
send_dir = "./__wavtmp"

# 波形ビジュアライズのための画像設定
img_width  = 640
img_height = 480


# ------------------------------------------------
# キューと状態変数
audio_queue      = queue.Queue()
audio_queue_maxsize = 10
recording        = False
audio_active     = False
record_frames           = []
silence_buf_time = 0.0
active_buf_time  = 0.0

# ------------------------------------------------
# Run on GPU with FP16
#model = WhisperModel(model_size, device="cuda", compute_type="float16")

# or run on GPU with INT8
# model = WhisperModel(model_size, device="cuda", compute_type="int8_float16")
# or run on CPU with INT8
#model = WhisperModel(model_size, device="cpu", compute_type="int8")

print(f"Loading Whisper model '{model_size}' on device '{whisper_device}' with compute type '{compute_type}'...")
model = WhisperModel(model_size, device=whisper_device, compute_type=compute_type)
print("Whisper model loaded successfully.")


def transcribe_audio(audio_segment):
    print("Transcribing audio segment...")

    wav = prepare_whisper_audio(audio_segment)

    if wav.size == 0:
        print("No audio samples to transcribe.")
        return

    if len(wav) < SAMPLE_RATE * 0.5:
        print("Audio is too short. Speak longer or record a longer segment.")
        return

    segments, info = model.transcribe(
        wav,
        beam_size=3,
        language="ja",
        # デバッグ時はいったんtimestampありにする
        without_timestamps=True,
        # デバッグ時は「無音判定で捨てられている」可能性を潰す
        no_speech_threshold=True,
        log_prob_threshold=True,
        # まずはVADなしで素の挙動を見る
        vad_filter=False,
        condition_on_previous_text=False,
    )

    #print(f"Detected language {info.language} with probability {info.language_probability}")


    #segments = list(segments)
    #print(f"segments count: {len(segments)}")

    #for segment in segments:
    #    print(
    #        f"[{segment.start:.2f} -> {segment.end:.2f}] "
    #        f"no_speech={segment.no_speech_prob:.3f}, "
    #        f"avg_logprob={segment.avg_logprob:.3f}, "
    #        f"text={segment.text!r}"
    #    )

    return segments, info


def flush_audio_queue():
    global record_frames

    frames_to_process = copy.deepcopy(record_frames)
    wav = np.concatenate(frames_to_process, axis=0)

    samples_seconds = wav.shape[0] / SAMPLE_RATE

    record_frames.clear()

    print(f"Flushing audio queue. Total frames: {len(frames_to_process)}, Total samples: {wav.shape[0]}, Duration: {samples_seconds:.2f} seconds")

    return wav

def prepare_whisper_audio(wav: np.ndarray) -> np.ndarray:
    wav = np.asarray(wav)

    # sounddeviceの mono 入力は (samples, 1) になりがちなので 1次元化
    if wav.ndim == 2:
        if wav.shape[1] == 1:
            wav = wav[:, 0]
        else:
            wav = wav.mean(axis=1)

    wav = wav.astype(np.float32, copy=False)
    wav = np.nan_to_num(wav)

    if wav.size == 0:
        return wav

    peak = float(np.max(np.abs(wav)))
    rms = float(np.sqrt(np.mean(wav ** 2)))

    print(
        f"Audio debug: shape={wav.shape}, dtype={wav.dtype}, "
        f"min={wav.min():.4f}, max={wav.max():.4f}, "
        f"peak={peak:.4f}, rms={rms:.6f}, duration={len(wav) / SAMPLE_RATE:.2f}s"
    )

    # デバッグ段階では [-1, 1] に収める
    if peak > 1.0:
        print("Warning: audio peak exceeds 1.0; clipping to [-1, 1].")
        wav = np.clip(wav, -1.0, 1.0)

    return wav

# 画像をshiftピクセル左に移動する関数
# 画像の右端に空白を追加
def shift_x(img, dx, dy):


    # 画像サイズ
    height = img.shape[0]  # 高さ
    width  = img.shape[1]  # 幅

    # 平行移動の変換行列を作成
    affine_matrix = np.float32([[1,0,dx],[0,1,dy]])

    # アファイン変換適用
    affined_img = cv2.warpAffine(img, affine_matrix, (width,height))

    return affined_img


def visualize_amplitude(img=None, amplitude_min=0.0,amplitude_max=1.0, width=640, height=480, audio_active=True, is_recording=False, frames_in_buffer_count=0, rate=1.0):

    stepwise = 2

    if img is None:
        img = np.zeros((height, width, 3), dtype=np.uint8)  # 黒い画像を作成

    # openCVの画像を1ピクセル左に移動する
    img = shift_x(img, -1*stepwise, 0)

    # 画像レベルの0点を中央に設定
    center_y = height // 2

    wave_height_max = center_y - 10  # 波形の最大高さ 

    hakei_color = (160, 160, 160)
    robot_color = (100, 255, 100)
    audio_label = "AUDIO : OFF"
    robot_label = "ROBOT : AVAILABLE"
    
    if audio_active:
        hakei_color = (255, 255, 255)
        audio_label = "AUDIO : ACTIVE"

    if is_recording:
        hakei_color = (100, 255, 100)
        audio_label = "AUDIO : IN SEGMENT"


    # ラベル表示用エリアの下地を黒く塗りつぶし
    cv2.rectangle(img, (0,0), (400,70), (0,0,0), -1)

    # 画面左端に状態表示
    cv2.putText(img, f"{audio_label}:{frames_in_buffer_count:03}", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.5, hakei_color, 1)
    cv2.putText(img, robot_label, (10, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.5, robot_color, 1)

    cv2.putText(img, f"AUDIO_GAIN:{AUDIO_GAIN:.3f}", (220, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.5, hakei_color, 1)



    # 0点の線を引く
    cv2.line(img, (0, center_y), (width-1, center_y), (255, 255, 255), 1)

    # 録音開始レベルの線を引く
    y_startline_upper = center_y - int(BEGIN_SEGMENT_THRESH * wave_height_max * rate)
    y_startline_lower = center_y + int(BEGIN_SEGMENT_THRESH * wave_height_max * rate)

    cv2.line(img, (0, y_startline_upper), (width-1, y_startline_upper), (50, 50, 200), 1)
    cv2.line(img, (0, y_startline_lower), (width-1, y_startline_lower), (50, 50, 200), 1)

    # 録音終了レベルの線を引く
    y_endline_upper = center_y - int(END_SEGMENT_THRESH * wave_height_max * rate)
    y_endline_lower = center_y + int(END_SEGMENT_THRESH * wave_height_max * rate)
    cv2.line(img, (0, y_endline_upper), (width-1, y_endline_upper), (200, 50, 50), 1)
    cv2.line(img, (0, y_endline_lower), (width-1, y_endline_lower), (200, 50, 50), 1)


    #print(f"Amplitude: {amplitude}")
    # 音声レベルの波形を描画
    # 振幅を正規化して高さを計算
    norm_amplitude_min = int((amplitude_min) * (wave_height_max) * rate)
    norm_amplitude_max = int((amplitude_max) * (wave_height_max) * rate)
    #print(f"norm_amplitude:{norm_amplitude}")
    
    # 振幅の線を描画
    cv2.line(img, (width-1, center_y), (width-1, center_y - norm_amplitude_min), hakei_color, stepwise)
    cv2.line(img, (width-1, center_y), (width-1, center_y - norm_amplitude_max), hakei_color, stepwise)

    return img



def audio_callback(indata, frames_count, time_info, status):

    # InputStream のコールバック。キューへ生データを流し込むだけ。
    if status:
        print(f"Audio callback status: {status}", flush=True)
    
    audio_queue.put(indata.copy() * AUDIO_GAIN)  # 録音音声のゲイン調整を適用してキューに追加


    while audio_queue_maxsize < audio_queue.qsize():
        try:
            #print("audio_queue is full. Discarding old data...")
            audio_queue.get_nowait()  # キューが満杯の場合、古いデータを破棄
        except queue.Empty:
            break


def transcribe_callback(segments, info):
    
    print(f"Detected language {info.language} with probability {info.language_probability}")

    for segment in segments:
        print(
            f"[{segment.start:.2f} -> {segment.end:.2f}] "
            f"no_speech={segment.no_speech_prob:.3f}, "
            f"avg_logprob={segment.avg_logprob:.3f}, "
            f"text={segment.text!r}"
        )


def transcribe_thread(audio_device_id, sample_rate, channels, chunk_size):


    global recording, record_frames

    print("Transcription thread started.")

    with sd.InputStream(device=audio_device_id, samplerate=sample_rate, channels=channels, blocksize=chunk_size, callback=audio_callback):
        while True:
            try:
                audio_data = audio_queue.get(timeout=1)
                
                if recording:
                    record_frames.append(audio_data)

            except queue.Empty:
                continue
            except KeyboardInterrupt:
                print("Transcription thread interrupted. Exiting...")
                break




def transcribe_thread(callback=transcribe_callback):

    global recording, record_frames, silence_buf_time, active_buf_time,AUDIO_GAIN


    print("Start recording...  [Ctrl+C] on console->Quit")
    with sd.InputStream( samplerate=SAMPLE_RATE, channels=CHANNELS, blocksize=CHUNK_SIZE, callback=audio_callback):


        visualization_img = None 
        while True:
            
            try:
                audio_data = audio_queue.get(timeout=1)
                
                
                if recording:
                    record_frames.append(audio_data)
                
                

                amplitude_max = np.max(audio_data)
                amplitude_min = np.min(audio_data)
                
                visualization_img = visualize_amplitude(img=visualization_img, 
                                                        amplitude_min=amplitude_min, 
                                                        amplitude_max=amplitude_max, 
                                                        width=img_width, height=img_height, 
                                                        audio_active=audio_active, 
                                                        is_recording=recording, 
                                                        frames_in_buffer_count=len(record_frames), 
                                                        rate=1.0)




                cv2.imshow("Waveform", visualization_img)
                key = cv2.waitKey(1)

                if key == ord('9'):
                    AUDIO_GAIN += 1
                if key == ord('8'):
                    AUDIO_GAIN -= 1

                if key == ord('t'):
                    
                    print("Transcribe last recorded segment...")
                    wav = flush_audio_queue()
                    segments, info = transcribe_audio(wav)
                    callback(segments, info)


                if key == ord('r'):
                    
                    print("switch recording mode...")
                    recording = not recording

                              
                if key == ord('q'):
                    print("Quit by user.")
                    break


            except KeyboardInterrupt:
                print("KeyboardInterrupt: Exiting...")
                break


if __name__ == "__main__":
    
    
    device_list = sd.query_devices()
    print("Available audio devices:")
    for i, device in enumerate(device_list):
        print(f"{i}: {device['name']} (max input channels: {device['max_input_channels']})")


    print(f"Using audio device ID: {recording_device_id}")


    # 音声認識スレッドを開始
    transcribe_thread = threading.Thread(target=transcribe_thread)

    transcribe_thread.start()

    while transcribe_thread.is_alive():
        transcribe_thread.join(timeout=1.0)


    print("Transcription thread has exited. Cleaning up...")


