#include "WiPepper.hpp"

WiPepper wiPepper;

bool setup(BelaContext* context, void* userData)
{
    return wiPepper.setup(context);
}

void render(BelaContext* context, void* userData)
{
    wiPepper.processBlock(context);
}

void cleanup(BelaContext* context, void* userData)
{
}
