import numpy as np
from scipy.io.wavfile import read, write
from scipy.signal import resample, convolve, tukey
import matplotlib.pyplot as plt
import sounddevice as sd
import time

def make_header(H, FFTSIZE, num_frames_in_H):
    data_string = "#ifndef TRANSFER_FUNCTION_PARTITIONED_H\n#define TRANSFER_FUNCTION_PARTITIONED_H\n"
    data_string += "\n // Real, complex every other\n"
    numbers_per_row = 200
    data_string += "float H[%d][%d] = {{" %(num_frames_in_H, 2 * FFTSIZE)

    for i in range(num_frames_in_H):
        for n in range(FFTSIZE):
            data_string += "%.18e, "%np.real(H[i,n])
            data_string += "%.18e, "%np.imag(H[i,n])
            if np.mod((n+1), numbers_per_row) == 0:
                data_string += "\n"
        data_string += "},\n{"

    data_string = data_string[:-5]
    data_string += "}};"
    data_string += "\n\n"

    data_string += "#endif"

    with open("transfer_function_partitioned.h", "w") as header_file:
        print(f"{data_string}", file=header_file)

fs, guitar_in = read("/Users/marwidme/Documents/Personal/AudioSamples/guitar_in_f.wav")
fs, notes_file = read("/Users/marwidme/Documents/Personal/AudioSamples/sitar/notes.wav")

#fs, notes_file = read("high_pitch_short/notes.wav")
#fs, notes_file = read("longer_low_pitch.wav")
length_in = len(guitar_in)

output = np.zeros(3 * length_in)

notes_list = []
len_mask = 100
cut_mask = [0] * len_mask
eps = 0.000001


a_note = []
initial_zero_masks = 10
for n in range(1, len(notes_file)):

    a_note.append(notes_file[n])

    if (n % len_mask == 0 and len(a_note) > (2 * len_mask) and
        np.max(np.abs(np.array(a_note[-2 * len_mask : -len_mask]))) < eps and
        np.max(np.abs(np.array(a_note[-len_mask :]))) > 100 * eps):

        notes_list.append(a_note[:-initial_zero_masks*len_mask])
        tmp = a_note[-initial_zero_masks*len_mask:]
        a_note.clear()
        for k in range(initial_zero_masks * len_mask):
            a_note.append(tmp[k])



notes_list.pop(0)
print("Found %d notes" %(len(notes_list)))
max_length = 0
for i,_ in enumerate(notes_list):
    if len(notes_list[i]) > max_length:
        max_length = len(notes_list[i])

summed_notes_list = np.zeros(max_length)
for i,_ in enumerate(notes_list):
    summed_notes_list[0:len(notes_list[i])] += notes_list[i]

#summed_notes_list = summed_notes_list[:80000]
summed_notes_list = summed_notes_list[:512]

summed_notes_list *= tukey(len(summed_notes_list), 1)


write("summed_notes.wav", 44100, (summed_notes_list * 32767).astype(np.int16))

framesize = 512
num_frames = length_in // framesize
num_frames_in_H = len(summed_notes_list) // framesize

padding = 512
H = np.zeros((num_frames_in_H, framesize + padding), dtype=complex)
D = np.zeros((num_frames_in_H, framesize + padding), dtype=complex)
C = np.zeros((num_frames_in_H, framesize + padding), dtype=complex)

for i in range(num_frames_in_H):
    H[i,:] = np.fft.fft(np.concatenate((summed_notes_list[i * framesize : (i+1) * framesize], np.zeros(padding))))
    #H[i,:] = np.ones(framesize + padding)

make_header(H, framesize + padding, num_frames_in_H)

convolved = np.zeros(length_in + padding)
before = time.perf_counter()

for i in range(num_frames):
    inbuf = guitar_in[i * framesize : (i+1) * framesize]
    INBUF = np.fft.fft(np.concatenate((inbuf,np.zeros(padding))))
    D[i % num_frames_in_H, :] = INBUF[:]

    C = np.zeros(framesize + padding, dtype=complex)
    for j in range(num_frames_in_H):
        C += D[(i - j) % num_frames_in_H, :] * H[j, :]

    c = np.real(np.fft.ifft(C))
    convolved[i * framesize : (i+1) * framesize + padding] += c
after = time.perf_counter()

print("Time it took: %fs" %(after-before))

#convolved = convolve(guitar_in, summed_notes_list, mode='full', method='fft')

max_in = np.max(np.abs(guitar_in))
convolved = convolved / (np.max(np.abs(convolved))) * max_in
