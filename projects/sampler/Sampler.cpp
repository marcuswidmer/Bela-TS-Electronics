#include "Sampler.hpp"
#include "../main_project/mainCommon.hpp"
#include "../mainCommon.hpp"
#include "Voice.hpp"

#include <functional>
#include <libraries/AudioFile/AudioFile.h>
#include <libraries/Midi/Midi.h>
#include <Bela.h>
#include <dirent.h>
#include <stdio.h>
#include <iostream>
#include <random>

#ifndef USE_NO_MIDI
void midiMessageCallback(MidiChannelMessage message, void* arg)
{
    auto * sampler = static_cast<Sampler *>(arg);
	if (message.getType() == kmmNoteOn || (message.getType() == kmmNoteOff)) {
		if (message.getType() == kmmNoteOn) {
            sampler->playNewVoice(message.getDataByte(0) - sampler->getMidiNoteOffset(), message.getDataByte(1) / 128.0f);
			rt_printf("note on: %d. Vel: %d\n", message.getDataByte(0),message.getDataByte(1));

        } else if (message.getType() == kmmNoteOff) {
            sampler->releaseVoice(message.getDataByte(0) - sampler->getMidiNoteOffset());
			rt_printf("note off: %d\n", message.getDataByte(0));
		}

	} else if (message.getType() == kmmControlChange) {
        rt_printf("control change\n");
    }
}
#endif

Sampler::Sampler() : ds_(), secondDs_(), droneDs_({.loopSamples = true})
{
    #ifndef USE_NO_MIDI
        midi_ = new Midi();
    #endif
}

void Sampler::init(int fs, std::function<void(float)> ledCb)
{
    for (int i = 0; i < numRegVoices_; ++i) {
        voices_[i] = new Voice(&ds_, &secondDs_);
        voices_[i]->init(fs);
    }

    for (int i = numRegVoices_; i < numVoices_; ++i) {
        voices_[i] = new Voice(&droneDs_, &secondDs_);
        voices_[i]->init(fs);
    }

#ifndef USE_NO_MIDI
    midi_->readFrom(midiPort_);
	midi_->enableParser(true);
	midi_->getParser()->setCallback(midiMessageCallback, (void*) this);
#endif

    ledCb_ = ledCb;
}

void Sampler::reinit()
{
#ifndef USE_NO_MIDI
    delete midi_;
    midi_ = new Midi();
    midi_->readFrom(midiPort_);
    midi_->enableParser(true);
    midi_->getParser()->setCallback(midiMessageCallback, (void*)this);
#endif
}

void Sampler::process(float out[2])
{
    float out_tmp[2] = {};
    out[0] = 0.0f;
    out[1] = 0.0f;

    for (int i = 0; i < numRegVoices_; ++i) {
        voices_[i]->process(out_tmp, firstSecondMix_);
        out[0] += amplitude_ * out_tmp[0];
        out[1] += amplitude_ * out_tmp[1];
    }

    for (int i = numRegVoices_; i < numVoices_; ++i) {
        voices_[i]->process(out_tmp, firstSecondMix_);
        out[0] += amplitude_ * droneAmpl_ * out_tmp[0];
        out[1] += amplitude_ * droneAmpl_ * out_tmp[1];
    }
}

void Sampler::playNewVoice(int note, float velocity)
{
    if (!ds_.valid)
        return;

    currentVoiceIdx_++;
    if (currentVoiceIdx_ >= numRegVoices_)
        currentVoiceIdx_ = 0;

    voices_[currentVoiceIdx_]->setNote(note);
    voices_[currentVoiceIdx_]->setPlaying(true, velocity);
    activeVoices_.insert({note, currentVoiceIdx_});
}

void Sampler::playNewDroneVoice(int note)
{
    if (!droneDs_.valid)
        return;

    int droneVoiceIdx = numRegVoices_;

    voices_[droneVoiceIdx]->setNote(note);
    voices_[droneVoiceIdx]->setPlaying(true, 1.0f);
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
    voices_[it->second]->setPlaying(false);
    activeVoices_.erase(it);
}

void Sampler::setProgram(int program)
{
    program_ = program;
    if (program_ != prevProgram_) {
        if (ledCb_)
            ledCb_(0.01);

        invalidateDataSets();
        switch(program_) {
            case 0:
                loadDataSet("../sampler/samples/ufordragelig_trommer", droneDs_, true);
                playNewDroneVoice(1);
                break;
            case 1:
                loadDataSet("../sampler/samples/miami_skate_boy_trommer", droneDs_, true);
                playNewDroneVoice(1);
                break;
            case 2:
                loadDataSet("../sampler/samples/kjipe", ds_);
                loadDataSet("../sampler/samples/pling_plong_loop", droneDs_, true);
                playNewDroneVoice(1);
                break;
            case 3:
                // TODO: These samples are exported as 48kHz fs
                loadDataSet("../sampler/samples/violin_melotron", ds_, true);
                loadDataSet("../sampler/samples/flute_melotron", secondDs_, true);
                break;
            default:
                break;
        }
        prevProgram_ = program_;
    }
}

void Sampler::setAnalogIns(AnalogIns ins)
{
    program_ = convertToProgram(ins.input_0);

    if (program_ != prevProgram_) {
        invalidateDataSets();
        switch(program_) {
            case 0:
                loadDataSet("../sampler/samples/ufordragelig_trommer", droneDs_, true);
                playNewDroneVoice(1);
                break;
            case 1:
                loadDataSet("../sampler/samples/miami_skate_boy_trommer", droneDs_, true);
                playNewDroneVoice(1);
                break;
            case 2:
                loadDataSet("../sampler/samples/kjipe", ds_);
                loadDataSet("../sampler/samples/pling_plong_loop", droneDs_, true);
                playNewDroneVoice(1);
                break;
            case 3:
                // TODO: These samples are exported as 48kHz fs
                loadDataSet("../sampler/samples/violin_melotron", ds_, true);
                loadDataSet("../sampler/samples/flute_melotron", secondDs_, true);
                break;
            default:
                break;
        }
        prevProgram_ = program_;
    }

    amplitude_ = ins.input_1 / POT_COMP_FACTOR;
    firstSecondMix_ = ins.input_2 / POT_COMP_FACTOR;
    droneAmpl_ = ins.input_3 / POT_COMP_FACTOR;

    bool play = ins.input_7 > 0.42;
    if (play != play_)
    {
        if (play) {
            // std::random_device rd;
            // std::mt19937 gen(rd());
            // std::uniform_int_distribution<> dist(ds_.minNote, ds_.maxNote);
            testNote_++;
            if (testNote_ > ds_.maxNote)
                testNote_ = ds_.minNote;
            playNewVoice(testNote_, 1.0f);
        }
        else {
            releaseVoice(testNote_);
        }
        play_ = play;
    }
}

void Sampler::invalidateDataSets()
{
    ds_.valid = false;
    secondDs_.valid = false;
    droneDs_.valid = false;
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
    printf("loading %s\n", path.c_str());
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
        //printf("sample_.numFrames for %s: %d\n", gFilename.c_str(), ds.numFrames[note]);

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
    testNote_ = ds_.minNote;

    for (int i = 0; i < numVoices_; ++i) {
        voices_[i]->setNote(0);
    }
}

int Sampler::convertToProgram(float analogIn)
{
    if (analogIn < PROGRAM_LEVEL_1 / 2)
        return 3;
    else if (analogIn >= PROGRAM_LEVEL_1 - (PROGRAM_LEVEL_1 / 2) && analogIn < PROGRAM_LEVEL_2 - (PROGRAM_LEVEL_1 / 2))
        return 2;
    else if (analogIn >= PROGRAM_LEVEL_2 - (PROGRAM_LEVEL_1 / 2) && analogIn < PROGRAM_LEVEL_3 - (PROGRAM_LEVEL_1 / 2))
        return 1;
    else if (analogIn >= PROGRAM_LEVEL_3 - (PROGRAM_LEVEL_1 / 2))
        return 0;
    return 3;
}

float Sampler::convertFromProgram(int program)
{
    for (int i = 0; i < 100; ++i) {
       if (convertToProgram(1.0f / i) == program)
        return 1.0f / i;
    }
    return -1.0f;
}

void Sampler::setRelease(float r)
{
    for (int i = 0; i < numVoices_; ++i)
        voices_[i]->setRelease(r);
}

void Sampler::setMainLevel(float lev)
{
    amplitude_ = lev;
}

void Sampler::setDroneLevel(float lev)
{
    droneAmpl_ = lev;
}

