#pragma once

#include <basis/seadTypes.h>

namespace al {
class IUseSceneObjHolder;
}

class InputSeparator {
public:
    InputSeparator(const al::IUseSceneObjHolder*, bool);

    void reset();
    void update();
    void updateForSnapShotMode();
    bool isTriggerUiLeft();
    bool checkDominant(bool);
    bool isTriggerUiRight();
    bool isTriggerUiUp();
    bool isTriggerUiDown();
    bool isHoldUiLeft();
    bool isHoldUiRight();
    bool isHoldUiUp();
    bool isHoldUiDown();
    bool isRepeatUiLeft();
    bool isRepeatUiRight();
    bool isRepeatUiUp();
    bool isRepeatUiDown();
    bool isTriggerSnapShotMode();
    bool isTriggerIncrementPostProcessingFilterPreset();
    bool isTriggerDecrementPostProcessingFilterPreset();

private:
    const al::IUseSceneObjHolder* mSceneObjHolder = nullptr;
    bool mIsVertical = false;
    bool mIsHold = false;
    u8 mPaddingA[2] = {};
    s32 mRepeatInterval = 8;
    s32 mRepeatCounter = 0;
    u32 mPadding14 = 0;
};

static_assert(sizeof(InputSeparator) == 0x18);
