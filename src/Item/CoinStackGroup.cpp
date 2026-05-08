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

namespace {
constexpr f32 cFallDistance = 74.5f;

union RandomWork {
    sead::Random random;

    RandomWork() {}

    ~RandomWork() {}
};
}  // namespace

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
    const sead::Vector3f& trans = al::getTrans(this);

    if (stackAmount == 0)
        return;

    f32 transX = trans.x;
    f32 transY = trans.y;
    f32 transZ = trans.z;
    CoinStack* previousStack = nullptr;
    for (u32 index = 0; index != (u32)stackAmount; index++) {
        CoinStack* newStack = new CoinStack("CoinStack");
        newStack->init(initInfo);
        RandomWork random;
        sead::Vector3f stackTrans;

        if (index != 0) {
            random.random.init();
            f32 value = random.random.getF32();
            f32 randomX = (value * 10.0f) * ((value > 0.5f) ? 1.0f : -1.0f) + 0.0f;
            f32 stackY = index * 74.5f;
            random.random.init();
            value = random.random.getF32();
            f32 randomZ = (value * 10.0f) * ((value > 0.5f) ? 1.0f : -1.0f) + 0.0f;
            stackTrans.set(transX + randomX, transY + stackY, transZ + randomZ);
            newStack->postInit(this, stackTrans, previousStack, mClippingPos, clippingRadius,
                               &cFallDistance);
            previousStack = newStack;
            continue;
        }

        stackTrans.set(transX, transY, transZ);
        newStack->postInit(this, stackTrans, previousStack, mClippingPos, clippingRadius,
                           &cFallDistance);
        mCoinStack = newStack;
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
