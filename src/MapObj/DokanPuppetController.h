#pragma once

#include "Library/Nerve/NerveStateBase.h"

namespace al {
class HitSensor;
class LiveActor;
class SensorMsg;
}  // namespace al

class DokanInfo;

class DokanPuppetController : public al::NerveStateBase {
public:
    DokanPuppetController(al::LiveActor* actor);
    ~DokanPuppetController() override;

    bool judgeMsgBindStart(al::HitSensor* self, al::HitSensor* other,
                           const al::LiveActor* actor, const DokanInfo* info);
    void startBind(al::HitSensor* self, al::HitSensor* other, const al::LiveActor* actor,
                   const al::LiveActor* target, const DokanInfo* info,
                   const DokanInfo* targetInfo, bool isReverse, bool isDirect);
    void warp();
    void out();
    void cancelBind();
    void setIsHome(bool isHome) { mIsHome = isHome; }
    bool isBackDoorChangeStageRequested() const { return mIsBackDoorChangeStageRequested; }

    void exeWait();
    void exeMoveIn();
    void exeIn();
    void exeDirectIn();
    void exeOut();
    void exeOnlyWarp();

private:
    u8 _11[0x8f] = {};
    bool mIsBackDoorChangeStageRequested = false;
    u8 _a1[0x3] = {};
    bool mIsHome = false;
    u8 _a5[0x3] = {};
};

static_assert(sizeof(DokanPuppetController) == 0xa8);
