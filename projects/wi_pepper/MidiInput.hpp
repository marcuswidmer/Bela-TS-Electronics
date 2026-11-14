#pragma once
#include <cstdint>

// Raw-byte decoder shared by hardware and simulator. Realtime messages may
// appear anywhere, including inside running-status messages and SysEx.
class MidiInput {
public:
    void reset() { status_ = count_ = 0; sysex_ = false; }
    template<class Receiver> void process(uint8_t byte, Receiver& receiver) {
        if(byte >= 0xf8) { receiver.midiRealtime(byte); return; }
        if(byte & 0x80) {
            count_ = 0;
            if(byte == 0xf0) { sysex_ = true; status_ = 0; return; }
            if(byte == 0xf7) { sysex_ = false; status_ = 0; return; }
            sysex_ = false;
            status_ = byte;
            if(byte == 0xf6 || byte == 0xf4 || byte == 0xf5) status_ = 0;
            return;
        }
        if(sysex_ || !status_) return;
        data_[count_++] = byte;
        const uint8_t type = status_ & 0xf0, channel = status_ & 15;
        const unsigned int needed = status_ == 0xf1 || status_ == 0xf3 || type == 0xc0 || type == 0xd0 ? 1 : 2;
        if(count_ < needed) return;
        if(status_ == 0xf2) receiver.midiSongPosition(data_[0] | unsigned(data_[1]) << 7);
        else if(type == 0x90) receiver.midiNoteOn(data_[0], data_[1], channel);
        else if(type == 0x80) receiver.midiNoteOff(data_[0], channel, data_[1]);
        else if(type == 0xb0) receiver.midiControlChange(data_[0], data_[1], channel);
        else if(type == 0xe0) receiver.midiPitchBend(data_[0] | unsigned(data_[1]) << 7, channel);
        count_ = 0;
        if(status_ >= 0xf0) status_ = 0;
    }
private:
    uint8_t status_ = 0, count_ = 0, data_[2]{};
    bool sysex_ = false;
};
