import torch
import soundfile as sf
#import torchaudio
from torch.utils.data import Dataset


#audio: numpy array w/ the audio samples
#sample_rate: samples/s
    #----> ex: audio, sample_rate = sf.read("path/to/audio.wav")

#convert audio to tensors w/ pytorch
dry_waveform, dry_sample_rate = sf.read(
    "data/DRY-with-markers/trainval/nam.input.wav",
    dtype="float32"
)

wet_waveform, wet_sample_rate = sf.read(
    "data/audacity-distortion/trainval/nam.target.wav",
    dtype="float32"
)

dry_waveform = torch.from_numpy(dry_waveform)
wet_waveform = torch.from_numpy(wet_waveform)

print("dry shape: ", dry_waveform.shape)
print("wet shape: ", wet_waveform.shape)

print("dry sample_rate: ", dry_sample_rate)
print("wet sample_rate: ", wet_sample_rate)





