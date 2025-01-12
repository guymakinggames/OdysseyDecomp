#include "Item/CoinStackGroup.h"

#include <random/seadRandom.h>

#include "Library/Clipping/ClippingActorHolder.h"
#include "Library/LiveActor/ActorClippingFunction.h"
#include "Library/LiveActor/ActorInitFunction.h"
#include "Library/LiveActor/ActorInitUtil.h"
#include "Library/LiveActor/ActorPoseKeeper.h"
#include "Library/LiveActor/ActorPoseUtil.h"
#include "Library/LiveActor/ActorSceneInfo.h"
#include "Library/LiveActor/ActorSensorFunction.h"
#include "Library/LiveActor/ActorSensorUtil.h"
#include "Library/Math/MathUtil.h"
#include "Library/Placement/PlacementFunction.h"
#include "Library/Placement/PlacementId.h"
#include "Library/Stage/StageSwitchKeeper.h"
#include "Library/Stage/StageSwitchUtil.h"
#include "Library/Thread/FunctorV0M.h"

#include "Item/CoinStack.h"
#include "System/GameDataUtil.h"

CoinStackGroup::CoinStackGroup(const char* name) : al::LiveActor(name) {}

void CoinStackGroup::init(const al::ActorInitInfo& initInfo) {
    using CoinStackGroupFunctor = al::FunctorV0M<CoinStackGroup*, void (CoinStackGroup::*)()>;

    al::initActorSceneInfo(this, initInfo);
    al::initActorPoseTFSV(this);
    al::initActorSRT(this, initInfo);
    al::initActorClipping(this, initInfo);
    initHitSensor(1);
    al::addHitSensor(this, initInfo, "Body", 0xd, 0.0f, 8, sead::Vector3f::zero);
    al::initExecutorMapObjMovement(this, initInfo);
    al::initStageSwitch(this, initInfo);
    mPlacementId = new al::PlacementId();
    al::tryGetArg(&mStackAmount, initInfo, "StacksAmount");
    al::tryGetArg(&mIsMustSave, initInfo, "MustSave");
    if (mIsMustSave && al::tryGetPlacementId(mPlacementId, initInfo))
        rs::tryFindCoinStackSave(&mStackAmount, this, mPlacementId);
    generateCoinStackGroup(initInfo, mStackAmount);
    if (al::isValidStageSwitch(this, "SwitchAppear")) {
        if (al::listenStageSwitchOn(
                this, "SwitchAppear",
                CoinStackGroupFunctor(this, &CoinStackGroup::makeStackAppear)) &&
            al::listenStageSwitchOff(
                this, "SwitchAppear",
                CoinStackGroupFunctor(this, &CoinStackGroup::makeStackDisappear)) &&
            al::trySyncStageSwitchAppear(this)) {
            return;
        }
    }
    if (mCoinStack != nullptr)
        mCoinStack->makeStackAppear();
    makeActorAlive();
}

void CoinStackGroup::control() {
    for (CoinStack* stack = mCoinStack; stack != nullptr; stack = stack->getAbove())
        stack->setTransY(al::getTrans(stack).y - stack->getFallSpeed());
}

bool CoinStackGroup::receiveMsg(const al::SensorMsg* message, al::HitSensor* other,
                                al::HitSensor* self) {
    bool isMsgChangeAlpha = al::isMsgChangeAlpha(message);

    if (!isMsgChangeAlpha)
        return isMsgChangeAlpha;

    mCoinStack->changeAlpha(al::getChangeAlphaValue(message));
    return isMsgChangeAlpha;
}

void CoinStackGroup::makeActorDead() {
    if (mCoinStack != nullptr)
        mCoinStack->makeStackDisappear();
    al::LiveActor::makeActorDead();
}

void CoinStackGroup::makeActorAlive() {
    al::LiveActor::makeActorAlive();
    if (mCoinStack != nullptr)
        mCoinStack->makeStackAppear();
}

void CoinStackGroup::generateCoinStackGroup(const al::ActorInitInfo& initInfo, s32 stackAmount) {
    f32 clippingRadius = updateClippingInfo(stackAmount);
    sead::Vector3f trans = al::getTrans(this);

    if (stackAmount == 0)
        return;

    CoinStack* previousStack = nullptr;
    for (s32 index = 0; index != stackAmount; index++) {
        CoinStack* newStack = new CoinStack("CoinStack");
        newStack->init(initInfo);

        if (index == 0) {
            newStack->postInit(this, trans, previousStack, mClippingPos, clippingRadius,
                               &fallDistance);
            mCoinStack = newStack;
            previousStack = newStack;
            continue;
        }
        sead::Random rnd;
        rnd.init();
        f32 randx = rnd.getF32();
        f32 fVar2 = 1.0f;
        if (randx <= 0.5f)
            fVar2 = -1.0f;
        rnd.init();
        f32 randz = rnd.getF32();
        f32 fVar3 = 1.0f;
        if (randz <= 0.5f)
            fVar3 = -1.0f;
        sead::Vector3f strans;
        strans.x = trans.x + randx * 10.0f * fVar2 + 0.0f;
        strans.z = trans.y + randz * 10.0f * fVar3 + 0.0f;
        strans.y = trans.z + index * 74.5f;
        newStack->postInit(this, strans, previousStack, mClippingPos, clippingRadius,
                           &fallDistance);
        previousStack = newStack;
    }
}

void CoinStackGroup::makeStackAppear() {
    if (mCoinStack != nullptr)
        mCoinStack->makeStackAppear();
}

void CoinStackGroup::makeStackDisappear() {
    if (mCoinStack != nullptr)
        mCoinStack->makeStackDisappear();
}

f32 CoinStackGroup::setStackAsCollected(CoinStack* stack) {
    mStackAmount--;
    al::invalidateClipping(this);
    if (mIsMustSave)
        rs::saveCoinStack(this, mPlacementId, mStackAmount);

    if (mStackAmount == 0) {
        kill();
        return 0.0;
    }

    if (mCoinStack == stack)
        mCoinStack = stack->getAbove();

    return updateClippingInfo(mStackAmount);
}

f32 CoinStackGroup::updateClippingInfo(u32 stackAmount) {
    f32 clippingRadius = stackAmount * 0.75f * 74.5f;
    mClippingPos = al::getTrans(this) + sead::Vector3f(0.0f, clippingRadius * 0.75f, 0.0f);

    al::setClippingInfo(this, clippingRadius, &mClippingPos);
    return clippingRadius;
}

void CoinStackGroup::validateClipping() {
    al::validateClipping(this);
}
