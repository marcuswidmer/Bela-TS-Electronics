#include "Sampler.hpp"
#include "../main_project/mainCommon.hpp"
#include "../mainCommon.hpp"
#include "Sample.hpp"

#include <libraries/AudioFile/AudioFile.h>
#include <libraries/Midi/Midi.h>
#include <Bela.h>
#include <dirent.h>
#include <stdio.h>
#include <iostream>

#ifndef USE_NO_MIDI
void midiMessageCallback(MidiChannelMessage message, void* arg)
{
    auto * sampler = static_cast<Sampler *>(arg);
	if (message.getType() == kmmNoteOn || (message.getType() == kmmNoteOff)) {
		if (message.getType() == kmmNoteOn) {
            sampler->playNewVoice(message.getDataByte(0) - sampler->getMidiNoteOffset(), message.getDataByte(1) / 128.0f);
			//rt_printf("note on: %d. Vel: %d\n", message.getDataByte(0),message.getDataByte(1));

        } else if (message.getType() == kmmNoteOff) {
            sampler->releaseVoice(message.getDataByte(0) - sampler->getMidiNoteOffset());
			//rt_printf("note off: %d\n", message.getDataByte(0));
		}

	} else if (message.getType() == kmmControlChange) {
        //rt_printf("control change\n");
    }
}
#endif

Sampler::Sampler() : ds_(), secondDs_()
{
    #ifndef USE_NO_MIDI
        midi_ = new Midi();
    #endif
}

void Sampler::init(int fs)
{
    for (int i = 0; i < numVoices_; ++i) {
        samples_[i] = new Sample(&ds_, &secondDs_);
        samples_[i]->init(fs);
    }

#ifndef USE_NO_MIDI
    midi_->readFrom(midiPort_);
	midi_->enableParser(true);
	midi_->getParser()->setCallback(midiMessageCallback, (void*) this);
#endif
}

void Sampler::process(float out[2])
{
    float out_tmp[2];
    out_tmp[0] = 0.0f;
    out_tmp[1] = 0.0f;
    out[0] = 0.0f;
    out[1] = 0.0f;

    for (int i = 0; i < numVoices_; ++i) {
        samples_[i]->process(out_tmp);
        out[0] += amplitude_ * out_tmp[0];
        out[1] += amplitude_ * out_tmp[1];
    }
}

void Sampler::playNewVoice(int note, float velocity)
{
    if (!ds_.valid)
        return;

    currentVoiceIdx_++;
    if (currentVoiceIdx_ >= numVoices_)
        currentVoiceIdx_ = 0;

    samples_[currentVoiceIdx_]->setNote(note);
    samples_[currentVoiceIdx_]->setPlaying(true, velocity);
    activeVoices_.insert({note, currentVoiceIdx_});
}

void Sampler::releaseVoice(int note)
{
    if (!activeVoices_.count(note)) {
#ifndef USE_NO_MIDI
        rt_printf("Inactive voice released. This should not happen\n");
#endif
        return;
    }

    auto voicesWithNote = activeVoices_.equal_range(note);
    auto it = voicesWithNote.first;
    samples_[it->second]->setPlaying(false);
    activeVoices_.erase(it);
}

void Sampler::setAnalogIns(AnalogIns ins)
{
    if (ins.input_0 < PROGRAM_LEVEL_1 / 2)
        dataSet_ = 0;
    else if (ins.input_0 >= PROGRAM_LEVEL_1 - (PROGRAM_LEVEL_1 / 2) && ins.input_0 < PROGRAM_LEVEL_2 - (PROGRAM_LEVEL_1 / 2))
        dataSet_ = 1;
    else if (ins.input_0 >= PROGRAM_LEVEL_2 - (PROGRAM_LEVEL_1 / 2) && ins.input_0 < PROGRAM_LEVEL_3 - (PROGRAM_LEVEL_1 / 2))
        dataSet_ = 2;
    else if (ins.input_0 >= PROGRAM_LEVEL_3 - (PROGRAM_LEVEL_1 / 2))
        dataSet_ = 3;

    if (dataSet_ != prevDataSet_) {
        switch(dataSet_) {
            case 0:
                loadDataSet("samples/output_notes", ds_);
                break;
            case 1:
                loadDataSet("samples/output_notes_2", ds_);
                break;
            case 2:
                loadDataSet("samples/kjipe", ds_);
                break;
            case 3:
                loadDataSet("samples/violin_melotron", ds_, true);
                loadDataSet("samples/flute_melotron", secondDs_, true);
                break;
            default:
                invalidateDataSets();
                break;
        }
        prevDataSet_ = dataSet_;
    }

    amplitude_ = ins.input_1;
}

void Sampler::invalidateDataSets()
{
    ds_.valid = false;
    secondDs_.valid = false;
}

void Sampler::loadDataSet(std::string path, DataSet & ds, bool stereo)
{
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        std::cerr << "Error: Could not open directory " << path << "\n";
        return;
    }

    ds.minNote = 1000;
    ds.maxNote = 0;
    ds.valid = false;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        std::string filename = entry->d_name;
        if (filename.size() < 4 || filename.substr(filename.size() - 4) != ".wav") {
            continue;
        }

        // Construct the full file path
        std::string gFilename = path + "/" + filename;

        // Extract the numeric note ID from the filename (before the extension)
        std::string stem = filename.substr(0, filename.size() - 4); // Remove ".wav"
        int note = 0;
        try {
            note = std::stoi(stem);
        } catch (const std::invalid_argument&) {
            std::cerr << "Warning: Could not extract numeric ID from " << gFilename << "\n";
            continue; // Skip files without a valid numeric ID
        }

        ds.numFrames[note] = AudioFileUtilities::getNumFrames(gFilename);
        printf("sample_.numFrames for %s: %d\n", gFilename.c_str(), ds.numFrames[note]);

        if (stereo) {
            ds.dataL[note] = AudioFileUtilities::load(gFilename, ds.numFrames[note], 0)[0];
            ds.dataR[note] = AudioFileUtilities::load(gFilename, ds.numFrames[note], 0)[1];
        } else {
            ds.dataL[note] = AudioFileUtilities::load(gFilename, ds.numFrames[note], 0)[0];
            ds.dataR[note] = AudioFileUtilities::load(gFilename, ds.numFrames[note], 0)[0];
        }
        ds.minNote = std::min(note, ds.minNote);
        ds.maxNote = std::max(note, ds.maxNote);
    }

    printf("Min note: %d. Max note: %d. Num notes are: %d\n", ds.minNote, ds.maxNote, ds.maxNote - ds.minNote + 1);
    closedir(dir);

    ds.valid = true;

    for (int i = 0; i < numVoices_; ++i) {
        samples_[i]->setNote(0);
    }
}
