#pragma once

#include <container/seadPtrArray.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.h"

namespace al {
struct ActorInitInfo;
class HitSensor;
class JointSpringController;
class SensorMsg;
class ParabolicPath;
class Graph;
}  // namespace al

class EnemyStateReset;
class ItemGenerator;
class RabbitGraphVertex;

class Rabbit : public al::LiveActor {
public:
    Rabbit(const char*, const al::Graph*, bool isRabbitGraphMoon);
    void init(const al::ActorInitInfo& info) override;
    void attackSensor(al::HitSensor* self, al::HitSensor* other) override;
    bool receiveMsg(const al::SensorMsg* message, al::HitSensor* other,
                    al::HitSensor* self) override;
    void control() override;
    void endClipped() override;
    void kill() override;

    void initItem(s32, s32, const al::ActorInitInfo&);
    void appearReset();
    void resetParam();
    void setNerveJumpOrMove(al::LiveActor*, const RabbitGraphVertex*, const RabbitGraphVertex*,
                            bool);
    void setNerveJumpOrMoveStart(al::LiveActor*, const RabbitGraphVertex*,
                                 const RabbitGraphVertex*);
    void onMoveEndUpdateCurrentVertexAndNextNerve();
    RabbitGraphVertex* tryFindNextVertex();
    void fall(f32 velocity);
    void reduceStamina();
    void trySetPoseGraphMoveDir(f32);
    f32 getMoveSpeed() const;

    void exeReset();
    void exeStandby();
    void exeFind();
    void exeEndTired();
    void exeMove();
    void exeWait();
    void exeProvoke();
    void exeBreak();
    void exeTurn();
    void exeTurnReverse();
    void exeMoveStart();
    void exeJumpPath();
    void exeEndJump();
    void exeCatch();
    void exeGiveMoon();
    void exeGiveItem();
    void exeDisappear();

    void onMoveEnd();

private:
    const al::Graph* mGraph = nullptr;
    RabbitGraphVertex* mCurrentVertex = nullptr;
    RabbitGraphVertex* mNextVertex = nullptr;
    RabbitGraphVertex* mDestinationVertex = nullptr;
    s32 mMoveType = 2;

    ItemGenerator* mItemGenerator = nullptr;
    s32 mItemCount = 0;
    s32 mAppearItemNum = 1;
    s32 mAppearItemId = -1;

    al::HitSensor* mRewardReceiverSensor = nullptr;
    sead::PtrArray<RabbitGraphVertex> mDestinations;
    al::ParabolicPath* mParabolicPath = nullptr;
    f32 mPathMoveDistance = 0.0f;
    bool mIsRabbitGraphMoon = false;
    bool mIsTired = false;

    s32 mTiredTimer = 0;
    f32 mStamina = 1200.0f;
    bool mIsCaught = false;

    s32 mRandomWait = 0xf0;
    sead::PtrArray<al::JointSpringController> mJointSpringArray;
    sead::Quatf mTurnStartQuat = sead::Quatf::unit;
    f32 mTurnAngle = 0.0f;
    s32 mMoveFrame = 0;
    bool mIsSwoon = false;

    //

    al::HitSensor* mKickSensor = nullptr;
    s32 mKickSensorTimer = 0;
    f32 mShadowDropLength = 100.0f;
    bool mIsEnableAutoUpdateShadowMaskLength = false;
    f32 mSpineAngle = 0.0f;
    EnemyStateReset* mEnemyStateReset = nullptr;
    al::HitSensor* mOtherHitSensor = nullptr;
    s32 mExplosionSensorTimer = 0;
    sead::Vector3f mHomePosition = sead::Vector3f::zero;
    bool mIsDisableCatchByBindPlayer = false;
};

static_assert(sizeof(Rabbit) == 0x1f0);
