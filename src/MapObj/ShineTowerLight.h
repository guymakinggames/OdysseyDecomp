#pragma once

#include <basis/seadTypes.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

namespace al {
class LiveActor;
}  // namespace al

class ShineTowerLight {
public:
    ShineTowerLight(al::LiveActor* actor, const sead::Matrix34f* matrix);

    void update(bool isWait, f32 frame);

    void setLightingWorldMap(bool isLightingWorldMap) { mIsLightingWorldMap = isLightingWorldMap; }

private:
    al::LiveActor* mActor = nullptr;
    sead::Matrix34f mMatrix = sead::Matrix34f::ident;
    sead::Vector3f mFrontDir = sead::Vector3f::zero;
    bool mIsLightingWorldMap = false;
    char _45[0x3];
};

static_assert(sizeof(ShineTowerLight) == 0x48);
