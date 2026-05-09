#pragma once

#include "Library/Demo/DemoActor.h"

class DemoPlayer : public al::DemoActor {
public:
    DemoPlayer(const char* name);

    void forceSetCostumeAndCapType(const char* costumeName, const char* capName);
    void init(const al::ActorInitInfo& info) override;
    void initAfterCreateFromFactory(const al::ActorInitInfo& info,
                                    const al::ActorInitInfo& linkInfo,
                                    const sead::Matrix34f* mtx, bool isResetPosition) override;
    void makeActorAlive() override;
    void control() override;
    void startAction(s32 actionIndex) override;
    void resetDynamics() override;

private:
    u8 _170[0xe8];
};

static_assert(sizeof(DemoPlayer) == 0x258);
