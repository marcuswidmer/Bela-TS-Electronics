#ifndef BITCRUSHER_HPP
#define BITCRUSHER_HPP

#include <string>
#include "../main_project/mainCommon.hpp"

class Bitcrusher
{
public:
    Bitcrusher();
    float process(float inSamp);
    void setAnalogIns(struct AnalogIns ins);
private:
    int levels_;
    float scale_;
};

#endif
