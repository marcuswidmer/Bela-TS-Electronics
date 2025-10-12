#include "Sample.hpp"
#include "../main_project/mainCommon.hpp"

#include <libraries/AudioFile/AudioFile.h>
#include <stdio.h>
#include <memory>
#include <chrono>
#include <ctime>
#include <iostream>
#include <dirent.h>

Sample::Sample()
{
}

Sample::~Sample()
{
    delete envGen_;
}

void Sample::init(int fs)
{
    std::string directoryPath = "./guitar_chorus_mockasin_samples/output_notes_2";

    // Open the directory
    DIR* dir = opendir(directoryPath.c_str());
    if (!dir) {
        std::cerr << "Error: Could not open directory " << directoryPath << "\n";
        return;
    }

    struct dirent* entry; // Represents a directory entry
    while ((entry = readdir(dir)) != nullptr) {
        // Skip "." and ".." entries
        if (entry->d_name[0] == '.') {
            continue;
        }

        // Check if the file has a ".wav" extension
        std::string filename = entry->d_name;
        if (filename.size() < 4 || filename.substr(filename.size() - 4) != ".wav") {
            continue;
        }

        // Construct the full file path
        std::string gFilename = directoryPath + "/" + filename;

        // Extract the numeric note ID from the filename (before the extension)
        std::string stem = filename.substr(0, filename.size() - 4); // Remove ".wav"
        int note = 0;
        try {
            note = std::stoi(stem); // Convert the numeric part to an integer
        } catch (const std::invalid_argument&) {
            std::cerr << "Warning: Could not extract numeric ID from " << gFilename << "\n";
            continue; // Skip files without a valid numeric ID
        }

        // Get the number of frames for the current file
        numFrames_[note] = AudioFileUtilities::getNumFrames(gFilename);
        printf("sample_.numFrames for %s: %d\n", gFilename.c_str(), numFrames_[note]);

        // Load the left (channel 0) and right (channel 1) audio data
        dataL_[note] = AudioFileUtilities::load(gFilename, numFrames_[note], 0)[0];
        dataR_[note] = AudioFileUtilities::load(gFilename, numFrames_[note], 0)[0];
        minNote_ = std::min(note, minNote_);
        maxNote_ = std::max(note, maxNote_);
    }
    note_ = minNote_; // Init to min
    printf("Num notes are: %d\n", maxNote_);
    closedir(dir);

    envGen_ = new ADSR(fs);
}

void Sample::process(float out[2])
{
    if (note_ < minNote_ || note_ > maxNote_)
        return;

    int state = envGen_->getState();
    if (state != state_) {
        state_ = state;
        //printf("ADSR state: %d\n", state);
    }

    float env = envGen_->process();

    out[0] = velocity_ * env * dataL_.at(note_)[readCounter_];
    out[1] = velocity_ * env * dataR_.at(note_)[readCounter_];

    if (readCounter_ < getNumFrames())
        readCounter_++;
    else
        setPlaying(false);

}

void Sample::setPlaying(bool playing, float velocity)
{
    envGen_->gate(playing);

    if (playing) {
        readCounter_ = 0;
        velocity_ = velocity;
    }
}

void Sample::setNote(int note)
{
    note_ = note;
}

int Sample::getNumFrames()
{
    return numFrames_.at(note_);
}