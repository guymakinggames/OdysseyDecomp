#pragma once

#include <basis/seadTypes.h>

#include "Library/Scene/ISceneObj.h"

class KoopaShip;
class ShineTowerRocket;

class KoopaShipDemoRequester : public al::ISceneObj {
public:
    KoopaShipDemoRequester(KoopaShip* koopaShip);

    void startKoopaShipDemoHomeFlyAway();
    bool isEnableStartWipeKoopaShipDemoHomeFlyAway() const;
    bool isEnableEndKoopaShipDemoHomeFlyAway() const;
    void endKoopaShipDemoHomeFlyAway();
    const char* getSceneObjName() const override;
    ~KoopaShipDemoRequester() override;

private:
    KoopaShip* mKoopaShip = nullptr;
};

static_assert(sizeof(KoopaShipDemoRequester) == 0x10);

namespace rs {
void createAndRegistKoopaShipDemoRequester(KoopaShip* koopaShip);
bool tryStartKoopaShipDemoHomeFlyAway(ShineTowerRocket* shineTowerRocket);
bool isEnableStartWipeKoopaShipDemoHomeFlyAway(const ShineTowerRocket* shineTowerRocket);
bool isEnableEndKoopaShipDemoHomeFlyAway(const ShineTowerRocket* shineTowerRocket);
void endKoopaShipDemoHomeFlyAway(ShineTowerRocket* shineTowerRocket);
s32 getKoopaShipWipeDemoHomeFlyAwayStep();
}  // namespace rs
