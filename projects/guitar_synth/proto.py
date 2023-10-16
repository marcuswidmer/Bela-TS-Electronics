import numpy as np
from scipy.io.wavfile import read, write
from scipy.signal import resample, convolve, tukey, correlate
import matplotlib.pyplot as plt
import sounddevice as sd
import time

fs, data = read("../low-fi-machine/jazz_lick.wav")
data = data / 32767

framesize = 16
num_frames = len(data) // framesize

freqs = [
	440.00,
	466.16,
	493.88,
	523.25,
	554.37,
	587.33,
	622.25,
	659.26,
	698.46,
	739.99,
	783.99,
	830.60]

N = len(freqs)
S = 50
wavetable = np.zeros((N, framesize))
wavetable_s = np.zeros((N, S * framesize))
wavetable_l = np.zeros((N, len(data)))
t = np.linspace(0, framesize / fs, framesize)
t_s = np.linspace(0, S * framesize / fs, S * framesize)
t_l = np.linspace(0, len(data) / fs, len(data))

for i in range(N):
	wavetable[i, :] = np.sin(2 * np.pi * freqs[i] * t)
	wavetable_s[i, :] = np.sin(2 * np.pi * freqs[i] * t_s)
	wavetable_l[i, :] = np.sin(2 * np.pi * freqs[i] * t_l)

corr = np.zeros((N, num_frames))

for i in range(num_frames - S):
	for j in range(N):
		corr[j, i] = np.max(np.abs(correlate(data[i * framesize : i * framesize + (S * framesize)], wavetable_s[j, :])))
		wavetable_l[j, i * framesize : (i + 1) * framesize] *= corr[j, i]

#plt.imshow(corr, aspect = int(num_frames / N), interpolation = None)
out = np.sum(wavetable_l, axis = 0)
out = out / (np.max(np.abs(out))) * 0.2
plt.plot(out)
plt.show()


sd.play(out, fs, blocking = True)