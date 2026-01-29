#include "Enemy/Pukupuku.h"

#include "Library/Area/AreaObj.h"
#include "Library/Area/AreaObjUtil.h"
#include "Library/Base/StringUtil.h"
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
#include "Util/SensorMsgFunction.h"

sead::Vector2f getHackMoveStickRawVec(const IUsePlayerHack* hack) asm(
    "_ZN2rs19getHackMoveStickRawEPK14IUsePlayerHack");

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

inline f32 calcHackMoveStickRawLength(const IUsePlayerHack* hack) {
    return getHackMoveStickRawVec(hack).length();
}

inline bool isHackInputOn(const IUsePlayerHack* hack) {
    return rs::isHoldHackJump(hack) || rs::isHoldHackAction(hack) ||
           calcHackMoveStickRawLength(hack) > 0.1f;
}

inline void decayJointRotateX(sead::Vector3f* rotate) {
    f32 rotateX = rotate->x;
    f32 step = -2.0f;
    f32 absRotateX = -rotateX;
    if (rotateX > 0.0f)
        absRotateX = rotateX;
    else
        step = 2.0f;

    rotateX += step;
    if (absRotateX < 2.0f)
        rotateX = 0.0f;

    rotate->x = rotateX;
}

inline void updateWaterSurfaceEffectMtx(sead::Matrix34f* mtx, al::WaterSurfaceFinder* finder,
                                        al::LiveActor* actor) {
    if (finder->isFoundSurface())
        al::calcMatrixFromActorPoseAndWaterSurfaceH(mtx, finder, actor);
    else
        mtx->setBase(3, al::getTrans(actor));
}
}  // namespace

static sead::Vector3f g_7101e62d50 = {0.0f, 50.0f, 50.0f};
static al::EnemyStateBlowDownParam g_7101e62d10 = {"BlowDown", 8.0f, 13.0f, 0.5f, 1.0f, 120, true};
static al::EnemyStateBlowDownParam g_7101e62d30 = {"BlowDown", 16.0f, 32.0f, 1.0f,
                                                   0.95f,      120,   true};
static PlayerHackStartShaderParam g_7101e62d5c = {true, -1.0f, 10, 20};

const struct {
    f32 stability;
    f32 friction;
    f32 limitDegree;
} g_71018a3dac[] = {
    {0.5f, 0.1f, 45.0f}, {0.5f, 0.5f, 45.0f}, {0.3f, 0.2f, 15.0f}, {0.3f, 0.2f, 15.0f}};

const char* const g_7101ca5700[] = {"Tail1", "Tail2", "WingLeft", "WingRight"};

bool FUN_7100177c74(sead::Vector3f* out, al::LiveActor* actor);
void FUN_710017b094(Pukupuku* param_1, const sead::Vector3f& param_2);

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

    EnemyStateSwoonInitParam enemyStateSwoonInitParam("SwoonStart", "SwoonLoop", "SwoonEnd",
                                                      nullptr, "SwoonStartFall",
                                                      "SwoonStartLand");
    enemyStateSwoonInitParam.swoonDuration = 300;
    enemyStateSwoonInitParam.isCancelLoopOnProhibitedArea = true;
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
    al::initJointLocalRotator(this, &_168, "AllRoot");
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
    al::setEffectFollowMtxPtr(this, "WaterAreaIn", &mWaterAreaInEffectFollowMtx);
    al::setEffectFollowMtxPtr(this, "WaterAreaOut", &mWaterAreaOutEffectFollowMtx);
    al::setEffectNamedMtxPtr(this, "WaterSurface", &mWaterSurfaceEffectFollowMtx);

    al::offCollide(this);

    al::makeMtxQuatPos(&_1a0, al::getQuat(this), al::getTrans(this));

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

// void Pukupuku::attackSensor(al::HitSensor* self, al::HitSensor* other) {}

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

// bool Pukupuku::receiveMsg(const al::SensorMsg* message, al::HitSensor* other, al::HitSensor*
// self) {}

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

    _168.set(sead::Vector3f::zero);
    _150 = 0;
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

// NON_MATCHING
void Pukupuku::updateWaterCondition() {
    const al::LiveActor* actor = this;
    const al::IUseAreaObj* areaUser = actor != nullptr ? actor : nullptr;
    al::AreaObj* waterArea = al::tryFindAreaObj(areaUser, "WaterArea", al::getTrans(this));
    if (waterArea) {
        sead::Vector3f nearestEdge;
        if (al::calcNearestAreaObjEdgePos(&nearestEdge, waterArea, al::getTrans(this))) {
            f32 nearestEdgeX = nearestEdge.x;
            f32 nearestEdgeY = nearestEdge.y;
            f32 nearestEdgeZ = nearestEdge.z;
            const sead::Vector3f& trans = al::getTrans(this);
            f32 edgeOffsetX = nearestEdgeX - trans.x;
            f32 edgeOffsetY = nearestEdgeY - trans.y;
            f32 edgeOffsetZ = nearestEdgeZ - trans.z;
            if (edgeOffsetX * edgeOffsetX + edgeOffsetY * edgeOffsetY +
                    edgeOffsetZ * edgeOffsetZ <
                10000.0f)
                _290.set(nearestEdge);
        }
    }

    if (al::isInWater(this))
        _29c.set(al::getTrans(this));
    else
        _2a8.set(al::getTrans(this));

    mWaterSurfaceFinder->update(al::getTrans(this), -al::getGravity(this), 200.0f);

    al::WaterSurfaceFinder* waterSurfaceFinder = mWaterSurfaceFinder;
    bool isMaterialWater;
    bool isMaterialPuddle = false;
    if (al::isInWater(this) && !waterSurfaceFinder->isFoundSurface()) {
        isMaterialWater = true;
    } else if (!waterSurfaceFinder->isFoundSurface()) {
        isMaterialWater = false;
    } else if (!(waterSurfaceFinder->getDistance() < -30.0f)) {
        isMaterialWater = true;
    } else if (al::isCollidedGround(this) && waterSurfaceFinder->getDistance() >= -80.0f) {
        isMaterialWater = false;
        isMaterialPuddle = true;
    } else {
        isMaterialWater = false;
    }

    if (isMaterialWater) {
        _158++;
        _154 = 0;
    } else {
        _154++;
        _158 = 0;
    }

    if (mWaterSurfaceFinder->isFoundSurface()) {
        const sead::Vector3f& surfacePosition = mWaterSurfaceFinder->getSurfacePosition();
        _2e8.x = surfacePosition.x;
        _2e8.y = surfacePosition.y;
        _2e8.z = surfacePosition.z;
    }

    if (al::isCollidedGround(this)) {
        al::setMaterialCode(this, al::getCollidedFloorMaterialCodeName(this));
        al::updateMaterialCodeWater(this, isMaterialWater);
        al::updateMaterialCodePuddle(this, isMaterialPuddle);
    }
}

// void Pukupuku::control() {}

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

// NON_MATCHING
void Pukupuku::updateInputUpDown() {
    if (!FUN_7100175f24(this))
        return;

    IUsePlayerHack* playerHack = mPlayerHack;
    bool isHoldDashInput = false;
    if (rs::isHoldHackAction(playerHack))
        isHoldDashInput = rs::isHoldHackJump(playerHack);

    bool isSkipInput = false;
    if (_2d9) {
        mIsTriggerSwimDash = false;
        if (isHoldDashInput)
            isSkipInput = true;
        else
            _2d9 = false;
    } else if (isHoldDashInput) {
        auto isSwimDashFlagsClear = [](u16 flags) {
            u16 lowFlags = flags & 0xff;
            if (lowFlags != 0)
                return false;
            return flags <= 0xff;
        };
        if (isSwimDashFlagsClear(_2d8))
            _2d8 = 0x101;
        isSkipInput = true;
    }

    if (!isSkipInput) {
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

// NON_MATCHING
void Pukupuku::updateVelocity() {
    if (isNerveInWater()) {
        f32 speedRate = sead::Mathf::clamp(al::getVelocity(this).length() / 15.0f, 0.0f, 1.0f);
        bool isStickStop = true;
        if (mPlayerHack)
            isStickStop = !(calcHackMoveStickRawLength(mPlayerHack) > 0.1f);

        sead::Vector3f frontDir;
        al::calcFrontDir(&frontDir, this);

        bool isNearGround = false;
        if (al::isCollidedGround(this)) {
            f32 dot = (-sead::Vector3f::ey).dot(frontDir);
            sead::Mathf::cos(sead::Mathf::pi() / 12.0f);
            isNearGround = dot > 0.9659258f;
        }

        bool isNearCeiling = false;
        if (al::isCollidedCeiling(this)) {
            f32 dot = sead::Vector3f::ey.dot(frontDir);
            sead::Mathf::cos(sead::Mathf::pi() / 12.0f);
            isNearCeiling = dot > 0.9659258f;
        }

        f32 scale = speedRate * 0.05f + 0.92f;
        if (isStickStop && (isNearGround || isNearCeiling))
            scale = 0.1f;
        al::scaleVelocity(this, scale);

        al::limitVelocity(this, 20.0f);
        return;
    }

    al::addVelocityToGravity(this, 2.0f);
    if (al::isOnGround(this, 0) && !al::isCollidedFloorCode(this, "Slide"))
        al::scaleVelocity(this, 0.5f);
    else
        al::scaleVelocity(this, 0.998f);

    al::limitVelocity(this, 50.0f);
}

void Pukupuku::exeReaction() {
    if (al::isFirstStep(this))
        al::startAction(this, "SwimReaction");

    if (al::isActionEnd(this))
        al::setNerve(this, &NrvPukupuku.Wait);
}

// NON_MATCHING
void Pukupuku::exeWaitRollingRail() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        _19c = true;

        sead::Vector3f railSide;
        if (FUN_7100177c74(&railSide, this) && sead::Vector3f::ey.dot(railSide) > 0.0f)
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
        f32 crossY = frontDir.z * _178.x - _178.z * frontDir.x;
        if ((_19c && crossY > 0.0f) || (!_19c && crossY < 0.0f))
            angle = 360.0f - angle;

        if (angle > 0.0f && (angle <= _184 || al::isNearZero(_184 - angle, 0.001f))) {
            f32 rate = al::calcNerveRate(this, al::getActionFrameMax(this, "RollingRail"));
            f32 turn = sead::Mathf::clamp(angle * rate * rate, 0.0f, 45.0f);
            if (_19c)
                turn = -turn;
            al::rotateQuatYDirDegree(al::getQuatPtr(this), al::getQuat(this), turn);
            _184 = angle;
        }
    }

    if (al::isGreaterStep(this, 20))
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

// NON_MATCHING
void Pukupuku::exeWait() {
    updateWaterCondition();

    if (al::isFirstStep(this)) {
        if (mWaterSurfaceFinder->isFoundSurface())
            al::startAction(this, "SwimSurfaceEnemy");
        else
            al::startAction(this, "SwimWaitWater");
    }

    if (mWaterSurfaceFinder->isFoundSurface() && al::getNerveStep(this) % 3 == 0)
        al::tryAddRippleTiny(this);

    if (al::isExistRail(this)) {
        if (al::isActionEnd(this) && (al::isActionPlaying(this, "RollingRail") ||
                                      al::isActionPlaying(this, "RollingRailReverse"))) {
            if (mWaterSurfaceFinder->isFoundSurface())
                al::startAction(this, "SwimSurfaceEnemy");
            else
                al::startAction(this, "SwimWaitWater");
        }

        s32 railPointNo = al::getRailPointNo(this);
        if (mMoveType == 0)
            al::moveSyncRailLoop(this, 2.0f);
        else if (mMoveType == 1)
            al::moveSyncRailTurn(this, 2.0f);
        else
            al::moveSyncRail(this, 2.0f);

        if (mMoveType == 0 && mRailPointNo != railPointNo && !al::isRailReachedGoal(this)) {
            mRailPointNo = railPointNo;
            sead::Vector3f railSide;
            if (FUN_7100177c74(&railSide, this)) {
                if (sead::Vector3f::ey.dot(railSide) > 0.0f)
                    al::startAction(this, "RollingRailReverse");
                else
                    al::startAction(this, "RollingRail");
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

// f32 Pukupuku::getAccel(IUsePlayerHack*) const {}

// void Pukupuku::exeCaptureSwimStart() {}

void Pukupuku::onWaterOut() {
    if (al::isInWater(this))
        return;

    AreaObjFilterWaterIgnore filterWaterIgnore;
    AreaObjFilterWater filterWater;
    al::AreaObj* currentArea =
        al::tryFindAreaObjWithFilter(this, "WaterArea", al::getTrans(this), &filterWaterIgnore);
    al::AreaObj* previousArea = al::tryFindAreaObjWithFilter(this, "WaterArea", _29c, &filterWater);

    al::AreaObj* area = currentArea;
    sead::Vector3f pos1 = al::getTrans(this);
    sead::Vector3f pos2 = _29c;
    bool isReverseNormal;

    if (area && !previousArea) {
        pos1.set(_29c);
        pos2.set(al::getTrans(this));
        isReverseNormal = true;
    } else {
        isReverseNormal = false;
        if (!area || !previousArea) {
            area = previousArea;
        } else if (area->isInVolume(pos1) && area->isInVolume(pos2)) {
            area = previousArea;
        } else {
            pos1.set(_29c);
            pos2.set(al::getTrans(this));
            isReverseNormal = true;
        }
    }

    if (!area)
        return;

    sead::Vector3f hitPosition;
    sead::Vector3f normal;
    if (!al::checkAreaObjCollisionByArrow(&hitPosition, &normal, area, pos1, pos2))
        return;

    sead::Vector3f frontDir;
    al::calcFrontDir(&frontDir, this);
    if (al::isParallelDirection(frontDir, normal, 0.01f))
        al::calcUpDir(&frontDir, this);
    if (isReverseNormal)
        normal.negate();

    al::makeMtxFrontUpPos(&mWaterAreaOutEffectFollowMtx, normal, frontDir, hitPosition);
    al::emitEffect(this, "WaterAreaOut", nullptr);
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
void FUN_7100178da4(f32 param_1, Pukupuku* param_2) {
    sead::Vector3f frontDir;
    al::calcFrontDir(&frontDir, param_2);

    sead::Vector3f velocity;
    velocity.set(frontDir);

    const f32 dotLimit = 0.9659258f;
    f32 dot = frontDir.dot(sead::Vector3f::ey);
    sead::Mathf::cos(sead::Mathf::pi() / 12.0f);
    if (!(dot > /* sead::Mathf::cos(sead::Mathf::pi() / 12.0f) */ dotLimit)) {
        dot = (-frontDir).dot(sead::Vector3f::ey);
        sead::Mathf::cos(sead::Mathf::pi() / 12.0f);
        if (dot > /* sead::Mathf::cos(sead::Mathf::pi() / 12.0f) */ dotLimit)
            velocity.set(-sead::Vector3f::ey);
    } else {
        velocity.set(sead::Vector3f::ey);
    }

    al::addVelocity(param_2, velocity * param_1);
}

void Pukupuku::approachSurface() {
    sead::Vector3f upDir;
    al::calcUpDir(&upDir, this);

    f32 angle = sead::Mathf::clamp((al::calcAngleDegree(upDir, sead::Vector3f::ey) - 15.0f) / 30.0f,
                                   0.0f, 1.0f);

    f32 invAngle = 1.0f - angle;
    f32 surfaceSpring = invAngle * 0.008f + angle * 0.002f;
    f32 surfaceDamp = invAngle * 0.9f + angle * 0.988f;

    al::approachWaterSurfaceSpringDumper(this, mWaterSurfaceFinder, 5.0f, 12.0f, 1.0f,
                                         surfaceSpring, surfaceDamp);
}

// bool Pukupuku::updatePoseSwim() {}

bool Pukupuku::isTriggerSwimDash() const {
    return mIsTriggerSwimDash;
}

// void Pukupuku::onWaterIn() {}

// void Pukupuku::exeCaptureSwim() {}

// void Pukupuku::exeCaptureReactionWall() {}

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
    f32 inputUpDown = _2c0;
    if (inputUpDown >= -0.5f)
        return false;
    if (_2e0 == 30)
        return true;
    if (rs::isTriggerHackSwing(mPlayerHack))
        return true;

    bool isInput = isHackInputOn(mPlayerHack);
    bool isEnable = isInput && !al::isNerve(this, &NrvPukupuku.CaptureSwimDash);
    isEnable = isEnable || al::isNerve(this, &NrvPukupuku.CaptureSwimDash);

    if (al::getVelocity(this).y > 10.0f)
        return inputUpDown < -0.1f && isEnable;

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

// NON_MATCHING
void Pukupuku::exeCaptureWait() {
    if (al::isFirstStep(this)) {
        if (al::isNerve(this, &NrvPukupuku.CaptureWaitTurnStart)) {
            al::startAction(this, mWaterSurfaceFinder->isFoundSurface() ? "SwimWaitStartSurface" :
                                                                          "SwimWaitStartWater");
        } else {
            al::tryStartActionIfNotPlaying(this, mWaterSurfaceFinder->isFoundSurface() ?
                                                     "SwimWaitSurface" :
                                                     "SwimWaitWaterHack");
        }
    }

    updateWaterCondition();

    f32 stick = rs::getHackMoveStickRaw(mPlayerHack);
    f32 stickAbs = sead::Mathf::abs(stick);
    if (al::isNerve(this, &NrvPukupuku.CaptureWait)) {
        if (stickAbs > 0.2f) {
            al::setNerve(this, &NrvPukupuku.CaptureWaitTurnStart);
            return;
        }
    } else if (al::isNerve(this, &NrvPukupuku.CaptureWaitTurnStart)) {
        if (al::isActionPlaying(this, "SwimWaitStartWater") ||
            al::isActionPlaying(this, "SwimWaitStartSurface")) {
            if (al::isActionEnd(this)) {
                if (stickAbs > 0.2f)
                    al::setNerve(this, &NrvPukupuku.CaptureWaitTurn);
                else
                    al::setNerve(this, &NrvPukupuku.CaptureWait);
                return;
            }
        } else if (stickAbs <= 0.2f) {
            al::setNerve(this, &NrvPukupuku.CaptureWait);
            return;
        }
    } else if (al::isNerve(this, &NrvPukupuku.CaptureWaitTurn) && stickAbs <= 0.2f) {
        al::setNerve(this, &NrvPukupuku.CaptureWait);
        return;
    }

    if (isNerveInWater()) {
        if (mIsTriggerSwimDash) {
            bool isSwingLeft = rs::isTriggerHackSwingLeftHand(mPlayerHack);
            if (mWaterSurfaceFinder->isFoundSurface())
                al::startAction(this, isSwingLeft ? "DashLSurface" : "DashRSurface");
            else
                al::startAction(this, isSwingLeft ? "DashLWater" : "DashRWater");
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

        if (isHackInputOn(mPlayerHack)) {
            al::setNerve(this, &NrvPukupuku.CaptureSwimStart);
            return;
        }

        if (checkJumpOutCondition()) {
            sead::Vector3f frontDir;
            al::calcFrontDir(&frontDir, this);
            frontDir.y = 0.0f;
            if (al::tryNormalizeOrZero(&frontDir))
                al::setVelocity(this, sead::Vector3f::ey * 65.0f + frontDir * 15.0f);

            al::startHitReaction(this, "ジャンプ");
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

        updateWaterSurfaceEffectMtx(&mWaterSurfaceEffectFollowMtx, mWaterSurfaceFinder, this);
        FUN_710017605c(1.0f, this);
        al::startAction(this, "Land");
        al::setNerve(this, &NrvPukupuku.CaptureLandGround);
        return;
    } else if ((!al::isNerve(this, &NrvPukupuku.CaptureJumpOut) || al::isGreaterStep(this, 30)) &&
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
    decayJointRotateX(&_168);

    if (isNerveInWater()) {
        approachSurface();
        if (al::isActionPlaying(this, "SwimWaitWaterHack")) {
            if (mWaterSurfaceFinder->isFoundSurface())
                al::startAction(this, "SwimWaitSurface");
        } else if (al::isActionPlaying(this, "SwimWaitSurface") &&
                   !mWaterSurfaceFinder->isFoundSurface()) {
            al::startAction(this, "SwimWaitWaterHack");
        }
    }
}

// NON_MATCHING
void Pukupuku::exeCaptureReactionWall() {
    if (al::isFirstStep(this))
        al::startAction(this, "ReactionWall");

    if (checkCollidedFloorDamageAndNextNerve())
        return;

    updateWaterCondition();
    updateVelocity();
    approachSurface();

    if (al::isActionEnd(this)) {
        if (isHackInputOn(mPlayerHack))
            al::setNerve(this, &NrvPukupuku.CaptureSwim);
        else
            al::setNerve(this, &NrvPukupuku.CaptureWait);
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
        if (isHackInputOn(mPlayerHack))
            al::setNerve(this, &NrvPukupuku.CaptureSwim);
        else
            al::setNerve(this, &NrvPukupuku.CaptureWait);
    }
}

// NON_MATCHING
void Pukupuku::exeCaptureRolling() {
    if (al::isFirstStep(this)) {
        updateWaterSurfaceEffectMtx(&mWaterSurfaceEffectFollowMtx, mWaterSurfaceFinder, this);
        bool isRollingR = al::isNerve(this, &NrvPukupuku.CaptureRollingR);
        if (isRollingR) {
            al::startAction(this, mWaterSurfaceFinder->isFoundSurface() ? "RollingRSurface" :
                                                                          "RollingRWater");
        } else {
            al::startAction(this, mWaterSurfaceFinder->isFoundSurface() ? "RollingLSurface" :
                                                                          "RollingLWater");
        }
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

    if (al::isGreaterStep(this, 15) && isHackInputOn(mPlayerHack))
        FUN_7100178da4(getAccel(mPlayerHack), this);

    if (al::isGreaterStep(this, 12) && _2c4) {
        if (al::isNerve(this, &NrvPukupuku.CaptureRollingR))
            al::setNerve(this, &NrvPukupuku.CaptureRollingR);
        else
            al::setNerve(this, &NrvPukupuku.CaptureRollingL);
        return;
    }

    if (al::isActionEnd(this)) {
        if (isHackInputOn(mPlayerHack))
            al::setNerve(this, &NrvPukupuku.CaptureSwim);
        else
            al::setNerve(this, &NrvPukupuku.CaptureWait);
    }
}

bool Pukupuku::updateGroundTimeLimit() {
    if (_150 < 600)
        _150++;

    return _150 == 600;
}

void Pukupuku::exeCaptureWaitGround() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "WaitGround");
        FUN_710017605c(1.0f, this);
        rs::hideTutorial(this);
    }

    FUN_710017605c(0.04f, this);
    decayJointRotateX(&_168);
    updateWaterCondition();
    updateVelocity();

    if (updateGroundTimeLimit()) {
        endCapture();
        revive(2);
        return;
    }

    if (_158 >= 3) {
        _150 = 0;
        onWaterIn();
        rs::showTutorial(this);
        al::setNerve(this, &NrvPukupuku.CaptureWait);
        return;
    }

    if (!al::isOnGround(this, 0))
        return;

    sead::Vector3f moveVec;
    rs::calcHackerMoveVec(&moveVec, mPlayerHack, sead::Vector3f::ey);
    IUsePlayerHack* playerHack = mPlayerHack;
    if (rs::isTriggerHackPreInputAnyButton(playerHack) || rs::isTriggerHackSwing(playerHack)) {
        al::startHitReaction(this, "ジャンプ");
        mHackerStateNormalJump->set_38({15.0f, 30.0f, 2.0f});
        al::setNerve(this, &NrvPukupuku.CaptureJumpGround);
        return;
    }

    al::addVelocity(this, moveVec * 3.0f);
    FUN_710017b094(this, moveVec);
}

void FUN_710017b094(Pukupuku* param_1, const sead::Vector3f& param_2) {
    sead::Vector3f local_30 = param_2;
    if (!al::tryNormalizeOrZero(&local_30))
        return;

    al::turnQuatZDirRadian(al::getQuatPtr(param_1), al::getQuat(param_1), local_30,
                           sead::Mathf::deg2rad(10.0f));
    sead::Vector3f frontDir;
    al::calcFrontDir(&frontDir, param_1);

    if (al::isParallelDirection(frontDir, sead::Vector3f::ey))
        al::makeQuatUpNoSupport(al::getQuatPtr(param_1), sead::Vector3f::ey);
    else
        al::makeQuatUpFront(al::getQuatPtr(param_1), sead::Vector3f::ey, frontDir);
}

// NON_MATCHING
void Pukupuku::exeCaptureJumpGround() {
    if (updateGroundTimeLimit()) {
        endCapture();
        revive(2);
        return;
    }

    bool isStateEnd = al::updateNerveState(this);
    updateWaterCondition();
    al::limitVelocity(this, 50.0f);

    if (al::isGreaterStep(this, 5) && _158 >= 3) {
        _150 = 0;
        rs::showTutorial(this);
        onWaterIn();
        al::setNerve(this, &NrvPukupuku.CaptureWait);
        return;
    }

    if (!isStateEnd)
        return;

    if (al::isInWater(this)) {
        if (!mWaterSurfaceFinder->isFoundSurface())
            al::updateMaterialCodePuddle(this, true);
    } else if (mWaterSurfaceFinder->isFoundSurface()) {
        if (mWaterSurfaceFinder->getDistance() >= -30.0f ||
            (al::isCollidedGround(this) && mWaterSurfaceFinder->getDistance() >= -80.0f)) {
            al::updateMaterialCodePuddle(this, true);
        }
    }

    updateWaterSurfaceEffectMtx(&mWaterSurfaceEffectFollowMtx, mWaterSurfaceFinder, this);
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
    decayJointRotateX(&_168);
    updateVelocity();
    updateWaterCondition();

    if (al::isOnGround(this, 0)) {
        sead::Vector3f moveVec;
        rs::calcHackerMoveVec(&moveVec, mPlayerHack, sead::Vector3f::ey);
        IUsePlayerHack* playerHack = mPlayerHack;
        if (rs::isTriggerHackPreInputAnyButton(playerHack) || rs::isTriggerHackSwing(playerHack)) {
            al::startHitReaction(this, "ジャンプ");
            mHackerStateNormalJump->set_38({15.0f, 30.0f, 2.0f});
            al::setNerve(this, &NrvPukupuku.CaptureJumpGround);
            return;
        }

        al::addVelocity(this, moveVec * 3.0f);
        FUN_710017b094(this, moveVec);
    }

    if (_158 >= 3) {
        _150 = 0;
        onWaterIn();
        al::setNerve(this, &NrvPukupuku.CaptureWait);
        return;
    }

    if (al::isActionEnd(this))
        al::setNerve(this, &NrvPukupuku.CaptureWaitGround);
}

void Pukupuku::exeBlowDown() {
    if (al::updateNerveState(this)) {
        if (!al::isNerve(this, &NrvPukupuku.BlowDownFromCapture) &&
            !al::isNerve(this, &NrvPukupuku.BlowDownWithoutMsg)) {
            al::appearItem(this);
        }

        al::startHitReaction(this, "死亡");
        al::setNerve(this, &NrvPukupuku.Revive);
    }
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
    if (al::updateNerveStateAndNextNerve(this, &NrvPukupuku.Wait)) {
        al::resetMtxPosition(this, _1a0);
        if (al::isExistRail(this))
            al::setSyncRailToNearestPos(this);
        mPlayerHack = nullptr;
    }
}

void Pukupuku::exeDemoWaitToRevive() {
    if (!rs::isActiveDemo(this)) {
        al::showModelIfHide(this);
        al::setNerve(this, &NrvPukupuku.Revive);
    }
}
