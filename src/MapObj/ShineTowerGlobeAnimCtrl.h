#pragma once

#include "Library/LiveActor/LiveActor.h"

namespace al {
struct ActorInitInfo;
}  // namespace al

class ShineTowerGlobeAnimCtrl : public al::LiveActor {
public:
    ShineTowerGlobeAnimCtrl(al::LiveActor* globeActor)
        : al::LiveActor("地球儀のアニメ"), mGlobeActor(globeActor) {}

    void init(const al::ActorInitInfo& info) override;
    void startClipped() override;
    void control() override;
    void start(f32 accel);
    void updateMusicBox();

    void setWorldMapCameraAnim(f32 rate, s32 step) {
        mWorldMapCameraAnimRate = rate;
        mMusicBoxTimer = step;
    }

    bool isHoldingMusicBox() const { return mIsHoldingMusicBox; }

private:
    al::LiveActor* mGlobeActor = nullptr;
    f32 mWorldMapCameraAnimRate = 1.0f;
    s32 mMusicBoxTimer = -1;
    s32 mBrakeTimer = 0;
    char _11c[0x4];
    const char* mMusicBoxName = nullptr;
    bool mIsHoldingMusicBox = false;
    char _129[0x7];
};

static_assert(sizeof(ShineTowerGlobeAnimCtrl) == 0x130);
