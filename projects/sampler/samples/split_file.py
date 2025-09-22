from pydub import AudioSegment
from pydub.silence import detect_nonsilent

# Function to generate note names dynamically
def generate_note_names(start_note="F1", num_notes=31):
    """
    Dynamically generate note names starting from a given note.

    Args:
        start_note (str): Starting note (e.g., "F1").
        num_notes (int): Number of notes to generate.

    Returns:
        list: List of generated note names.
    """
    # Order of notes in an octave
    note_order = ["E", "F", "F#", "G", "G#", "A", "A#", "B", "C", "C#", "D", "D#"]

    # Extract the starting octave and note
    start_octave = int(start_note[-1])
    start_index = note_order.index(start_note[:-1])

    # Generate the notes
    notes = []
    current_octave = start_octave
    for i in range(num_notes):
        # Get the current note
        note = note_order[start_index % len(note_order)]
        notes.append(f"{note}{current_octave}")

        # Move to the next note
        start_index += 1
        if start_index % len(note_order) == 0:  # Move to the next octave
            current_octave += 1

    return notes

def split_notes_auto(input_wav, output_directory, start_note="F1", silence_thresh=-30, min_silence_len=300, start_number=14, buffer_start_ms=100, buffer_end_ms=500):
    """
    Automatically detects and splits notes in a WAV file using silence detection.
    Ensures each note includes a buffer at the start and end of its duration.

    Args:
        input_wav (str): Path to the input WAV file.
        output_directory (str): Directory to save the output WAV files.
        start_note (str): Starting note (e.g., "F1").
        silence_thresh (int): Silence threshold in dBFS. Default is -30.
        min_silence_len (int): Minimum length of silence between notes in milliseconds. Default is 300 ms.
        start_number (int): Starting number for numbering the output files. Default is 14.
        buffer_start_ms (int): Milliseconds of audio to include before each note's start time. Default is 100.
        buffer_end_ms (int): Milliseconds of audio to include after each note's end time. Default is 500.
    """
    # Load the input WAV file
    audio = AudioSegment.from_wav(input_wav)

    # Detect nonsilent ranges (start and end times of notes)
    nonsilent_ranges = detect_nonsilent(audio, min_silence_len=min_silence_len, silence_thresh=silence_thresh)

    # Number of detected notes
    num_notes = len(nonsilent_ranges)

    if num_notes == 0:
        raise ValueError("No notes were detected in the input WAV file.")

    # Generate the note names dynamically
    note_names = generate_note_names(start_note=start_note, num_notes=num_notes)

    print(f"Detected {num_notes} notes in the audio file.")

    # Split and export each note
    for i in range(num_notes):
        # Start time with buffer (ensure it doesn't go below 0)
        start_ms = max(nonsilent_ranges[i][0] - buffer_start_ms, 0)
        # End time with buffer (ensure it doesn't exceed the audio length)
        end_ms = nonsilent_ranges[i + 1][0] if i + 1 < num_notes else len(audio)
        end_ms = min(end_ms - buffer_end_ms, len(audio))

        # Extract the audio segment
        note_audio = audio[start_ms:end_ms]

        # Assign note name based on index
        note_name = note_names[i]

        # Add numbering to the filename, starting from `start_number`
        number = f"{start_number + i:02}"  # Format as 2-digit number starting from `start_number`
        output_file = f"{output_directory}/{number}_{note_name}.wav"

        # Save the note audio
        note_audio.export(output_file, format="wav")
        print(f"Exported {output_file}")

# Example usage
if __name__ == "__main__":
    input_wav_path = "../../flute_bip_1_ampl.wav"       # Replace with the path to your input WAV file
    output_dir = "flute_melotron"        # Replace with your desired output directory
    start_note = "E0"                  # Replace with the starting note, if different (e.g., "C1")
    start_number = 1                 # Numbering starts from 14
    buffer_start_ms = 20              # Include 100 milliseconds before each note's start time
    buffer_end_ms = 500                # Include 500 milliseconds after each note's end time
    silence_thresh = -60            # Silence threshold for more sensitive detection
    min_silence_len = 300              # Minimum silence length between notes

    # Ensure the output directory exists
    import os
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    # Automatically split the notes
    split_notes_auto(input_wav_path, output_dir, start_note=start_note, start_number=start_number, buffer_start_ms=buffer_start_ms, buffer_end_ms=buffer_end_ms, silence_thresh=silence_thresh, min_silence_len=min_silence_len)