#pragma once

#include "Library/LiveActor/LiveActor.h"
#include "Library/Math/RumbleCalculator.h"

class DemoShine : public al::LiveActor {
public:
    DemoShine(al::RumbleCalculator* rumbleCalculator)
        : al::LiveActor("ダミーシャイン"), mRumbleCalculator(rumbleCalculator) {}

    void startDemo(s32 index);
    void control() override;

    bool isReactionStarted() const { return mIsReactionStarted; }

private:
    al::RumbleCalculator* mRumbleCalculator = nullptr;
    s32 mStep = 0;
    bool mIsReactionStarted = false;
    char _115[0x3];
};

static_assert(sizeof(DemoShine) == 0x118);
