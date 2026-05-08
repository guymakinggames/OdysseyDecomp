#include "Enemy/Pukupuku.h"

#include "Library/Area/AreaObj.h"
#include "Library/Area/AreaObjUtil.h"
#include "Library/Base/StringUtil.h"
#include "Library/Collision/CollisionParts.h"
#include "Library/Collision/CollisionPartsKeeperUtil.h"
#include "Library/Effect/EffectSystemInfo.h"
#include "Library/Fluid/JointRippleGenerator.h"
#include "Library/Item/ItemUtil.h"
#include "Library/Joint/JointControllerKeeper.h"
#include "Library/Joint/JointSpringController.h"
#include "Library/LiveActor/ActorActionFunction.h"
#include "Library/LiveActor/ActorAnimFunction.h"
#include "Library/LiveActor/ActorClippingFunction.h"
#include "Library/LiveActor/ActorCollisionFunction.h"
#include "Library/LiveActor/ActorFlagFunction.h"
#include "Library/LiveActor/ActorInitUtil.h"
#include "Library/LiveActor/ActorModelFunction.h"
#include "Library/LiveActor/ActorMovementFunction.h"
#include "Library/LiveActor/ActorPoseUtil.h"
#include "Library/LiveActor/ActorSceneFunction.h"
#include "Library/LiveActor/ActorSensorUtil.h"
#include "Library/LiveActor/LiveActorFunction.h"
#include "Library/Math/MathUtil.h"
#include "Library/Matrix/MatrixUtil.h"
#include "Library/Movement/EnemyStateBlowDown.h"
#include "Library/Nature/NatureUtil.h"
#include "Library/Nature/WaterSurfaceFinder.h"
#include "Library/Nerve/NerveSetupUtil.h"
#include "Library/Nerve/NerveUtil.h"
#include "Library/Placement/PlacementFunction.h"
#include "Library/Rail/RailUtil.h"
#include "Library/Shadow/ActorShadowUtil.h"

#include "Enemy/EnemyStateReviveInsideScreen.h"
#include "Enemy/EnemyStateSwoon.h"
#include "Enemy/HackerDepthShadowMapCtrl.h"
#include "Npc/SphinxQuizRouteKillExecutor.h"
#include "Player/HackerStateNormalJump.h"
#include "Player/PlayerHackStartShaderCtrl.h"
#include "Util/DemoUtil.h"
#include "Util/Hack.h"
#include "Util/InputInterruptTutorialUtil.h"
#include "Util/ItemUtil.h"
#include "Util/ObjUtil.h"
#include "Util/SensorMsgFunction.h"

namespace al {
bool isExistPrePassLight(const LiveActor* actor, const char* name);
bool isActivePrePassLight(const LiveActor* actor, const char* name);
void killPrePassLight(LiveActor* actor, const char* name, s32 step);
void appearPrePassLight(LiveActor* actor, const char* name, s32 step);
f32 getPrePassSpotLightCurrentLength(const LiveActor* actor, const char* name);
void setPrePassSpotLightDegree(LiveActor* actor, const char* name, f32 degree);
}  // namespace al

namespace {
NERVE_IMPL(Pukupuku, Wait)
NERVE_IMPL(Pukupuku, Swoon)
NERVE_IMPL(Pukupuku, Revive)
NERVE_IMPL(Pukupuku, BlowDown)
NERVE_IMPL_(Pukupuku, BlowDownFromCapture, BlowDown)
NERVE_IMPL_(Pukupuku, BlowDownWithoutMsg, BlowDown)
NERVE_IMPL(Pukupuku, CaptureJumpGround)
NERVE_IMPL(Pukupuku, CaptureWait)
NERVE_IMPL_(Pukupuku, CaptureWaitTurnStart, CaptureWait)
NERVE_IMPL_(Pukupuku, CaptureWaitTurn, CaptureWait)
NERVE_IMPL(Pukupuku, CaptureSwimStart)
NERVE_IMPL(Pukupuku, CaptureSwim)
NERVE_IMPL_(Pukupuku, CaptureSwimDash, CaptureSwim)
NERVE_IMPL(Pukupuku, CaptureReactionWall)
NERVE_IMPL(Pukupuku, CaptureAttack)
NERVE_IMPL_(Pukupuku, CaptureRollingL, CaptureRolling)
NERVE_IMPL_(Pukupuku, CaptureRollingR, CaptureRolling)
NERVE_IMPL(Pukupuku, WaitTurnToRailDir)
NERVE_IMPL(Pukupuku, WaitRollingRail)
NERVE_IMPL(Pukupuku, Reaction)
NERVE_IMPL(Pukupuku, Trample)
NERVE_IMPL(Pukupuku, CaptureStart)
NERVE_IMPL(Pukupuku, CaptureStartEnd)
NERVE_IMPL(Pukupuku, DemoWaitToRevive)
NERVE_IMPL_(Pukupuku, CaptureWaitAir, CaptureWait)
NERVE_IMPL_(Pukupuku, CaptureJumpOut, CaptureWait)
NERVE_IMPL(Pukupuku, CaptureWaitGround)
NERVE_IMPL(Pukupuku, CaptureLandGround)

NERVES_MAKE_STRUCT(Pukupuku, Wait, Swoon, Revive, BlowDown, BlowDownFromCapture, BlowDownWithoutMsg,
                   CaptureJumpGround, CaptureWait, CaptureWaitTurnStart, CaptureWaitTurn,
                   CaptureSwimStart, CaptureSwim, CaptureSwimDash, CaptureReactionWall,
                   CaptureAttack, CaptureRollingL, CaptureRollingR, WaitTurnToRailDir,
                   WaitRollingRail, Reaction, Trample, CaptureStart, CaptureStartEnd,
                   DemoWaitToRevive, CaptureWaitAir, CaptureJumpOut, CaptureWaitGround,
                   CaptureLandGround)

class AreaObjFilterWaterIgnore : public al::AreaObjFilterBase {
    bool isValidArea(al::AreaObj* areaObj) const override { return al::isWaterAreaIgnore(areaObj); }
};

class AreaObjFilterWater : public al::AreaObjFilterBase {
    bool isValidArea(al::AreaObj* areaObj) const override {
        return !al::isWaterAreaIgnore(areaObj);
    }
};

bool isHackInputActive(const IUsePlayerHack* hack) {
    return rs::isHoldHackJump(hack) || rs::isHoldHackAction(hack) ||
           rs::getHackMoveStickRaw(hack).length() > 0.1f;
}

const char* getPukupukuDashAction(const al::WaterSurfaceFinder* finder, const IUsePlayerHack* hack) {
    if (finder->isFoundSurface())
        return rs::isTriggerHackSwingLeftHand(hack) ? "DashLSurface" : "DashRSurface";

    return rs::isTriggerHackSwingLeftHand(hack) ? "DashLWater" : "DashRWater";
}

const char* getPukupukuSwimAction(const al::WaterSurfaceFinder* finder) {
    return finder->isFoundSurface() ? "SwimSurface" : "SwimWater";
}

const char* getPukupukuWaitAction(const al::WaterSurfaceFinder* finder) {
    return finder->isFoundSurface() ? "SwimWaitSurface" : "SwimWaitWaterHack";
}

void updateWaterSurfaceMtx(sead::Matrix34f* mtx, const al::LiveActor* actor,
                           const al::WaterSurfaceFinder* finder) {
    if (finder->isFoundSurface()) {
        sead::Vector3f frontDir;
        al::calcFrontDir(&frontDir, actor);
        al::makeMtxUpFrontPos(mtx, finder->getSurfaceNormal(), frontDir,
                              finder->getSurfacePosition());
    } else {
        mtx->setTranslation(al::getTrans(actor));
    }
}

void decayRootRotX(sead::Vector3f* rotator) {
    f32 step = 2.0f;
    f32 value = -rotator->x;
    if (rotator->x > 0.0f) {
        step = -2.0f;
        value = rotator->x;
    }

    rotator->x = 2.0f > value ? 0.0f : rotator->x + step;
}
}  // namespace

static sead::Vector3f g_7101e62d50 = {0.0f, 50.0f, 50.0f};
static al::EnemyStateBlowDownParam g_7101e62d10 = {"BlowDown", 8.0f, 13.0f, 0.5f, 1.0f, 120, true};
static al::EnemyStateBlowDownParam g_7101e62d30 = {"BlowDown", 16.0f, 32.0f, 1.0f,
                                                   0.95f,      120,   true};
static PlayerHackStartShaderParam g_7101e62d5c = {true, -1.0f, 10, 20};

bool FUN_7100175f24(Pukupuku* pukupuku);
void FUN_710017605c(f32 rate, Pukupuku* pukupuku);
bool FUN_7100177c74(sead::Vector3f* out, al::LiveActor* actor);
__attribute__((noinline)) void FUN_7100178da4(f32 accel, Pukupuku* pukupuku);
void FUN_710017b094(Pukupuku* actor, const sead::Vector3f& dir);

const struct {
    f32 stability;
    f32 friction;
    f32 limitDegree;
} g_71018a3dac[] = {
    {0.5f, 0.1f, 45.0f}, {0.5f, 0.5f, 45.0f}, {0.3f, 0.2f, 15.0f}, {0.3f, 0.2f, 15.0f}};

const char* const g_7101ca5700[] = {"Tail1", "Tail2", "WingLeft", "WingRight"};

Pukupuku::Pukupuku(const char* name) : LiveActor(name) {}

// NON_MATCHING
void Pukupuku::init(const al::ActorInitInfo& info) {
    s32 lightType = 1;
    al::tryGetArg(&lightType, info, "LightType");
    const char* suffix;
    switch (lightType) {
    case 0:
        suffix = "LightLow";
        break;
    case 2:
        suffix = "LightMiddle";
        break;
    default:
        suffix = "LightHigh";
        break;
    }
    al::initActorWithArchiveName(this, info, "Pukupuku", suffix);

    bool isPukupukuSnow = al::isObjectName(info, "PukupukuSnow");
    mIsPukupukuSnow = isPukupukuSnow;

    al::startVisAnimAndSetFrameAndStop(this, "CapOnOff", 0.0f);
    const char* mtpAnimName = "CapOnOffSnow";
    if (!isPukupukuSnow)
        mtpAnimName = "CapOnOff";
    al::startMtpAnimAndSetFrameAndStop(this, mtpAnimName, 0.0f);

    const char* capName = "Snow";
    if (!mIsPukupukuSnow)
        capName = nullptr;
    mCapTargetInfo = rs::createCapTargetInfo(this, capName);
    mWaterSurfaceFinder = new al::WaterSurfaceFinder(this);

    al::initNerve(this, &NrvPukupuku.Wait, 6);

    al::tryGetArg(&mMoveType, info, "MoveType");

    if (al::isExistRail(info, "Rail")) {
        al::setSyncRailToNearestPos(this);
        mRailPointNo = al::getRailPointNo(this);
    }

    EnemyStateSwoonInitParam enemyStateSwoonInitParam = {
        "SwoonStart", "SwoonLoop", "SwoonEnd", nullptr, "SwoonStartFall", "SwoonStartLand"};
    mEnemyStateSwoon = new EnemyStateSwoon(this, "SwoonStart", "Swoon", "SwoonEnd", false, true);
    mEnemyStateSwoon->initParams(enemyStateSwoonInitParam);
    al::initNerveState(this, mEnemyStateSwoon, &NrvPukupuku.Swoon, "気絶");
    mEnemyStateReviveInsideScreen = new EnemyStateReviveInsideScreen(this);
    al::initNerveState(this, mEnemyStateReviveInsideScreen, &NrvPukupuku.Revive, "画面内復活");
    mEnemyStateBlowDown = new al::EnemyStateBlowDown(this, &g_7101e62d10, "吹き飛び状態");
    al::initNerveState(this, mEnemyStateBlowDown, &NrvPukupuku.BlowDown, "吹き飛び");
    al::addNerveState(this, mEnemyStateBlowDown, &NrvPukupuku.BlowDownFromCapture,
                      "吹き飛び[憑依中]");
    al::addNerveState(this, mEnemyStateBlowDown, &NrvPukupuku.BlowDownWithoutMsg,
                      "吹き飛び[メッセージなし死亡]");
    mHackerStateNormalJump = new HackerStateNormalJump(this, &mPlayerHack, "JumpGround", nullptr);
    mHackerStateNormalJump->set_38({15.0f, 30.0f, 2.0f});
    mHackerStateNormalJump->set_48({8.0f, 8.0f});
    al::initNerveState(this, mHackerStateNormalJump, &NrvPukupuku.CaptureJumpGround,
                       "地上飛び跳ね");

    al::initJointControllerKeeper(this, 7);
    al::initJointLocalRotator(this, &joinRotator, "AllRoot");
    mJointSpringControllers.allocBuffer(4, nullptr);
    for (s32 i = 0; i < 4; i++) {
        al::JointSpringController* jointSpringController =
            al::initJointSpringController(this, g_7101ca5700[i]);
        jointSpringController->setStability(g_71018a3dac[i].stability);
        jointSpringController->setFriction(g_71018a3dac[i].friction);
        jointSpringController->setLimitDegree(g_71018a3dac[i].limitDegree);
        al::StringTmp<128>{"ダイナミクス[%s]", g_7101ca5700[i]}.cstr();
        mJointSpringControllers.pushBack(jointSpringController);
    }

    al::setEffectFollowMtxPtr(this, "SwimSurfaceTrace", &mSwimSurfaceTraceEffectFollowMtx);
    al::setEffectFollowMtxPtr(this, "WaterAreaIn", &mWaterAreaIn);
    al::setEffectFollowMtxPtr(this, "WaterAreaOut", &mWaterAreaOutEffectFollowMtx);
    al::setEffectNamedMtxPtr(this, "WaterSurface", &mWaterSurface);

    al::offCollide(this);

    al::makeMtxQuatPos(&mSpawnPosition, al::getQuat(this), al::getTrans(this));

    al::invalidateDepthShadowMap(this);
    _2b4 = al::getShadowMaskDropLength(this, "シャドウマスク");
    al::setShadowMaskDropLength(this, 100.0f, "シャドウマスク");

    mJointRippleGenerator = new al::JointRippleGenerator(this);
    mJointRippleGenerator->set({120.0f, 0.0f, 0.0f}, "Tail2", 0.15f, 110.0f, 2.0f, 250.0f);

    mPlayerHackStartShaderCtrl = new PlayerHackStartShaderCtrl(this, &g_7101e62d5c);
    mHackerDepthShadowMapCtrl = new HackerDepthShadowMapCtrl(this, "Ground", 100.0f, 0.3f, 0.5f);

    makeActorAlive();
}

void Pukupuku::initAfterPlacement() {
    rs::tryRegisterSphinxQuizRouteKillSensorAfterPlacement(al::getHitSensor(this, "Body"));
}

bool Pukupuku::isNerveInWater() const {
    return al::isNerve(this, &NrvPukupuku.CaptureWait) ||
           al::isNerve(this, &NrvPukupuku.CaptureWaitTurnStart) ||
           al::isNerve(this, &NrvPukupuku.CaptureWaitTurn) ||
           al::isNerve(this, &NrvPukupuku.CaptureSwimStart) ||
           al::isNerve(this, &NrvPukupuku.CaptureSwim) ||
           al::isNerve(this, &NrvPukupuku.CaptureSwimDash) ||
           al::isNerve(this, &NrvPukupuku.CaptureReactionWall) ||
           al::isNerve(this, &NrvPukupuku.CaptureAttack) ||
           al::isNerve(this, &NrvPukupuku.CaptureRollingL) ||
           al::isNerve(this, &NrvPukupuku.CaptureRollingR) ||
           (al::isNerve(this, &NrvPukupuku.Swoon) && _19d) ||
           al::isNerve(this, &NrvPukupuku.Wait) ||
           al::isNerve(this, &NrvPukupuku.WaitTurnToRailDir) ||
           al::isNerve(this, &NrvPukupuku.WaitRollingRail) ||
           al::isNerve(this, &NrvPukupuku.Reaction) || al::isNerve(this, &NrvPukupuku.Trample) ||
           al::isNerve(this, &NrvPukupuku.BlowDown) ||
           al::isNerve(this, &NrvPukupuku.BlowDownFromCapture) ||
           al::isNerve(this, &NrvPukupuku.BlowDownWithoutMsg);
}

void Pukupuku::attackSensor(al::HitSensor* self, al::HitSensor* other) {
    if (!FUN_7100175f24(this) || al::isNerve(this, &NrvPukupuku.CaptureStart) ||
        al::isNerve(this, &NrvPukupuku.CaptureStartEnd)) {
        if (al::isNerve(this, &NrvPukupuku.Swoon) ||
            al::isNerve(this, &NrvPukupuku.Trample)) {
            if (!al::isSensorEnemyBody(self))
                return;

            al::sendMsgPushAndKillVelocityToTarget(this, self, other);
            if (al::isNerve(this, &NrvPukupuku.Swoon) && al::isLessStep(this, 20))
                return;

            rs::sendMsgPushToPlayer(other, self);
            return;
        }

        if (al::isNerve(this, &NrvPukupuku.Wait) ||
            al::isNerve(this, &NrvPukupuku.Reaction) ||
            al::isNerve(this, &NrvPukupuku.WaitTurnToRailDir)) {
            if (al::isSensorName(self, "CheckPukupuku") &&
                rs::sendMsgIsExistPukupuku(other, self)) {
                sead::Vector3f diff = al::getSensorPos(other) - al::getSensorPos(self);
                diff.y = 0.0f;
                if (diff.length() < 50.0f) {
                    f32 distance = al::calcDistanceV(sead::Vector3f::ey, self, other);
                    if (distance < _2b8)
                        _2b8 = distance;
                    _2bc = true;
                }
            }

            if (al::isSensorName(self, "Attack")) {
                if (al::sendMsgEnemyAttack(other, self)) {
                    al::setNerve(this, &NrvPukupuku.Trample);
                } else {
                    al::sendMsgPushAndKillVelocityToTarget(this, self, other);
                    rs::sendMsgPushToPlayer(other, self);
                }
            }
        }
        return;
    }

    if (al::isSensorEnemyBody(self) && rs::sendMsgHackerNoReaction(mPlayerHack, other, self))
        return;

    if (al::isSensorName(self, "Attack") && al::isNerve(this, &NrvPukupuku.CaptureAttack) &&
        al::getVelocity(this).length() > 10.0f &&
        (rs::sendMsgHackAttackMapObj(other, self) || rs::sendMsgHackAttack(other, self) ||
         rs::sendMsgBreakPartsBreak(other, self)))
        return;

    if (al::isSensorName(self, "AttackJumpGround") &&
        (al::isNerve(this, &NrvPukupuku.CaptureJumpGround) ||
         al::isNerve(this, &NrvPukupuku.CaptureJumpOut)) &&
        al::isGreaterStep(this, 20) && al::getVelocity(this).y < -1.0f &&
        rs::trySendMsgPlayerReflectOrTrample(this, self, other)) {
        FUN_710017605c(1.0f, this);
        mHackerStateNormalJump->set_38(15.0f, 30.0f, 2.0f);
        al::setNerve(this, &NrvPukupuku.CaptureJumpGround);
        return;
    }

    if (al::isSensorName(self, "Attack")) {
        if (rs::sendMsgHackAttackKick(other, self))
            return;

        al::sendMsgPushAndKillVelocityToTarget(this, self, other);
    }

    if (al::isNerve(this, &NrvPukupuku.CaptureRollingL) ||
        al::isNerve(this, &NrvPukupuku.CaptureRollingR) ||
        al::isNerve(this, &NrvPukupuku.CaptureSwimDash) ||
        al::isNerve(this, &NrvPukupuku.CaptureReactionWall)) {
        al::Triangle triangle;
        sead::Vector3f hitPos;
        sead::Vector3f arrow = al::getSensorPos(other) - al::getSensorPos(self);
        if (alCollisionUtil::getFirstPolyOnArrow(this, &hitPos, &triangle, al::getSensorPos(self),
                                                 arrow, nullptr, nullptr)) {
            const al::CollisionParts* parts = triangle.getCollisionParts();
            if (parts->getConnectedHost() != al::getSensorHost(other) &&
                (hitPos - al::getSensorPos(other)).length() >= al::getSensorRadius(other))
                return;
        }

        if (al::isNerve(this, &NrvPukupuku.CaptureRollingL) ||
            al::isNerve(this, &NrvPukupuku.CaptureRollingR)) {
            if (al::isSensorName(self, "AttackRolling")) {
                if (rs::sendMsgHackerNoReaction(mPlayerHack, other, self))
                    return;

                if (rs::sendMsgHackAttackMapObj(other, self) ||
                    rs::sendMsgHackAttack(other, self) ||
                    rs::sendMsgPukupukuRollingAttack(other, self)) {
                    al::startHitReaction(this, "アタック");
                    return;
                }
            }
        } else if (al::isNerve(this, &NrvPukupuku.CaptureSwimDash)) {
            if (al::isSensorName(self, "Dash") && rs::sendMsgPukupukuDash(other, self)) {
                al::setNerve(this, &NrvPukupuku.CaptureReactionWall);
                return;
            }
        } else if (al::isLessStep(this, 5) && al::isSensorName(self, "Dash") &&
                   rs::sendMsgPukupukuDash(other, self)) {
            return;
        }
    }

    if (al::isSensorName(self, "Kiss") && _2dc == 60)
        rs::sendMsgPukupukuKiss(other, self);
}

bool FUN_7100175f24(Pukupuku* pukupuku) {
    return al::isNerve(pukupuku, &NrvPukupuku.CaptureStart) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureStartEnd) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureWait) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureWaitTurnStart) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureWaitTurn) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureSwimStart) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureSwim) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureSwimDash) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureReactionWall) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureWaitAir) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureJumpOut) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureWaitGround) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureJumpGround) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureLandGround) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureAttack) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureRollingL) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureRollingR);
}

void FUN_710017605c(f32 rate, Pukupuku* pukupuku) {
    sead::Vector3f frontDir;
    al::calcFrontDir(&frontDir, pukupuku);

    sead::Quatf quat;
    if (al::isParallelDirection(sead::Vector3f::ey, frontDir))
        al::makeQuatUpNoSupport(&quat, sead::Vector3f::ey);
    else
        al::makeQuatUpFront(&quat, sead::Vector3f::ey, frontDir);

    al::slerpQuat(al::getQuatPtr(pukupuku), al::getQuat(pukupuku), quat, rate);
}

bool Pukupuku::receiveMsg(const al::SensorMsg* message, al::HitSensor* other,
                          al::HitSensor* self) {
    if (!al::isSensorEnemyBody(self))
        return false;

    if (rs::isMsgSphinxQuizRouteKill(message)) {
        if (al::isNerve(this, &NrvPukupuku.DemoWaitToRevive) ||
            al::isNerve(this, &NrvPukupuku.BlowDown))
            return false;

        al::addDemoActor(this);
        al::tryKillEmitterAndParticleAll(this);
        al::tryStartAction(this, "DemoWaitToRevive");
        al::startHitReaction(this, "死亡");
        al::hideModelIfShow(this);
        al::setNerve(this, &NrvPukupuku.DemoWaitToRevive);
        return true;
    }

    if (rs::tryReceiveMsgInitCapTargetAndSetCapTargetInfo(message, mCapTargetInfo))
        return true;

    bool disregard = al::isMsgPlayerDisregard(message) ||
                     rs::isMsgPlayerDisregardHomingAttack(message) ||
                     rs::isMsgPlayerDisregardTargetMarker(message);
    if ((!al::isSensorName(self, "Body") || al::isNerve(this, &NrvPukupuku.BlowDownFromCapture) ||
         al::isNerve(this, &NrvPukupuku.BlowDownWithoutMsg) ||
         al::isNerve(this, &NrvPukupuku.BlowDown) ||
         al::isNerve(this, &NrvPukupuku.DemoWaitToRevive)) &&
        disregard)
        return true;

    if (rs::isMsgHackMarioDead(message) || rs::isMsgHackMarioCheckpointFlagWarp(message)) {
        if (rs::isMsgHackMarioCheckpointFlagWarp(message)) {
            al::hideSilhouetteModelIfShow(this);
            if (!al::isVisAnimPlaying(this, "CapOnOff") ||
                al::getVisAnimFrame(this) != 0.0f) {
                const char* mtpAnimName = mIsPukupukuSnow ? "CapOnOffSnow" : "CapOnOff";
                al::startVisAnimAndSetFrameAndStop(this, "CapOnOff", 0.0f);
                al::startMtpAnimAndSetFrameAndStop(this, mtpAnimName, 0.0f);
            }
        }

        endCapture();
        revive(2);
        return true;
    }

    if (al::isMsgGoalKill(message)) {
        if (FUN_7100175f24(this))
            endCapture();

        revive(2);
        return true;
    }

    if (rs::isMsgKillByHomeDemo(message) || rs::isMsgHackMarioDemo(message)) {
        al::tryKillEmitterAndParticleAll(this);
        if (rs::isMsgHackMarioDemo(message))
            endCapture();

        revive(0);
        return true;
    }

    if (rs::isMsgHackMarioInWater(message))
        return true;

    if (al::isNerve(this, &NrvPukupuku.BlowDownFromCapture) ||
        al::isNerve(this, &NrvPukupuku.BlowDownWithoutMsg) ||
        al::isNerve(this, &NrvPukupuku.Revive) ||
        al::isNerve(this, &NrvPukupuku.DemoWaitToRevive))
        return rs::isMsgCapCancelLockOn(message);

    if (!FUN_7100175f24(this) &&
        rs::tryReceiveMsgNpcScareByEnemyIgnoreTargetHack(message, mCapTargetInfo))
        return true;

    bool isEnemyWait = al::isNerve(this, &NrvPukupuku.Wait) ||
                       al::isNerve(this, &NrvPukupuku.Reaction) ||
                       al::isNerve(this, &NrvPukupuku.WaitTurnToRailDir) ||
                       al::isNerve(this, &NrvPukupuku.Trample);

    if (!isEnemyWait) {
        if (al::isNerve(this, &NrvPukupuku.Swoon)) {
            if (rs::isMsgCapCancelLockOn(message) ||
                mEnemyStateSwoon->tryReceiveMsgEnableLockOn(message) ||
                mEnemyStateSwoon->tryReceiveMsgEndSwoon(message))
                return true;

            if (mEnemyStateSwoon->tryReceiveMsgStartHack(message)) {
                mPlayerHack = rs::startHack(self, other, nullptr);
                startCapture();
                al::setNerve(this, &NrvPukupuku.CaptureStart);
                return true;
            }

            if (rs::isMsgBlowDown(message) || rs::isMsgUtsuboAttack(message)) {
                rs::requestHitReactionToAttacker(message, self, other);
                mEnemyStateBlowDown->start(other, self);
                mEnemyStateBlowDown->setParam(mWaterSurfaceFinder->isFoundSurface() ?
                                                  &g_7101e62d30 :
                                                  &g_7101e62d10);
                rs::setAppearItemFactorAndOffsetByMsg(this, message, other);
                al::setNerve(this, &NrvPukupuku.BlowDown);
                return true;
            }

            if (al::tryReceiveMsgPushAndAddVelocity(this, message, other, self, 3.0f))
                return true;

            if (al::isMsgPlayerTrampleReflect(message)) {
                if (al::getVelocity(al::getSensorHost(other)).y >= 0.0f)
                    return false;

                rs::requestHitReactionToAttacker(message, self, other);
                return true;
            }

            if (rs::isMsgPlayerAndCapObjHipDropAll(message)) {
                rs::requestHitReactionToAttacker(message, self, other);
                rs::setAppearItemFactorAndOffsetByMsg(this, message, other);
                al::setNerve(this, &NrvPukupuku.BlowDownFromCapture);
                return true;
            }

            return false;
        }

        if (al::isNerve(this, &NrvPukupuku.CaptureStart) ||
            al::isNerve(this, &NrvPukupuku.CaptureStartEnd) || !FUN_7100175f24(this))
            return false;

        if (rs::isMsgEnableMapCheckPointWarp(message)) {
            if (al::isNerve(this, &NrvPukupuku.CaptureWaitGround) ||
                al::isNerve(this, &NrvPukupuku.CaptureJumpGround) ||
                al::isNerve(this, &NrvPukupuku.CaptureLandGround))
                return rs::isMsgEnableMapCheckPointWarpCollidedGround(message, this);

            return isNerveInWater();
        }

        if (rs::isMsgWaterRoadIn(message)) {
            sead::Vector3f dir = al::getSensorPos(self) - al::getSensorPos(other);
            if (al::tryNormalizeOrZero(&dir))
                al::addVelocity(this, dir * 15.0f);
            return true;
        }

        if (al::isMsgString(message))
            return al::isEqualString(al::getMsgString(message), "IsHackPukupuku");

        if (rs::isMsgHackerDamageAndCancel(message)) {
            if (rs::isActiveHackStartDemo(mPlayerHack))
                rs::endHackStartDemo(mPlayerHack, this);

            if (!rs::requestDamage(mPlayerHack))
                return false;

            rs::requestHitReactionToAttacker(message, self, other);
            return true;
        }

        if (rs::isMsgHackSyncDamageVisibility(message)) {
            rs::syncDamageVisibility(this, mPlayerHack);
            return true;
        }

        if (rs::tryReceiveMsgPushToPlayerAndAddVelocity(this, message, other, self, 3.0f))
            return true;

        if (rs::isMsgRequestPlayerSpinJump(message)) {
            f32 power;
            rs::tryGetRequestPlayerSpinJumpInfo(&power, message);
            mHackerStateNormalJump->set_38(50.0f, 60.0f, 2.0f);
            al::setNerve(this, &NrvPukupuku.CaptureJumpGround);
            return true;
        }

        if (rs::isMsgCancelHackByDokan(message)) {
            endCapture();
            revive(2);
            return true;
        }

        if (rs::isMsgCancelHack(message)) {
            endCapture();
            if (al::isNerve(this, &NrvPukupuku.CaptureWaitGround) ||
                al::isNerve(this, &NrvPukupuku.CaptureJumpGround) ||
                al::isNerve(this, &NrvPukupuku.CaptureLandGround)) {
                revive(2);
                return true;
            }

            al::setVelocityZero(this);
            al::onCollide(this);
            al::invalidateClipping(this);
            _19d = isNerveInWater();
            al::setNerve(this, &NrvPukupuku.Swoon);
            return true;
        }

        return false;
    }

    if (al::isSensorName(self, "Body") && rs::isMsgIsExistPukupuku(message))
        return true;

    if (al::isMsgPlayerTrampleReflect(message)) {
        if (al::isNerve(this, &NrvPukupuku.Trample) && al::isLessStep(this, 10))
            return false;

        rs::requestHitReactionToAttacker(message, self, other);
        al::setNerve(this, &NrvPukupuku.Trample);
        return true;
    }

    if (rs::isMsgPlayerAndCapObjHipDropAll(message)) {
        rs::requestHitReactionToAttacker(message, self, other);
        rs::setAppearItemFactorAndOffsetByMsg(this, message, other);
        al::setNerve(this, &NrvPukupuku.BlowDownFromCapture);
        return true;
    }

    if (rs::isMsgCapEnableLockOn(message) || rs::isMsgCapCancelLockOn(message))
        return true;

    if (rs::isMsgStartHack(message)) {
        mPlayerHack = rs::startHack(self, other, nullptr);
        startCapture();
        al::setNerve(this, &NrvPukupuku.CaptureStart);
        return true;
    }

    if (rs::isMsgBlowDown(message) || rs::isMsgUtsuboAttack(message)) {
        rs::requestHitReactionToAttacker(message, self, other);
        mEnemyStateBlowDown->start(other, self);
        mEnemyStateBlowDown->setParam(mWaterSurfaceFinder->isFoundSurface() ? &g_7101e62d30 :
                                                                          &g_7101e62d10);
        rs::setAppearItemFactorAndOffsetByMsg(this, message, other);
        al::setNerve(this, &NrvPukupuku.BlowDown);
        return true;
    }

    return false;
}

void Pukupuku::endCapture() {
    FUN_710017605c(1.0f, this);

    sead::Vector3f frontDir;
    al::calcFrontDir(&frontDir, this);

    sead::Vector3f pos =
        al::getTrans(this) + g_7101e62d50.y * sead::Vector3f::ey + g_7101e62d50.z * frontDir;

    rs::endHackFromTargetPos(&mPlayerHack, pos, frontDir);
    al::validateClipping(this);
    al::offCollide(this);
    al::onGroupClipping(this);
    al::showModelIfHide(this);

    joinRotator.set(sead::Vector3f::zero);
    mGroundTimeLimit = 0;
}

void Pukupuku::revive(s32 hitType) {
    if (al::isNerve(this, &NrvPukupuku.Revive))
        return;

    al::setNerve(this, &NrvPukupuku.Revive);

    switch (hitType) {
    case 2:
        al::startHitReaction(this, "死亡");
        break;
    case 1:
        al::startHitReaction(this, "消滅");
        break;
    default:
        break;
    }
}

void Pukupuku::startCapture() {
    al::invalidateClipping(this);
    al::onCollide(this);
    al::offGroupClipping(this);
    al::addDemoActor(al::tryGetSubActor(this, "ライト"));
    rs::startHackStartDemo(mPlayerHack, this);
}

void Pukupuku::updateEffectWaterSurface() {
    if (mWaterSurfaceFinder->isFoundSurface()) {
        sead::Vector3f frontDir;
        al::calcFrontDir(&frontDir, this);
        al::makeMtxUpFrontPos(&mSwimSurfaceTraceEffectFollowMtx, sead::Vector3f::ey, frontDir,
                              mWaterSurfaceFinder->getSurfacePosition());
    }

    if ((al::isNerve(this, &NrvPukupuku.CaptureSwimStart) ||
         al::isNerve(this, &NrvPukupuku.CaptureSwim) ||
         al::isNerve(this, &NrvPukupuku.CaptureSwimDash)) &&
        mWaterSurfaceFinder->isNearSurface(100.0f)) {
        if (!al::isEffectEmitting(this, "SwimSurfaceTrace"))
            al::emitEffect(this, "SwimSurfaceTrace", nullptr);
    } else if (al::isEffectEmitting(this, "SwimSurfaceTrace")) {
        al::deleteEffect(this, "SwimSurfaceTrace");
    }
}

void Pukupuku::updateWaterCondition() {
    al::AreaObj* areaObj = al::tryFindAreaObj(this, "WaterArea", al::getTrans(this));
    if (areaObj) {
        sead::Vector3f nearest;
        if (al::calcNearestAreaObjEdgePos(&nearest, areaObj, al::getTrans(this))) {
            f32 nearestX = nearest.x;
            f32 nearestY = nearest.y;
            f32 nearestZ = nearest.z;
            const sead::Vector3f& trans = al::getTrans(this);
            f32 diffX = nearestX - trans.x;
            f32 diffY = nearestY - trans.y;
            f32 diffZ = nearestZ - trans.z;
            if (diffX * diffX + diffY * diffY + diffZ * diffZ < 10000.0f)
                _290.set(nearest);
        }
    }

    if (al::isInWater(this))
        _29c.set(al::getTrans(this));
    else
        _2a8.set(al::getTrans(this));

    al::WaterSurfaceFinder* finder = mWaterSurfaceFinder;
    finder->update(al::getTrans(this), -al::getGravity(this), 5000.0f);

    bool isWater = false;
    bool isPuddle = false;
    if (al::isInWater(this)) {
        if (!finder->isFoundSurface() || finder->getDistance() >= -30.0f) {
            isWater = true;
            _158++;
            _154 = 0;
        } else if (al::isCollidedGround(this) && finder->getDistance() >= -80.0f) {
            isPuddle = true;
            _154++;
            _158 = 0;
        } else {
            _154++;
            _158 = 0;
        }
    } else {
        if (finder->isFoundSurface() && finder->getDistance() >= -30.0f) {
            isWater = true;
            _158++;
            _154 = 0;
        } else if (finder->isFoundSurface() && al::isCollidedGround(this) &&
                   finder->getDistance() >= -80.0f) {
            isPuddle = true;
            _154++;
            _158 = 0;
        } else {
            _154++;
            _158 = 0;
        }
    }

    if (mWaterSurfaceFinder->isFoundSurface()) {
        const sead::Vector3f& surfacePosition = mWaterSurfaceFinder->getSurfacePosition();
        _2e8.x = surfacePosition.x;
        _2e8.y = surfacePosition.y;
        _2e8.z = surfacePosition.z;
    }

    if (al::isCollidedGround(this)) {
        al::setMaterialCode(this, al::getCollidedFloorMaterialCodeName(this));
        al::updateMaterialCodeWater(this, isWater);
        al::updateMaterialCodePuddle(this, isPuddle);
    }
}

void Pukupuku::control() {
    updateEffectWaterSurface();

    if (!FUN_7100175f24(this)) {
        al::hideSilhouetteModelIfShow(this);
        bool isPukupukuSnow = mIsPukupukuSnow;
        if (!al::isVisAnimPlaying(this, "CapOnOff") || al::getVisAnimFrame(this) != 0.0f) {
            al::startVisAnimAndSetFrameAndStop(this, "CapOnOff", 0.0f);
            al::startMtpAnimAndSetFrameAndStop(
                this, isPukupukuSnow ? "CapOnOffSnow" : "CapOnOff", 0.0f);
        }
    } else {
        if (al::isHideModel(this))
            al::hideSilhouetteModelIfShow(this);
        else
            al::showSilhouetteModelIfHide(this);

        if (rs::isHackCapSeparateFlying(mPlayerHack)) {
            bool isPukupukuSnow = mIsPukupukuSnow;
            if (!al::isVisAnimPlaying(this, "CapOnOff") || al::getVisAnimFrame(this) != 3.0f) {
                al::startVisAnimAndSetFrameAndStop(this, "CapOnOff", 3.0f);
                al::startMtpAnimAndSetFrameAndStop(
                    this, isPukupukuSnow ? "CapOnOffSnow" : "CapOnOff", 0.0f);
            }
        } else {
            bool isCaptureStart = al::isNerve(this, &NrvPukupuku.CaptureStart);
            bool isPukupukuSnow = mIsPukupukuSnow;
            bool isVisAnimPlaying = al::isVisAnimPlaying(this, "CapOnOff");
            if (isCaptureStart) {
                if (!isVisAnimPlaying || al::getVisAnimFrame(this) != 1.0f) {
                    al::startVisAnimAndSetFrameAndStop(this, "CapOnOff", 1.0f);
                    al::startMtpAnimAndSetFrameAndStop(
                        this, isPukupukuSnow ? "CapOnOffSnow" : "CapOnOff", 1.0f);
                }
            } else if (!isVisAnimPlaying || al::getVisAnimFrame(this) != 2.0f) {
                al::startVisAnimAndSetFrameAndStop(this, "CapOnOff", 2.0f);
                al::startMtpAnimAndSetFrameAndStop(
                    this, isPukupukuSnow ? "CapOnOffSnow" : "CapOnOff", 1.0f);
            }
        }

        mJointRippleGenerator->updateAndGenerate();

        if (mGroundTimeLimit >= 421) {
            if (_2f4) {
                al::stopDitherAnimAutoCtrl(this);
                _2f4 = false;
            }

            f32 rate = (mGroundTimeLimit - 420) / 180.0f;
            al::setModelAlphaMask(
                this, sead::Mathf::abs(sead::Mathf::cos(rate * sead::Mathf::pi2() * 3.0f)) *
                          0.9f +
                          0.1f);
        } else if (!_2f4) {
            al::restartDitherAnimAutoCtrl(this);
            al::setModelAlphaMask(this, 1.0f);
            _2f4 = true;
        }

        if (al::isNerve(this, &NrvPukupuku.CaptureWaitGround) ||
            al::isNerve(this, &NrvPukupuku.CaptureJumpGround) ||
            al::isNerve(this, &NrvPukupuku.CaptureLandGround))
            mHackerDepthShadowMapCtrl->setActive(true);
        else
            mHackerDepthShadowMapCtrl->setActive(false);
        mHackerDepthShadowMapCtrl->update(nullptr);
    }

    if (!FUN_7100175f24(this) || al::isNerve(this, &NrvPukupuku.CaptureStart) ||
        al::isNerve(this, &NrvPukupuku.CaptureStartEnd) ||
        al::isNerve(this, &NrvPukupuku.CaptureRollingL) ||
        al::isNerve(this, &NrvPukupuku.CaptureRollingR) ||
        (al::isActionPlaying(this, "TurnPlayer") && !al::isActionEnd(this)) ||
        al::isNerve(this, &NrvPukupuku.CaptureWaitGround) ||
        al::isNerve(this, &NrvPukupuku.CaptureJumpGround) ||
        al::isNerve(this, &NrvPukupuku.CaptureLandGround)) {
        if (al::isExistPrePassLight(this, "Front") && al::isActivePrePassLight(this, "Front")) {
            al::killPrePassLight(this, "Front", -1);
            al::tryGetSubActor(this, "ライト")->kill();
        }
    } else {
        if (al::isExistPrePassLight(this, "Front") && !al::isActivePrePassLight(this, "Front")) {
            al::appearPrePassLight(this, "Front", -1);
            al::tryGetSubActor(this, "ライト")->appear();
        }

        f32 scale =
            sead::Mathf::clamp(al::getPrePassSpotLightCurrentLength(this, "Front") / 1000.0f,
                               0.1f, 1.5f);
        al::setScale(al::tryGetSubActor(this, "ライト"), scale, 1.0f, 1.0f);
        al::setPrePassSpotLightDegree(this, "Front",
                                      sead::Mathf::pow(scale * 1000.0f, -1.09f) * 9973.0f);
    }

    updateInputRolling();
    updateInputKiss();
    updateInputUpDown();

    if ((al::isNerve(this, &NrvPukupuku.CaptureSwimStart) ||
         al::isNerve(this, &NrvPukupuku.CaptureSwim) ||
         al::isNerve(this, &NrvPukupuku.CaptureSwimDash)) &&
        mWaterSurfaceFinder->isFoundSurface() && rs::isHoldHackJump(mPlayerHack)) {
        if (_2e0 < 30)
            _2e0++;
    } else {
        _2e0 = 0;
    }

    if (!FUN_7100175f24(this)) {
        bool isDropLengthLow = _2bc;
        f32 current = al::getShadowMaskDropLength(this, "シャドウマスク");
        if (isDropLengthLow) {
            al::setShadowMaskDropLength(this, current * 0.995f + _2b8 * 0.005f,
                                        "シャドウマスク");
        } else {
            al::setShadowMaskDropLength(this, current * 0.995f + _2b4 * 0.005f,
                                        "シャドウマスク");
        }
        _2bc = false;
    }

    if (FUN_7100175f24(this) && !al::isNerve(this, &NrvPukupuku.CaptureStart)) {
        al::invalidateShadowMask(this);
        al::offDepthShadowModel(this);
        al::validateDepthShadowMap(this);
    } else {
        al::validateShadowMask(this);
        al::onDepthShadowModel(this);
        al::invalidateDepthShadowMap(this);
    }
}

inline bool isTriggerHackSwingAnyHand(const IUsePlayerHack* param_1) {
    return rs::isTriggerHackSwingLeftHand(param_1) || rs::isTriggerHackSwingRightHand(param_1);
}

// NON_MATCHING
void Pukupuku::updateInputRolling() {
    if (!FUN_7100175f24(this))
        return;

    if (isTriggerHackSwingAnyHand(mPlayerHack) ||
        sead::Mathf::abs(rs::getHackStickRotateSpeed(mPlayerHack)) > 20.0f) {
        _2c4 = true;
        if (sead::Mathf::abs(rs::getHackStickRotateSpeed(mPlayerHack)) > 20.0f)
            _2c5 = rs::getHackStickRotateSpeed(mPlayerHack) < 0.0f;
        else
            _2c5 = rs::isTriggerHackSwingRightHand(mPlayerHack);
        _2c8 = 8;
        return;
    }

    if (_2c8 > 0) {
        _2c8--;
        if (_2c8 == 0)
            _2c4 = false;
    }
}

void Pukupuku::updateInputKiss() {
    if (!FUN_7100175f24(this))
        return;

    if (al::isOnGround(this, 0) && rs::isHoldHackAction(mPlayerHack)) {
        sead::Vector3f frontDir;
        al::calcFrontDir(&frontDir, this);
        if (al::calcAngleDegree(frontDir, -sead::Vector3f::ey) < 5.0f && _2dc < 60) {
            _2dc++;

            return;
        }
    }

    _2dc = 0;
}

void Pukupuku::updateInputUpDown() {
    if (!FUN_7100175f24(this))
        return;

    IUsePlayerHack* hack = mPlayerHack;
    bool holdActionJump = false;
    if (rs::isHoldHackAction(hack))
        holdActionJump = rs::isHoldHackJump(hack);

    if (_2d9) {
        mIsTriggerSwimDash = false;
        if (!holdActionJump)
            _2d9 = false;
    }

    if (holdActionJump) {
        if (!mIsTriggerSwimDash && !_2d9) {
            mIsTriggerSwimDash = true;
            _2d9 = true;
        }
    } else {
        if (rs::isHoldHackJump(mPlayerHack))
            _2c0 -= 0.1f;
        else if (rs::isHoldHackAction(mPlayerHack))
            _2c0 += 0.1f;
        else
            _2c0 *= 0.1f;
    }

    _2c0 = sead::Mathf::clamp(_2c0, -1.0f, 1.0f);
}

bool Pukupuku::isSwimTypeA() const {
    return true;
}

void Pukupuku::updateVelocity() {
    if (isNerveInWater()) {
        f32 rate = sead::Mathf::clamp(al::getVelocity(this).length() / 15.0f, 0.0f, 1.0f);
        bool isInputOff = !mPlayerHack || rs::getHackMoveStickRaw(mPlayerHack).length() <= 0.1f;

        sead::Vector3f frontDir;
        al::calcFrontDir(&frontDir, this);

        bool isCollidedGroundFront = false;
        if (al::isCollidedGround(this))
            isCollidedGroundFront =
                (-frontDir).dot(sead::Vector3f::ey) > sead::Mathf::cos(sead::Mathf::deg2rad(15.0f));

        bool isCollidedCeilingFront = false;
        if (al::isCollidedCeiling(this)) {
            const f32 frontX = frontDir.x;
            const f32 frontY = frontDir.y;
            const f32 frontZ = frontDir.z;
            const sead::Vector3f& up = sead::Vector3f::ey;
            isCollidedCeilingFront = frontX * up.x + frontY * up.y + frontZ * up.z >
                                     sead::Mathf::cos(sead::Mathf::deg2rad(15.0f));
        }

        al::scaleVelocity(this,
                          isInputOff && (isCollidedGroundFront || isCollidedCeilingFront) ?
                              0.1f :
                              rate * 0.05f + 0.92f);
        al::limitVelocity(this, 20.0f);
    } else {
        al::addVelocityToGravity(this, 2.0f);
        if (al::isOnGround(this, 0) && !al::isCollidedFloorCode(this, "Slide"))
            al::scaleVelocity(this, 0.5f);
        else
            al::scaleVelocity(this, 0.998f);

        al::limitVelocity(this, 50.0f);
    }
}

void Pukupuku::exeReaction() {
    if (al::isFirstStep(this))
        al::startAction(this, "SwimReaction");

    if (al::isActionEnd(this))
        al::setNerve(this, &NrvPukupuku.Wait);
}

void Pukupuku::exeWaitRollingRail() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        _19c = true;

        sead::Vector3f railRollAxis;
        if (FUN_7100177c74(&railRollAxis, this) && railRollAxis.dot(sead::Vector3f::ey) > 0.0f)
            _19c = false;

        al::startAction(this, _19c ? "RollingRail" : "RollingRailReverse");
        al::moveSyncRailTurn(this, 0.0f);
        al::calcRailMoveDir(&_178, this);
        _184 = 720.0f;
    }

    if (al::isExistRail(this) && !al::isParallelDirection(_178, sead::Vector3f::ey, 0.01f)) {
        sead::Vector3f frontDir;
        al::calcFrontDir(&frontDir, this);

        f32 angle = al::calcAngleDegree(frontDir, _178);
        f32 sign = frontDir.z * _178.x - _178.z * frontDir.x;
        if ((_19c && sign > 0.0f) || (!_19c && sign < 0.0f))
            angle = 360.0f - angle;

        if (angle > 0.0f && (angle <= _184 || al::isNearZero(_184 - angle, 0.001f))) {
            s32 actionFrameMax =
                al::getActionFrameMax(this, _19c ? "RollingRail" : "RollingRailReverse");
            f32 rate = al::calcNerveRate(this, actionFrameMax);
            f32 rotate = sead::Mathf::clamp(rate * rate * angle, 0.0f, 45.0f);
            sead::Quatf* quat = al::getQuatPtr(this);
            al::rotateQuatYDirDegree(quat, *quat, _19c ? -rotate : rotate);
            _184 = angle;
        }
    }

    if (al::isGreaterEqualStep(this, 20))
        al::moveSyncRail(this, 2.0f);

    if (al::isActionEnd(this))
        al::setNerve(this, &NrvPukupuku.Wait);
}

bool FUN_7100177c74(sead::Vector3f* out, al::LiveActor* actor) {
    f32 railCoord = al::getRailCoord(actor);
    sead::Vector3f railPos = al::getRailPos(actor);

    sead::Vector3f posNext;
    al::calcRailPosAtCoord(&posNext, actor,
                           railCoord + (al::isRailGoingToEnd(actor) ? 100.0f : -100.0f));

    sead::Vector3f posPrev;
    al::calcRailPosAtCoord(&posPrev, actor,
                           railCoord + (al::isRailGoingToEnd(actor) ? -100.0f : 100.0f));

    sead::Vector3f distNext = posNext;
    distNext -= railPos;
    al::verticalizeVec(&distNext, al::getGravity(actor), distNext);
    if (!al::tryNormalizeOrZero(&distNext)) {
        al::calcFrontDir(&distNext, actor);
        al::verticalizeVec(&distNext, al::getGravity(actor), distNext);
        if (!al::tryNormalizeOrZero(&distNext))
            return false;
    }

    sead::Vector3f distPrev = railPos;
    distPrev -= posPrev;
    al::verticalizeVec(&distPrev, al::getGravity(actor), distPrev);
    if (!al::tryNormalizeOrZero(&distPrev)) {
        al::calcFrontDir(&distPrev, actor);
        distPrev.negate();
        al::verticalizeVec(&distPrev, al::getGravity(actor), distPrev);
        if (!al::tryNormalizeOrZero(&distPrev))
            return false;
    }

    out->set(distPrev.cross(distNext));

    return !(out->length() < 0.05f);
}

void Pukupuku::exeWait() {
    updateWaterCondition();

    if (al::isFirstStep(this))
        al::startAction(this, mWaterSurfaceFinder->isFoundSurface() ? "SwimSurfaceEnemy" :
                                                                    "SwimWaitWater");

    if (mWaterSurfaceFinder->isFoundSurface() && al::getNerveStep(this) % 3 == 0)
        al::tryAddRippleTiny(this);

    if (al::isExistRail(this)) {
        if ((al::isActionPlaying(this, "RollingRail") ||
             al::isActionPlaying(this, "RollingRailReverse")) &&
            al::isActionEnd(this))
            al::startAction(this, mWaterSurfaceFinder->isFoundSurface() ? "SwimSurfaceEnemy" :
                                                                        "SwimWaitWater");

        bool isSamePoint;
        if (mRailPointNo == al::getRailPointNo(this)) {
            isSamePoint = true;
        } else {
            isSamePoint = false;
            mRailPointNo = al::getRailPointNo(this);
        }

        s32 moveType = mMoveType;
        if (moveType != 0) {
            al::moveSyncRail(this, 5.0f);
            if (moveType == 1 && (al::isRailReachedStart(this) || al::isRailReachedEnd(this))) {
                al::setNerve(this, &NrvPukupuku.WaitTurnToRailDir);
                return;
            }
        } else {
            if (al::moveSyncRailLoop(this, 5.0f))
                al::turnToRailDirImmediately(this);

            if (mRailPointNo != 0 &&
                !(isSamePoint | (mRailPointNo == al::getRailPointNum(this) - 1))) {
                sead::Vector3f railRollAxis;
                bool isValidRailRollAxis = FUN_7100177c74(&railRollAxis, this);
                al::startAction(this,
                                isValidRailRollAxis &&
                                        railRollAxis.dot(sead::Vector3f::ey) >= 0.0f ?
                                    "RollingRailReverse" :
                                    "RollingRail");
            }
        }

        sead::Vector3f railMoveDir;
        al::calcRailMoveDir(&railMoveDir, this);
        if (!al::isParallelDirection(railMoveDir, sead::Vector3f::ey, 0.01f))
            al::turnToRailDir(this, al::calcNerveRate(this, 60) * 5.0f);
    } else {
        updateVelocity();
    }

    checkCollidedFloorDamageAndNextNerve();
}

bool Pukupuku::checkCollidedFloorDamageAndNextNerve() {
    if (al::isCollidedFloorCode(this, "Needle")) {
        if (FUN_7100175f24(this)) {
            rs::requestDamage(mPlayerHack);

            return false;
        }

        sead::Vector3f frontDir;
        al::calcFrontDir(&frontDir, this);

        mEnemyStateBlowDown->start(-frontDir);
        mEnemyStateBlowDown->setParam(mWaterSurfaceFinder->isFoundSurface() ? &g_7101e62d30 :
                                                                              &g_7101e62d10);

        al::setNerve(this, &NrvPukupuku.BlowDownWithoutMsg);

        return true;
    }

    if (!al::isCollidedFloorCode(this, "DamageFire") && !al::isCollidedFloorCode(this, "Needle") &&
        !al::isCollidedFloorCode(this, "Poison"))
        return false;

    if (FUN_7100175f24(this))
        endCapture();

    revive(2);

    return true;
}

void Pukupuku::exeWaitTurnToRailDir() {
    if (al::isFirstStep(this))
        al::startAction(this, "Turn");

    if (al::isExistRail(this)) {
        if (mMoveType == 1) {
            sead::Vector3f frontDir;
            al::calcFrontDir(&frontDir, this);

            sead::Vector3f railMoveDir;
            al::calcRailMoveDir(&railMoveDir, this);

            if (al::calcAngleDegree(frontDir, railMoveDir) < 20.0f) {
                al::setNerve(this, &NrvPukupuku.Wait);
                return;
            }
        }

        al::turnToRailDir(this, 5.0f);
    }

    checkCollidedFloorDamageAndNextNerve();
}

void Pukupuku::exeSwoon() {
    updateWaterCondition();
    updateVelocity();
    if (checkCollidedFloorDamageAndNextNerve())
        return;

    if (!_19d) {
        if (al::isOnGround(this, 0)) {
            revive(2);

            return;
        }

        if (_158 > 2)
            _19d = true;
    } else if (_154 > 10) {
        _19d = false;
    }

    if (al::updateNerveState(this))
        revive(1);
}

void Pukupuku::exeCaptureStart() {
    if (rs::isHackStartDemoEnterMario(mPlayerHack))
        al::setNerve(this, &NrvPukupuku.CaptureStartEnd);
}

void Pukupuku::exeCaptureStartEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "HackStart");
        mJointRippleGenerator->reset();
        mPlayerHackStartShaderCtrl->start();
    }

    mPlayerHackStartShaderCtrl->update();

    sead::Vector3f moveDir = {0.0f, 0.0f, 0.0f};
    rs::calcHackerMoveDir(&moveDir, mPlayerHack, sead::Vector3f::ey);
    al::turnToDirection(this, moveDir, 6.0f);

    if (al::isActionEnd(this)) {
        mPlayerHackStartShaderCtrl->end();
        rs::endHackStartDemo(mPlayerHack, this);
        al::setNerve(this, &NrvPukupuku.CaptureWait);
    }
}

f32 Pukupuku::getAccel(IUsePlayerHack* hack) const {
    f32 accel;
    if (al::isNerve(this, &NrvPukupuku.CaptureSwimDash)) {
        accel = 1.815f;
    } else if (rs::isHoldHackJump(hack) && rs::isHoldHackAction(hack)) {
        accel = 0.8f;
    } else if (rs::isHoldHackJump(hack) || rs::isHoldHackAction(hack)) {
        sead::Vector3f frontDir;
        al::calcFrontDir(&frontDir, this);
        const f32 frontX = frontDir.x;
        const f32 frontY = frontDir.y;
        const f32 frontZ = frontDir.z;
        const sead::Vector3f& up = sead::Vector3f::ey;
        f32 dot = frontX * up.x + frontY * up.y + frontZ * up.z;
        sead::Mathf::cos(0.2617994f);
        if (dot > 0.9659258f) {
            accel = sead::Mathf::abs(_2c0) * 0.8f;
        } else {
            dot = -frontX * up.x - frontY * up.y - frontZ * up.z;
            sead::Mathf::cos(0.2617994f);
            if (dot > 0.9659258f)
                accel = sead::Mathf::abs(_2c0) * 0.8f;
            else
                accel = 0.0f;
        }
    } else {
        accel = rs::getHackMoveStickRaw(mPlayerHack).length() * 0.8f;
    }

    if (!al::isNerve(this, &NrvPukupuku.CaptureSwimDash) &&
        mWaterSurfaceFinder->isFoundSurface() && rs::isHoldHackJump(mPlayerHack) &&
        !rs::isHoldHackAction(mPlayerHack))
        accel *= 0.05f;

    return accel;
}

void Pukupuku::exeCaptureSwimStart() {
    if (al::isFirstStep(this))
        al::startAction(this, mWaterSurfaceFinder->isFoundSurface() ? "SwimStartSurface" :
                                                                    "SwimStartWater");

    if (checkCollidedFloorDamageAndNextNerve())
        return;

    updateWaterCondition();
    if (_154 >= 11) {
        onWaterOut();
        al::setNerve(this, &NrvPukupuku.CaptureWaitAir);
        return;
    }

    updateWaterSurfaceMtx(&mWaterSurface, this, mWaterSurfaceFinder);

    if (tryAddVelocityWaterSurfaceJumpOut()) {
        al::startHitReaction(this, "水面ジャンプ");
        al::setNerve(this, &NrvPukupuku.CaptureJumpOut);
        return;
    }

    if (isHackInputActive(mPlayerHack))
        FUN_7100178da4(getAccel(mPlayerHack), this);

    updateVelocity();
    approachSurface();
    updatePoseSwim();

    if (mIsTriggerSwimDash) {
        al::startAction(this, getPukupukuDashAction(mWaterSurfaceFinder, mPlayerHack));
        al::setNerve(this, &NrvPukupuku.CaptureSwimDash);
    } else if (_2c4) {
        const al::Nerve* rollingR = &NrvPukupuku.CaptureRollingR;
        const al::Nerve* rollingL = &NrvPukupuku.CaptureRollingL;
        al::setNerve(this, _2c5 ? rollingR : rollingL);
    } else if (al::isActionEnd(this)) {
        if (isHackInputActive(mPlayerHack))
            al::setNerve(this, &NrvPukupuku.CaptureSwim);
        else
            al::setNerve(this, &NrvPukupuku.CaptureWait);
    }
}

// NON_MATCHING
void Pukupuku::onWaterOut() {
    if (al::isInWater(this))
        return;

    AreaObjFilterWaterIgnore filterWaterIgnore;
    AreaObjFilterWater filterWater;
    al::AreaObj* ignoreArea =
        al::tryFindAreaObjWithFilter(this, "WaterArea", al::getTrans(this), &filterWaterIgnore);
    al::AreaObj* waterArea = al::tryFindAreaObjWithFilter(this, "WaterArea", _29c, &filterWater);

    sead::Vector3f start = al::getTrans(this);
    sead::Vector3f end = _29c;
    al::AreaObj* areaObj = ignoreArea;
    bool isReverse;

    if (ignoreArea && !waterArea) {
        start.set(_29c);
        end.set(al::getTrans(this));
        isReverse = true;
    } else {
        isReverse = false;
        if (ignoreArea && waterArea) {
            if (ignoreArea->isInVolume(start) && ignoreArea->isInVolume(end)) {
                isReverse = false;
                areaObj = waterArea;
            } else {
                start.set(_29c);
                end.set(al::getTrans(this));
                isReverse = true;
            }
        } else {
            areaObj = waterArea;
        }
    }

    if (!areaObj)
        return;

    sead::Vector3f hitPos;
    sead::Vector3f normal;
    if (al::checkAreaObjCollisionByArrow(&hitPos, &normal, areaObj, start, end)) {
        sead::Vector3f frontDir;
        al::calcFrontDir(&frontDir, this);
        if (al::isParallelDirection(frontDir, normal))
            al::calcUpDir(&frontDir, this);

        if (isReverse)
            normal.negate();

        al::makeMtxFrontUpPos(&mWaterAreaOutEffectFollowMtx, normal, frontDir, hitPos);
        al::emitEffect(this, "WaterAreaOut", nullptr);
    }
}

bool Pukupuku::tryAddVelocityWaterSurfaceJumpOut() {
    if (!checkJumpOutCondition())
        return false;

    sead::Vector3f frontDir;
    al::calcFrontDir(&frontDir, this);
    frontDir.y = 0.0f;

    if (al::tryNormalizeOrZero(&frontDir))
        al::setVelocity(this, sead::Vector3f::ey * 65.0f + frontDir * 15.0f);

    return true;
}

// NON_MATCHING
__attribute__((noinline)) void FUN_7100178da4(f32 param_1, Pukupuku* param_2) {
    sead::Vector3f frontDir;
    al::calcFrontDir(&frontDir, param_2);

    f32 x = frontDir.x;
    f32 y = frontDir.y;
    f32 z = frontDir.z;

    f32 dot = frontDir.dot(sead::Vector3f::ey);
    sead::Mathf::cos(0.2617994f);
    if (dot > 0.9659258f) {
        x = sead::Vector3f::ey.x;
        y = sead::Vector3f::ey.y;
        z = sead::Vector3f::ey.z;
    } else {
        dot = (-frontDir).dot(sead::Vector3f::ey);
        sead::Mathf::cos(0.2617994f);
        if (dot > 0.9659258f) {
            y = -sead::Vector3f::ey.y;
            x = -sead::Vector3f::ey.x;
            z = -sead::Vector3f::ey.z;
        }
    }

    sead::Vector3f velocity = {x * param_1, y * param_1, z * param_1};
    al::addVelocity(param_2, velocity);
}

// NON_MATCHING
void Pukupuku::approachSurface() {
    sead::Vector3f upDir;
    al::calcUpDir(&upDir, this);

    f32 angle = sead::Mathf::clamp((al::calcAngleDegree(upDir, sead::Vector3f::ey) - 15.0f) / 30.0f,
                                   0.0f, 1.0f);

    f32 invAngle = 1.0f - angle;
    f32 spring = angle * 0.002f + invAngle * 0.008f;
    f32 damping = invAngle * 0.9f + angle * 0.988f;
    al::approachWaterSurfaceSpringDumper(this, mWaterSurfaceFinder, 5.0f, 12.0f, 1.0f, spring,
                                         damping);
}

bool Pukupuku::updatePoseSwim() {
    sead::Vector2f hackMoveStick = rs::getHackMoveStickRaw(mPlayerHack);

    sead::Vector3f frontDir;
    al::calcFrontDir(&frontDir, this);

    sead::Vector3f normalfrontDir = frontDir;
    normalfrontDir.y = 0.0f;
    normalfrontDir.normalize();

    f32 fVar133 = -sead::Mathf::clamp(_2c0 * 2.0f, -1.0f, 1.0f);

    bool result = false;
    if (mWaterSurfaceFinder->isFoundSurface()) {
        result = !rs::isHoldHackJump(mPlayerHack);
        if (result)
            fVar133 = sead::Mathf::clampMax(fVar133, 0.0f);
    }

    bool bVar1;
    f32 angle;
    if (al::isNerve(this, &NrvPukupuku.CaptureJumpOut)) {
        angle = sead::Mathf::clamp(al::getVelocity(this).y + 20.0f, 0.0f, 30.0f);
        angle = (angle / -30.0f + 1.0f) * -89.0f;
        bVar1 = true;
    } else {
        bVar1 = false;
        angle = fVar133 * 89.0f;
    }

    sead::Vector3f hori = sead::Mathf::cos(sead::Mathf::deg2rad(angle)) * normalfrontDir;
    hori += sead::Mathf::sin(sead::Mathf::deg2rad(angle)) * sead::Vector3f::ey;
    hori.normalize();

    sead::Quatf quat;
    al::makeQuatFrontUp(&quat, hori, sead::Vector3f::ey);

    f32 fVar8 = sead::Mathf::abs(fVar133);
    if (al::isNearZero(fVar133) || bVar1)
        fVar8 = 1.0f;
    else
        fVar8 = fVar8 * fVar8;

    f32 fVar13 = 0.2f;
    f32 fVar10 = 0.1f;
    if (rs::isHoldHackJump(mPlayerHack)) {
        if (!rs::isHoldHackAction(mPlayerHack)) {
            fVar13 = 0.45f;
            fVar10 = 0.45f;
        }
    }

    f32 fVar15 = 0.1f;
    if (isNerveInWater()) {
        fVar15 = fVar13;
        if (al::getVelocity(this).length() >= 1.0f)
            fVar15 = fVar10;
    }

    al::slerpQuat(al::getQuatPtr(this), al::getQuat(this), quat, fVar8 * fVar15);

    sead::Vector3f frontDir2;
    al::calcFrontDir(&frontDir2, this);

    bool bVar2 = false;
    if (rs::calcHackerMoveDir(&frontDir, mPlayerHack, sead::Vector3f::ey) &&
        al::calcAngleDegree(hori, normalfrontDir) > 90.0f && rs::isHoldHackAction(mPlayerHack)) {
        bVar2 = !rs::isHoldHackJump(mPlayerHack);
    }

    fVar13 = bVar2 ? 160.0f : -8.0f;
    fVar10 = sead::Mathf::clamp(al::getVelocity(this).length() / 10.0f, 0.0f, 1.0f);

    fVar8 = sead::Mathf::acos(sead::Mathf::clamp(normalfrontDir.dot(frontDir2), 0.0f, 1.0f));
    fVar8 = sead::Mathf::rad2deg(fVar8) / 89.0f;
    fVar8 = (6.0f - fVar10) * fVar8 + (fVar13 * fVar10 + 20.0f) * (1.0f - fVar8);

    f32 fVar7 = sead::Mathf::abs(hackMoveStick.length() * fVar8);

    if (bVar2 && fVar7 > 90.0f)
        al::startAction(this, "TurnPlayer");

    fVar13 = (hori.dot(normalfrontDir) > 0.0f) ? -fVar7 : fVar7;

    sead::Vector3f rotatedVec;
    al::rotateVectorDegree(&rotatedVec, frontDir2, sead::Vector3f::ey, fVar13);

    sead::Vector3f rotatedVec2 = rotatedVec.cross(sead::Vector3f::ey);
    al::makeQuatFrontSide(al::getQuatPtr(this), rotatedVec, rotatedVec2);

    return result;
}

bool Pukupuku::isTriggerSwimDash() const {
    return mIsTriggerSwimDash;
}

void Pukupuku::onWaterIn() {
    if (al::getVelocity(this).y < -10.0f && mWaterSurfaceFinder->isFoundSurface()) {
        sead::Vector3f surfaceNormal = mWaterSurfaceFinder->getSurfaceNormal();
        if (al::calcAngleDegree(surfaceNormal, sead::Vector3f::ey) < 30.0f) {
            al::WaterSurfaceFinder* finder = mWaterSurfaceFinder;
            bool isFoundSurface = finder->isFoundSurface();
            const sead::Vector3f* effectPosPtr = &finder->getSurfacePosition();
            if (!isFoundSurface)
                effectPosPtr = &_2e8;

            sead::Vector3f effectPos = *effectPosPtr;
            al::startHitReactionHitEffect(this, "水に入る", effectPos);
        }
    }

    al::scaleVelocity(this, 0.3f);
    if (!al::isInWater(this))
        return;

    AreaObjFilterWater filterWater;
    AreaObjFilterWaterIgnore filterWaterIgnore;
    al::AreaObj* waterArea =
        al::tryFindAreaObjWithFilter(this, "WaterArea", al::getTrans(this), &filterWater);
    al::AreaObj* ignoreArea =
        al::tryFindAreaObjWithFilter(this, "WaterArea", _2a8, &filterWaterIgnore);

    sead::Vector3f start = al::getTrans(this);
    sead::Vector3f end = _2a8;
    al::AreaObj* areaObj = waterArea;
    bool isReverse = false;

    if (!waterArea && ignoreArea) {
        isReverse = true;
        areaObj = ignoreArea;
    } else if (waterArea && ignoreArea) {
        if (waterArea->isInVolume(start) && waterArea->isInVolume(end)) {
            isReverse = true;
            areaObj = ignoreArea;
        } else {
            start.set(_2a8);
            end.set(al::getTrans(this));
        }
    } else if (waterArea) {
        start.set(_2a8);
        end.set(al::getTrans(this));
    } else {
        return;
    }

    if (!areaObj)
        return;

    sead::Vector3f hitPos;
    sead::Vector3f normal;
    if (al::checkAreaObjCollisionByArrow(&hitPos, &normal, areaObj, start, end)) {
        sead::Vector3f frontDir;
        al::calcFrontDir(&frontDir, this);
        if (al::isParallelDirection(frontDir, normal))
            al::calcUpDir(&frontDir, this);

        if (isReverse)
            normal.negate();

        al::makeMtxFrontUpPos(&mWaterAreaIn, normal, frontDir, hitPos);
        al::emitEffect(this, "WaterAreaIn", nullptr);
    }
}

void Pukupuku::exeCaptureSwim() {
    if (al::isFirstStep(this))
        _2f8 = 0;

    if (checkCollidedFloorDamageAndNextNerve())
        return;

    updateWaterCondition();

    bool isWallHit = al::isCollidedWallVelocity(this);
    bool isCeilingHit = al::isCollidedCeilingVelocity(this);
    if (isWallHit || isCeilingHit) {
        const sead::Vector3f& normal =
            isWallHit ? al::getCollidedWallNormal(this) : al::getCollidedCeilingNormal(this);
        sead::Vector3f velocity = al::getVelocity(this);
        if (velocity.length() <= 20.0f) {
            if (!al::isNerve(this, &NrvPukupuku.CaptureSwimDash) &&
                al::tryNormalizeOrZero(&velocity)) {
                sead::Vector3f parallel;
                al::parallelizeVec(&parallel, normal, al::getVelocity(this));
                al::addVelocity(this, normal * (parallel.length() * 0.8f));
            }
        } else if (al::tryNormalizeOrZero(&velocity)) {
            if ((-velocity).dot(normal) > sead::Mathf::cos(1.309f)) {
                const sead::Vector3f& pos =
                    isWallHit ? al::getCollidedWallPos(this) : al::getCollidedCeilingPos(this);
                al::startHitReactionHitEffect(this, "壁ヒット", pos);
                sead::Vector3f parallel;
                al::parallelizeVec(&parallel, normal, al::getVelocity(this));
                al::addVelocity(this, normal * (parallel.length() * 1.5f));
                al::setNerve(this, &NrvPukupuku.CaptureReactionWall);
                return;
            }

            sead::Vector3f parallel;
            al::parallelizeVec(&parallel, normal, al::getVelocity(this));
            al::addVelocity(this, normal * (parallel.length() * 0.8f));
        }
    }

    FUN_7100178da4(getAccel(mPlayerHack), this);
    updateVelocity();

    bool isVerticalInput = false;
    sead::Vector3f frontDir;
    al::calcFrontDir(&frontDir, this);
    if (frontDir.dot(sead::Vector3f::ey) > sead::Mathf::cos(sead::Mathf::deg2rad(15.0f)) ||
        (-frontDir).dot(sead::Vector3f::ey) > sead::Mathf::cos(sead::Mathf::deg2rad(15.0f))) {
        f32 stickLength = rs::getHackMoveStickRaw(mPlayerHack).length();
        if (stickLength > 0.1f) {
            sead::Vector3f upDir;
            al::calcUpDir(&upDir, this);
            if (frontDir.dot(sead::Vector3f::ey) > sead::Mathf::cos(sead::Mathf::deg2rad(15.0f)))
                upDir.negate();

            al::addVelocity(this, upDir * (stickLength * 0.6f));
            joinRotator.x += stickLength * 2.0f * (frontDir.dot(sead::Vector3f::ey) > 0.0f ?
                                                       1.0f :
                                                       -1.0f);
            joinRotator.x = sead::Mathf::clamp(joinRotator.x, -30.0f, 30.0f);
            isVerticalInput = true;
        }
    }

    updateWaterSurfaceMtx(&mWaterSurface, this, mWaterSurfaceFinder);

    if (checkJumpOutCondition()) {
        tryAddVelocityWaterSurfaceJumpOut();
        al::startHitReaction(this, "水面ジャンプ");
        al::setNerve(this, &NrvPukupuku.CaptureJumpOut);
        return;
    }

    approachSurface();
    updatePoseSwim();

    if (_2c4 && (!al::isNerve(this, &NrvPukupuku.CaptureSwimDash) ||
                 al::isGreaterStep(this, 15))) {
        if (_2c5)
            al::setNerve(this, &NrvPukupuku.CaptureRollingR);
        else
            al::setNerve(this, &NrvPukupuku.CaptureRollingL);
        return;
    }

    if (al::isNerve(this, &NrvPukupuku.CaptureSwim)) {
        if (isHackInputActive(mPlayerHack)) {
            _2f8 = 15;
        } else if (rs::getHackMoveStickRaw(mPlayerHack).length() <= 0.1f && _2f8 > 0) {
            _2f8--;
            if (_2f8 == 0) {
                al::setNerve(this, &NrvPukupuku.CaptureWait);
                return;
            }
        }
    }

    if (!isVerticalInput)
        decayRootRotX(&joinRotator);

    if (_154 >= 11) {
        onWaterOut();
        al::setNerve(this, &NrvPukupuku.CaptureWaitAir);
        return;
    }

    if (al::isNerve(this, &NrvPukupuku.CaptureSwimDash)) {
        if (al::isGreaterEqualStep(this, 45) && mIsTriggerSwimDash) {
            al::startAction(this, getPukupukuDashAction(mWaterSurfaceFinder, mPlayerHack));
            al::setNerve(this, &NrvPukupuku.CaptureSwimDash);
        } else if (al::isGreaterStep(this, 90)) {
            al::setNerve(this, &NrvPukupuku.CaptureSwim);
        }
    } else {
        if ((!al::isActionPlaying(this, "TurnPlayer") || al::isActionEnd(this)))
            al::tryStartActionIfNotPlaying(this, getPukupukuSwimAction(mWaterSurfaceFinder));

        if (mIsTriggerSwimDash) {
            al::startAction(this, getPukupukuDashAction(mWaterSurfaceFinder, mPlayerHack));
            al::setNerve(this, &NrvPukupuku.CaptureSwimDash);
        }
    }
}

void Pukupuku::exeCaptureReactionWall() {
    if (al::isFirstStep(this))
        al::startAction(this, "ReactionWall");

    if (checkCollidedFloorDamageAndNextNerve())
        return;

    updateWaterCondition();
    updateVelocity();
    approachSurface();

    if (al::isActionEnd(this)) {
        IUsePlayerHack* hack = mPlayerHack;
        if (rs::isHoldHackJump(hack) || rs::isHoldHackAction(hack) ||
            rs::getHackMoveStickRaw(hack).length() > 0.1f)
            al::setNerve(this, &NrvPukupuku.CaptureSwim);
        else
            al::setNerve(this, &NrvPukupuku.CaptureWait);
    }
}

bool Pukupuku::checkJumpOutCondition() const {
    if (!mWaterSurfaceFinder->isFoundSurface())
        return false;
    if (!isNerveInWater())
        return false;
    if (al::isNerve(this, &NrvPukupuku.CaptureWaitAir))
        return false;
    if (al::isNerve(this, &NrvPukupuku.CaptureJumpOut))
        return false;

    rs::getHackMoveStickRaw(mPlayerHack);
    f32 upDown = _2c0;
    if (upDown >= -0.5f)
        return false;
    if (_2e0 == 30)
        return true;
    if (rs::isTriggerHackSwing(mPlayerHack))
        return true;

    IUsePlayerHack* hack = mPlayerHack;
    bool isInputActive =
        ((rs::isHoldHackJump(hack) || rs::isHoldHackAction(hack) ||
          rs::getHackMoveStickRaw(hack).length() > 0.1f) &&
         !al::isNerve(this, &NrvPukupuku.CaptureSwimDash)) ||
        al::isNerve(this, &NrvPukupuku.CaptureSwimDash);

    if (al::getVelocity(this).y > 10.0f)
        return upDown < -0.1f && isInputActive;

    return false;
}

void Pukupuku::updateCameraCaptureWait() {
    sead::Vector3f frontDir;
    al::calcFrontDir(&frontDir, this);

    if (al::isParallelDirection(frontDir, sead::Vector3f::ey))
        return;

    frontDir.y = 0.0f;
    frontDir.normalize();

    _140.set(frontDir);
}

void Pukupuku::exeCaptureWait() {
    if (al::isFirstStep(this)) {
        if (al::isNerve(this, &NrvPukupuku.CaptureWaitTurnStart))
            al::startAction(this, mWaterSurfaceFinder->isFoundSurface() ? "SwimWaitStartSurface" :
                                                                        "SwimWaitStartWater");
        else
            al::tryStartActionIfNotPlaying(this, getPukupukuWaitAction(mWaterSurfaceFinder));
    }

    updateWaterCondition();

    f32 stickX = sead::Mathf::abs(rs::getHackMoveStickRaw(mPlayerHack).x);
    if (al::isNerve(this, &NrvPukupuku.CaptureWait) && stickX > 0.2f) {
        al::setNerve(this, &NrvPukupuku.CaptureWaitTurnStart);
        return;
    }

    if (al::isNerve(this, &NrvPukupuku.CaptureWaitTurnStart)) {
        if (al::isActionPlaying(this, "SwimWaitStartWater") ||
            al::isActionPlaying(this, "SwimWaitStartSurface")) {
            if (al::isActionEnd(this)) {
                const al::Nerve* waitTurn = &NrvPukupuku.CaptureWaitTurn;
                const al::Nerve* wait = &NrvPukupuku.CaptureWait;
                al::setNerve(this, stickX > 0.2f ? waitTurn : wait);
                return;
            }
        } else if (stickX <= 0.2f) {
            al::setNerve(this, &NrvPukupuku.CaptureWait);
            return;
        }
    } else if (al::isNerve(this, &NrvPukupuku.CaptureWaitTurn) && stickX <= 0.2f) {
        al::setNerve(this, &NrvPukupuku.CaptureWait);
        return;
    }

    if (isNerveInWater()) {
        if (mIsTriggerSwimDash) {
            al::startAction(this, getPukupukuDashAction(mWaterSurfaceFinder, mPlayerHack));
            al::setNerve(this, &NrvPukupuku.CaptureSwimDash);
            return;
        }

        if (_2c4) {
            if (_2c5)
                al::setNerve(this, &NrvPukupuku.CaptureRollingR);
            else
                al::setNerve(this, &NrvPukupuku.CaptureRollingL);
            return;
        }

        if (isHackInputActive(mPlayerHack)) {
            al::setNerve(this, &NrvPukupuku.CaptureSwimStart);
            return;
        }

        if (checkJumpOutCondition()) {
            sead::Vector3f frontDir;
            al::calcFrontDir(&frontDir, this);
            frontDir.y = 0.0f;

            if (al::tryNormalizeOrZero(&frontDir))
                al::setVelocity(this, sead::Vector3f::ey * 65.0f + frontDir * 15.0f);

            al::startHitReaction(this, "水面ジャンプ");
            al::setNerve(this, &NrvPukupuku.CaptureJumpOut);
            return;
        }

        if (_154 >= 11) {
            onWaterOut();
            al::setNerve(this, &NrvPukupuku.CaptureWaitAir);
            return;
        }
    } else if (al::isOnGround(this, 0) && !al::isCollidedFloorCode(this, "Slide")) {
        if (al::isNerve(this, &NrvPukupuku.CaptureWaitAir)) {
            al::setNerve(this, &NrvPukupuku.CaptureWaitGround);
            return;
        }

        updateWaterSurfaceMtx(&mWaterSurface, this, mWaterSurfaceFinder);
        FUN_710017605c(1.0f, this);
        al::startAction(this, "Land");
        al::setNerve(this, &NrvPukupuku.CaptureLandGround);
        return;
    }

    if ((!al::isNerve(this, &NrvPukupuku.CaptureJumpOut) ||
         (al::isNerve(this, &NrvPukupuku.CaptureJumpOut) && al::isGreaterStep(this, 30))) &&
        _158 >= 3) {
        onWaterIn();
        al::setNerve(this, &NrvPukupuku.CaptureWait);
        return;
    }

    if (checkCollidedFloorDamageAndNextNerve())
        return;

    updateCameraCaptureWait();
    updateVelocity();
    updatePoseSwim();
    decayRootRotX(&joinRotator);

    if (isNerveInWater()) {
        approachSurface();
        al::WaterSurfaceFinder* finder = mWaterSurfaceFinder;
        if (al::isActionPlaying(this, "SwimWaitWaterHack")) {
            if (finder->isFoundSurface())
                al::startAction(this, "SwimWaitSurface");
        } else if (al::isActionPlaying(this, "SwimWaitSurface") && !finder->isFoundSurface()) {
            al::startAction(this, "SwimWaitWaterHack");
        }
    }
}

void Pukupuku::exeCaptureAttack() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Attack");

        sead::Vector3f frontDir;
        al::calcFrontDir(&frontDir, this);
        al::setVelocity(this, frontDir * 55.0f);
    }

    if (al::isCollidedWall(this)) {
        al::HitSensor* collidedWallSensor = al::getCollidedWallSensor(this);
        if (al::getVelocity(this).length() > 10.0f)
            rs::sendMsgHackAttack(collidedWallSensor, al::getHitSensor(this, "Attack"));
    }

    if (checkCollidedFloorDamageAndNextNerve())
        return;

    updateWaterCondition();
    if (!isNerveInWater() && al::isOnGround(this, 0)) {
        al::setNerve(this, &NrvPukupuku.CaptureWaitGround);
        return;
    }

    if (al::isGreaterStep(this, 20) && isNerveInWater())
        FUN_7100178da4(getAccel(mPlayerHack), this);

    updateVelocity();
    if (al::isActionEnd(this)) {
        IUsePlayerHack* hack = mPlayerHack;
        if (rs::isHoldHackJump(hack) || rs::isHoldHackAction(hack) ||
            rs::getHackMoveStickRaw(hack).length() > 0.1f)
            al::setNerve(this, &NrvPukupuku.CaptureSwim);
        else
            al::setNerve(this, &NrvPukupuku.CaptureWait);
    }
}

void Pukupuku::exeCaptureRolling() {
    if (al::isFirstStep(this)) {
        updateWaterSurfaceMtx(&mWaterSurface, this, mWaterSurfaceFinder);

        al::WaterSurfaceFinder* finder = mWaterSurfaceFinder;
        bool isRollingR = al::isNerve(this, &NrvPukupuku.CaptureRollingR);
        const char* actionR = finder->isFoundSurface() ? "RollingRSurface" : "RollingRWater";
        const char* actionL = finder->isFoundSurface() ? "RollingLSurface" : "RollingLWater";
        al::startAction(this, isRollingR ? actionR : actionL);
    }

    if (checkCollidedFloorDamageAndNextNerve())
        return;

    updateWaterCondition();
    updateVelocity();
    if (_154 >= 11) {
        onWaterOut();
        al::setNerve(this, &NrvPukupuku.CaptureWaitAir);
        return;
    }

    if (al::isGreaterStep(this, 15) && isHackInputActive(mPlayerHack))
        FUN_7100178da4(getAccel(mPlayerHack), this);

    if (al::isGreaterStep(this, 12) && _2c4) {
        const al::Nerve* nerve = &NrvPukupuku.CaptureRollingL;
        if (al::isNerve(this, &NrvPukupuku.CaptureRollingR))
            nerve = &NrvPukupuku.CaptureRollingR;
        al::setNerve(this, nerve);
        return;
    }

    if (al::isActionEnd(this)) {
        if (isHackInputActive(mPlayerHack))
            al::setNerve(this, &NrvPukupuku.CaptureSwim);
        else
            al::setNerve(this, &NrvPukupuku.CaptureWait);
    }
}

bool Pukupuku::updateGroundTimeLimit() {
    if (mGroundTimeLimit < 600)
        mGroundTimeLimit++;

    return mGroundTimeLimit == 600;
}

void FUN_710017b094(Pukupuku* actor, const sead::Vector3f& param_2) {
    sead::Vector3f local_30 = param_2;
    if (!al::tryNormalizeOrZero(&local_30))
        return;

    al::turnQuatZDirRadian(al::getQuatPtr(actor), al::getQuat(actor), local_30,
                           sead::Mathf::deg2rad(10.0f));
    sead::Vector3f frontDir;
    al::calcFrontDir(&frontDir, actor);

    if (al::isParallelDirection(frontDir, sead::Vector3f::ey))
        al::makeQuatUpNoSupport(al::getQuatPtr(actor), sead::Vector3f::ey);
    else
        al::makeQuatUpFront(al::getQuatPtr(actor), sead::Vector3f::ey, frontDir);
}

void Pukupuku::exeCaptureWaitGround() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "WaitGround");
        FUN_710017605c(1.0f, this);
        rs::hideTutorial(this);
    }
    FUN_710017605c(0.04f, this);

    f32 fVar9 = 2.0f;
    f32 fVar8 = -joinRotator.x;
    if (joinRotator.x > 0.0f) {
        fVar9 = -2.0f;
        fVar8 = joinRotator.x;
    }

    joinRotator.x = 2.0f > fVar8 ? 0.0f : joinRotator.x + fVar9;
    updateWaterCondition();
    updateVelocity();

    if (updateGroundTimeLimit()) {
        endCapture();
        revive(2);
        return;
    }

    if (2 < _158) {
        mGroundTimeLimit = 0;
        onWaterIn();
        rs::showTutorial(this);
        al::setNerve(this, &NrvPukupuku.CaptureWait);
        return;
    }

    if (al::isOnGround(this, 0)) {
        sead::Vector3f moveVec;
        rs::calcHackerMoveVec(&moveVec, mPlayerHack, sead::Vector3f::ey);
        IUsePlayerHack* playerHack = mPlayerHack;
        if (!rs::isTriggerHackPreInputAnyButton(playerHack) &&
            !rs::isTriggerHackSwing(playerHack)) {
            al::addVelocity(this, moveVec * 3.0f);
            FUN_710017b094(this, moveVec);
        } else {
            al::startHitReaction(this, "地上ジャンプ開始");
            mHackerStateNormalJump->set_38(15.0f, 30.0f, 2.0f);
            al::setNerve(this, &NrvPukupuku.CaptureJumpGround);
        }
    }
}

void Pukupuku::exeCaptureJumpGround() {
    if (updateGroundTimeLimit()) {
        endCapture();
        revive(2);
        return;
    }

    bool nerveState = al::updateNerveState(this);
    updateWaterCondition();
    al::limitVelocity(this, 50.0f);
    if (al::isGreaterStep(this, 5) && 2 < _158) {
        mGroundTimeLimit = 0;
        rs::showTutorial(this);
        onWaterIn();
        al::setNerve(this, &NrvPukupuku.CaptureWait);
        return;
    }

    if (!nerveState)
        return;

    al::WaterSurfaceFinder* finder = mWaterSurfaceFinder;

    if (al::isInWater(this)) {
        if (!finder->isFoundSurface() || finder->getDistance() >= -30.0f ||
            (al::isCollidedGround(this) && finder->getDistance() >= -80.0f))
            al::updateMaterialCodePuddle(this, true);
    } else {
        if (finder->isFoundSurface() &&
            (finder->getDistance() >= -30.0f ||
             (al::isCollidedGround(this) && finder->getDistance() >= -80.0f)))
            al::updateMaterialCodePuddle(this, true);
    }

    al::WaterSurfaceFinder* finder2 = mWaterSurfaceFinder;
    if (finder2->isFoundSurface()) {
        sead::Matrix34f* waterSurface = &mWaterSurface;
        sead::Vector3f frontDir;
        al::calcFrontDir(&frontDir, this);
        al::makeMtxUpFrontPos(waterSurface, finder2->getSurfaceNormal(), frontDir,
                              finder2->getSurfacePosition());
    } else {
        const sead::Vector3f& trans = al::getTrans(this);
        mWaterSurface.setTranslation(trans);
    }

    al::startAction(this, "Land");
    al::setNerve(this, &NrvPukupuku.CaptureLandGround);
}

void Pukupuku::exeCaptureLandGround() {
    if (updateGroundTimeLimit()) {
        endCapture();
        revive(2);
        return;
    }

    FUN_710017605c(0.04f, this);
    decayRootRotX(&joinRotator);
    updateVelocity();
    updateWaterCondition();

    if (al::isOnGround(this, 0)) {
        sead::Vector3f moveVec;
        rs::calcHackerMoveVec(&moveVec, mPlayerHack, sead::Vector3f::ey);
        IUsePlayerHack* playerHack = mPlayerHack;
        if (rs::isTriggerHackPreInputAnyButton(playerHack) || rs::isTriggerHackSwing(playerHack)) {
            al::startHitReaction(this, "地上ジャンプ開始");
            mHackerStateNormalJump->set_38(15.0f, 30.0f, 2.0f);
            al::setNerve(this, &NrvPukupuku.CaptureJumpGround);
            return;
        }

        al::addVelocity(this, moveVec * 3.0f);
        FUN_710017b094(this, moveVec);
    }

    if (_158 > 2) {
        mGroundTimeLimit = 0;
        onWaterIn();
        al::setNerve(this, &NrvPukupuku.CaptureWait);
        return;
    }

    if (al::isActionEnd(this))
        al::setNerve(this, &NrvPukupuku.CaptureWaitGround);
}

void Pukupuku::exeBlowDown() {
    if (!al::updateNerveState(this))
        return;

    if (!al::isNerve(this, &NrvPukupuku.BlowDownFromCapture) &&
        !al::isNerve(this, &NrvPukupuku.BlowDownWithoutMsg)) {
        al::appearItem(this);
    }

    al::startHitReaction(this, "死亡");
    al::setNerve(this, &NrvPukupuku.Revive);
}

void Pukupuku::exeTrample() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startAction(this, "PressDown");
    }

    if (al::isActionEnd(this)) {
        al::appearItem(this);
        al::startHitReaction(this, "死亡");
        al::setNerve(this, &NrvPukupuku.Revive);
    }
}

void Pukupuku::exeRevive() {
    if (!al::updateNerveStateAndNextNerve(this, &NrvPukupuku.Wait))
        return;

    al::resetMtxPosition(this, mSpawnPosition);
    if (al::isExistRail(this))
        al::setSyncRailToNearestPos(this);
    mPlayerHack = nullptr;
}

void Pukupuku::exeDemoWaitToRevive() {
    if (!rs::isActiveDemo(this)) {
        al::showModelIfHide(this);
        al::setNerve(this, &NrvPukupuku.Revive);
    }
}
