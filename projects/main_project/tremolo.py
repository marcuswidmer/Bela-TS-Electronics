import numpy as np
import math
import matplotlib.pyplot as plt

b = 30
a = 1000
numWaves = 10
cont = np.zeros([a, b])

allWaves = np.zeros([a,numWaves])
for waveNum in range(numWaves):
	sumWave = np.zeros(a)
	for i in range(a):
		for k in range(waveNum + 1):
			cont[i, k] = 1 / ((2 * k) + 1) * math.sin(((2 * k) + 1) * 2 * np.pi * i / a)
			sumWave[i] += cont[i, k]

	allWaves[:,waveNum] = sumWave;



#for i in range(a):
#	print(allWaves[i, 29], ", ")

plt.plot(allWaves)
plt.show()