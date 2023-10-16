import numpy as np
import matplotlib.pyplot as plt

N = 1000
numHarmonics = 9
A = 1
T_p = 0.5
T = 1

harmonics = np.zeros([N, numHarmonics])
harmonics[:,0] = A * T_p / T # a_0 
t = np.linspace(-T/2, T/2, N)
for i in range(1, numHarmonics):
	harmonics[:,i] = 2 * A / (i * np.pi) * np.sin(i * np.pi * T_p / T) * np.cos(i * 2 * np.pi / T * t)

xSums = np.zeros([N, numHarmonics - 1])
for i in range(numHarmonics - 1):
	xSums[:,i] = np.sum(harmonics[:,0:(i+1)], axis = 1)

plt.plot(t, xSums[:,1:])
plt.show()  	

plt.plot(t, xSums[:,numHarmonics -2])
plt.show()