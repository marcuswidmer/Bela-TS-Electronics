import subprocess
import sys
from pathlib import Path
import wave
import numpy as np
import matplotlib.pyplot as plt


def build_and_run_tests(project_root: Path):
    build_dir = project_root / "build"

    # Run: cd tests/build && make
    subprocess.run(["make"], cwd=str(build_dir), check=True)

    # Run: ./tests/build/WiSustainerTest from project root
    subprocess.run([str(project_root / "build" / "WiModularTest")],
                   cwd=str(project_root), check=True)


def find_wav(project_root: Path, name: str) -> Path:
    # Prefer file in project root if binary was run from root.
    w1 = project_root / name
    if w1.exists():
        return w1

    # Fallback: if binary ran in tests/build, the WAV might be there.
    w2 = project_root / "build" / name
    if w2.exists():
        return w2

    raise FileNotFoundError(f"{name} not found in project root or tests/build.")


def load_wav_16bit(path: Path):
    with wave.open(str(path), "rb") as wf:
        n_channels = wf.getnchannels()
        sampwidth = wf.getsampwidth()
        sample_rate = wf.getframerate()
        n_frames = wf.getnframes()

        if sampwidth != 2:
            raise ValueError(f"Expected 16-bit PCM WAV (2 bytes per sample), got {sampwidth} bytes.")

        raw_bytes = wf.readframes(n_frames)
        samples = np.frombuffer(raw_bytes, dtype=np.int16)

        if n_channels == 2:
            samples = samples.reshape(-1, 2)
        elif n_channels == 1:
            samples = samples.reshape(-1, 1)
        else:
            raise ValueError(f"Unsupported channel count: {n_channels} (only mono or stereo supported).")

        # Normalize to [-1.0, 1.0].
        audio = samples.astype(np.float32) / 32768.0
        return audio, sample_rate, n_channels


def plot_overlay(original_audio: np.ndarray, original_sr: int, original_ch: int,
                 title_orig: str):
    t_orig = np.arange(original_audio.shape[0]) / float(original_sr)

    # Choose colors and styles.
    color_orig_l = "tab:blue"
    color_orig_r = "tab:orange"
    color_rms = "tab:red"

    # If original is mono.
    if original_ch == 1:
        plt.figure(figsize=(10, 4))
        plt.plot(t_orig, original_audio[:, 0], lw=0.7, color=color_orig_l, label=title_orig)
        # Overlay RMS (mono or stereo -> use first channel).
        plt.title("Overlay: original and RMS")
        plt.xlabel("Time (s)")
        plt.ylabel("Amplitude")
        plt.grid(True, alpha=0.3)
        plt.legend()
        plt.tight_layout()
        plt.show()
        return

    # Original is stereo: plot two subplots and overlay RMS.
    fig, axes = plt.subplots(2, 1, figsize=(11, 6), sharex=False)

    # Left channel
    axes[0].plot(t_orig, original_audio[:, 0], lw=0.7, color=color_orig_l, label=f"{title_orig} — Left")
    axes[0].set_title("Left channel")
    axes[0].set_ylabel("Amplitude")
    axes[0].grid(True, alpha=0.3)
    axes[0].legend()

    # Right channel
    axes[1].plot(t_orig, original_audio[:, 1], lw=0.7, color=color_orig_r, label=f"{title_orig} — Right")
    axes[1].set_title("Right channel")
    axes[1].set_xlabel("Time (s)")
    axes[1].set_ylabel("Amplitude")
    axes[1].grid(True, alpha=0.3)
    axes[1].legend()

    fig.suptitle("Overlay: original and RMS")
    fig.tight_layout()
    plt.show()


def main():
    project_root = Path(__file__).resolve().parent

    # Build and run tests to generate the WAV.
    build_and_run_tests(project_root)

    # Locate the generated WAV files.
    wav_orig_path = find_wav(project_root, "cvOut0Seq.wav")

    # Load WAV data.
    original_audio, original_sr, original_ch = load_wav_16bit(wav_orig_path)

    # Plot overlay.
    plot_overlay(original_audio, original_sr, original_ch,
                 title_orig=wav_orig_path.name)


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as e:
        print(f"Command failed with exit code {e.returncode}: {e.cmd}", file=sys.stderr)
        sys.exit(e.returncode)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)