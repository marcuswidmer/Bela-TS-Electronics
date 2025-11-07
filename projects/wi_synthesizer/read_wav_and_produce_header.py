import numpy as np
from scipy.io import wavfile
import os
import matplotlib.pyplot as plt
from scipy.signal import resample

def wav_to_c_header(wav_path: str, name: str = None, filename: str = None):
    # Derive names if not given
    base = os.path.splitext(os.path.basename(wav_path))[0]
    if name is None:
        name = base
    if filename is None:
        filename = f"{base}.h"

    # Read wav file
    sample_rate, data = wavfile.read(wav_path)

    #74 start
    #4559 end
    for i in range(300):
        print(i, data[i])

    for i in range(len(data) - 300, len(data)):
        print(i, data[i])

    croppedData = data[26:2226]

    plt.plot(croppedData)
    plt.show()

    resampledData = resample(croppedData, 2205)

    resampledData = resampledData / np.max(np.abs(resampledData))

    plt.plot(resampledData)
    plt.show()

    plt.plot(np.concatenate((resampledData, resampledData)))
    plt.show()
    # for i in range(3000, len(data)):
    #     print(i, data[i])

    # Convert to float32 in [-1, 1]
    if np.issubdtype(resampledData.dtype, np.integer):
        max_val = np.iinfo(resampledData.dtype).max
        resampledData = resampledData.astype(np.float32) / max_val
    else:
        resampledData = resampledData.astype(np.float32)

    # Flatten interleaved channels
    if resampledData.ndim == 2:
        print(f"Detected {resampledData.shape[1]} channels; flattening interleaved resampledData.")
        resampledData = resampledData.flatten()

    n = resampledData.shape[0]
    header_guard = f"{name.upper()}_H_"

    # Write header
    with open(filename, "w") as f:
        f.write(f"#ifndef {header_guard}\n")
        f.write(f"#define {header_guard}\n\n")

        f.write(f"// Source: {os.path.basename(wav_path)}\n")
        f.write(f"// Sample rate: {sample_rate} Hz\n")
        f.write(f"// Length: {n} samples\n\n")

        f.write(f"static const int {name}_sample_rate = {sample_rate};\n")
        f.write(f"static const int {name}_len = {n};\n\n")

        f.write(f"static const float {name}[{n}] = {{\n")
        for i, val in enumerate(resampledData):
            f.write(f"    {val:.6f}f")
            if i < n - 1:
                f.write(",")
            f.write("\n")
        f.write("};\n\n")

        f.write(f"#endif // {header_guard}\n")

    print(f"✅ Wrote {filename} with {n} samples at {sample_rate} Hz")

# Example usage:
wav_to_c_header("analog_strings_alt.wav", name="analog_strings_alt", filename="tables/analog_strings_alt.h")
