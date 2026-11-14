#!/usr/bin/env python3
"""Build and run Pepper's native regressions and simulator tests without Bela."""
from pathlib import Path
import subprocess
import sys
import tempfile

PROJECT = Path(__file__).resolve().parents[1]
REPO = PROJECT.parents[1]
SOURCES = [PROJECT / name for name in (
    "WiPepper.cpp", "WiSequencer.cpp", "WiAudioDrivenDrumMachine.cpp",
    "KarplusResonator.cpp", "JunoSynth.cpp")]


def main():
    subprocess.run(["g++", "-std=c++11", "-fsyntax-only", "-Iinclude", "-I.",
                    str(PROJECT / "render.cpp"), *map(str, SOURCES)], cwd=REPO, check=True)
    with tempfile.TemporaryDirectory(prefix="wi-pepper-tests-") as temp:
        for test in sorted((PROJECT / "tests").glob("*_test.cc")):
            binary = Path(temp) / test.stem
            print(f"Checking {test.name}", flush=True)
            subprocess.run(["g++", "-std=c++14", "-O2", "-I" + str(PROJECT),
                            str(test), *map(str, SOURCES), "-o", str(binary)], check=True)
            args = [str(binary)]
            if test.stem == "audio_drums_test":
                args.append(str(PROJECT))
            subprocess.run(args, check=True)
    subprocess.run([sys.executable, "-m", "unittest", "discover", "-s",
                    str(PROJECT / "simulator"), "-p", "test_*.py", "-v"], check=True)


if __name__ == "__main__":
    main()
