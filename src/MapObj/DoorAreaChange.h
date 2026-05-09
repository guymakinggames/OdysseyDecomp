#pragma once

#include "Library/LiveActor/LiveActor.h"

namespace al {
struct ActorInitInfo;
class HitSensor;
class SensorMsg;
}  // namespace al

class DoorAreaChange : public al::LiveActor {
public:
    DoorAreaChange(const char* name);

    void init(const al::ActorInitInfo& info) override;
    void switchCloseAgain();
    void start();
    void appear() override;
    bool receiveMsg(const al::SensorMsg* message, al::HitSensor* other,
                    al::HitSensor* self) override;
    bool isOpen() const;
    void setNoStart();
    void enableStart();
    void setHomeDoor(bool isHomeDoor);
    void exeNoStart();
    void exeNoStartWithMessage();
    void exeCloseWait();
    void exeOpen();
    void exeOpenWait();
    void exeCloseBefore();
    void exeClose();

private:
    char _108[0x10];
};

static_assert(sizeof(DoorAreaChange) == 0x118);
