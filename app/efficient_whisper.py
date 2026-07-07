import torch
import efficient_whisper as whisper


model = whisper.load_model("large", device="cpu")
model.encoder = torch.jit.script(model.encoder)
model.decoder = torch.jit.script(model.decoder)
_ = model.half()
_ = model.cuda()



for audio_data in audio_data_list:
    result = model.transcribe(
        audio_data,
        verbose=True,
        language='japanese',
        beam_size=5,
        fp16=True,
        without_timestamps=True
    )
