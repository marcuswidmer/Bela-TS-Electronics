import numpy as np
import sounddevice as sd
from scipy.io.wavfile import read
from scipy.signal import kaiser, tukey
import matplotlib.pyplot as plt


def my_tukey(N):
    alpha = 0.1
    wind = np.zeros(N)

    for index in range(N):
        if index > N // 2:
            n = N - index
        else:
            n = index

        if n >= 0 and n < (alpha * N / 2):
            wind[index] = 0.5 * (1.0 - np.cos(2.0 * np.pi * index / (alpha * N)))
        elif n >= (alpha * N / 2) and n <= N / 2:
            wind[index] = 1.0;

    return wind

fs, data = read("../low-fi-machine/jazz_lick.wav")

T_start = 3 * fs
T_stop = int(3.6 * fs)

data = data[T_start : T_stop] / 32767
w = tukey(len(data), 0.1)

w_self_made = my_tukey(len(data))
plt.plot(w)
plt.plot(w_self_made+0.1)
plt.show()

data = (data * w)

DATA = np.fft.rfft(data)
DATA = DATA[0 : int(len(DATA) * 1)]
DATA = np.concatenate((DATA, np.zeros(int(len(DATA) * 4)).astype(np.complex)))
data_resamp = np.fft.irfft(DATA)

plt.plot(np.abs(DATA))
plt.show()

sound = np.array([])
reps = 3
for i in range(reps):
    sound = np.concatenate((sound, data_resamp))


#print(np.array(sound).shape)
# if (np.max(np.abs(sound)) < 0.5):
#   sd.play(np.array(sound), blocking = True)