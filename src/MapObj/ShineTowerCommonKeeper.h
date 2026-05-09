#pragma once

#include <basis/seadTypes.h>
#include <math/seadMatrix.h>

namespace al {
struct ActorInitInfo;
class LayoutActor;
class LiveActor;
}  // namespace al

class ShineTowerScaleHolder;

class ShineTowerCommonKeeper {
public:
    ShineTowerCommonKeeper(al::LiveActor* actor, sead::Matrix34f* matrix, s32 mode,
                           const al::ActorInitInfo& info);

    void offLight(al::LiveActor* actor);
    void update();
    void updateCommon();
    void updateLayoutTotalCount(s32 count);
    void updateSensor();
    f32 calcScale() const;
    void setMeterRotateForWorld(bool isRotate);
    void startDemo(s32 count);
    void updateForDemo();
    bool isEndDemo() const;
    bool isLightOn() const;
    void setTotalCountLayout(al::LiveActor* actor) const;

private:
    ShineTowerScaleHolder* mScaleHolder = nullptr;
    sead::Matrix34f* mMatrix = nullptr;
    al::LiveActor* mActor = nullptr;
    bool mIsLightOn = true;
    char _19[0x7];
    al::LayoutActor* mTotalCountLayout = nullptr;
};

static_assert(sizeof(ShineTowerCommonKeeper) == 0x28);
