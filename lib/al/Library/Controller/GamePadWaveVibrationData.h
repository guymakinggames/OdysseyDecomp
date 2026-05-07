#pragma once

#include "Library/HostIO/HioNode.h"
#include "Library/Controller/IAudioFrameProcess.h"

namespace al {

class WaveVibrationHolder : public al::HioNode, public aal::IAudioFrameProcess {
public:
    WaveVibrationHolder(const al::GamePadSystem*);

private:
};

}  // namespace al
