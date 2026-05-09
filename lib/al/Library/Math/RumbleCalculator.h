#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

namespace al {
class RumbleCalculator {
public:
    RumbleCalculator(f32 frequency, f32 phase, f32 amplitude, u32 step);
    void setParam(f32 frequency, f32 phase, f32 amplitude, u32 step);
    void start(u32 step);
    void calc();
    void reset();

    bool isActive() const { return mStep < mEndStep; }
    f32 getValueY() const { return mValue.y; }

private:
    void* mVtable = nullptr;
    s32 mStep = 0;
    s32 mEndStep = 0;
    sead::Vector3f mValue = sead::Vector3f::zero;
    f32 mFrequency = 0.0f;
    f32 mPhase = 0.0f;
    f32 mAmplitude = 0.0f;
};

static_assert(sizeof(RumbleCalculator) == 0x28);

class RumbleCalculatorCosMultLinear : public RumbleCalculator {
public:
    RumbleCalculatorCosMultLinear(f32 frequency, f32 phase, f32 amplitude, u32 step);
};
}  // namespace al
