import numpy as np
import matplotlib.pyplot as plt
from scipy.io.wavfile import read, write
from scipy.signal import resample, convolve, tukey, correlate, spectrogram, find_peaks

notes = ["E0", "F0", "F#0", "G0", "G#0", "A0", "A#0", "B0", "C0", "C#0", "D0", "D#0", "E1", "F1", "F#1", "G1", "G#1", "A1", "A#1", "B1", "C1", "C#1", "D1", "D#1", "E2", "F2", "F#2", "G2", "G#2", "A2", "A#2", "B2", "C2", "C#2", "D2", "D#2", "E3", "F3", "F#3", "G3", "G#3", "A3", "A#3", "B3", "C3", "C#3", "D3"]
samples = []
max_len = 0
for note in notes:
	fs, data = read("audio_samples/%s.wav" %note)
	if data.shape[0] > max_len:
		max_len = data.shape[0]

	samples.append(data)

freqs = np.linspace(0, fs//2 +1, max_len // 2 + 1)

ffts = []
samples_padded = []
for sample in samples:
	sample_padded = np.concatenate((sample, np.zeros(max_len - sample.shape[0])))
	samples_padded.append(sample_padded)
	fft = np.abs(np.fft.rfft(sample_padded))
	ffts.append((fft > 20) *  fft)

test_chord = 0.1 * samples_padded[0] + 0.3*samples_padded[4] + 0.5 * samples_padded[7]

print(fs)

correlations = [0]
for fft in ffts:
	correlation = np.sum(np.abs(np.fft.rfft(test_chord)) * fft)
	#correlation = np.sum(ffts[0] * fft)
	correlations.append(correlation)

plt.plot(correlations)
plt.show()

peaks = []

peaks, properties = find_peaks(correlations, height = 0)
soerted_heights  = properties["peak_heights"]
heights = properties["peak_heights"]
# for peak in find_peaks(correlations)[0]:
# 	peaks.append(peak - 1)

print(soerted_heights)
print(heights)





