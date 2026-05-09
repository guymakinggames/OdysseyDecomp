#pragma once

#include "Library/LiveActor/LiveActor.h"

class ShineTowerRocket;

class DamageModel : public al::LiveActor {
public:
    DamageModel(ShineTowerRocket* shineTowerRocket)
        : al::LiveActor("ダメージモデル"), mShineTowerRocket(shineTowerRocket) {}

    void control() override;

    void setFollowDamage(bool isFollowDamage) { mIsFollowDamage = isFollowDamage; }

private:
    ShineTowerRocket* mShineTowerRocket = nullptr;
    bool mIsFollowDamage = false;
    char _111[0x7];
};

static_assert(sizeof(DamageModel) == 0x118);
