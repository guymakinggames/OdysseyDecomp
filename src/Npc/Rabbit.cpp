#include "Npc/Rabbit.h"

#include "Library/Collision/CollisionPartsKeeperUtil.h"
#include "Library/Collision/CollisionPartsTriangle.h"
#include "Library/Effect/EffectKeeper.h"
#include "Library/Effect/EffectSystemInfo.h"
#include "Library/Joint/JointControllerKeeper.h"
#include "Library/Joint/JointSpringController.h"
#include "Library/LiveActor/ActorActionFunction.h"
#include "Library/LiveActor/ActorClippingFunction.h"
#include "Library/LiveActor/ActorCollisionFunction.h"
#include "Library/LiveActor/ActorFlagFunction.h"
#include "Library/LiveActor/ActorInitUtil.h"
#include "Library/LiveActor/ActorModelFunction.h"
#include "Library/LiveActor/ActorMovementFunction.h"
#include "Library/LiveActor/ActorPoseUtil.h"
#include "Library/LiveActor/ActorSensorUtil.h"
#include "Library/Math/MathUtil.h"
#include "Library/Math/ParabolicPath.h"
#include "Library/Nature/NatureUtil.h"
#include "Library/Nerve/NerveSetupUtil.h"
#include "Library/Nerve/NerveUtil.h"
#include "Library/Placement/PlacementFunction.h"
#include "Library/Rail/VertexGraph.h"
#include "Library/Shadow/ActorShadowUtil.h"
#include "Library/Stage/StageSwitchKeeper.h"
#include "Library/Stage/StageSwitchUtil.h"
#include "Library/Thread/FunctorV0M.h"

#include "Enemy/EnemyStateReset.h"
#include "Npc/RabbitGraph.h"
#include "Util/ItemGenerator.h"
#include "Util/ItemUtil.h"
#include "Util/PlayerUtil.h"
#include "Util/SensorMsgFunction.h"

namespace {
NERVE_IMPL(Rabbit, Reset)
NERVE_IMPL_(Rabbit, StandbyWait, Standby)
NERVE_IMPL_(Rabbit, StandbyRest, Standby)
NERVE_IMPL(Rabbit, Find)
NERVE_IMPL(Rabbit, EndTired)
NERVE_ON_END_IMPL(Rabbit, Move)
NERVE_IMPL_(Rabbit, Jump, Move)
NERVE_IMPL(Rabbit, Wait)
NERVE_IMPL_(Rabbit, Rest, Wait)
NERVE_IMPL_(Rabbit, WaitTired, Wait)
NERVE_IMPL(Rabbit, Provoke)
NERVE_IMPL(Rabbit, Break)
NERVE_IMPL(Rabbit, Turn)
NERVE_IMPL(Rabbit, TurnReverse)
NERVE_IMPL(Rabbit, MoveStart)
NERVE_IMPL(Rabbit, JumpPath)
NERVE_IMPL(Rabbit, EndJump)
NERVE_IMPL_(Rabbit, CatchToGiveMoon, Catch)
NERVE_IMPL_(Rabbit, CatchToGiveItem, Catch)
NERVE_IMPL(Rabbit, GiveMoon)
NERVE_IMPL(Rabbit, GiveItem)
NERVE_IMPL(Rabbit, Disappear)

NERVES_MAKE_STRUCT(Rabbit, StandbyWait, Reset, Move, Jump, Turn, TurnReverse, Wait, Rest, Provoke,
                   EndTired, JumpPath, MoveStart, Break, Find, WaitTired, StandbyRest,
                   CatchToGiveMoon, CatchToGiveItem, EndJump, GiveMoon, GiveItem, Disappear)

RabbitGraphVertex* toRabbitVertex(al::Graph::Vertex* vertex) {
    return static_cast<RabbitGraphVertex*>(vertex);
}

RabbitGraphVertex* toRabbitVertex(al::Graph::PosVertex* vertex) {
    return static_cast<RabbitGraphVertex*>(vertex);
}

RabbitGraphEdge* toRabbitEdge(al::Graph::Edge* edge) {
    return static_cast<RabbitGraphEdge*>(edge);
}

f32 calcDistanceToPlayerBody(const al::LiveActor* actor, const sead::Vector3f& pos) {
    return (rs::getPlayerBodyPos(actor) - pos).length();
}

f32 calcPlayerChaseDistance(const al::LiveActor* actor) {
    return rs::isPlayerHackTRex(actor) ? 3000.0f : 900.0f;
}

bool isSwoonStartEnd(const Rabbit* rabbit) {
    return al::isActionPlaying(rabbit, "SwoonStart") && al::isActionEnd(rabbit);
}

bool isRunActionPlaying(const Rabbit* rabbit) {
    return al::isActionPlaying(rabbit, "RunFine") || al::isActionPlaying(rabbit, "RunTired") ||
           al::isActionPlaying(rabbit, "RunTiredSlow");
}

void slerpQuatToGround(Rabbit* rabbit, f32 rate) {
    const sead::Vector3f& groundNormal = al::getCollidedGroundNormal(rabbit);
    sead::Vector3f front;
    sead::Quatf targetQuat;
    al::calcQuatFront(&front, rabbit);
    al::makeQuatUpFront(&targetQuat, groundNormal, front);
    al::slerpQuat(al::getQuatPtr(rabbit), al::getQuat(rabbit), targetQuat, rate);
}

void slerpQuatToWorldUpFront(Rabbit* rabbit, f32 rate) {
    sead::Vector3f front;
    sead::Quatf targetQuat;
    al::calcQuatFront(&front, rabbit);
    al::makeQuatUpFront(&targetQuat, sead::Vector3f::ey, front);
    al::slerpQuat(al::getQuatPtr(rabbit), al::getQuat(rabbit), targetQuat, rate);
}
}  // namespace

Rabbit::Rabbit(const char* name, const al::Graph* graph, bool isRabbitGraphMoon)
    : al::LiveActor(name), mGraph(graph), mIsRabbitGraphMoon{isRabbitGraphMoon} {}

void Rabbit::init(const al::ActorInitInfo& initInfo) {
    using RabbitFunctor = al::FunctorV0M<Rabbit*, void (Rabbit::*)()>;

    al::initActorWithArchiveName(this, initInfo, "Rabbit", mIsRabbitGraphMoon ? "Moon" : nullptr);
    mCurrentVertex =
        (RabbitGraphVertex*)al::findNearestPosVertex(mGraph, al::getTrans(this), -1.0f);
    al::tryGetArg(&mMoveType, initInfo, "MoveType");
    al::tryGetArg(&mAppearItemNum, initInfo, "AppearItemNum");
    al::tryGetArg(&mIsEnableAutoUpdateShadowMaskLength, initInfo,
                  "IsEnableAutoUpdateShadowMaskLength");
    al::tryGetArg(&mIsDisableCatchByBindPlayer, initInfo, "IsDisableCatchByBindPlayer");
    s32 linkChildNum = al::calcLinkChildNum(initInfo, "RabbitDestination");
    if (0 < linkChildNum) {
        mDestinations.allocBuffer(linkChildNum, nullptr);
        for (s32 i = 0; i < linkChildNum; i++) {
            sead::Vector3f linkpos;
            al::getChildLinkT(&linkpos, initInfo, "RabbitDestination", i);
            RabbitGraphVertex* selected = nullptr;
            f32 minLength = sead::Mathf::maxNumber();
            for (s32 e = 0; e < mGraph->getVertexCount(); e++) {
                RabbitGraphVertex* vertex = (RabbitGraphVertex*)mGraph->getVertex(e);
                if (!vertex->getBool2()) {
                    sead::Vector3f diff = linkpos;
                    diff -= vertex->getPos();
                    f32 size = diff.length();
                    if (size < minLength) {
                        selected = vertex;
                        minLength = size;
                    }
                }
            }
            if (selected != nullptr)
                mDestinations.pushBack(selected);
        }
    }
    s32 itemType = rs::getItemType(initInfo);
    s32 itemNum = mAppearItemNum;
    if (itemType == 0x11) {
        mItemGenerator = new ItemGenerator();
        mItemGenerator->initNoLinkShine(this, initInfo, true);
    } else {
        rs::tryInitItem(this, itemType, initInfo, false);
    }
    mAppearItemId = itemType;
    if (itemType != 0)
        itemNum = 1;
    mAppearItemNum = itemNum;
    al::startActionAtRandomFrame(this, "Wait");
    mParabolicPath = new al::ParabolicPath();
    al::initJointControllerKeeper(this, 5);
    mJointSpringArray.allocBuffer(4, nullptr);
    al::JointSpringController* spring = al::initJointSpringController(this, "EarL1");
    spring->setStability(0.3f);
    spring->setFriction(0.6f);
    spring->setLimitDegree(45.0f);
    mJointSpringArray.pushBack(spring);
    al::JointSpringController* spring1 = al::initJointSpringController(this, "EarL2");
    spring1->setStability(0.3f);
    spring1->setFriction(0.6f);
    spring1->setLimitDegree(45.0f);
    mJointSpringArray.pushBack(spring1);
    al::JointSpringController* spring2 = al::initJointSpringController(this, "EarR1");
    spring2->setStability(0.3f);
    spring2->setFriction(0.6f);
    spring2->setLimitDegree(45.0f);
    mJointSpringArray.pushBack(spring2);
    al::JointSpringController* spring3 = al::initJointSpringController(this, "EarR2");
    spring3->setStability(0.3f);
    spring3->setFriction(0.6f);
    spring3->setLimitDegree(45.0f);
    mJointSpringArray.pushBack(spring3);
    al::initJointLocalZRotator(this, &mSpineAngle, "Spine");
    mShadowDropLength = al::getShadowMaskDropLength(this, "Hip");

    if (!al::listenStageSwitchOnOff(this, "SwitchRabbitAppear",
                                    RabbitFunctor(this, &Rabbit::resetParam),
                                    RabbitFunctor(this, &Rabbit::kill))) {
        al::initNerve(this, &NrvRabbit.StandbyWait, 0);
    } else {
        mEnemyStateReset = new EnemyStateReset(this, initInfo, nullptr);
        al::initNerve(this, &NrvRabbit.StandbyWait, 1);
        al::initNerveState(this, mEnemyStateReset, &NrvRabbit.Reset, "リセット");
        mHomePosition.set(al::getTrans(this));
    }
    makeActorDead();
}

void Rabbit::attackSensor(al::HitSensor* self, al::HitSensor* other) {
    if (al::isSensorEnemyAttack(self) && !rs::sendMsgPushToMotorcycle(other, self)) {
        if (rs::sendMsgRabbitKick(other, self)) {
            mKickSensorTimer = 30;
            mKickSensor = other;
        }
        if (al::isNerve(this, &NrvRabbit.CatchToGiveMoon) ||
            al::isNerve(this, &NrvRabbit.CatchToGiveItem) ||
            al::isNerve(this, &NrvRabbit.GiveMoon) || al::isNerve(this, &NrvRabbit.GiveItem)) {
            rs::sendMsgPushToPlayer(other, self);
        }
        if (mOtherHitSensor != nullptr && 0 < mExplosionSensorTimer) {
            if (al::getSensorHost(mOtherHitSensor) == al::getSensorHost(other))
                return;
        }
        al::sendMsgPushAndKillVelocityToTarget(this, self, other);
    }
}

bool Rabbit::receiveMsg(const al::SensorMsg* message, al::HitSensor* other, al::HitSensor* self) {
    if (al::isNerve(this, &NrvRabbit.Reset))
        return false;
    if (al::isNerve(this, &NrvRabbit.Disappear))
        return false;
    if (!al::isNerve(this, &NrvRabbit.CatchToGiveMoon) &&
        !al::isNerve(this, &NrvRabbit.CatchToGiveItem) && !al::isNerve(this, &NrvRabbit.GiveMoon) &&
        !al::isNerve(this, &NrvRabbit.GiveItem) && al::isSensorName(self, "Attack") &&
        al::tryReceiveMsgPushAndAddVelocityH(this, message, other, self, 5.0f)) {
        return true;
    }
    if (al::isMsgExplosion(message)) {
        mOtherHitSensor = other;
        mExplosionSensorTimer = 0x78;
        return false;
    }
    if (!rs::isMsgCapAttack(message) && !rs::isMsgHosuiAttack(message) &&
        !al::isMsgKickStoneAttack(message) && !rs::isMsgYoshiTongueAttack(message) &&
        !rs::isMsgSeedAttack(message) && !rs::isMsgRadishAttack(message) &&
        !rs::isMsgGamaneBullet(message) && !rs::isMsgMayorItemReflect(message) &&
        !rs::isMsgHammerBrosHammerEnemyAttack(message) &&
        !rs::isMsgHammerBrosHammerHackAttack(message) && !rs::isMsgFireDamageAll(message)) {
        if ((!al::isMsgPlayerItemGet(message) && !rs::isMsgPlayerRabbitGet(message) &&
             !rs::isMsgPlayerAndCapHipDropAll(message) && !al::isMsgPlayerTrampleReflect(message) &&
             !rs::isMsgHackAttack(message)) ||
            (!al::isNerve(this, &NrvRabbit.StandbyWait) &&
             !al::isNerve(this, &NrvRabbit.StandbyRest) && !al::isNerve(this, &NrvRabbit.Find) &&
             !al::isNerve(this, &NrvRabbit.Turn) && !al::isNerve(this, &NrvRabbit.MoveStart) &&
             !al::isNerve(this, &NrvRabbit.Move) && !al::isNerve(this, &NrvRabbit.Break) &&
             !al::isNerve(this, &NrvRabbit.EndJump) && !al::isNerve(this, &NrvRabbit.Provoke) &&
             !al::isNerve(this, &NrvRabbit.Jump) && !al::isNerve(this, &NrvRabbit.JumpPath) &&
             !al::isNerve(this, &NrvRabbit.Wait) && !al::isNerve(this, &NrvRabbit.WaitTired) &&
             !al::isNerve(this, &NrvRabbit.Rest))) {
            return false;
        }
        if (mIsDisableCatchByBindPlayer && rs::isPlayerBinding(this))
            return false;
        if (!mIsCaught) {
            rs::requestHitReactionToAttacker(message, self, other);
            mIsCaught = true;
            al::startHitReaction(this, "接触");
            if ((al::isNerve(this, &NrvRabbit.Jump) || al::isNerve(this, &NrvRabbit.JumpPath)) &&
                al::tryStartActionIfNotPlaying(this, "SwoonStart")) {
                al::setActionFrameRate(this, 1.0f);
            }
        }
        mRewardReceiverSensor = other;
        if (al::isNerve(this, &NrvRabbit.StandbyWait) ||
            al::isNerve(this, &NrvRabbit.StandbyRest) || al::isNerve(this, &NrvRabbit.Find) ||
            al::isNerve(this, &NrvRabbit.Turn) || al::isNerve(this, &NrvRabbit.MoveStart) ||
            al::isNerve(this, &NrvRabbit.Move) || al::isNerve(this, &NrvRabbit.Break) ||
            al::isNerve(this, &NrvRabbit.EndJump) || al::isNerve(this, &NrvRabbit.Provoke) ||
            al::isNerve(this, &NrvRabbit.Wait) || al::isNerve(this, &NrvRabbit.WaitTired) ||
            al::isNerve(this, &NrvRabbit.Rest)) {
            al::Nerve* nerve = &NrvRabbit.CatchToGiveItem;
            if (mAppearItemId == 0x11)
                nerve = &NrvRabbit.CatchToGiveMoon;

            al::setNerve(this, nerve);
        }
        return true;
    }
    if ((mKickSensor != nullptr && mKickSensor == other && mKickSensorTimer >= 1) ||
        al::isActionPlaying(this, "SwoonStart") ||
        (al::isNerve(this, &NrvRabbit.Move) && 5 >= mMoveFrame)) {
        return true;
    }
    if (!al::isNerve(this, &NrvRabbit.StandbyWait) && !al::isNerve(this, &NrvRabbit.StandbyRest) &&
        !al::isNerve(this, &NrvRabbit.Find) && !al::isNerve(this, &NrvRabbit.Turn) &&
        !al::isNerve(this, &NrvRabbit.MoveStart) && !al::isNerve(this, &NrvRabbit.Move) &&
        !al::isNerve(this, &NrvRabbit.Break) && !al::isNerve(this, &NrvRabbit.EndJump) &&
        !al::isNerve(this, &NrvRabbit.Provoke) && !al::isNerve(this, &NrvRabbit.Jump) &&
        !al::isNerve(this, &NrvRabbit.JumpPath) && !al::isNerve(this, &NrvRabbit.Wait) &&
        !al::isNerve(this, &NrvRabbit.WaitTired) && !al::isNerve(this, &NrvRabbit.Rest)) {
        if (!al::isNerve(this, &NrvRabbit.CatchToGiveMoon) &&
            !al::isNerve(this, &NrvRabbit.CatchToGiveItem) &&
            !al::isNerve(this, &NrvRabbit.GiveMoon) && !al::isNerve(this, &NrvRabbit.GiveItem)) {
            return al::isNerve(this, &NrvRabbit.EndTired);
        }
    } else {
        al::startHitReaction(this, "投げ物ヒット");
        rs::requestHitReactionToAttacker(message, self, other);
        mTiredTimer = 300;
        mIsTired = true;
        mIsSwoon = true;
        mStamina = 1200.0f;
        if (!al::tryStartActionIfNotPlaying(this, "SwoonStart"))
            return true;
        al::setActionFrameRate(this, 1.0f);
    }
    return true;
}

void Rabbit::control() {
    if (!al::isNerve(this, &NrvRabbit.Move) && !al::isNerve(this, &NrvRabbit.Jump) &&
        !al::isNerve(this, &NrvRabbit.Turn) && !al::isNerve(this, &NrvRabbit.TurnReverse)) {
        mMoveFrame = 0;
    } else {
        mMoveFrame++;
    }

    if (mIsTired) {
        if (0 < mTiredTimer)
            mTiredTimer--;
        if ((al::isNerve(this, &NrvRabbit.Move) || al::isNerve(this, &NrvRabbit.Wait) ||
             al::isNerve(this, &NrvRabbit.Rest) || al::isNerve(this, &NrvRabbit.Provoke)) &&
            mTiredTimer == 0) {
            mIsTired = false;
            mIsSwoon = false;
            al::setNerve(this, &NrvRabbit.EndTired);
            return;
        }
    }

    if (0 < mKickSensorTimer) {
        mKickSensorTimer--;
        if (mKickSensorTimer == 0)
            mKickSensor = nullptr;
    }

    if (0 < mExplosionSensorTimer) {
        mExplosionSensorTimer--;
        if (mExplosionSensorTimer == 0)
            mOtherHitSensor = nullptr;
    }

    f32 frameRate;
    if (((mTiredTimer > 0 || 400.0f > mStamina) && !al::isNerve(this, &NrvRabbit.Provoke) &&
         !al::isNerve(this, &NrvRabbit.Wait) &&
         (!al::isNerve(this, &NrvRabbit.WaitTired) || al::isActionPlaying(this, "WaitSwoon"))) &&
        (!al::isNerve(this, &NrvRabbit.Rest) && !al::isNerve(this, &NrvRabbit.CatchToGiveItem) &&
         !al::isNerve(this, &NrvRabbit.CatchToGiveMoon) &&
         !al::isNerve(this, &NrvRabbit.EndTired) && !al::isNerve(this, &NrvRabbit.GiveMoon) &&
         !al::isNerve(this, &NrvRabbit.GiveItem) && !al::isNerve(this, &NrvRabbit.Disappear))) {
        if (!al::isEffectEmitting(this, "Sweat"))
            al::emitEffect(this, "Sweat", nullptr);
        if (al::isActionPlaying(this, "RunFine")) {
            al::startAction(this, mTiredTimer > 0 ? "RunTiredSlow" : "RunTired");
            frameRate = 0.8f;
            al::setActionFrameRate(this, frameRate);
        } else if (mTiredTimer > 0 && al::isActionPlaying(this, "RunTired")) {
            al::startAction(this, "RunTiredSlow");
            frameRate = 0.8f;
            al::setActionFrameRate(this, frameRate);
        }

    } else {
        if (al::isEffectEmitting(this, "Sweat"))
            al::deleteEffect(this, "Sweat");
        if (al::isActionPlaying(this, "RunTired") || al::isActionPlaying(this, "RunTiredSlow")) {
            al::startAction(this, "RunFine");
            frameRate = 1.0f;
            al::setActionFrameRate(this, frameRate);
        }
    }

    al::tryAddRippleSmall(this);
    if (mItemGenerator != nullptr && mItemGenerator->isShine())
        mItemGenerator->tryUpdateHintTransIfExistShine();
    if (mIsEnableAutoUpdateShadowMaskLength) {
        if (al::isOnGround(this, 0)) {
            al::setShadowMaskDropLength(this, 100.0f, "Hip");
            return;
        }
        sead::Vector3f position;
        al::calcJointOffsetPos(&position, this, "Hip", al::getShadowMaskOffset(this, "Hip"));
        al::Triangle triangle;
        sead::Vector3f poly;
        if (alCollisionUtil::getFirstPolyOnArrow(this, &poly, &triangle, position,
                                                 -sead::Vector3f::ey * mShadowDropLength, nullptr,
                                                 nullptr)) {
            sead::Vector3f shadowLength = poly;
            shadowLength -= position;
            f32 newLength = sead::Mathf::clampMin(shadowLength.length(), 10.0f);
            al::setShadowMaskDropLength(this, newLength, "Hip");

        } else {
            al::setShadowMaskDropLength(this, mShadowDropLength, "Hip");
        }
    }
}

void Rabbit::endClipped() {
    al::LiveActor::endClipped();
    resetParam();
}

void Rabbit::kill() {
    al::LiveActor::kill();
}

void Rabbit::initItem(s32 itemId, s32 itemCount, const al::ActorInitInfo& initInfo) {
    if (itemId == 0x11) {
        mItemGenerator = new ItemGenerator();
        mItemGenerator->initNoLinkShine(this, initInfo, true);
    } else {
        rs::tryInitItem(this, itemId, initInfo, false);
    }
    mAppearItemId = itemId;
    mAppearItemNum = itemId == 0 ? itemCount : 1;
}

void Rabbit::appearReset() {
    if (al::isAlive(this))
        return;
    al::LiveActor::appear();
    al::resetPosition(this, mHomePosition);
    mCurrentVertex =
        (RabbitGraphVertex*)al::findNearestPosVertex(mGraph, al::getTrans(this), -1.0f);
    resetParam();
    al::setNerve(this, &NrvRabbit.Reset);
}

void Rabbit::resetParam() {
    mTiredTimer = 0;
    mIsTired = false;
    mIsSwoon = false;
    mStamina = 1200.0f;
}

void Rabbit::setNerveJumpOrMove(al::LiveActor* actor, const RabbitGraphVertex* va,
                                const RabbitGraphVertex* vb, bool isMoveStart) {
    RabbitGraphEdge* edge = (RabbitGraphEdge*)al::tryFindEdgeStartVertex(va, vb);
    s32 val = edge->getValue();
    if ((val & 0xff) == 0) {
        al::validateClipping(this);
        al::Nerve* nerve;
        if (!isMoveStart)
            nerve = &NrvRabbit.Move;
        else
            nerve = &NrvRabbit.MoveStart;
        al::setNerve(actor, nerve);
    } else {
        al::invalidateClipping(this);
        if (val < 0x100)
            al::setNerve(actor, &NrvRabbit.JumpPath);
        else
            al::setNerve(actor, &NrvRabbit.Jump);
    }
}

void Rabbit::setNerveJumpOrMoveStart(al::LiveActor* actor, const RabbitGraphVertex* va,
                                     const RabbitGraphVertex* vb) {
    setNerveJumpOrMove(actor, va, vb, true);
}

void Rabbit::onMoveEndUpdateCurrentVertexAndNextNerve() {
    if (mNextVertex->getBool2()) {
        RabbitGraphVertex* selected;
        if (mNextVertex->getEdgeCount() <= 0) {
            selected = nullptr;
        } else {
            f32 maxDistance = 0.0f;
            selected = nullptr;
            s32 i = 0;
            do {
                RabbitGraphEdge* edge = toRabbitEdge(mNextVertex->getEdge(i));
                RabbitGraphVertex* nextVertex = edge->getVertex2();
                if (nextVertex != mCurrentVertex && edge->getVertex1() == mNextVertex) {
                    f32 distance = calcDistanceToPlayerBody(this, nextVertex->getPos());
                    if (distance > maxDistance) {
                        maxDistance = distance;
                        selected = nextVertex;
                    }
                }
                i++;
            } while (i < mNextVertex->getEdgeCount());
        }

        mCurrentVertex = mNextVertex;
        mNextVertex = selected;
        setNerveJumpOrMove(this, mCurrentVertex, mNextVertex, false);
        return;
    }

    RabbitGraphVertex* previousVertex = mCurrentVertex;
    mCurrentVertex = mNextVertex;
    mNextVertex = nullptr;

    RabbitGraphVertex* nextVertex = tryFindNextVertex();
    sead::Vector3f playerOffset = rs::getPlayerBodyPos(this);
    playerOffset -= al::getTrans(this);
    if (nextVertex != nullptr && playerOffset.length() < calcPlayerChaseDistance(this)) {
        mNextVertex = nextVertex;

        sead::Vector3f front;
        al::calcFrontDir(&front, this);
        sead::Vector3f direction = mNextVertex->getPos();
        direction -= al::getTrans(this);
        direction.y = 0.0f;

        if (!al::tryNormalizeOrZero(&direction) || al::calcAngleDegree(direction, front) <= 80.0f) {
            setNerveJumpOrMove(this, mCurrentVertex, mNextVertex, false);
        } else {
            al::Nerve* nerve = &NrvRabbit.Turn;
            if (mNextVertex == previousVertex)
                nerve = &NrvRabbit.TurnReverse;
            al::setNerve(this, nerve);
        }
        return;
    }

    al::validateClipping(this);
    if (al::isNerve(this, &NrvRabbit.Move) && isRunActionPlaying(this)) {
        al::setNerve(this, &NrvRabbit.Break);
        return;
    }

    al::Nerve* nerve = &NrvRabbit.Wait;
    if (mTiredTimer > 0)
        nerve = &NrvRabbit.WaitTired;
    al::setNerve(this, nerve);
}

RabbitGraphVertex* Rabbit::tryFindNextVertex() {
    if (mMoveType != 0) {
        if (mMoveType != 2) {
            if (mMoveType != 1)
                return nullptr;

            mDestinationVertex = toRabbitVertex(
                al::findNearestPosVertex(mGraph, rs::getPlayerBodyPos(this), -1.0f));
        } else {
            RabbitGraphVertex* selected = nullptr;
            f32 maxDistance = 0.0f;
            if (mDestinations.size() > 0) {
                for (s32 i = 0; i < mDestinations.size(); i++) {
                    RabbitGraphVertex* vertex = mDestinations[i];
                    f32 distance = calcDistanceToPlayerBody(this, vertex->getPos());
                    if (distance > maxDistance) {
                        maxDistance = distance;
                        selected = vertex;
                    }
                }
            } else {
                for (s32 i = 0; i < mGraph->getVertexCount(); i++) {
                    RabbitGraphVertex* vertex = toRabbitVertex(mGraph->getVertex(i));
                    if (vertex->getBool2())
                        continue;

                    f32 distance = calcDistanceToPlayerBody(this, vertex->getPos());
                    if (distance > maxDistance) {
                        maxDistance = distance;
                        selected = vertex;
                    }
                }
            }

            if (selected != nullptr)
                mDestinationVertex = selected;
        }
    } else {
        s32 edgeCount = mCurrentVertex->getEdgeCount();
        sead::FixedPtrArray<RabbitGraphVertex, 10> candidates;
        if (edgeCount > 0) {
            for (s32 i = 0; i != edgeCount; i++) {
                RabbitGraphEdge* edge = toRabbitEdge(mCurrentVertex->getEdge(i));
                if (edge->getVertex1() == mCurrentVertex && edge->getWeight() < 65536.0f)
                    candidates.pushBack(edge->getVertex2());
            }
        }

        s32 index = al::getRandom(candidates.size()) % candidates.size();
        if (u32(candidates.size()) > u32(index))
            return candidates(index);
        return nullptr;
    }

    if (mDestinationVertex != nullptr) {
        RabbitGraphVertex* destinationVertex = mDestinationVertex;
        sead::FixedObjArray<al::Graph::VertexInfo, 512> path;
        if (al::calcShortestPath(&path, mGraph, mCurrentVertex->getIndex(),
                                 destinationVertex->getIndex())) {
            RabbitGraphVertex* nextVertex = nullptr;
            for (s32 index = destinationVertex->getIndex();;) {
                if (index < 0)
                    break;

                al::Graph::VertexInfo* info = path(index);
                index = info->prevIndex;
                if (index < 0)
                    continue;

                if (info->weight < 65536.0f)
                    nextVertex = toRabbitVertex(info->vertex);
            }

            if (nextVertex != nullptr)
                return nextVertex;
        }
    }

    RabbitGraphVertex* selected = nullptr;
    f32 maxDistance = 0.0f;
    s32 lastEdgeIndex = mCurrentVertex->getEdgeCount() - 1;
    if (lastEdgeIndex < 0)
        return nullptr;

    for (s32 i = 0;; i++) {
        RabbitGraphEdge* edge = toRabbitEdge(mCurrentVertex->getEdge(i));
        if (edge->getVertex1() == mCurrentVertex) {
            sead::Vector3f playerOffset = rs::getPlayerBodyPos(this) - edge->getVertex2()->getPos();
            if (playerOffset.length() > maxDistance) {
                maxDistance = playerOffset.length();
                selected = edge->getVertex2();
            }
        }
        if (lastEdgeIndex == i)
            break;
    }

    return selected;
}

void Rabbit::fall(f32 velocity) {
    if (al::isOnGround(this, 0)) {
        al::addVelocityToDirection(this, -al::getCollidedGroundNormal(this), velocity);
        al::scaleVelocity(this, 0.5f);
        return;
    }
    al::addVelocityToGravity(this, velocity);
    al::scaleVelocity(this, 0.998f);
}

void Rabbit::reduceStamina() {
    if (mTiredTimer == 0) {
        mStamina = sead::Mathf::max(mStamina + -1.0f, 0.0f);
        if (mStamina == 0.0) {
            mIsTired = true;
            mTiredTimer = 300;
            mStamina = 1200.0f;
        }
    }
}

void Rabbit::trySetPoseGraphMoveDir(f32 delay) {
    if (mCurrentVertex == nullptr || mNextVertex == nullptr)
        return;

    sead::Vector3f position = mNextVertex->getPos();
    position -= al::getTrans(this);
    f32 length = sead::Mathf::clamp(position.length() / 500.0f, 0.0f, 1.0f);
    position.y = 0.0f;

    if (!al::tryNormalizeOrZero(&position))
        return;

    if (1.0f - length > 0.0f && mNextVertex->getEdgeCount() == 4) {
        al::Graph::Edge** edges = mNextVertex->getEdgeArray();
        RabbitGraphVertex* currentVertex = mCurrentVertex;
        RabbitGraphVertex* selected = nullptr;
        for (s32 i = 0; i < mNextVertex->getEdgeCount(); i++) {
            RabbitGraphVertex* v2 = toRabbitVertex(edges[i]->getVertex2());
            selected = v2 != mNextVertex && v2 != currentVertex ? v2 : selected;
        }
        sead::Vector3f nipon = selected->getPos();
        nipon -= mNextVertex->getPos();
        nipon.y = 0;
        if (al::tryNormalizeOrZero(&nipon)) {
            f32 fVar11 = (1.0f - length) * 0.5f;
            position = fVar11 * nipon + (1.0f - fVar11) * position;
        }
    }

    if (!al::isParallelDirection(position, sead::Vector3f::ey)) {
        sead::Quatf quat;
        al::makeQuatFrontUp(&quat, position, sead::Vector3f::ey);
        al::slerpQuat(al::getQuatPtr(this), al::getQuat(this), quat, delay);
    }
}

inline f32 getJumpSpeed(bool mIsRabbitGraphMoon, s32 mTiredTimer) {
    return !mIsRabbitGraphMoon ? 25.0f : mTiredTimer > 0 ? 12.0f : 20.0f;
}

f32 Rabbit::getMoveSpeed() const {
    if (al::isNerve(this, &NrvRabbit.Jump) || al::isNerve(this, &NrvRabbit.JumpPath))
        return getJumpSpeed(mIsRabbitGraphMoon, mTiredTimer);

    f32 speed = mTiredTimer > 0 ? 12.0f : mIsRabbitGraphMoon ? 20.0f : 25.0f;
    f32 moddifier = sead::Mathf::clamp(mMoveFrame / 30.0f, 0.0f, 1.0f);
    return speed * moddifier;
}

void Rabbit::exeReset() {
    if (al::updateNerveState(this))
        al::setNerve(this, &NrvRabbit.StandbyWait);
}

void Rabbit::exeStandby() {
    if (al::isFirstStep(this)) {
        if (!al::isNerve(this, &NrvRabbit.StandbyWait)) {
            al::startAction(this, "Rest");
        } else {
            mRandomWait = al::getRandom(0x1e0, 0xf0);
            al::tryStartActionIfNotPlaying(this, "Wait");
        }
    }
    fall(0.98f);
    if (mCurrentVertex->getBool())
        al::resetPosition(this, mCurrentVertex->getPos());
    sead::Vector3f position = rs::getPlayerBodyPos(this);
    position -= al::getTrans(this);
    f32 len = position.length();
    f32 uVar2 = rs::isPlayerHackTRex(this) ? 3000.0f : 1500.0f;
    if (len < uVar2) {
        al::setNerve(this, &NrvRabbit.Find);
    } else if (!mIsTired) {
        if (!al::isNerve(this, &NrvRabbit.StandbyWait)) {
            if (!al::isGreaterStep(this, 0xb4))
                return;
            al::setNerve(this, &NrvRabbit.StandbyWait);
        } else {
            if (!al::isGreaterStep(this, mRandomWait))
                return;
            al::setNerve(this, &NrvRabbit.StandbyRest);
        }
    } else {
        al::setNerve(this, &NrvRabbit.WaitTired);
    }
}

void Rabbit::exeFind() {
    if (al::isFirstStep(this))
        al::startAction(this, "Find");

    fall(0.98);
    if (mCurrentVertex->getBool())
        al::resetPosition(this, mCurrentVertex->getPos());
    al::turnToTarget(this, rs::getPlayerPos(this), 10.0f);

    if (al::isActionEnd(this)) {
        al::Nerve* nerve = &NrvRabbit.Wait;
        if (mTiredTimer > 0)
            nerve = &NrvRabbit.WaitTired;
        al::setNerve(this, nerve);
    }
}

void Rabbit::exeEndTired() {
    if (al::isFirstStep(this))
        al::startAction(this, "EndTired");
    fall(0.98f);
    if (al::isActionEnd(this)) {
        if (mNextVertex == nullptr)
            al::setNerve(this, &NrvRabbit.Wait);
        else
            setNerveJumpOrMove(this, mCurrentVertex, mNextVertex, false);
    }
}

void Rabbit::exeMove() {
    if (al::isFirstStep(this)) {
        if (al::isNerve(this, &NrvRabbit.Move)) {
            if (!al::isActionPlaying(this, "SwoonStart") &&
                !al::isActionPlaying(this, "RunTired") &&
                !al::isActionPlaying(this, "RunTiredSlow")) {
                const char* action = "RunFine";
                if (al::isNerve(this, &NrvRabbit.Move) && mTiredTimer > 0)
                    action = mIsSwoon ? "RunSwoon" : "RunFine";
                al::tryStartActionIfNotPlaying(this, action);
            }
        } else if (al::isNerve(this, &NrvRabbit.Jump)) {
            if (!al::isActionPlaying(this, "JumpStart") && !al::isActionPlaying(this, "JumpLoop"))
                al::startAction(this, "JumpStart");
            slerpQuatToWorldUpFront(this, 1.0f);
        }
    }

    if (al::isNerve(this, &NrvRabbit.Move)) {
        if (isSwoonStartEnd(this)) {
            const char* action = "RunFine";
            if (al::isNerve(this, &NrvRabbit.Move) && mTiredTimer > 0)
                action = mIsSwoon ? "RunSwoon" : "RunFine";
            al::tryStartActionIfNotPlaying(this, action);
        }
    } else if (al::isNerve(this, &NrvRabbit.Jump)) {
        if (al::isActionPlaying(this, "JumpStart") && al::isActionEnd(this))
            al::tryStartActionIfNotPlaying(this, "JumpLoop");
        else if (isSwoonStartEnd(this))
            al::startAction(this, "SwoonLoop");
    }

    reduceStamina();
    fall(0.01f);
    if (al::isOnGround(this, 0))
        slerpQuatToGround(this, 0.1f);
    trySetPoseGraphMoveDir(0.25f);

    sead::Vector3f direction = mNextVertex->getPos();
    direction -= al::getTrans(this);
    f32 distance = direction.length();

    if (distance - getMoveSpeed() <= 20.0f) {
        if (mNextVertex->getBool1() || !al::isNerve(this, &NrvRabbit.Jump)) {
            onMoveEndUpdateCurrentVertexAndNextNerve();
        } else if (mIsCaught) {
            al::Nerve* nerve = &NrvRabbit.CatchToGiveItem;
            if (mAppearItemId == 0x11)
                nerve = &NrvRabbit.CatchToGiveMoon;
            al::setNerve(this, nerve);
        } else {
            al::setNerve(this, &NrvRabbit.EndJump);
        }
        return;
    }

    if (al::tryNormalizeOrZero(&direction)) {
        sead::Vector3f moveDirection = direction;
        const sead::Vector3f& trans = al::getTrans(this);
        f32 speed = getMoveSpeed();
        sead::Vector3f nextTrans = speed * moveDirection + trans;
        al::setTrans(this, nextTrans);
    }
}

void Rabbit::exeWait() {
    if ((al::isFirstStep(this) && !al::isActionPlaying(this, "SwoonStart")) ||
        isSwoonStartEnd(this)) {
        if (al::isNerve(this, &NrvRabbit.WaitTired)) {
            al::startAction(this, mIsSwoon ? "WaitSwoon" : "WaitTired");
        } else if (al::isNerve(this, &NrvRabbit.Wait)) {
            mRandomWait = al::getRandom(0x1e0, 0xf0);
            al::startAction(this, "Wait");
        } else {
            al::startAction(this, "Rest");
        }
    }

    mStamina = sead::Mathf::min(mStamina + 0.05f, 1200.0f);
    fall(0.98f);
    if (al::isOnGround(this, 0))
        slerpQuatToGround(this, 0.1f);

    sead::Vector3f playerOffset = rs::getPlayerBodyPos(this);
    playerOffset -= al::getTrans(this);
    f32 playerDistance = playerOffset.length();
    if (playerDistance < calcPlayerChaseDistance(this)) {
        mNextVertex = tryFindNextVertex();
        if (mNextVertex != nullptr && mCurrentVertex != mDestinationVertex) {
            setNerveJumpOrMoveStart(this, mCurrentVertex, mNextVertex);
            return;
        }
    }

    if (al::isNerve(this, &NrvRabbit.Wait)) {
        if (playerDistance < 1500.0f && mTiredTimer == 0) {
            al::setNerve(this, &NrvRabbit.Provoke);
            return;
        }
        if (mIsTired) {
            al::setNerve(this, &NrvRabbit.WaitTired);
            return;
        }
    }

    if (mCurrentVertex->getBool())
        al::resetPosition(this, mCurrentVertex->getPos());

    if (al::isNerve(this, &NrvRabbit.Wait)) {
        if (al::isGreaterStep(this, mRandomWait))
            al::setNerve(this, &NrvRabbit.Rest);
    } else if (al::isNerve(this, &NrvRabbit.Rest)) {
        if (al::isGreaterStep(this, 180))
            al::setNerve(this, &NrvRabbit.Wait);
    } else if (al::isNerve(this, &NrvRabbit.WaitTired) && mTiredTimer == 0) {
        al::setNerve(this, &NrvRabbit.Wait);
    }
}

void Rabbit::exeProvoke() {
    if ((al::isFirstStep(this) && !al::isActionPlaying(this, "SwoonStart")) ||
        isSwoonStartEnd(this)) {
        al::startAction(this, "Provoke");
    }

    mStamina = sead::Mathf::min(mStamina + 0.05f, 1200.0f);
    fall(0.98f);
    if (al::isOnGround(this, 0))
        slerpQuatToGround(this, 0.1f);

    if (mIsTired) {
        al::setNerve(this, &NrvRabbit.WaitTired);
        return;
    }

    {
        sead::Vector3f direction = rs::getPlayerPos(this) - al::getTrans(this);
        if (al::tryNormalizeOrZero(&direction)) {
            sead::Vector3f up;
            al::calcUpDir(&up, this);
            if (!al::isParallelDirection(direction, up, 0.01f))
                al::turnToDirectionAxis(this, direction, up, 6.0f);
        }
    }

    sead::Vector3f playerOffset = rs::getPlayerBodyPos(this);
    playerOffset -= al::getTrans(this);
    f32 playerDistance = playerOffset.length();
    if (playerDistance < calcPlayerChaseDistance(this)) {
        mNextVertex = tryFindNextVertex();
        if (mNextVertex != nullptr) {
            setNerveJumpOrMoveStart(this, mCurrentVertex, mNextVertex);
            return;
        }
    } else if (playerDistance > 1500.0f) {
        al::Nerve* nerve = &NrvRabbit.Wait;
        if (mTiredTimer > 0)
            nerve = &NrvRabbit.WaitTired;
        al::setNerve(this, nerve);
        return;
    }

    sead::Vector3f front;
    sead::Quatf targetQuat;
    al::calcFrontDir(&front, this);
    al::makeQuatUpFront(&targetQuat, sead::Vector3f::ey, front);
    al::slerpQuat(al::getQuatPtr(this), al::getQuat(this), targetQuat, 0.25f);

    if (mCurrentVertex->getBool())
        al::resetPosition(this, mCurrentVertex->getPos());
}

void Rabbit::exeBreak() {
    if (al::isFirstStep(this))
        al::startAction(this, "Break");

    fall(0.98f);

    if (al::isActionEnd(this)) {
        al::Nerve* nerve = &NrvRabbit.Wait;
        if (mTiredTimer > 0)
            nerve = &NrvRabbit.WaitTired;
        al::setNerve(this, nerve);
    }
}

void Rabbit::exeTurn() {
    if (al::isFirstStep(this))
        al::startAction(this, mTiredTimer > 0 ? "TurnTired" : "Turn");

    fall(0.01f);
    trySetPoseGraphMoveDir(0.1f);

    if (al::isGreaterEqualStep(this, 15))
        setNerveJumpOrMove(this, mCurrentVertex, mNextVertex, false);
}

void Rabbit::exeTurnReverse() {
    if (al::isFirstStep(this)) {
        al::startAction(this, mTiredTimer > 0 ? "TurnTired" : "Turn");
        al::calcQuat(&mTurnStartQuat, this);

        sead::Vector3f diff = mNextVertex->getPos();
        sead::Vector3f frontDir;
        diff -= al::getTrans(this);
        al::tryNormalizeOrZero(&diff);
        al::calcFrontDir(&frontDir, this);
        mTurnAngle = al::calcAngleDegree(frontDir, diff);
        f32 cc = diff.z;
        if (frontDir.z * diff.x - cc * frontDir.x > 0.0f)  // mismatch here
            mTurnAngle = 360.0f - mTurnAngle;
    }
    fall(0.01f);

    const char* action = al::isActionPlaying(this, "Turn") ? "Turn" : "TurnTired";
    f32 fVar9 = al::calcNerveEaseInOutRate(this, al::getActionFrameMax(this, action));
    al::rotateQuatYDirDegree(al::getQuatPtr(this), mTurnStartQuat, -mTurnAngle * fVar9);

    if (al::isActionEnd(this))
        setNerveJumpOrMove(this, mCurrentVertex, mNextVertex, false);
}

void Rabbit::exeMoveStart() {
    if (al::isFirstStep(this))
        al::startAction(this, "RunStart");
    fall(0.01f);
    trySetPoseGraphMoveDir(0.1f);
    if (al::isActionEnd(this))
        al::setNerve(this, &NrvRabbit.Move);
}

void Rabbit::exeJumpPath() {
    if (al::isFirstStep(this)) {
        const sead::Vector3f& end = mNextVertex->getPos();
        const sead::Vector3f& start = mCurrentVertex->getPos();
        f32 height = sead::Mathf::clampMin(end.y - start.y, 0.0f) + 300.0f;
        al::ParabolicPath* path = mParabolicPath;
        {
            sead::Vector3f up = -al::getGravity(this);
            path->initFromUpVector(start, end, up, height);
        }
        mPathMoveDistance = 0.0f;
        al::tryStartActionIfNotPlaying(this, "JumpStart");
        slerpQuatToWorldUpFront(this, 1.0f);
    }

    reduceStamina();
    al::scaleVelocity(this, al::isOnGround(this, 0) ? 0.5f : 0.998f);

    if (al::isActionPlaying(this, "JumpStart") && al::isActionEnd(this))
        al::tryStartActionIfNotPlaying(this, "JumpLoop");
    else if (isSwoonStartEnd(this))
        al::tryStartActionIfNotPlaying(this, "SwoonLoop");

    mPathMoveDistance += getMoveSpeed();
    f32 rate =
        sead::Mathf::clamp(mPathMoveDistance / mParabolicPath->getTotalLength(32), 0.0f, 1.0f);

    sead::Vector3f prevTrans = al::getTrans(this);
    mParabolicPath->calcPosition(al::getTransPtr(this), rate);

    sead::Vector3f moveDir = al::getTrans(this);
    moveDir -= prevTrans;
    if (al::tryNormalizeOrZero(&moveDir)) {
        sead::Vector3f front;
        al::calcFrontDir(&front, this);
        f32 angle = al::calcAngleDegree(front, moveDir);
        if (moveDir.y < 0.0f)
            angle = -angle;
        mSpineAngle = mSpineAngle * 0.8f + angle * 0.2f;
        mSpineAngle = sead::Mathf::clamp(mSpineAngle, -45.0f, 45.0f);
    }

    if (mNextVertex->getBool()) {
        mParabolicPath->initFromUpVector(mCurrentVertex->getPos(), mNextVertex->getPos(),
                                         -al::getGravity(this), 300.0f);
    }

    if (rate >= 1.0f) {
        mSpineAngle = 0.0f;
        if (mIsCaught) {
            al::Nerve* nerve = &NrvRabbit.CatchToGiveItem;
            if (mAppearItemId == 0x11)
                nerve = &NrvRabbit.CatchToGiveMoon;
            al::setNerve(this, nerve);
        } else {
            al::setNerve(this, &NrvRabbit.EndJump);
        }
    } else {
        trySetPoseGraphMoveDir(0.25f);
    }
}

void Rabbit::exeEndJump() {
    if (al::isFirstStep(this)) {
        if (al::isActionPlaying(this, "JumpStart") || al::isActionPlaying(this, "JumpLoop"))
            al::startAction(this, "JumpEnd");
        else if (al::isActionPlaying(this, "SwoonStart") || al::isActionPlaying(this, "SwoonLoop"))
            al::startAction(this, "SwoonLand");
        al::startHitReaction(this, "着地");
        mCurrentVertex = mNextVertex;
        mNextVertex = nullptr;
    }
    mStamina = sead::Mathf::min(mStamina + 0.05f, 1200.0f);
    fall(0.98);
    if (mCurrentVertex->getBool())
        al::resetPosition(this, mCurrentVertex->getPos());
    if (al::isActionEnd(this)) {
        al::Nerve* nerve = &NrvRabbit.Wait;
        if (mTiredTimer > 0)
            nerve = &NrvRabbit.WaitTired;
        al::setNerve(this, nerve);
    }
}

void Rabbit::exeCatch() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Catch");
        al::setVelocityZero(this);
    }
    if (al::isActionEnd(this)) {
        al::Nerve* nerve = &NrvRabbit.GiveItem;
        if (al::isNerve(this, &NrvRabbit.CatchToGiveMoon))
            nerve = &NrvRabbit.GiveMoon;
        al::setNerve(this, nerve);
    }
}

void Rabbit::exeGiveMoon() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear");
        sead::Vector3f napa;
        sead::Quatf nipon;
        al::calcQuatFront(&napa, this);
        al::makeQuatUpFront(&nipon, sead::Vector3f::ey, napa);
        al::slerpQuat(al::getQuatPtr(this), al::getQuat(this), nipon, 1.0f);
        al::setVelocityZero(this);
    }
    if (al::isStep(this, 30) && mItemGenerator != nullptr && mItemGenerator->isShine()) {
        mItemGenerator->generate(al::getTrans(this) + sead::Vector3f(0.0f, 250.0f, 0.0f),
                                 al::getQuat(this));
        al::startHitReaction(this, "シャイン出現");
    }
    if (al::isActionEnd(this))
        al::setNerve(this, &NrvRabbit.Disappear);
}

void Rabbit::exeGiveItem() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear");
        sead::Vector3f napa;
        sead::Quatf nipon;
        al::calcQuatFront(&napa, this);
        al::makeQuatUpFront(&nipon, sead::Vector3f::ey, napa);
        al::slerpQuat(al::getQuatPtr(this), al::getQuat(this), nipon, 1.0f);
        al::setVelocityZero(this);
    }

    if (al::isLessStep(this, 0x1e))
        return;

    if (mItemCount != mAppearItemNum) {
        if (rs::tryAppearMultiCoinFromObj(this, mRewardReceiverSensor, al::getNerveStep(this),
                                          200.0f)) {
            const char* reaction = "アイテム出現";
            if (mAppearItemId == 0xc)
                reaction = "ライフアップアイテム出現";
            if (mAppearItemId == 0)
                reaction = "コイン出現";
            al::startHitReaction(this, reaction);
            mItemCount++;
        }
        return;
    }

    if (al::isActionEnd(this))
        al::setNerve(this, &NrvRabbit.Disappear);
}

void Rabbit::exeDisappear() {
    if (al::isFirstStep(this))
        al::startAction(this, "Disappear");
    if (al::isActionEnd(this)) {
        al::onStageSwitch(this, "SwitchCatchOn");
        kill();
    }
}

void Rabbit::onMoveEnd() {
    if (!al::isActionPlaying(this, "RunTired") && !al::isActionPlaying(this, "RunTiredSlow"))
        al::setActionFrameRate(this, 1.0f);
}
