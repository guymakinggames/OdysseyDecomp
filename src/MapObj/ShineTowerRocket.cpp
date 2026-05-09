#include "MapObj/ShineTowerRocket.h"

#include <new>

#include "Library/Base/Macros.h"
#include "Library/Base/StringUtil.h"
#include "Library/Area/AreaShapeCube.h"
#include "Library/Bgm/BgmLineFunction.h"
#include "Library/Camera/CameraUtil.h"
#include "Library/Collision/CollisionPartsKeeperUtil.h"
#include "Library/Collision/PartsConnectorUtil.h"
#include "Library/Demo/DemoFunction.h"
#include "Library/Effect/EffectSystemInfo.h"
#include "Library/Event/EventFlowFunction.h"
#include "Library/Event/EventFlowUtil.h"
#include "Library/Joint/JointControllerKeeper.h"
#include "Library/Layout/LayoutActorUtil.h"
#include "Library/Message/MessageHolder.h"
#include "Library/LiveActor/ActorActionFunction.h"
#include "Library/LiveActor/ActorAreaFunction.h"
#include "Library/LiveActor/ActorAnimFunction.h"
#include "Library/LiveActor/ActorClippingFunction.h"
#include "Library/LiveActor/ActorCollisionFunction.h"
#include "Library/LiveActor/ActorFlagFunction.h"
#include "Library/LiveActor/ActorInitFunction.h"
#include "Library/LiveActor/ActorInitUtil.h"
#include "Library/LiveActor/ActorModelFunction.h"
#include "Library/LiveActor/ActorMovementFunction.h"
#include "Library/LiveActor/ActorPoseUtil.h"
#include "Library/LiveActor/ActorResourceFunction.h"
#include "Library/LiveActor/ActorSceneFunction.h"
#include "Library/LiveActor/ActorSensorUtil.h"
#include "Library/LiveActor/LiveActorFunction.h"
#include "Library/LiveActor/LiveActorGroup.h"
#include "Library/Math/MathUtil.h"
#include "Library/Math/RateParam.h"
#include "Library/Matrix/MatrixUtil.h"
#include "Library/Nerve/NerveSetupUtil.h"
#include "Library/Nerve/NerveUtil.h"
#include "Library/Obj/CollisionObj.h"
#include "Library/Obj/PartsModel.h"
#include "Library/Play/Layout/WipeSimple.h"
#include "Library/Placement/PlacementFunction.h"
#include "Library/Player/PlayerUtil.h"
#include "Library/Resource/ResourceFunction.h"
#include "Library/Se/SeFunction.h"
#include "Library/Shadow/ActorShadowUtil.h"
#include "Library/Stage/StageSwitchUtil.h"
#include "Library/Thread/FunctorV0M.h"

#include "Demo/DemoPlayer.h"
#include "Layout/ShopLayoutInfo.h"
#include "MapObj/CapHanger.h"
#include "MapObj/CapMessageShowInfo.h"
#include "MapObj/ChangeStageInfo.h"
#include "MapObj/CheckpointFlag.h"
#include "MapObj/DamageModel.h"
#include "MapObj/DemoShine.h"
#include "MapObj/DoorAreaChange.h"
#include "MapObj/Dokan.h"
#include "MapObj/DokanPuppetController.h"
#include "MapObj/GoalMark.h"
#include "MapObj/KoopaShipDemoRequester.h"
#include "MapObj/LinkObjAliveDeadCtrl.h"
#include "MapObj/PlayerStartInfoHolder.h"
#include "MapObj/ShineTowerBackDoor.h"
#include "MapObj/ShineTowerGlobeAnimCtrl.h"
#include "MapObj/ShineTowerCommonKeeper.h"
#include "MapObj/ShineTowerLight.h"
#include "MapObj/ShineTowerRocketFunction.h"
#include "Npc/Bird.h"
#include "Player/CapTargetInfo.h"
#include "Scene/WipeHolderRequester.h"
#include "System/GameDataFunction.h"
#include "System/GameDataHolder.h"
#include "System/GameDataHolderAccessor.h"
#include "System/GameDataHolderWriter.h"
#include "System/GameDataUtil.h"
#include "Util/ClothUtil.h"
#include "Util/CapManHeroDemoUtil.h"
#include "Util/DemoUtil.h"
#include "Util/Hack.h"
#include "Util/InputInterruptTutorialUtil.h"
#include "Util/ItemUtil.h"
#include "Util/NpcEventFlowUtil.h"
#include "Util/PlayerDemoUtil.h"
#include "Util/PlayerUtil.h"
#include "Util/PlayerPuppetFunction.h"
#include "Util/SensorMsgFunction.h"
#include "Util/SpecialBuildUtil.h"

namespace {
void invalidateDitherAnimAll(al::LiveActor*);
void validateDitherAnimAll(al::LiveActor*);
void invalidateOcclusionQueryAll(al::LiveActor*);
void validateOcclusionQueryAll(al::LiveActor*);
void resetSubActorPositionAll(al::LiveActor*);
void setModelAlphaMaskSubActorAll(al::LiveActor*, f32);
__attribute__((used, noinline)) void setupCapTargetInfoForHomeGlobe(sead::Vector3f*, CapTargetInfo*,
                                                                    al::LiveActor*, al::LiveActor*);
__attribute__((noinline)) s32 calcRestShineNum(const al::LiveActor*);
__attribute__((noinline)) bool isDamageHomeModel(const al::LiveActor*);
__attribute__((noinline)) bool isNeedShowHomeSkyMessage(const ShineTowerRocket*);
__attribute__((used, noinline)) bool isPayShineEnoughForUnlock(const al::LiveActor*);
__attribute__((noinline)) bool isNeedDemoWalkPlayerToPoint(const al::LiveActor*);
__attribute__((noinline)) bool isEnableUnlockWorldByPayShine(const al::LiveActor*, s32, s32);
__attribute__((used, noinline)) f32 calcHomeMeterAnimFrame(const al::LiveActor*, s32);
__attribute__((used, noinline)) bool isHomeMeterComplete(const al::LiveActor*);

static const sead::Vector3f sEntranceCameraOffset = {0.0f, 0.0f, 0.0f};
static const sead::Vector3f sEntranceCameraOffsetWithRock = {0.0f, 200.0f, 0.0f};
static const sead::Vector3f sEntranceAreaOffset = {0.0f, 0.0f, 250.0f};
static const sead::Vector3f sFixDoorwayCameraPosOffset = {0.0f, 500.0f, 1400.0f};
static const sead::Vector3f sFixDoorwayCameraAtOffset = {0.0f, 400.0f, 0.0f};
static const sead::Vector3f sWorldMapCameraAtOffset = {-46.0f, -37.0f, -1000.0f};
static const sead::Vector3f sWorldMapCameraPosOffset = {-41.0f, -36.0f, 1700.0f};
static const sead::Vector3f sPayCameraPosOffset = {0.0f, 200.0f, 2700.0f};
static const sead::Vector3f sPayCameraAtOffset = {0.0f, 760.0f, 0.0f};
static const sead::Vector3f sGoalMarkOffset = {0.0f, 1000.0f, 0.0f};
static const sead::Vector3f sClippingOffset = {-0.0f, -0.0f, -0.0f};
static const sead::Vector3f sCapHangerOffsetFlag = {0.0f, 60.0f, 0.0f};
static const sead::Vector3f sCapHangerOffsetMuffler2L00 = {0.0f, 135.0f, 0.0f};
static const sead::Vector3f sCapHangerOffsetMuffler2L01 = {0.0f, 85.0f, 0.0f};
static const sead::Vector3f sCapHangerOffsetMuffler2R00 = {0.0f, 83.0f, 0.0f};
static const sead::Vector3f sCapHangerOffsetMuffler2R01 = {0.0f, 115.0f, 0.0f};
static const sead::Vector3f* const sCapHangerOffsets[] = {
    &sCapHangerOffsetFlag,       &sCapHangerOffsetMuffler2L00,
    &sCapHangerOffsetMuffler2L01, &sCapHangerOffsetMuffler2R00,
    &sCapHangerOffsetMuffler2R01,
};

ALWAYS_INLINE void resumeActiveBgmForReceiveEvent(const ShineTowerRocket& actor) {
    al::resumeActiveBgm(&actor, 180);
}

NERVE_IMPL(ShineTowerRocket, Wait);

class ShineTowerRocketNrvWorldMap : public al::Nerve {
public:
    void execute(al::NerveKeeper* keeper) const override {
        ShineTowerRocket* rocket = keeper->getParent<ShineTowerRocket>();
        if (al::isFirstStep(rocket))
            rs::setDemoInfoDemoName(rocket, "ワールドマップデモ");

        if (rocket->getShineTowerLight() && al::isGreaterStep(rocket, 2))
            rocket->getShineTowerLight()->setLightingWorldMap(true);
    }
};

NERVE_IMPL(ShineTowerRocket, DemoAppearPlayerFromHome);
NERVE_IMPL(ShineTowerRocket, DemoReturnToHome);
NERVE_IMPL(ShineTowerRocket, DemoAppearFromEntrance);
NERVE_IMPL(ShineTowerRocket, WaitDemo);
NERVE_IMPL_(ShineTowerRocket, WaitIgnoreLockOn, Wait);
NERVE_IMPL_(ShineTowerRocket, WaitAfterReturnToHome, Wait);
NERVE_IMPL(ShineTowerRocket, Reaction);
NERVE_IMPL(ShineTowerRocket, NoStartEarth);
NERVE_IMPL(ShineTowerRocket, NoStartAndCoin);
NERVE_IMPL(ShineTowerRocket, DemoPrepare);
NERVE_IMPL_(ShineTowerRocket, DemoPrepareNoShine, DemoPrepare);
NERVE_IMPL(ShineTowerRocket, GoToWorldMapWithCamera);
NERVE_IMPL(ShineTowerRocket, DemoWorldTakeoff);
NERVE_IMPL(ShineTowerRocket, DemoWorldTakeoffNext);
NERVE_IMPL_(ShineTowerRocket, DemoWalkPlayerToPointNoShine, DemoWalkPlayerToPoint);
NERVE_IMPL(ShineTowerRocket, DemoAppearShine);
NERVE_IMPL(ShineTowerRocket, DemoMeterRotate);
NERVE_IMPL(ShineTowerRocket, NoStart);
NERVE_IMPL(ShineTowerRocket, GoToWorldMapWithFade);
NERVE_IMPL(ShineTowerRocket, DemoSelectGoOtherWorld);
NERVE_IMPL(ShineTowerRocket, DemoWaitAfterAppearShine);
NERVE_IMPL(ShineTowerRocket, DemoWaitBeforeScaleUpDirect);
NERVE_IMPL(ShineTowerRocket, DemoMeterUpPrev);
NERVE_IMPL(ShineTowerRocket, DemoScaleUp);
NERVE_IMPL(ShineTowerRocket, DemoInformNewItem);
NERVE_IMPL(ShineTowerRocket, DemoInformPeachCastleCap);
NERVE_IMPL(ShineTowerRocket, DemoMeterUp);
NERVE_IMPL(ShineTowerRocket, DemoAwardMoon);
NERVE_IMPL(ShineTowerRocket, DemoUpLevel);
NERVE_IMPL(ShineTowerRocket, DemoInformPowerUpMessage);
NERVE_IMPL(ShineTowerRocket, DemoKoopaShipFade);
NERVE_IMPL(ShineTowerRocket, DemoUpLevelWaitFade);
NERVE_IMPL(ShineTowerRocket, DemoUpLevelOpenFade);
NERVE_IMPL(ShineTowerRocket, DemoInformNewHome);
NERVE_IMPL(ShineTowerRocket, DemoInformNewHomeMessage);
NERVE_IMPL(ShineTowerRocket, DemoInformCompleteShineFadeWait);
NERVE_IMPL(ShineTowerRocket, DemoInformCompleteShine);
NERVE_IMPL(ShineTowerRocket, DemoUpLevelCamera);
NERVE_IMPL(ShineTowerRocket, DemoUpLevelCloseFade);

class ShineTowerRocketNrvNoStartEnter : public al::Nerve {
public:
    void execute(al::NerveKeeper* keeper) const override;
};

class ShineTowerRocketNrvDemoInformCompleteShineFadeIn : public al::Nerve {
public:
    void execute(al::NerveKeeper* keeper) const override;
};

class ShineTowerRocketNrvDemoMeterUpPost : public al::Nerve {
public:
    void execute(al::NerveKeeper* keeper) const override;
};

class ShineTowerRocketNrvDemoInformPowerUp : public al::Nerve {
public:
    void execute(al::NerveKeeper* keeper) const override;
};

class ShineTowerRocketNrvDemoKoopaShip : public al::Nerve {
public:
    void execute(al::NerveKeeper* keeper) const override;
};

class ShineTowerRocketNrvDemoInformRepairHome : public al::Nerve {
public:
    void execute(al::NerveKeeper* keeper) const override;
};

class ShineTowerRocketNrvDemoInformCompleteShineFadeOut : public al::Nerve {
public:
    void execute(al::NerveKeeper* keeper) const override;
};

class ShineTowerRocketNrvDemoAppearPlayerFromHomeAfter : public al::Nerve {
public:
    void execute(al::NerveKeeper* keeper) const override;
};

class ShineTowerRocketNrvDemoWarpWorld : public al::Nerve {
public:
    void execute(al::NerveKeeper* keeper) const override;
};

NERVES_MAKE_NOSTRUCT(ShineTowerRocket, DemoScaleUp, DemoInformCompleteShineFadeWait,
                     DemoInformCompleteShineFadeOut, DemoInformCompleteShine,
                     DemoInformPowerUpMessage, DemoKoopaShipFade, DemoMeterUp, DemoUpLevel,
                     DemoInformPowerUp, DemoUpLevelWaitFade, DemoUpLevelOpenFade,
                     DemoInformNewHome, WorldMap, DemoAppearPlayerFromHome, DemoReturnToHome);
[[maybe_unused]] ShineTowerRocketNrvDemoMeterUpPost DemoMeterUpPost;
[[maybe_unused]] ShineTowerRocketNrvDemoWaitAfterAppearShine DemoWaitAfterAppearShine;
[[maybe_unused]] ShineTowerRocketNrvDemoAppearPlayerFromHomeAfter DemoAppearPlayerFromHomeAfter;
NERVES_MAKE_STRUCT(ShineTowerRocket, Wait, DemoAppearFromEntrance, WaitDemo, WaitIgnoreLockOn,
                   WaitAfterReturnToHome, Reaction, NoStartEarth, NoStartAndCoin, DemoPrepare,
                   DemoPrepareNoShine, GoToWorldMapWithCamera, DemoWorldTakeoff,
                   DemoWorldTakeoffNext, NoStartEnter, DemoWalkPlayerToPointNoShine,
                   DemoAppearShine, DemoMeterRotate, NoStart, GoToWorldMapWithFade,
                   DemoSelectGoOtherWorld, DemoWaitBeforeScaleUpDirect,
                   DemoMeterUpPrev, DemoInformNewItem, DemoInformCompleteShineFadeIn,
                   DemoInformPeachCastleCap, DemoAwardMoon, DemoKoopaShip,
                   DemoInformRepairHome, DemoInformNewHomeMessage, DemoUpLevelCamera,
                   DemoWarpWorld, DemoUpLevelCloseFade);
}  // namespace

template class al::FunctorV0M<ShineTowerRocket*, void (ShineTowerRocket::*)()>;

ShineTowerRocket::ShineTowerRocket(const char* name) : al::LiveActor(name) {}

void ShineTowerRocket::init(const al::ActorInitInfo& info) {
    al::initActorSceneInfo(this, info);

    if (!isDamageHomeModel(this) || rs::isModeE3LiveRom()) {
        al::initActorWithArchiveName(this, info, "ShineTower", nullptr);
    } else if (GameDataFunction::isWorldSky(GameDataHolderAccessor(this))) {
        al::initActorWithArchiveName(this, info, "ShineTower", nullptr);

        mDamageModel = new DamageModel(this);
        al::initActorWithArchiveName(mDamageModel, info, "ShineTowerDamage", "SkyWorld");
        mDamageModel->makeActorAlive();
        al::hideModelIfShow(this);
        al::initJointControllerKeeper(mDamageModel, 3);
        al::initJointLocalMtxController(mDamageModel, &mScaleRootMtx, "ScaleRoot");
        al::registerSubActorSyncClipping(this, mDamageModel);
    } else {
        al::initActorWithArchiveName(this, info, "ShineTowerDamage", nullptr);
    }

    al::initNerve(this, &NrvShineTowerRocket.Wait, 3);
    mAddDemoInfo = al::registDemoRequesterToAddDemoInfo(this, info, 0);
    al::registActorToDemoInfo(this, info);

    auto* doorCollision = new al::CollisionObj(
        info, al::findOrCreateResource("ObjectData/ShineTower", nullptr), "Door",
        al::getHitSensor(this, "Collision"), getBaseMtx(), nullptr);
    doorCollision->makeActorAlive();
    al::setCollisionPartsSpecialPurposeName(doorCollision, "CameraMoveLimit");

    rs::registerShineTowerRocketToDemoDirector(this);

    if (GameDataFunction::isWorldCity(GameDataHolderAccessor(this))) {
        if (GameDataFunction::getScenarioNo(this) == 1 ||
            GameDataFunction::getScenarioNo(this) == 6 ||
            GameDataFunction::getScenarioNo(this) == 9)
            al::updateMaterialCodeWet(this, true);
        else
            al::setMaterialCode(this, "Stone");
    } else {
        al::initMaterialCode(this, info);
    }

    mWorldMapCameraPosRateParam = new al::RateParamV3f();
    mWorldMapCameraAtRateParam = new al::RateParamV3f();
    mDemoPeachCastleCapActor = al::tryCreateLinksActorFromFactorySingle(info, "PeachCastleCap");

    mHomeDemoActorGroup =
        new al::DeriveActorGroup<CapHanger>("帽子ひっかけポイント[ホーム]", 5);
    for (s32 i = 0; i < mHomeDemoActorGroup->getMaxActorCount(); i++) {
        CapHanger* capHanger = new CapHanger("帽子ひっかけポイント[ホーム]", false);
        al::initCreateActorNoPlacementInfo(capHanger, info);
        mHomeDemoActorGroup->registerActor(capHanger);
    }

    al::calcJointPos(al::getTransPtr(mHomeDemoActorGroup->getDeriveActor(0)), this, "Flag00");
    al::calcJointPos(al::getTransPtr(mHomeDemoActorGroup->getDeriveActor(1)), this,
                     "Muffler2L00");
    al::calcJointPos(al::getTransPtr(mHomeDemoActorGroup->getDeriveActor(2)), this,
                     "Muffler2L01");
    al::calcJointPos(al::getTransPtr(mHomeDemoActorGroup->getDeriveActor(3)), this,
                     "Muffler2R00");
    al::calcJointPos(al::getTransPtr(mHomeDemoActorGroup->getDeriveActor(4)), this,
                     "Muffler2R01");

    for (s32 i = 0; i < mHomeDemoActorGroup->getMaxActorCount(); i++) {
        CapHanger* capHanger = mHomeDemoActorGroup->getDeriveActor(i);
        capHanger->initItem(0, 1, info);

        sead::Vector3f* trans = al::getTransPtr(capHanger);
        trans->add(*sCapHangerOffsets[i]);
        al::resetPosition(capHanger);
    }

    if (al::isExistLinkChild(info, "BeforeClear", 0)) {
        s32 worldId = GameDataFunction::getCurrentWorldId(GameDataHolderAccessor(this)) + 1;
        while (true) {
            if (worldId >= GameDataFunction::getWorldNum(GameDataHolderAccessor(this))) {
                sead::Matrix34f beforeClearMtx = sead::Matrix34f::ident;
                al::getLinksMatrix(&beforeClearMtx, info, "BeforeClear");
                al::resetMtxPosition(this, beforeClearMtx);
                mHomeDemoActorGroup->makeActorDeadAll();
                break;
            }

            if (GameDataFunction::isAlreadyGoWorld(GameDataHolderAccessor(this), worldId++))
                break;
        }
    }

    mEntranceCameraAreaShape = new al::AreaShapeCube(al::AreaShapeCube::OriginType::Base);
    {
        sead::Vector3f entranceAreaPos = sead::Vector3f::zero;
        al::multVecPose(&entranceAreaPos, this, sEntranceAreaOffset);
        mEntranceCameraAreaMtx = *getBaseMtx();
        mEntranceCameraAreaMtx.setTranslation(entranceAreaPos);
        mEntranceCameraAreaShape->setBaseMtxPtr(&mEntranceCameraAreaMtx);
        mEntranceCameraAreaShape->setScale(sead::Vector3f(0.2f, 0.5f, 0.5f));
    }

    {
        sead::Vector3f fixDoorwayCameraPos = sead::Vector3f::zero;
        sead::Vector3f fixDoorwayCameraAt = sead::Vector3f::zero;
        al::multVecPose(&fixDoorwayCameraPos, this, sFixDoorwayCameraPosOffset);
        al::multVecPose(&fixDoorwayCameraAt, this, sFixDoorwayCameraAtOffset);
        mFixDoorwayCameraTicket =
            al::initFixDoorwayCamera(this, "ホーム出入口", fixDoorwayCameraPos, fixDoorwayCameraAt);
    }

    using ShineTowerRocketFunctor = al::FunctorV0M<ShineTowerRocket*, void (ShineTowerRocket::*)()>;
    al::listenStageSwitchOnOff(this, "SwitchDither",
                               ShineTowerRocketFunctor(this, &ShineTowerRocket::onSwitchDither),
                               ShineTowerRocketFunctor(this, &ShineTowerRocket::offSwitchDither));

    if (GameDataFunction::getPayShineNum(GameDataHolderAccessor(this)) >= 1)
        al::tryOnStageSwitch(this, "SwitchKidsModeOn");

    if (rs::isModeE3Rom() || rs::isModeE3LiveRom()) {
        rs::tryInitItem(this, 0, info, false);
        mIsAppearCoin = true;
    }

    mShineTowerCommonKeeper = new ShineTowerCommonKeeper(this, &mScaleRootMtx, 0, info);
    mShineTowerCommonKeeper->setMeterRotateForWorld(true);
    if (mShineTowerCommonKeeper->isLightOn())
        mShineTowerLight = new ShineTowerLight(this, getBaseMtx());

    mDoorAreaChange = new DoorAreaChange("ホームドア");
    al::initCreateActorWithPlacementInfo(mDoorAreaChange, info);
    al::invalidateClipping(mDoorAreaChange);
    al::registerSubActorSyncClippingAndHide(this, mDoorAreaChange);
    al::resetMtxPosition(mDoorAreaChange, *al::getJointMtxPtr(this, "ShineTowerDoor"));
    mDoorAreaChange->setHomeDoor(false);

    bool isHomeNoStart = GameDataFunction::isCrashHome(GameDataHolderAccessor(this)) ||
                         GameDataFunction::isBossAttackedHome(GameDataHolderAccessor(this)) ||
                         !GameDataFunction::isEnableCap(GameDataHolderAccessor(this)) ||
                         rs::isInvalidChangeStage(this);
    if (isHomeNoStart)
        mDoorAreaChange->setNoStart();

    mDokanPuppetController = new DokanPuppetController(this);
    mDokanPuppetController->setIsHome(true);
    mDokanInfo = new DokanInfo(this, info);

    mDokanDemoActor = new al::LiveActor("土管ダミー");
    al::initActorSceneInfo(mDokanDemoActor, info);
    al::initActorPoseTFSV(mDokanDemoActor);
    al::initActorSRT(mDokanDemoActor, info);
    al::initActorSeKeeper(mDokanDemoActor, info, "Dokan");
    al::initActorClipping(mDokanDemoActor, info);
    al::copyPose(mDokanDemoActor, this);
    mDokanDemoActor->makeActorDead();

    mDokanDemoLinkId.clear();
    al::makeMtxSRT(&mDokanDemoMtx, this);
    al::tryGetLinksTR(&mCameraRotateEastRot, &mCameraRotateEastTrans, info, "CameraRotateEast");
    al::tryGetLinksTR(&mCameraRotateWestRot, &mCameraRotateWestTrans, info, "CameraRotateWest");

    ShineTowerBackDoor* backDoor = new ShineTowerBackDoor("ホーム裏口");
    mHomeActor = backDoor;
    if (isHomeNoStart)
        backDoor->setNoStart(true);
    al::initCreateActorNoPlacementInfo(backDoor, info);
    backDoor->attachToHostJoint(this, "BackDoor");
    al::registerSubActorSyncClippingAndHide(this, backDoor);

    mDemoEventFlowExecutor = rs::initEventFlowForSystem(this, info, nullptr, "Home", nullptr);
    mDemoEventFlowExecutorSub = rs::initEventFlowForSystem(this, info, "Dirty", "Home", nullptr);
    al::initEventReceiver(mDemoEventFlowExecutor, this);
    al::initEventReceiver(mDemoEventFlowExecutorSub, this);
    if (!GameDataFunction::isActivateHome(GameDataHolderAccessor(this))) {
        mPlayerRestartLinkId.setCurrent(1);
        mDokanActor = al::tryCreateLinkArea(info, "InformationArea", "InformationArea");
    }

    al::MessageTagDataHolder* tagDataHolder = al::initMessageTagDataHolder(2);
    al::registerMessageTagDataScore(tagDataHolder, "Score", &mPlayerRestartId);
    rs::initEventMessageTagDataHolder(mDemoEventFlowExecutor, tagDataHolder);
    rs::initEventMessageTagDataHolder(mDemoEventFlowExecutorSub, tagDataHolder);
    mEventFlowExecutor = mDemoEventFlowExecutor;

    mCapTargetInfo = rs::createCapTargetInfo(this, nullptr);
    if (rs::isModeE3Rom() || rs::isModeE3LiveRom())
        mCapTargetInfo->setLockOnAnimName("CapPointHookWait");

    mDemoAppearFromHomeCameraTicket =
        al::initDemoAnimCamera(this, info, al::getAnimResource(this), &mDemoPlayerMtx, "Anim");
    al::makeMtxSRT(&mDemoPlayerMtx, this);
    al::Resource* playerAnimationResource =
        al::findOrCreateResource("ObjectData/PlayerAnimation", nullptr);
    mDemoReturnToHomeCameraTicket = al::initDemoAnimCamera(
        this, info, playerAnimationResource, &mDemoReturnToHomeMtx, "ReturnToHome");
    al::calcFrontDir(&mEntranceCameraFront, this);
    mEntranceCameraTicket =
        al::initEntranceCameraNoSave(this, al::getPlacementInfo(info), "ホーム入り口");

    if (!GameDataFunction::isUnlockedWorld(
            GameDataHolderAccessor(this),
            GameDataFunction::getCurrentWorldId(GameDataHolderAccessor(this)))) {
        makeActorDead();
        return;
    }
    if (GameDataFunction::getCurrentWorldId(GameDataHolderAccessor(this)) == 0 &&
        !GameDataFunction::isUnlockedWorld(GameDataHolderAccessor(this), 1)) {
        makeActorDead();
        return;
    }

    mDemoWipe =
        new al::WipeSimple("ホーム用白フェード", "FadeWhite", al::getLayoutInitInfo(info), nullptr);
    mDemoWipe->appear();

    if (!GameDataFunction::isActivateHome(GameDataHolderAccessor(this))) {
        mDirtyModel = new al::LiveActor("ホーム汚れモデル");
        al::initChildActorWithArchiveNameNoPlacementInfo(mDirtyModel, info, "ShineTowerDirty",
                                                         nullptr);
        al::registerSubActorSyncClipping(this, mDirtyModel);
        al::resetActorPosition(mDirtyModel, this);
        mDirtyModel->makeActorAlive();
        al::initJointControllerKeeper(mDirtyModel, 3);
        al::initJointLocalScaleController(mDirtyModel, &mPlayerPuppetScale, "MoonTank");

        al::LiveActor* dirtyFrame = al::getSubActor(mDirtyModel, "フレーム");
        al::initJointControllerKeeper(dirtyFrame, 3);
        al::initJointLocalMtxController(dirtyFrame, &mScaleRootMtx, "ScaleRoot");

        al::LiveActor* dirtySail = al::getSubActor(dirtyFrame, "帆");
        if (isHomeMeterComplete(dirtySail)) {
            al::startAction(dirtySail, "Wait");
        } else {
            f32 meterFrame = calcHomeMeterAnimFrame(dirtySail, 0);
            al::startSklAnim(dirtySail, "Meter");
            al::setSklAnimFrame(dirtySail, 0, meterFrame);
            al::setSklAnimFrameRate(dirtySail, 0, 0.0f);
        }

        al::hideModelIfShow(this);
        al::invalidateCollisionParts(this);
        mDoorAreaChange->kill();
        mHomeActor->kill();
        mEventFlowExecutor = mDemoEventFlowExecutorSub;
        al::getSubActor(this, "フレーム")->kill();
    }

    if (!GameDataFunction::isLaunchHome(GameDataHolderAccessor(this))) {
        mShineTowerRock = new al::LiveActor("ホーム岩モデル");
        al::initChildActorWithArchiveNameNoPlacementInfo(mShineTowerRock, info, "ShineTowerRock",
                                                         nullptr);
        al::registerSubActorSyncClipping(this, mShineTowerRock);
        al::resetActorPosition(mShineTowerRock, this);
        mShineTowerRock->makeActorAlive();
        al::startAction(mShineTowerRock, "Wait");

        mWaterfallWorldDemoStepActor =
            al::tryCreateLinksActorFromFactorySingle(info, "WaterfallWorldDemoStep");
        if (mWaterfallWorldDemoStepActor) {
            mWaterfallWorldDemoStepActor->makeActorAlive();
            al::registerSubActor(this, mWaterfallWorldDemoStepActor);

            mWaterfallWorldHomeRockBreakActor = new al::LiveActor("ホーム滝壊れモデル");
            al::initChildActorWithArchiveNameNoPlacementInfo(
                mWaterfallWorldHomeRockBreakActor, info, "WaterfallWorldHomeRock001Break",
                nullptr);
            al::registerSubActor(this, mWaterfallWorldHomeRockBreakActor);
            al::resetActorPosition(mWaterfallWorldHomeRockBreakActor,
                                   mWaterfallWorldDemoStepActor);
            mWaterfallWorldHomeRockBreakActor->makeActorDead();
        }
    }

    if (GameDataFunction::isCrashHome(GameDataHolderAccessor(this)) ||
        GameDataFunction::isBossAttackedHome(GameDataHolderAccessor(this))) {
        mClashModel = new al::LiveActor("ホーム壊れ状態モデル");
        al::initChildActorWithArchiveNameNoPlacementInfo(mClashModel, info, "ShineTowerClash",
                                                         nullptr);
        al::registerSubActorSyncClipping(this, mClashModel);
        al::resetActorPosition(mClashModel, this);
        mClashModel->makeActorAlive();
        al::initJointControllerKeeper(mClashModel, 3);
        al::initJointLocalScaleController(mClashModel, &mPlayerPuppetScale, "MoonTank");
        al::initJointLocalMtxController(mClashModel, &mScaleRootMtx, "ScaleRoot");
        al::hideModelIfShow(this);
        al::invalidateCollisionParts(this);
        mDoorAreaChange->kill();
        mHomeActor->kill();
        al::getSubActor(this, "フレーム")->kill();
        al::getSubActor(this, "ステッカー")->kill();
    }

    if (mDirtyModel || mClashModel)
        mHomeDemoActorGroup->makeActorDeadAll();

    if (GameDataFunction::isWorldSea(GameDataHolderAccessor(this)))
        mRopeRootRotateDegree = -90.0f;
    else if (GameDataFunction::isWorldLake(GameDataHolderAccessor(this)))
        mRopeRootRotateDegree = 90.0f;
    else
        mRopeRootRotateDegree = 0.0f;
    al::initJointLocalYRotator(al::getSubActor(this, "フレーム"), &mRopeRootRotateDegree,
                               "RopeRoot");

    mDemoShineRumbleCalculator = new al::RumbleCalculatorCosMultLinear(2.5f, 2.0f, 0.125f, 10);
    mDemoShineGroup = new al::DeriveActorGroup<DemoShine>("ダミーシャイングループ", 55);
    for (s32 i = 0; i < mDemoShineGroup->getMaxActorCount(); i++) {
        DemoShine* demoShine = new DemoShine(mDemoShineRumbleCalculator);
        al::initActorWithArchiveName(demoShine, info, "Shine", "Dummy");
        demoShine->makeActorDead();
        mDemoShineGroup->registerActor(demoShine);
        if (GameDataFunction::isWorldPeach(GameDataHolderAccessor(this)))
            rs::setStageShineAnimFrame(demoShine, nullptr, 0, false);
        else
            rs::setStageShineAnimFrame(demoShine, nullptr, -1, false);
    }

    s32 birdNum = al::calcLinkChildNum(info, "BirdGroup");
    mBirdGroup = new al::DeriveActorGroup<Bird>("飛びたつ鳥", birdNum);
    if (birdNum > 0)
        al::createAndRegisterLinksActorFromFactory(mBirdGroup, info, "BirdGroup");
    mBirdGroup->makeActorDeadAll();

    mLinkObjAliveDeadCtrl = new LinkObjAliveDeadCtrl(info);
    if (mDirtyModel)
        mShineTowerCommonKeeper->setTotalCountLayout(al::getSubActor(mDirtyModel, "フレーム"));

    al::getLinksQT(&mDemoReturnPlayerQuat, &mDemoReturnPlayerTrans, al::getPlacementInfo(info),
                   "PlayerRestartPos");
    mWorldMapCameraTicket =
        al::initProgramableCamera(this, "発射カメラ", &mWorldMapCameraPos, &mWorldMapCameraAt,
                                  nullptr);
    mMtxConnector = al::tryCreateMtxConnector(this, info);
    al::initJointLocalYRotator(this, &mMeterRotateDegree, "SailRoot");
    al::initJointLocalYRotator(al::getSubActor(this, "フレーム"), &mMeterRotateDegree,
                               "SailRoot");
    al::initJointLocalScaleController(this, &mPlayerPuppetScale, "MoonTank");
    al::multVecPose(&mClippingOffset, this, sClippingOffset);
    al::LiveActor* frame = al::getSubActor(this, "フレーム");
    al::setClippingInfo(frame, 6000.0f, &mClippingOffset);
    al::tryExpandClippingByExpandObject(frame, info);

    mWorldMapFadeCameraTicket =
        al::initProgramableCamera(this, info, "PayCamera", &mPayCameraPos, &mPayCameraAt,
                                  nullptr);
    al::multVecPose(&mPayCameraPos, this, sPayCameraPosOffset);
    al::multVecPose(&mPayCameraAt, this, sPayCameraAtOffset);

    mPoseCopyActor = CapManHeroDemoUtil::createDemoCapManHero("ホーム用キャップ君", info, "Home");

    mDemoMarioCapActor = new al::LiveActor("ホーム用帽子");
    al::initChildActorWithArchiveNameNoPlacementInfo(mDemoMarioCapActor, info, "MarioCap",
                                                     "Dummy");
    mDemoMarioCapActor->makeActorDead();
    al::invalidateClipping(mDemoMarioCapActor);

    mCapManHeroEyesActor = new al::LiveActor("ホーム用キャップ君の目");
    al::initChildActorWithArchiveNameNoPlacementInfo(mCapManHeroEyesActor, info, "CapManHeroEyes",
                                                     nullptr);
    mCapManHeroEyesActor->makeActorDead();
    al::invalidateClipping(mCapManHeroEyesActor);

    if (al::isExistLinkChild(info, "DemoPlayer", 0)) {
        mDemoPlayerActor = new DemoPlayer("ホーム用デモプレイヤー");
        al::initLinksActor(mDemoPlayerActor, info, "DemoPlayer", 0);
        mDemoPlayerActor->makeActorDead();
        al::invalidateClipping(mDemoPlayerActor);

        if (GameDataFunction::isWorldSky(GameDataHolderAccessor(this))) {
            mDemoPlayerActor2 = new DemoPlayer("ホーム用デモプレイヤー2");
            al::initLinksActor(mDemoPlayerActor2, info, "DemoPlayer2", 0);
            mDemoPlayerActor2->makeActorDead();
            al::invalidateClipping(mDemoPlayerActor2);
        }
    }

    CheckpointFlag* checkpointFlag = new CheckpointFlag("ホームの旗");
    mHomeSubActor = checkpointFlag;
    checkpointFlag->initHomeFlag(info);
    al::resetMtxPosition(mHomeSubActor, *al::getJointMtxPtr(this, "CheckPointFlag"));
    al::invalidateClipping(mHomeSubActor);
    al::registerSubActorSyncClippingAndHide(this, mHomeSubActor);
    if (!GameDataFunction::isActivateHome(GameDataHolderAccessor(this)))
        mHomeSubActor->makeActorDead();

    al::PartsModel* globeActor = static_cast<al::PartsModel*>(al::getSubActor(this, "地球儀"));
    mShineTowerGlobeAnimCtrl = new ShineTowerGlobeAnimCtrl(globeActor);
    mShineTowerGlobeAnimCtrl->init(info);
    globeActor->offSyncAppearAndHide();
    if (GameDataFunction::isActivateHome(GameDataHolderAccessor(this)))
        globeActor->makeActorAlive();
    else
        globeActor->makeActorDead();

    mBackDoor = new GoalMark("ホーム目的地");
    al::initCreateActorWithPlacementInfo(mBackDoor, info);
    {
        sead::Vector3f goalMarkTrans = sGoalMarkOffset;
        al::multVecPose(&goalMarkTrans, this, goalMarkTrans);
        al::setTrans(mBackDoor, goalMarkTrans);
    }
    mBackDoor->kill();

    mIsPlayDemoAwardSpecial =
        GameDataFunction::isPlayDemoAwardSpecial(GameDataHolderAccessor(this));
    makeActorAlive();

    {
        sead::Vector3f globeCapPoint = sead::Vector3f::zero;
        al::LiveActor* dirtyModel = mDirtyModel;
        if (dirtyModel && al::isAlive(dirtyModel))
            al::calcJointPos(&globeCapPoint, dirtyModel, "GlobeCapPoint");
        else
            al::calcJointPos(&globeCapPoint, this, "GlobeCapPoint");
        GameDataFunction::setHomeTrans(this, globeCapPoint);
    }
    setupCapTargetInfoForHomeGlobe(&mCapTargetOffset, mCapTargetInfo, this, mDirtyModel);

    const char* playerStartId =
        GameDataFunction::tryGetPlayerStartId(GameDataHolderAccessor(this));
    rs::registerPlayerStartInfoToHolder(this, info, "HomeEntrance", nullptr, nullptr, nullptr);
    if (playerStartId && al::isEqualString(playerStartId, "HomeEntrance")) {
        mDoorAreaChange->setHomeDoor(true);
        al::invalidateEndEntranceCamera(this);
        al::setNerve(this, &NrvShineTowerRocket.DemoAppearFromEntrance);
    }
}

void ShineTowerRocket::onSwitchDither() {
    invalidateDitherAnimAll(this);
    setModelAlphaMaskSubActorAll(this, 0.062f);
}

void ShineTowerRocket::offSwitchDither() {
    validateDitherAnimAll(this);
    rs::setupHomeMeterDitherParam(this, mShineTowerCommonKeeper);
    setModelAlphaMaskSubActorAll(this, 1.0f);
}

void ShineTowerRocket::makeActorDead() {
    al::LiveActor::makeActorDead();

    if (mDoorAreaChange)
        mDoorAreaChange->makeActorDead();
    if (mHomeActor)
        mHomeActor->makeActorDead();
    if (mHomeSubActor)
        mHomeSubActor->makeActorDead();

    al::makeActorDeadSubActorAll(this);

    if (mBackDoor)
        mBackDoor->kill();
}

void ShineTowerRocket::makeActorAlive() {
    al::LiveActor::makeActorAlive();

    al::LiveActor* actor = this;
    const al::IUseSceneObjHolder* holder = actor;
    bool isActivateHome = false;
    {
        GameDataHolderAccessor accessor(holder);
        isActivateHome = GameDataFunction::isActivateHome(accessor);
    }

    if (isActivateHome) {
        {
            GameDataHolderAccessor accessor(holder);
            if (!GameDataFunction::isCrashHome(accessor)) {
                GameDataHolderAccessor accessor2(holder);
                if (!GameDataFunction::isBossAttackedHome(accessor2)) {
                    if (mDoorAreaChange)
                        mDoorAreaChange->makeActorAlive();
                    if (mHomeActor)
                        mHomeActor->makeActorAlive();
                }
            }
        }

        if (mHomeSubActor)
            mHomeSubActor->makeActorAlive();
        al::getSubActor(this, "地球儀")->makeActorAlive();
    }

    rs::setupHomeMeter(this);
}

void ShineTowerRocket::startClipped() {
    al::LiveActor::startClipped();
}

void ShineTowerRocket::initAfterPlacement() {
    if (mMtxConnector)
        al::attachMtxConnectorToCollision(mMtxConnector, this, false);
    al::tryExpandClippingByDepthShadowLength(this, &mClippingOffset);
    al::offSyncClippingSubActor(this, al::getSubActor(this, "フレーム"));

    if (al::isNerve(this, &NrvShineTowerRocket.DemoAppearFromEntrance)) {
        al::invalidateClipping(this);
        rs::requestStartDemoWithPlayer(this, false);
        rs::addDemoSubActor(this);
        invalidateDitherAnimAll(this);
        invalidateOcclusionQueryAll(this);
        return;
    }

    if (!GameDataFunction::isPlayDemoWorldWarp(GameDataHolderAccessor(this)) ||
        GameDataFunction::isCrashHome(GameDataHolderAccessor(this)) ||
        GameDataFunction::isBossAttackedHome(GameDataHolderAccessor(this))) {
        if (!GameDataFunction::isPlayDemoReturnToHome(GameDataHolderAccessor(this)))
            return;

        rs::addDemoReturnToHomeToList(this);
        al::invalidateClipping(this);
        invalidateDitherAnimAll(this);
        invalidateOcclusionQueryAll(this);
        al::setNerve(this, &NrvShineTowerRocket.WaitDemo);
    } else {
        rs::addDemoAppearFromHomeToList(this);
        al::LiveActor* player = al::findNearestPlayerActor(this);
        new (&mDemoReturnPlayerTrans) sead::Vector3f(al::getTrans(player));
        al::calcQuat(&mDemoReturnPlayerQuat, player);
        al::invalidateClipping(this);
        al::invalidateEndEntranceCamera(this);
        al::hideModelIfShow(this);
        al::getSubActor(this, "地球儀")->makeActorDead();
        invalidateDitherAnimAll(this);
        invalidateOcclusionQueryAll(this);
        al::setNerve(this, &NrvShineTowerRocket.WaitDemo);
    }
}

void ShineTowerRocket::control() {
    ShineTowerGlobeAnimCtrl** globeAnimCtrlPtr = &mShineTowerGlobeAnimCtrl;
    if (mDirtyModel) {
        if (al::isAlive(mDirtyModel)) {
            if (al::isAlive(*globeAnimCtrlPtr))
                (*globeAnimCtrlPtr)->makeActorDead();
        } else if (al::isDead(*globeAnimCtrlPtr)) {
            (*globeAnimCtrlPtr)->makeActorAlive();
        }
    } else if (al::isDead(*globeAnimCtrlPtr)) {
        (*globeAnimCtrlPtr)->makeActorAlive();
    }

    if (mShineTowerLight) {
        bool isWait = false;
        if (al::isNerve(this, &NrvShineTowerRocket.Wait))
            isWait = true;
        else if (al::isNerve(this, &NrvShineTowerRocket.WaitIgnoreLockOn))
            isWait = true;
        else
            isWait = al::isNerve(this, &NrvShineTowerRocket.WaitAfterReturnToHome);
        mShineTowerLight->update(isWait, al::getActionFrame(this));
    }

    if (al::isActiveCamera(mFixDoorwayCameraTicket)) {
        if (!mEntranceCameraAreaShape->isInVolume(rs::getPlayerPos(this)))
            al::endCamera(this, mFixDoorwayCameraTicket, -1, false);
    } else if (!rs::isPlayerHackTank(this)) {
        if (mEntranceCameraAreaShape->isInVolume(rs::getPlayerPos(this)))
            al::startCamera(this, mFixDoorwayCameraTicket, -1);
    }

    if (GameDataFunction::isCrashHome(GameDataHolderAccessor(this)) ||
        GameDataFunction::isBossAttackedHome(GameDataHolderAccessor(this)) ||
        !GameDataFunction::isEnableCap(GameDataHolderAccessor(this)) ||
        rs::isInvalidChangeStage(this)) {
        mHomeActor->setNoStart(true);
        mDoorAreaChange->setNoStart();
    } else {
        mHomeActor->setNoStart(false);
        mDoorAreaChange->enableStart();
    }

    mDokanDemoLinkId.shiftCurrentToPrevious();
    mDokanPuppetController->update();

    mPlayerRestartId = calcRestShineNum(this);

    s32 meterReactionCoolTime = mMeterReactionCoolTime - 1;
    if (meterReactionCoolTime < 0)
        meterReactionCoolTime = 0;
    mMeterReactionCoolTime = meterReactionCoolTime;

    if (mDemoShineRumbleCalculator->isActive()) {
        mDemoShineRumbleCalculator->calc();
        f32 scale = mDemoShineRumbleCalculator->getValueY() + 1.0f;
        mPlayerPuppetScale.set(scale, scale, scale);
    }

    if (al::isNerve(this, &NrvShineTowerRocket.Wait) && !rs::isActiveDemo(this) && mDokanActor) {
        bool isInArea = al::isInAreaObjPlayerAnyOne(this, mDokanActor);
        if (mPlayerRestartLinkId.isPreviousSet()) {
            if (!isInArea && rs::tryCloseObjectTutorialCapThrow(this))
                mPlayerRestartLinkId.clearPrevious();
        } else if (mPlayerRestartLinkId.isCurrentSet()) {
            if (isInArea && rs::tryAppearObjectTutorialCapThrow(this))
                mPlayerRestartLinkId.setPrevious(1);
        }
    }
}

bool ShineTowerRocket::isActiveDirtyModel() const {
    return mDirtyModel && al::isAlive(mDirtyModel);
}

void ShineTowerRocket::calcAnim() {
    if (!al::isNerve(this, &NrvShineTowerRocket.Wait) || al::isFirstStep(this)) {
        al::LiveActor::calcAnim();
        return;
    }

    if (mDirtyModel && al::isAlive(mDirtyModel))
        return;
    if (mDamageModel && al::isAlive(mDamageModel))
        return;
    if (mClashModel && al::isAlive(mClashModel))
        return;

    al::LiveActor::calcAnim();
}

bool ShineTowerRocket::isFirstDemo() const {
    return !GameDataFunction::isAlreadyGoWorld(GameDataHolderAccessor(this), mWorldId);
}

void ShineTowerGlobeAnimCtrl::start(f32 accel) {
    if (al::isSameSign(mWorldMapCameraAnimRate, accel)) {
        f32 rate = mWorldMapCameraAnimRate + accel;
        accel = -35.0f;
        if (!(rate < -35.0f)) {
            accel = rate;
            if (rate > 35.0f)
                accel = 35.0f;
        }
    }

    mWorldMapCameraAnimRate = accel;
    mMusicBoxTimer = 150;
    mBrakeTimer = 120;

    const char* musicBoxName = nullptr;
    if (GameDataFunction::getCurrentWorldId(GameDataHolderAccessor(mGlobeActor)) >=
            GameDataFunction::getWorldIndexCity() &&
        rs::isCollectedBgmCityWorldCelemony(mGlobeActor))
        musicBoxName = "MusicBox00";
    else
        musicBoxName = "MusicBox01";
    mMusicBoxName = musicBoxName;
}

bool ShineTowerRocket::isEnableSkipDemo() const {
    if (al::isNerve(this, &NrvShineTowerRocket.DemoWorldTakeoff))
        return true;
    return al::isNerve(this, &NrvShineTowerRocket.DemoWorldTakeoffNext);
}

bool ShineTowerRocket::receiveEvent(const al::EventFlowEventData* event) {
    if (al::isEventName(event, "GoOtherWorld")) {
        al::setNerve(this, &NrvShineTowerRocket.GoToWorldMapWithCamera);
        return true;
    }

    if (al::isEventName(event, "CancelGoOtherWorld")) {
        tryStartEntranceCamera(-1);
        resumeActiveBgmForReceiveEvent(*this);
        al::setNerve(this, &NrvShineTowerRocket.WaitIgnoreLockOn);
        return true;
    }

    return false;
}

void ShineTowerRocket::skipDemo() {
    GameDataHolderAccessor accessor(this);
    accessor->setStageChanging(true);
    rs::endEventCutSceneDemoOrTryEndEventCutSceneDemoBySkip(this);
}

void ShineTowerRocket::updateParts() {
    al::resetPosition(this);
    al::resetMtxPosition(mDoorAreaChange, *al::getJointMtxPtr(this, "ShineTowerDoor"));

    if (mDirtyModel && al::isAlive(mDirtyModel))
        al::resetMtxPosition(mHomeSubActor, *al::getJointMtxPtr(mDirtyModel, "CheckPointFlag"));
    else
        al::resetMtxPosition(mHomeSubActor, *al::getJointMtxPtr(this, "CheckPointFlag"));
    al::copyPose(mPoseCopyActor, this);
}

void ShineTowerRocket::tryEndEntranceCamera() {
    if (al::isActiveCamera(mEntranceCameraTicket))
        al::endCamera(this, mEntranceCameraTicket, -1, false);
}

void ShineTowerRocket::tryStartEntranceCamera(s32 step) {
    if (al::isActiveCamera(mEntranceCameraTicket))
        return;

    sead::Vector3f side = sead::Vector3f::zero;
    al::calcSideDir(&side, this);

    sead::Vector3f front = mEntranceCameraFront;
    al::rotateVectorDegree(&front, front, side, -20.0f);

    if (mShineTowerRock)
        al::setEntranceCameraParam(mEntranceCameraTicket, 1300.0f, front,
                                   sEntranceCameraOffsetWithRock);
    else
        al::setEntranceCameraParam(mEntranceCameraTicket, 1300.0f, front, sEntranceCameraOffset);

    al::startCamera(this, mEntranceCameraTicket, step);
}

bool ShineTowerRocket::receiveMsg(const al::SensorMsg* message, al::HitSensor* other,
                                  al::HitSensor* self) {
    if (al::isMsgAskSafetyPoint(message) || rs::isMsgPlayerDisregardTargetMarker(message))
        return true;

    if (rs::isMsgPlayerDisregardHomingAttack(message))
        return !al::isSensorName(self, "Body");

    if (al::isMsgPlayerDisregard(message)) {
        if (al::isSensorName(self, "Body"))
            return false;
        return !al::isSensorName(self, "BackDoorMarker");
    }

    if (rs::isMsgKoopaRingBeamInvalidTouch(message))
        return true;

    if (al::isSensorName(self, "MeterReaction")) {
        al::LiveActor* actor = this;
        const al::IUseSceneObjHolder* holder = actor ? actor : nullptr;
        if (!GameDataFunction::isActivateHome(GameDataHolderAccessor(holder)))
            return false;

        if (mClashModel && al::isAlive(mClashModel))
            return false;

        sead::Vector3f hitPos = sead::Vector3f::zero;
        al::calcPosBetweenSensors(&hitPos, other, self, 0.0f);

        bool isNearSail = false;
        for (s32 i = 1; i < 9; i++) {
            al::LiveActor* frame = al::getSubActor(this, "フレーム");
            al::StringTmp<32> sailName("帆%d", i);
            al::LiveActor* sail = al::getSubActor(frame, sailName.cstr());
            if (al::isDead(sail))
                continue;

            const sead::Vector3f& sailTrans = al::getTrans(sail);
            f32 sailX = sailTrans.x;
            f32 sailZ = sailTrans.z;

            sead::Vector3f front = sead::Vector3f::zero;
            al::calcFrontDir(&front, sail);
            front.y = 0.0f;

            sead::Vector3f toHit = sead::Vector3f::zero;
            toHit.x = hitPos.x - sailX;
            toHit.z = hitPos.z - sailZ;
            if (al::calcAngleDegree(front, toHit) < 20.0f) {
                isNearSail = true;
                break;
            }
        }

        if (isNearSail && (al::isMsgPlayerObjTouch(message) || rs::isMsgCapAttack(message))) {
            if (mMeterReactionCoolTime < 1) {
                mMeterReactionCoolTime = 20;
                if (al::isNerve(this, &NrvShineTowerRocket.Wait) ||
                    al::isNerve(this, &NrvShineTowerRocket.Reaction))
                    al::setNerve(this, &NrvShineTowerRocket.Reaction);
            } else if (mMeterReactionCoolTime >= 19) {
                mMeterReactionCoolTime = 20;
                return false;
            }
        }
        return false;
    }

    if (al::isSensorName(self, "MoonTank")) {
        if (!al::isMsgPlayerTrampleReflect(message) && !rs::isMsgCapReflect(message) &&
            !al::isMsgPlayerObjHipDropReflectAll(message))
            return false;

        if (al::isMsgPlayerTrampleReflect(message))
            al::startHitReaction(this, "ムーンタンク踏まれ");
        if (al::isMsgPlayerObjHipDropReflectAll(message))
            al::startHitReaction(this, "ムーンタンクヒップドロップ");

        rs::requestHitReactionToAttacker(message, self, other);
        return true;
    }

    if (al::isSensorName(self, "DokanMapObj")) {
        al::LiveActor* actor = this;
        const al::IUseSceneObjHolder* holder = actor ? actor : nullptr;
        if (GameDataFunction::isCrashHome(GameDataHolderAccessor(holder)))
            return false;
        if (GameDataFunction::isBossAttackedHome(GameDataHolderAccessor(holder)))
            return false;
        if (!GameDataFunction::isEnableCap(GameDataHolderAccessor(holder)))
            return false;
        if (rs::isInvalidChangeStage(this))
            return false;

        if (al::isMsgPlayerObjHipDropAll(message)) {
            mDokanDemoLinkId.setCurrent(1);
            return false;
        }
    }

    if (al::isSensorName(self, "Dokan")) {
        al::LiveActor* actor = this;
        const al::IUseSceneObjHolder* holder = actor ? actor : nullptr;
        if (GameDataFunction::isCrashHome(GameDataHolderAccessor(holder)) ||
            GameDataFunction::isBossAttackedHome(GameDataHolderAccessor(holder)) ||
            !GameDataFunction::isEnableCap(GameDataHolderAccessor(holder)) ||
            rs::isInvalidChangeStage(this))
            return false;

        al::resetPosition(mDokanDemoActor, al::getSensorPos(self));
        mDokanDemoActor->makeActorAlive();

        if (al::isMsgBindStart(message))
            return mDokanPuppetController->judgeMsgBindStart(other, self, mDokanDemoActor,
                                                             mDokanInfo);

        if (al::isMsgBindInit(message)) {
            bool isReverse = mDokanDemoLinkId.isCurrentOrPreviousSet();
            mDokanPuppetController->startBind(other, self, mDokanDemoActor, nullptr, mDokanInfo,
                                             nullptr, isReverse, false);
            al::invalidateClipping(this);
            return true;
        }

        if (al::isMsgBindCancel(message)) {
            mDokanPuppetController->cancelBind();
            return true;
        }
    }

    if (!al::isSensorName(self, "Body"))
        return false;

    if (rs::isMsgYoshiTongueAttack(message) || rs::isMsgGamaneBullet(message)) {
        rs::requestHitReactionToAttacker(message, self, other);
        return true;
    }

    if (al::isMsgPlayerTrampleReflect(message) ||
        al::isMsgPlayerObjHipDropReflectAll(message)) {
        sead::Vector3f hitPos = sead::Vector3f::zero;
        al::calcPosBetweenSensors(&hitPos, other, self, 0.0f);

        sead::Vector3f globePos = sead::Vector3f::zero;
        al::calcJointPos(&globePos, al::getSubActor(this, "地球儀"), "Globe");

        sead::Vector3f globeUp = sead::Vector3f::zero;
        al::calcJointUpDir(&globeUp, al::getSubActor(this, "地球儀"), "Globe");

        sead::Vector3f globeSide = globeUp.cross(sead::Vector3f::ey);
        sead::Vector3f hitDir = hitPos - globePos;
        f32 reactionSign = al::calcAngleDegree(globeSide, hitDir) > 90.0f ? -1.0f : 1.0f;

        if (al::isMsgPlayerTrampleReflect(message)) {
            al::startHitReaction(this, "地球儀踏まれ");
            if (!mDirtyModel || !al::isAlive(mDirtyModel))
                mShineTowerGlobeAnimCtrl->start(reactionSign * 15.0f);
        }

        if (al::isMsgPlayerObjHipDropReflectAll(message)) {
            al::startHitReaction(this, "地球儀ヒップドロップ");
            if (!mDirtyModel || !al::isAlive(mDirtyModel))
                mShineTowerGlobeAnimCtrl->start(reactionSign * 35.0f);
        }

        rs::requestHitReactionToAttacker(message, self, other);
        return true;
    }

    if (rs::isMsgCapKeepLockOn(message)) {
        if (!al::isNerve(this, &NrvShineTowerRocket.Wait) &&
            !al::isNerve(this, &NrvShineTowerRocket.WaitIgnoreLockOn) &&
            !al::isNerve(this, &NrvShineTowerRocket.NoStartEarth) &&
            !al::isNerve(this, &NrvShineTowerRocket.NoStartEnter) &&
            !al::isNerve(this, &NrvShineTowerRocket.NoStartAndCoin))
            return true;

        if (al::isNerve(this, &NrvShineTowerRocket.NoStartEarth) ||
            al::isNerve(this, &NrvShineTowerRocket.NoStartAndCoin)) {
            if (!al::isLessStep(this, 60)) {
                al::setNerve(this, &NrvShineTowerRocket.Wait);
                return false;
            }
            return true;
        }
    }

    if (rs::tryReceiveMsgInitCapTargetAndSetCapTargetInfo(message, mCapTargetInfo))
        return true;

    if (rs::isMsgCapAttack(message))
        return !GameDataFunction::isTalkedCapNearHomeInWaterfall(this);

    if (rs::isMsgCapStartLockOn(message) &&
        (al::isNerve(this, &NrvShineTowerRocket.Wait) ||
         al::isNerve(this, &NrvShineTowerRocket.Reaction))) {
        if (al::isSensorMapObj(self)) {
            if (!GameDataFunction::isTalkedCapNearHomeInWaterfall(this))
                return false;

            if (rs::isModeE3Rom() || rs::isModeE3LiveRom()) {
                al::invalidateClipping(this);
                al::setNerve(this, &NrvShineTowerRocket.NoStartAndCoin);
                return true;
            }

            if (!mDirtyModel || !al::isAlive(mDirtyModel))
                mShineTowerGlobeAnimCtrl->setWorldMapCameraAnim(1.0f, -1);

            al::tryOnStageSwitch(this, "SwitchKidsModeOn");
            s32 currentShineNum =
                GameDataFunction::getCurrentShineNum(GameDataHolderAccessor(this));
            mPlayerRestartLinkId.setCurrent(0);

            if (GameDataFunction::isFindKoopa(GameDataHolderAccessor(this)) ||
                isNeedShowHomeSkyMessage(this) || rs::isInvalidChangeStage(this)) {
                al::setNerve(this, &NrvShineTowerRocket.NoStartEarth);
            } else {
                mLinkObjAliveDeadCtrl->on();
                if (currentShineNum >= 1) {
                    mDemoShineNum = currentShineNum;
                    al::setNerve(this, &NrvShineTowerRocket.DemoPrepare);
                } else {
                    al::setNerve(this, &NrvShineTowerRocket.DemoPrepareNoShine);
                }
            }
            return true;
        }
    } else {
        if (rs::isMsgCapIgnoreCancelLockOn(message) &&
            ((!al::isNerve(this, &NrvShineTowerRocket.Wait) &&
              !al::isNerve(this, &NrvShineTowerRocket.WaitIgnoreLockOn) &&
              !al::isNerve(this, &NrvShineTowerRocket.NoStartEarth) &&
              !al::isNerve(this, &NrvShineTowerRocket.NoStartEnter) &&
              !al::isNerve(this, &NrvShineTowerRocket.NoStartAndCoin)) ||
             al::isNerve(this, &NrvShineTowerRocket.NoStartEarth)))
            return true;

        if (rs::isMsgCapCancelLockOn(message)) {
            if (al::isNerve(this, &NrvShineTowerRocket.NoStartAndCoin))
                al::setNerve(this, &NrvShineTowerRocket.Wait);
            else
                al::setNerve(this, &NrvShineTowerRocket.WaitIgnoreLockOn);
            return true;
        }
    }

    return false;
}

void ShineTowerRocket::attackSensor(al::HitSensor* self, al::HitSensor* other) {
    if ((al::isNerve(this, &NrvShineTowerRocket.DemoPrepare) ||
         al::isNerve(this, &NrvShineTowerRocket.DemoPrepareNoShine)) &&
        al::isSensorEye(self)) {
        rs::sendMsgKillByHomeDemo(other, self);
    }

    if (al::isNerve(this, &NrvShineTowerRocket.Wait) && al::isLessStep(this, 1) &&
        al::isSensorEye(self))
        rs::sendMsgEndHomeDemo(other, self);

    if (al::isSensorName(self, "Body") || al::isSensorName(self, "MoonTank")) {
        if (!rs::sendMsgPushToPlayer(other, self) && !al::sendMsgPush(other, self))
            rs::sendMsgPushToMotorcycle(other, self);
    }
}

void ShineTowerRocket::tryStartHitReactionDemoStart() {
    if (mIsStartHitReactionDemoStart) {
        mIsStartHitReactionDemoStart = false;
        al::startHitReaction(this, "デモ開始");
    }
}

void ShineTowerRocket::exeWait() {
    // NONMATCHING: 10 attempts exhausted.
    // Root cause: the behavior, function size, and local frame size are reconstructed, but
    // clang still colors the GameDataHolderAccessor / ChangeStageInfo stack slots differently.
    // Best source-friendly form uses a 0x2c0 frame and 0x6b8 text size; target reuses the
    // v61 storage at sp+0x18 while current places the same storage at sp+0x28, with entrance
    // activation/boss/enable-cap accessors assigned to different high slots. Tried compound
    // temporaries, scoped named accessors, C++17 if-initializers, nested vs sequential gates,
    // and event/audio scoped-accessor mixes without a full slot match.
    if (al::isFirstStep(this)) {
        validateDitherAnimAll(this);
        validateOcclusionQueryAll(this);
        if (mShineTowerLight)
            mShineTowerLight->setLightingWorldMap(false);
        mLinkObjAliveDeadCtrl->off();
        mMeterRotateDegree = 0.0f;
        if (mDirtyModel && al::isDead(mDirtyModel))
            mEventFlowExecutor = mDemoEventFlowExecutor;
        mPlayerRestartId = calcRestShineNum(this);
        if (mPlayerRestartId != 0)
            rs::startEventFlow(mEventFlowExecutor, "RestShineNum");
        else
            rs::startEventFlow(mEventFlowExecutor, "GoKoopa");
        mShineTowerCommonKeeper->update();
        mShineTowerCommonKeeper->updateSensor();
        if (al::isActiveCamera(mWorldMapFadeCameraTicket))
            al::endCamera(this, mWorldMapFadeCameraTicket, -1, false);

        if (mDirtyModel && al::isAlive(mDirtyModel)) {
            al::startAction(mDirtyModel, "WaitWaterfall");
        } else {
            const al::IUseSceneObjHolder* holder = this;
            if (!GameDataFunction::isLaunchHome(GameDataHolderAccessor(holder)) &&
                GameDataFunction::isActivateHome(GameDataHolderAccessor(holder))) {
                al::tryStartActionIfNotPlaying(this, "WaitWaterfallNormal");
            } else if (mDamageModel && al::isAlive(mDamageModel)) {
                al::tryStartActionIfNotPlaying(mDamageModel, "WaitNormal");
            } else if (mClashModel && al::isAlive(mClashModel)) {
                al::tryStartActionIfNotPlaying(mClashModel, "Wait");
                al::tryOnStageSwitch(this, "SwitchClashOn");
            } else {
                al::tryStartActionIfNotPlaying(this, "WaitNormal");
            }
        }

        bool isDirtyModelAlive = false;
        if (mDirtyModel)
            isDirtyModelAlive = !al::isDead(mDirtyModel);
        bool isClashModelAlive = false;
        if (mClashModel)
            isClashModelAlive = !al::isDead(mClashModel);
        if (!(isDirtyModelAlive || isClashModelAlive))
            al::validateCollisionParts(this);

        if (al::isNerve(this, &NrvShineTowerRocket.WaitAfterReturnToHome))
            al::invalidateClipping(this);
        else
            al::validateClipping(this);

        if (al::isNerve(this, &NrvShineTowerRocket.WaitIgnoreLockOn) &&
            mIsTryStartDemoStarted) {
            rs::endEventCutSceneDemo(this);
            al::validateLodModel(this);
            mIsTryStartDemoStarted = false;
        }

        updateParts();
    }

    const al::IUseSceneObjHolder* holder = this;
    bool isActivateHome = false;
    if (!GameDataFunction::isLaunchHome(GameDataHolderAccessor(holder)))
        isActivateHome = GameDataFunction::isActivateHome(GameDataHolderAccessor(holder));

    bool isClashAlive = false;
    if (mClashModel)
        isClashAlive = al::isAlive(mClashModel);
    bool isStartWaitAction = false;
    if (mDirtyModel)
        isStartWaitAction = !(isClashAlive | al::isAlive(mDirtyModel));
    else if (!isClashAlive)
        isStartWaitAction = true;
    if (isStartWaitAction) {
        bool isNear = al::isNearPlayer(this, 10000.0);
        if (isNear) {
            if (mDamageModel && al::isAlive(mDamageModel))
                al::tryStartActionIfNotPlaying(mDamageModel, "WaitNormal");
            else if (isActivateHome)
                al::tryStartActionIfNotPlaying(this, "WaitWaterfallNormal");
            else
                al::tryStartActionIfNotPlaying(this, "WaitNormal");
        } else {
            if (mDamageModel && al::isAlive(mDamageModel))
                al::tryStartActionIfNotPlaying(mDamageModel, "WaitNormalLow");
            else if (isActivateHome)
                al::tryStartActionIfNotPlaying(this, "WaitWaterfallNormalLow");
            else
                al::tryStartActionIfNotPlaying(this, "WaitNormalLow");
        }
    }

    if (al::isNerve(this, &NrvShineTowerRocket.WaitIgnoreLockOn) &&
        al::isGreaterEqualStep(this, 10)) {
        al::setNerve(this, &NrvShineTowerRocket.Wait);
        return;
    }

    if (al::isNerve(this, &NrvShineTowerRocket.WaitAfterReturnToHome)) {
        if (!rs::isPlayerOnGround(this))
            return;
        al::setNerve(this, &NrvShineTowerRocket.Wait);
        return;
    }

    if (isNearPlayerEntrance() && (mDoorAreaChange->isOpen() || al::isDead(mDoorAreaChange))) {
        if (rs::isInvalidChangeStage(this)) {
            rs::showCapMessage(this, "HomeDoorNoStart", 90, 0);
            al::setNerve(this, &NrvShineTowerRocket.NoStartEnter);
            return;
        }

        if (!GameDataFunction::isActivateHome(GameDataHolderAccessor(holder)))
            return;
        {
            GameDataHolderAccessor crashHomeAccessor(holder);
            if (GameDataFunction::isCrashHome(crashHomeAccessor))
                return;
        }
        {
            GameDataHolderAccessor bossAttackedHomeAccessor(holder);
            if (GameDataFunction::isBossAttackedHome(bossAttackedHomeAccessor))
                return;
        }
        {
            GameDataHolderAccessor enableCapAccessor(holder);
            if (!GameDataFunction::isEnableCap(enableCapAccessor))
                return;
        }
        if (rs::isInvalidChangeStage(this))
            return;

        GameDataHolder* gameDataHolder = GameDataFunction::getGameDataHolder(holder);
        ChangeStageInfo changeStageInfo(gameDataHolder, "HomeEntrance",
                                        GameDataFunction::getHomeShipStageName());
        GameDataFunction::tryChangeNextStage(GameDataHolderWriter(holder), &changeStageInfo);
    }

    if (mMtxConnector)
        al::connectPoseQT(this, mMtxConnector);

    {
        GameDataHolderAccessor findKoopaAccessor(holder);
        if (!GameDataFunction::isFindKoopa(findKoopaAccessor)) {
            if (!rs::isExistKoopaShipInSky(this)) {
                GameDataHolderAccessor unlockedAllWorldAccessor(holder);
                if (!GameDataFunction::isUnlockedAllWorld(unlockedAllWorldAccessor))
                    rs::updateEventFlow(mEventFlowExecutor);
            }
        }
    }

    if (mBackDoor) {
        if (rs::isSequenceGoToNextWorld(holder)) {
            if (al::isDead(mBackDoor)) {
                mBackDoor->appear();
                mBackDoor->reAppear();
            }
        } else if (al::isAlive(mBackDoor)) {
            mBackDoor->kill();
        }
    }

    if (mDokanPuppetController->isBackDoorChangeStageRequested()) {
        GameDataHolder* gameDataHolder = GameDataFunction::getGameDataHolder(holder);
        ChangeStageInfo changeStageInfo(gameDataHolder, "HomeBackDoor",
                                        GameDataFunction::getHomeShipStageName());
        GameDataFunction::tryChangeNextStage(GameDataHolderWriter(holder), &changeStageInfo);
    }

    if (!mShineTowerGlobeAnimCtrl->isHoldingMusicBox()) {
        {
            GameDataHolderAccessor crashHomeAccessor(holder);
            if (!GameDataFunction::isCrashHome(crashHomeAccessor)) {
                GameDataHolderAccessor bossAttackedHomeAccessor(holder);
                if (!GameDataFunction::isBossAttackedHome(bossAttackedHomeAccessor)) {
                    GameDataHolderAccessor activateHomeAccessor(holder);
                    if (GameDataFunction::isActivateHome(activateHomeAccessor)) {
                        al::holdSe(this, sead::SafeString("Engine_loop"));
                        al::holdSe(this, sead::SafeString("Tank_loop"));
                    }
                }
            }
        }

        GameDataHolderAccessor playDemoWorldWarpAccessor(holder);
        if (!GameDataFunction::isPlayDemoWorldWarp(playDemoWorldWarpAccessor)) {
            GameDataHolderAccessor crashHomeAccessor(holder);
            if (GameDataFunction::isCrashHome(crashHomeAccessor)) {
                al::holdSe(mClashModel, sead::SafeString("Crash_loop"));
            } else {
                GameDataHolderAccessor bossAttackedHomeAccessor(holder);
                if (GameDataFunction::isBossAttackedHome(bossAttackedHomeAccessor))
                    al::holdSe(mClashModel, sead::SafeString("Crash_loop"));
            }
        }
    }
}

void ShineTowerRocket::exeWaitDemo() {}

bool ShineTowerRocket::isActiveDamageModel() const {
    return mDamageModel && al::isAlive(mDamageModel);
}

bool ShineTowerRocket::isActiveDirtyOrClashModel() const {
    if (mDirtyModel && al::isAlive(mDirtyModel))
        return true;
    return mClashModel && al::isAlive(mClashModel);
}

void ShineTowerRocket::exeBackDoor() {
    rs::addPuppetVelocityFall(mPlayerPuppet);
}

void ShineTowerRocket::exeNoStartAndCoin() {
    if (al::isFirstStep(this) && mIsAppearCoin) {
        mIsAppearCoin = false;

        sead::Vector3f trans = sead::Vector3f::zero;
        al::LiveActor* dirtyModel = mDirtyModel;
        if (dirtyModel && al::isAlive(dirtyModel))
            al::calcJointPos(&trans, dirtyModel, "GlobeCapPoint");
        else
            al::calcJointPos(&trans, this, "GlobeCapPoint");

        sead::Quatf quat;
        al::calcQuat(&quat, this);
        rs::appearItemFromObj(this, trans, quat, 80.0f);
    }
}

void ShineTowerRocket::exeNoStartEarth() {
    if (al::isFirstStep(this)) {
        al::LiveActor* actor = this;
        const al::IUseSceneObjHolder* holder = actor;
        if (GameDataFunction::isFindKoopa(GameDataHolderAccessor(holder)))
            rs::showCapMessage(this, "Home_Cloud", 90, 0);
        else if (isNeedShowHomeSkyMessage(this))
            rs::showCapMessage(this, "Home_Sky", 90, 0);
        else
            rs::showCapMessage(this, "HomeEarthNoStart", 90, 0);
    }
}

void ShineTowerRocket::exeNoStart() {
    if (al::isFirstStep(this)) {
        if (al::isActiveCamera(mDemoAppearFromHomeCameraTicket))
            al::endCamera(this, mDemoAppearFromHomeCameraTicket, -1, false);
        if (!al::isActiveCamera(mWorldMapFadeCameraTicket))
            al::startCamera(this, mWorldMapFadeCameraTicket, 0);
    }

    if (al::isStep(this, 60)) {
        if (GameDataFunction::getCurrentWorldId(GameDataHolderAccessor(this)) == 1)
            if (mIsFirstPayNotEnough)
                rs::startEventFlow(mEventFlowExecutor, "FirstPayNotEnough");
            else
                rs::startEventFlow(mEventFlowExecutor, "PayNotEnoughAfterBreeda");
        else
            rs::startEventFlow(mEventFlowExecutor, "Crash");
        rs::setDemoInfoDemoName(this, "ワールドマップデモ");
    }

    if (al::isGreaterEqualStep(this, 60) && rs::updateEventFlow(mEventFlowExecutor)) {
        if (al::isActiveCamera(mWorldMapFadeCameraTicket))
            al::endCamera(this, mWorldMapFadeCameraTicket, -1, false);
        tryStartEntranceCamera(-1);
        al::setNerve(this, &NrvShineTowerRocket.WaitIgnoreLockOn);
    }
}

void ShineTowerRocket::exeGoToWorldMapWithCamera() {
    if (al::isFirstStep(this)) {
        setupWorldMapCameraParam();
        if (al::isActiveCamera(mWorldMapFadeCameraTicket))
            al::endCamera(this, mWorldMapFadeCameraTicket, -1, false);
        al::startCamera(this, mWorldMapCameraTicket, 0);
    }

    mWorldMapCameraPosRateParam->calcLerpValue(&mWorldMapCameraPos,
                                               al::calcNerveSquareOutRate(this, 30));
    mWorldMapCameraAtRateParam->calcLerpValue(&mWorldMapCameraAt,
                                              al::calcNerveSquareOutRate(this, 30));

    if (al::isGreaterEqualStep(this, 40)) {
        mIsWorldMapCamera = true;
        mShineTowerGlobeAnimCtrl->setWorldMapCameraAnim(1.0f, -1);
        al::setNerve(this, &WorldMap);
    }
}

void ShineTowerRocket::setupWorldMapCameraParam() {
    sead::Vector3f* cameraAt = &mWorldMapCameraAt;

    const sead::Vector3f& cameraAtCurrent = al::getCameraAt(this, 0);
    new (cameraAt) sead::Vector3f(cameraAtCurrent);

    sead::Vector3f* cameraPos = &mWorldMapCameraPos;
    const sead::Vector3f& cameraPosCurrent = al::getCameraPos(this, 0);
    new (cameraPos) sead::Vector3f(cameraPosCurrent);

    sead::Vector3f capPoint = sead::Vector3f::zero;
    al::LiveActor* dirtyModel = mDirtyModel;
    if (dirtyModel && al::isAlive(dirtyModel))
        al::calcJointPos(&capPoint, dirtyModel, "GlobeCapPoint");
    else
        al::calcJointPos(&capPoint, this, "GlobeCapPoint");

    sead::Vector3f atOffset = sWorldMapCameraAtOffset;
    al::multVecPoseNoTrans(&atOffset, this, atOffset);
    mWorldMapCameraAtRateParam->setParam(*cameraAt, capPoint + atOffset);

    sead::Vector3f cameraEnd = sead::Vector3f::zero;
    dirtyModel = mDirtyModel;
    if (dirtyModel && al::isAlive(dirtyModel))
        al::calcJointPos(&cameraEnd, dirtyModel, "GlobeCapPoint");
    else
        al::calcJointPos(&cameraEnd, this, "GlobeCapPoint");

    sead::Vector3f posOffset = sWorldMapCameraPosOffset;
    al::multVecPoseNoTrans(&posOffset, this, posOffset);
    mWorldMapCameraPosRateParam->setParam(*cameraPos, cameraEnd + posOffset);
}

void ShineTowerRocket::exeNoStartEnter() {
    if (isNearPlayerEntrance() && mDoorAreaChange->isOpen())
        al::setNerve(this, &NrvShineTowerRocket.NoStartEnter);
    else if (al::isGreaterEqualStep(this, 60))
        al::setNerve(this, &NrvShineTowerRocket.Wait);
}

void ShineTowerRocket::exeReaction() {
    if (al::isFirstStep(this)) {
        al::startHitReaction(this, "帆が揺れる");
        al::startAction(al::getSubActor(this, "フレーム"), "Reaction");
    }

    if (al::isActionEnd(al::getSubActor(this, "フレーム")))
        al::setNerve(this, &NrvShineTowerRocket.Wait);
}

bool ShineTowerRocket::isNearPlayerEntrance() const {
    sead::Vector3f playerPos;
    sead::Vector3f parallel;
    sead::Vector3f vertical;
    sead::Vector3f entranceFront(mEntranceCameraFront);
    new (&playerPos) sead::Vector3f(sead::Vector3f::zero);
    if (!al::tryFindNearestPlayerPos(&playerPos, this))
        return false;

    if (al::isNearPlayer(this, 200.0f))
        return true;

    f32 angle = al::calcAngleToTargetH(this, playerPos);
    if (angle > 90.0f || angle < -90.0f)
        return false;

    const sead::Vector3f& trans = al::getTrans(this);
    playerPos.x -= trans.x;
    f32 height = playerPos.y - trans.y;
    playerPos.z -= trans.z;
    new (&parallel) sead::Vector3f(sead::Vector3f::zero);
    new (&vertical) sead::Vector3f(sead::Vector3f::zero);
    playerPos.y = 0.0f;
    al::parallelizeVec(&parallel, entranceFront, playerPos);
    al::verticalizeVec(&vertical, entranceFront, playerPos);
    if (!(parallel.length() < 140.0f))
        return false;
    if (!(vertical.length() < 100.0f))
        return false;

    return height > 10.0f && height < 600.0f;
}

void ShineTowerRocket::exeDemoPrepare() {
    if (al::isFirstStep(this)) {
        al::requestCaptureScreenCover(this, 4);
        al::invalidateClipping(this);
        if (mBackDoor)
            al::killForceBeforeDemo(mBackDoor);
        mIsFirstPayNotEnough = false;
        mIsCompleteShine = false;
        mIsDemoPeachCastleCap = false;
        invalidateDitherAnimAll(this);
        invalidateOcclusionQueryAll(this);
    }

    if (!al::isLessStep(this, 2) && tryStartDemo()) {
        if (al::isActiveCamera(mEntranceCameraTicket))
            al::endCamera(this, mEntranceCameraTicket, -1, false);
        rs::addDemoLockOnCap(this);

        if (al::isNerve(this, &NrvShineTowerRocket.DemoPrepareNoShine)) {
            al::setNerve(this, &NrvShineTowerRocket.DemoWalkPlayerToPointNoShine);
        } else if (!GameDataFunction::isLaunchHome(GameDataHolderAccessor(this)) ||
                   GameDataFunction::isCrashHome(GameDataHolderAccessor(this)) ||
                   GameDataFunction::isBossAttackedHome(GameDataHolderAccessor(this)) ||
                   isPayShineEnoughForUnlock(this) ||
                   GameDataFunction::isGameClear(GameDataHolderAccessor(this))) {
            al::setNerve(this, &NrvShineTowerRocket.DemoAppearShine);
        } else {
            al::setNerve(this, &NrvShineTowerRocket.DemoMeterRotate);
        }
    }
}

bool ShineTowerRocket::tryStartDemo() {
    if (!rs::tryStartEventCutSceneDemo(this))
        return false;

    mIsStartHitReactionDemoStart = true;

    if (!mIsPlayDemoAwardSpecial)
        rs::setMarioGroundDepthShadowMapLength(this, 50.0f);

    if (!al::isHideModel(this) && mShineTowerRock)
        al::tryStartActionIfNotPlaying(this, "WaitWaterfallDemo");
    else if (!al::isHideModel(this))
        al::tryStartActionIfNotPlaying(this, "WaitDemo");
    else if (mDamageModel && al::isAlive(mDamageModel))
        al::tryStartActionIfNotPlaying(mDamageModel, "WaitDemo");

    al::invalidateClipping(this);
    rs::requestValidateDemoSkip(static_cast<IUseDemoSkip*>(this), this);
    al::invalidateLodModel(this);
    for (s32 i = 0; i < 55; i++)
        rs::addDemoActor(mDemoShineGroup->getActor(i), false);

    rs::addDemoActor(mPoseCopyActor, true);
    al::addDemoActorFromAddDemoInfo(this, mAddDemoInfo);

    mIsTryStartDemoStarted = true;
    mIsDemoWaitingToEnd = false;

    return true;
}

void ShineTowerRocket::exeDemoWalkPlayerToPoint() {
    if (al::isFirstStep(this)) {
        calcPlayerPoseForPayDemo();
        al::startCamera(this, mWorldMapFadeCameraTicket, 0);
        if (mIsStartHitReactionDemoStart) {
            mIsStartHitReactionDemoStart = false;
            al::startHitReaction(this, "帆が揺れる");
        }
    }

    rs::calcDemoMarioJointPosAllRoot(&mDemoPayPlayerTrans, this);
    mDemoPayPlayerTrans.y -= 5.0f;

    if (isNeedDemoWalkPlayerToPoint(this)) {
        al::setNerve(this, &NrvShineTowerRocket.NoStart);
        return;
    }

    if (GameDataFunction::checkEnableUnlockWorldSpecial1(this) ||
        GameDataFunction::checkEnableUnlockWorldSpecial2(this) ||
        isEnableUnlockWorldByPayShine(
            this, GameDataFunction::getCurrentWorldId(GameDataHolderAccessor(this)), 0)) {
        mIsWorldMap = true;
        al::setNerve(this, &NrvShineTowerRocket.GoToWorldMapWithFade);
    } else {
        al::setNerve(this, &NrvShineTowerRocket.DemoSelectGoOtherWorld);
    }
}

void ShineTowerRocket::calcPlayerPoseForPayDemo() {
    rs::startActionDemoPlayer(this, "DemoPayToHome");
    rs::clearDemoAnimInterpolatePlayer(this);
    rs::replaceDemoPlayer(this, al::getTrans(this), al::getQuat(this));

    sead::Vector3f playerTrans;
    rs::calcDemoMarioJointPosAllRoot(&playerTrans, this);

    sead::Quatf quat = sead::Quatf::unit;
    al::calcQuat(&quat, this);
    al::rotateQuatYDirDegree(&quat, quat, 180.0f);

    rs::startActionDemoPlayer(this, "Wait");
    rs::clearDemoAnimInterpolatePlayer(this);

    sead::Vector3f hitPos;

    sead::Vector3f arrowStart;
    arrowStart.setScaleAdd(50.0f, sead::Vector3f::ey, playerTrans);
    alCollisionUtil::getHitPosOnArrow(this, &hitPos, arrowStart, sead::Vector3f::ey * -100.0f,
                                      nullptr, nullptr);
    rs::replaceDemoPlayer(this, hitPos, quat);
}

void ShineTowerRocket::exeDemoAppearShine() {
    if (al::isFirstStep(this)) {
        if (mIsStartHitReactionDemoStart) {
            mIsStartHitReactionDemoStart = false;
            al::startHitReaction(this, "デモ開始");
        }

        const al::IUseSceneObjHolder* holder = this;
        GameDataFunction::setRequireSave(GameDataHolderWriter(this));
        rs::hideDemoPlayerSilhouette(this);
        rs::startActionDemoPlayer(this, "DemoPayToHome");

        const sead::Vector3f& trans = al::getTrans(this);
        const sead::Quatf& quat = al::getQuat(this);
        rs::replaceDemoPlayer(this, trans, quat);

        bool isAppearCameraActive = al::isActiveCamera(mDemoAppearFromHomeCameraTicket);
        const al::IUseCamera* cameraUser = this;
        if (isAppearCameraActive)
            al::endCamera(cameraUser, mDemoAppearFromHomeCameraTicket, -1, false);

        al::makeMtxSRT(&mDemoPlayerMtx, this);

        if (GameDataFunction::isLaunchHome(GameDataHolderAccessor(holder)))
            al::startAnimCamera(cameraUser, mDemoAppearFromHomeCameraTicket, "DemoPayToHome", 0);
        else
            al::startAnimCamera(cameraUser, mDemoAppearFromHomeCameraTicket,
                                "DemoPayToHomeWorldWaterfall", 0);

        for (s32 i = 0; i < 55; i++) {
            DemoShine* demoShine = mDemoShineGroup->getDeriveActor(i);
            demoShine->makeActorDead();
            al::invalidateClipping(demoShine);
            if (i <= 10 && i < mDemoShineNum) {
                const sead::Quatf& demoQuat = al::getQuat(this);
                const sead::Vector3f& demoTrans = al::getTrans(this);
                al::resetQuatPosition(demoShine, demoQuat, demoTrans);
                demoShine->startDemo(i);
            }
        }

        s32 demoShineIndex = mDemoShineNum;
        if (demoShineIndex >= 10)
            demoShineIndex = 10;
        mIsDemoShineInEffectEmitted = false;
        mDemoShineIndex = demoShineIndex;

        if (GameDataFunction::getTotalPayShineNum(GameDataHolderAccessor(holder)) < 999) {
            s32 totalPayShineNum =
                GameDataFunction::getTotalPayShineNum(GameDataHolderAccessor(holder));
            if (mDemoShineNum + totalPayShineNum >= 999)
                mIsCompleteShine = true;
        }

        if (GameDataFunction::getPayShineNum(GameDataHolderAccessor(holder)) == 0)
            mIsFirstPayNotEnough = true;

        rs::setDemoInfoDemoName(this, "ムーン奉納デモ");
    }

    if (al::isIntervalStep(this, 5, 0) && mDemoShineIndex < mDemoShineNum) {
        DemoShine* demoShine = mDemoShineGroup->tryFindDeadDeriveActor();
        if (!demoShine)
            return;

        mDemoShineIndex++;
        const sead::Quatf& demoQuat = al::getQuat(this);
        const sead::Vector3f& demoTrans = al::getTrans(this);
        al::resetQuatPosition(demoShine, demoQuat, demoTrans);
        demoShine->startDemo(mDemoShineIndex);
    }

    bool isAllDead = true;
    for (s32 i = 0; i < 55; i++) {
        DemoShine* demoShine = mDemoShineGroup->getDeriveActor(i);
        if (!mIsDemoShineInEffectEmitted && demoShine->isReactionStarted()) {
            mIsDemoShineInEffectEmitted = true;
            al::pauseActiveBgm(this, 180);
            if (mDirtyModel)
                al::tryEmitEffect(mDirtyModel, "DemoShineIn", nullptr);
            else
                al::tryEmitEffect(this, "DemoShineIn", nullptr);
        }

        bool isDead = !al::isAlive(demoShine);
        isAllDead &= isDead;
        if (!isDead)
            break;
    }

    if (isAllDead) {
        if (mDirtyModel)
            al::tryDeleteEffect(mDirtyModel, "DemoShineIn");
        else
            al::tryDeleteEffect(this, "DemoShineIn");
        al::setNerve(this, &DemoWaitAfterAppearShine);
    }
}

void ShineTowerRocket::exeDemoWaitAfterAppearShine() {
    if (al::isGreaterEqualStep(this, 30)) {
        calcPlayerPoseForPayDemo();
        al::LiveActor* actor = this;
        const al::IUseSceneObjHolder* holder = actor;
        if (GameDataFunction::isCrashHome(GameDataHolderAccessor(holder)) ||
            GameDataFunction::isBossAttackedHome(GameDataHolderAccessor(holder))) {
            if (tryLevelUp())
                return;
            al::setNerve(this, &NrvShineTowerRocket.NoStart);
            return;
        }

        if (isPayShineEnoughForUnlock(this) ||
            GameDataFunction::isGameClear(GameDataHolderAccessor(this)))
            al::setNerve(this, &NrvShineTowerRocket.DemoWaitBeforeScaleUpDirect);
        else
            al::setNerve(this, &NrvShineTowerRocket.DemoMeterUpPrev);
    }
}

bool ShineTowerRocket::tryLevelUp() {
    bool wasAllPaid = GameDataFunction::isPayShineAllInAllWorld(GameDataHolderAccessor(this));
    s32 currentWorldId = GameDataFunction::getCurrentWorldId(GameDataHolderAccessor(this));
    bool isLevelUp = isEnableUnlockWorldByPayShine(this, currentWorldId, mDemoShineNum);
    s32 payShineNum = mDemoShineNum;
    al::LiveActor* actor = this;
    const al::IUseSceneObjHolder* holder = actor ? actor : nullptr;

    if (GameDataFunction::isGameClear(GameDataHolderAccessor(holder)))
        GameDataFunction::addPayShineCurrentAll(GameDataHolderWriter(holder));
    else
        GameDataFunction::addPayShine(GameDataHolderWriter(holder), payShineNum);

    if (isLevelUp) {
        rs::calcDemoMarioJointPosAllRoot(&mDemoPayPlayerTrans, this);
        mDemoPayPlayerTrans.y -= 5.0f;
        mDemoPayPlayerQuat.set(rs::getDemoPlayerQuat(this));

        if (!wasAllPaid && GameDataFunction::isPayShineAllInAllWorld(GameDataHolderAccessor(this))) {
            mIsDemoPeachCastleCap = true;
            al::setNerve(this, &NrvShineTowerRocket.DemoInformCompleteShineFadeIn);
            return true;
        }

        if (GameDataFunction::getCurrentWorldId(GameDataHolderAccessor(this)) ==
            GameDataFunction::getWorldIndexSky()) {
            if (!GameDataFunction::isGameClear(GameDataHolderAccessor(this))) {
                mIsWorldMap = true;
                al::rotateQuatYDirDegree(&mDemoPayPlayerQuat, mDemoPayPlayerQuat, 180.0f);
                al::setNerve(this, &NrvShineTowerRocket.DemoUpLevelCloseFade);
                return true;
            }
        }

        if (GameDataFunction::isCrashHome(GameDataHolderAccessor(holder)) ||
            GameDataFunction::isBossAttackedHome(GameDataHolderAccessor(holder)) ||
            !GameDataFunction::isLaunchHome(GameDataHolderAccessor(this))) {
            al::setNerve(this, &NrvShineTowerRocket.DemoUpLevelCloseFade);
        } else {
            al::setNerve(this, &NrvShineTowerRocket.DemoUpLevelCamera);
        }
        return true;
    }

    if (wasAllPaid)
        return false;

    if (GameDataFunction::isPayShineAllInAllWorld(GameDataHolderAccessor(this)))
        mIsDemoPeachCastleCap = true;
    return false;
}

void ShineTowerRocket::exeDemoWaitBeforeScaleUpDirect() {
    if (al::isFirstStep(this)) {
        if (!mHomeMeterActor) {
            al::LiveActor* meterFrame = al::getSubActor(this, "フレーム");
            al::StringTmp<32> name("帆%d", 1);
            mHomeMeterActor = al::getSubActor(meterFrame, name.cstr());
        }

        calcCameraMtxMeterUpPrev();

        if (al::isActiveCamera(mDemoAppearFromHomeCameraTicket) && !mIsDemoWaitingToEnd) {
            al::CameraTicket* ticket = mDemoAppearFromHomeCameraTicket;
            const char* meterAnim = "Meter";
            s32 startStep = al::getSklAnimFrameMax(mHomeMeterActor, meterAnim);
            s32 endStep = al::getSklAnimFrameMax(mHomeMeterActor, meterAnim);
            al::startAnimCameraAnim(ticket, "DemoMeterUp", startStep, endStep, 0);
        }

        if (!al::isActiveCamera(mDemoAppearFromHomeCameraTicket)) {
            const al::IUseCamera* cameraUser = this;
            al::CameraTicket* ticket = mDemoAppearFromHomeCameraTicket;
            const char* meterAnim = "Meter";
            s32 startStep = al::getSklAnimFrameMax(mHomeMeterActor, meterAnim);
            s32 endStep = al::getSklAnimFrameMax(mHomeMeterActor, meterAnim);
            al::startAnimCameraWithStartStepAndEndStepAndPlayStep(
                cameraUser, ticket, "DemoMeterUp", startStep, endStep, 0, 0);
        }

        mIsDemoWaitingToEnd = true;
    }

    if (al::isGreaterEqualStep(this, 75))
        al::setNerve(this, &DemoScaleUp);
}

void ShineTowerRocket::calcCameraMtxMeterUpPrev() {
    al::makeMtxSRT(&mDemoPlayerMtx, this);

    f32 m00 = mDemoPlayerMtx.a[0];
    f32 m01 = mDemoPlayerMtx.a[1];
    f32 m02 = mDemoPlayerMtx.a[2];
    sead::Vector3f trans;
    trans.x = mDemoPlayerMtx.a[3];
    f32 m10 = mDemoPlayerMtx.a[4];
    f32 m11 = mDemoPlayerMtx.a[5];
    f32 m12 = mDemoPlayerMtx.a[6];
    trans.y = mDemoPlayerMtx.a[7];
    f32 m20 = mDemoPlayerMtx.a[8];
    f32 m21 = mDemoPlayerMtx.a[9];
    f32 m22 = mDemoPlayerMtx.a[10];
    trans.z = mDemoPlayerMtx.a[11];
    f32 scaleRate = al::lerpValue(mShineTowerCommonKeeper->calcScale(), 0.4f, 2.0f, 0.0f, 1.0f);
    f32 scale = al::lerpValue(0.55f, 1.6f, scaleRate);
    mDemoPlayerMtx.a[0] = m00 * scale;
    mDemoPlayerMtx.a[4] = m10 * scale;
    mDemoPlayerMtx.a[8] = m20 * scale;
    mDemoPlayerMtx.a[1] = m01 * scale;
    mDemoPlayerMtx.a[5] = m11 * scale;
    mDemoPlayerMtx.a[9] = m21 * scale;
    mDemoPlayerMtx.a[2] = m02 * scale;
    mDemoPlayerMtx.a[6] = m12 * scale;
    mDemoPlayerMtx.a[10] = m22 * scale;
    mDemoPlayerMtx.setTranslation(trans);

    al::LiveActor* actor = this;
    const al::IUseSceneObjHolder* holder = actor ? actor : nullptr;
    f32 angle = 0.0f;
    {
        GameDataHolderAccessor accessor(holder);
        if (GameDataFunction::isWorldSea(accessor)) {
            angle = -90.0f;
        } else {
            GameDataHolderAccessor accessor2(holder);
            if (GameDataFunction::isWorldLake(accessor2))
                angle = 90.0f;
        }
    }
    al::rotateMtxYDirDegree(&mDemoPlayerMtx, mDemoPlayerMtx, angle);

    sead::Vector3f scaleRoot = sead::Vector3f::zero;
    al::getJointLocalTrans(&scaleRoot, getModelKeeper(), "ScaleRoot");
    f32 rootX = scaleRoot.x;
    f32 rootY = scaleRoot.y;
    f32 rootZ = scaleRoot.z;
    f32 scaleOffset = scale - 1.0f;
    mDemoPlayerMtx.a[3] -= scaleOffset * rootX;
    mDemoPlayerMtx.a[7] -= scaleOffset * rootY;
    mDemoPlayerMtx.a[11] -= scaleOffset * rootZ;

    f32 meterOffset = al::lerpValue(-450.0f, 750.0f, scaleRate);
    sead::Vector3f offset;
    offset.setScale(sead::Vector3f::ey, meterOffset);
    mDemoPlayerMtx.a[3] += offset.x;
    mDemoPlayerMtx.a[7] += offset.y;
    mDemoPlayerMtx.a[11] += offset.z;
}

void ShineTowerRocket::exeDemoScaleUp() {
    bool isScaleAndColorChange = false;
    if (!mDirtyModel || !al::isAlive(mDirtyModel)) {
        s32 currentWorldId = GameDataFunction::getCurrentWorldId(GameDataHolderAccessor(this));
        if (isEnableUnlockWorldByPayShine(this, currentWorldId, mDemoShineNum)) {
            currentWorldId = GameDataFunction::getCurrentWorldId(GameDataHolderAccessor(this));
            if (currentWorldId != GameDataFunction::getWorldIndexClash()) {
                s32 bossWorldId = GameDataFunction::getWorldIndexBoss();
                if (currentWorldId != 1 && currentWorldId != bossWorldId &&
                    !GameDataFunction::isGameClear(GameDataHolderAccessor(this))) {
                    isScaleAndColorChange =
                        !GameDataFunction::isGameClear(GameDataHolderAccessor(this));
                }
            }
        }
    }

    if (al::isFirstStep(this)) {
        mShineTowerCommonKeeper->startDemo(mDemoShineNum);
        if (!mHomeMeterActor) {
            al::LiveActor* meterFrame = al::getSubActor(this, "フレーム");
            al::StringTmp<32> name("帆%d", 1);
            mHomeMeterActor = al::getSubActor(meterFrame, name.cstr());
        }

        if ((isScaleAndColorChange & 1) != 0)
            al::startHitReaction(mHomeMeterActor, "帆スケールして色変化する");
        else
            al::startHitReaction(mHomeMeterActor, "帆スケールする");
        al::startHitReaction(this, "帆スケールする");
        rs::setDemoInfoDemoName(this, "ホームスケールアップデモ");
    }

    mShineTowerCommonKeeper->updateForDemo();
    if ((isScaleAndColorChange & al::isStep(this, 30)) != 0) {
        al::startMclAnimAndSetFrameAndStop(mHomeMeterActor, "Color", 0.0f);
        al::startMtpAnimAndSetFrameAndStop(mHomeMeterActor, "Color", 0.0f);
    }

    if (!mShineTowerCommonKeeper->isEndDemo() || !al::isGreaterEqualStep(this, 160))
        return;

    if (mDirtyModel && al::isAlive(mDirtyModel)) {
        if (al::isActiveCamera(mDemoAppearFromHomeCameraTicket))
            al::endCamera(this, mDemoAppearFromHomeCameraTicket, -1, false);
        if (tryLevelUp())
            return;
        if (isNeedDemoWalkPlayerToPoint(this)) {
            if (!al::isActiveCamera(mWorldMapFadeCameraTicket))
                al::startCamera(this, mWorldMapFadeCameraTicket, -1);
            al::setNerve(this, &NrvShineTowerRocket.DemoWaitBeforeScaleUpDirect);
            return;
        }
    }

    s32 totalPayShineNum = GameDataFunction::getTotalPayShineNum(GameDataHolderAccessor(this));
    s32 nextTotalPayShineNum =
        mDemoShineNum + GameDataFunction::getTotalPayShineNum(GameDataHolderAccessor(this));
    bool isGameClear = GameDataFunction::isGameClear(GameDataHolderAccessor(this));
    if (rs::checkExistNewShopItem(this, totalPayShineNum, nextTotalPayShineNum, isGameClear)) {
        al::setNerve(this, &NrvShineTowerRocket.DemoInformNewItem);
        return;
    }

    if (tryLevelUp())
        return;

    if (isNeedDemoWalkPlayerToPoint(this)) {
        al::setNerve(this, &NrvShineTowerRocket.DemoWaitBeforeScaleUpDirect);
        return;
    }

    if (mIsDemoPeachCastleCap) {
        al::setNerve(this, &NrvShineTowerRocket.DemoInformCompleteShineFadeIn);
        return;
    }

    if (mIsCompleteShine) {
        al::setNerve(this, &NrvShineTowerRocket.DemoInformPeachCastleCap);
        return;
    }

    calcPlayerPoseForPayDemo();
    if (al::isActiveCamera(mWorldMapFadeCameraTicket))
        al::endCamera(this, mWorldMapFadeCameraTicket, -1, false);
    if (al::isActiveCamera(mDemoAppearFromHomeCameraTicket))
        al::endCamera(this, mDemoAppearFromHomeCameraTicket, -1, false);
    tryStartEntranceCamera(0);
    al::resumeActiveBgm(this, 180);
    al::setNerve(this, &NrvShineTowerRocket.WaitIgnoreLockOn);
}

void ShineTowerRocket::exeDemoMeterRotate() {
    if (al::isFirstStep(this)) {
        calcCameraMtx();
        al::startAnimCamera(this, mDemoAppearFromHomeCameraTicket, "DemoMeterRotate", 0);
        mMeterRotateDegree = 0.0f;
        rs::setDemoInfoDemoName(this, "ホームメーター回転デモ");
        if (mIsStartHitReactionDemoStart) {
            mIsStartHitReactionDemoStart = false;
            al::startHitReaction(this, "デモ開始");
        }
    }

    al::LiveActor* actor = this;
    const al::IUseSceneObjHolder* holder = actor;
    s32 homeLevel = GameDataFunction::getHomeLevel(GameDataHolderAccessor(holder));
    u32 meterLevel = rs::isModeE3MovieRom() ? 3 : homeLevel;
    s32 rotateLevel = meterLevel - 1;
    if (rotateLevel < 0)
        rotateLevel = meterLevel;

    f32 rotateDegree = ((rotateLevel >> 1) * 40.0f) + 40.0f;
    if ((meterLevel & 1) != 0)
        rotateDegree = -rotateDegree;
    mMeterRotateDegree = al::calcNerveValue(this, 60, 0.0f, rotateDegree);

    if (al::isStep(this, 63))
        al::startHitReaction(this, "奉納デモ[帆の回転終了]");

    if (al::isEndAnimCamera(mDemoAppearFromHomeCameraTicket)) {
        setupRotateMeter();
        al::setNerve(this, &NrvShineTowerRocket.DemoAppearShine);
    }
}

void ShineTowerRocket::calcCameraMtx() {
    al::makeMtxSRT(&mDemoPlayerMtx, this);

    f32 m00 = mDemoPlayerMtx.a[0];
    f32 m01 = mDemoPlayerMtx.a[1];
    f32 m02 = mDemoPlayerMtx.a[2];
    sead::Vector3f trans;
    trans.x = mDemoPlayerMtx.a[3];
    f32 m10 = mDemoPlayerMtx.a[4];
    f32 m11 = mDemoPlayerMtx.a[5];
    f32 m12 = mDemoPlayerMtx.a[6];
    trans.y = mDemoPlayerMtx.a[7];
    f32 m20 = mDemoPlayerMtx.a[8];
    f32 m21 = mDemoPlayerMtx.a[9];
    f32 m22 = mDemoPlayerMtx.a[10];
    trans.z = mDemoPlayerMtx.a[11];
    f32 scale = mShineTowerCommonKeeper->calcScale();
    al::lerpValue(scale, 0.4f, 2.0f, 0.0f, 1.0f);
    mDemoPlayerMtx.a[0] = m00 * scale;
    mDemoPlayerMtx.a[4] = m10 * scale;
    mDemoPlayerMtx.a[8] = m20 * scale;
    mDemoPlayerMtx.a[1] = m01 * scale;
    mDemoPlayerMtx.a[5] = m11 * scale;
    mDemoPlayerMtx.a[9] = m21 * scale;
    mDemoPlayerMtx.a[2] = m02 * scale;
    mDemoPlayerMtx.a[6] = m12 * scale;
    mDemoPlayerMtx.a[10] = m22 * scale;
    mDemoPlayerMtx.setTranslation(trans);

    al::LiveActor* actor = this;
    const al::IUseSceneObjHolder* holder = actor ? actor : nullptr;
    f32 angle = 0.0f;
    {
        GameDataHolderAccessor accessor(holder);
        if (GameDataFunction::isWorldSea(accessor)) {
            angle = -90.0f;
        } else {
            GameDataHolderAccessor accessor2(holder);
            if (GameDataFunction::isWorldLake(accessor2))
                angle = 90.0f;
        }
    }
    al::rotateMtxYDirDegree(&mDemoPlayerMtx, mDemoPlayerMtx, angle);

    sead::Vector3f scaleRoot = sead::Vector3f::zero;
    al::getJointLocalTrans(&scaleRoot, getModelKeeper(), "ScaleRoot");
    f32 rootX = scaleRoot.x;
    f32 rootY = scaleRoot.y;
    f32 rootZ = scaleRoot.z;
    f32 scaleOffset = scale - 1.0f;
    mDemoPlayerMtx.a[3] -= scaleOffset * rootX;
    mDemoPlayerMtx.a[7] -= scaleOffset * rootY;
    mDemoPlayerMtx.a[11] -= scaleOffset * rootZ;
}

void ShineTowerRocket::setupRotateMeter() {
    if (mDirtyModel) {
        GameDataHolderAccessor accessor(this);
        if (!GameDataFunction::isActivateHome(accessor)) {
            mHomeMeterActor = al::getSubActor(al::getSubActor(mDirtyModel, "フレーム"), "帆真ん中");
            return;
        }
    }

    al::LiveActor** homeMeterActor;
    if (rs::isModeE3MovieRom()) {
        al::LiveActor* meterFrame = al::getSubActor(this, "フレーム");
        {
            al::StringTmp<32> name("帆%d", 4);
            mHomeMeterActor = al::getSubActor(meterFrame, name.cstr());
        }
        homeMeterActor = &mHomeMeterActor;

        meterFrame = al::getSubActor(this, "フレーム");
        al::LiveActor* meter;
        {
            al::StringTmp<32> name("帆%d", 4);
            meter = al::getSubActor(meterFrame, name.cstr());
        }
        if (isHomeMeterComplete(meter)) {
            al::startAction(meter, "Wait");
        } else {
            f32 frame = calcHomeMeterAnimFrame(meter, 0);
            al::startSklAnim(meter, "Meter");
            al::setSklAnimFrame(meter, frame, 0);
            al::setSklAnimFrameRate(meter, 0.0f, 0);
        }
    } else {
        s32 homeLevel = GameDataFunction::getHomeLevel(GameDataHolderAccessor(this));
        s32 nextMeterIndex = homeLevel + 1;
        al::LiveActor* meterFrame = al::getSubActor(this, "フレーム");
        s32 meterIndex = 9;
        if (nextMeterIndex <= 9)
            meterIndex = nextMeterIndex;
        al::StringTmp<32> name("帆%d", meterIndex);
        al::LiveActor* meter = al::getSubActor(meterFrame, name.cstr());
        homeMeterActor = &mHomeMeterActor;
        *homeMeterActor = meter;
    }

    (*homeMeterActor)->makeActorAlive();
    al::LiveActor* meter = *homeMeterActor;
    s32 colorFrame = rs::getStageShineAnimFrame(meter);
    al::startMclAnimAndSetFrameAndStop(meter, "Color", colorFrame > 0 ? colorFrame : 0);
    al::startMtpAnimAndSetFrameAndStop(*homeMeterActor, "Color", 0.0f);

    s32 homeLevel = GameDataFunction::getHomeLevel(GameDataHolderAccessor(this));
    u32 meterLevel = rs::isModeE3MovieRom() ? 3 : homeLevel;
    s32 rotateLevel = meterLevel - 1;
    if (rotateLevel < 0)
        rotateLevel = meterLevel;

    f32 degree = ((rotateLevel >> 1) * 40.0f) + 40.0f;
    if ((meterLevel & 1) != 0)
        degree = -degree;
    mMeterRotateDegree = degree;
}

void ShineTowerRocket::exeDemoMeterUpPrev() {
    if (al::isFirstStep(this)) {
        setupRotateMeter();
        mDemoMeterUpStartFrame = calcHomeMeterAnimFrame(mHomeMeterActor, 0);
        mDemoMeterUpEndFrame = calcHomeMeterAnimFrame(mHomeMeterActor, mDemoShineNum);

        al::startSklAnim(mHomeMeterActor, "Meter");
        al::setSklAnimFrameAndStop(mHomeMeterActor, mDemoMeterUpStartFrame, 0);

        bool isAppearCameraActive = al::isActiveCamera(mDemoAppearFromHomeCameraTicket);
        const al::IUseCamera* cameraUser = this;
        if (isAppearCameraActive)
            al::endCamera(cameraUser, mDemoAppearFromHomeCameraTicket, -1, false);

        f32 startFrame = mDemoMeterUpStartFrame;
        f32 halfFrame = al::getSklAnimFrameMax(mHomeMeterActor, "Meter") * 0.5f;
        al::CameraTicket* ticket = mDemoAppearFromHomeCameraTicket;
        if (startFrame >= halfFrame) {
            const char* meterAnim = "Meter";
            s32 startStep = al::getSklAnimFrameMax(mHomeMeterActor, meterAnim);
            s32 endStep = al::getSklAnimFrameMax(mHomeMeterActor, meterAnim);
            al::startAnimCameraWithStartStepAndEndStepAndPlayStep(
                cameraUser, ticket, "DemoMeterUp", startStep, endStep, 0, 0);
        } else {
            al::startAnimCameraWithStartStepAndEndStepAndPlayStep(cameraUser, ticket, "DemoMeterUp",
                                                                  0, 0, 0, 0);
        }

        mIsDemoWaitingToEnd = true;
        calcCameraMtxMeterUpPrev();
    }

    if (al::isGreaterEqualStep(this, 75))
        al::setNerve(this, &DemoMeterUp);
}

void ShineTowerRocket::exeDemoMeterUp() {
    if (al::isFirstStep(this)) {
        if (mDemoMeterUpStartFrame >= al::getSklAnimFrameMax(mHomeMeterActor, "Meter") * 0.5f) {
            al::CameraTicket* ticket = mDemoAppearFromHomeCameraTicket;
            const char* meterAnim = "Meter";
            s32 startStep = al::getSklAnimFrameMax(mHomeMeterActor, meterAnim);
            s32 endStep = al::getSklAnimFrameMax(mHomeMeterActor, meterAnim);
            al::startAnimCameraAnim(ticket, "DemoMeterUp", startStep, endStep, 0);
        } else {
            f32 endFrame = mDemoMeterUpEndFrame;
            f32 halfFrame = al::getSklAnimFrameMax(mHomeMeterActor, "Meter") * 0.5f;
            al::CameraTicket* ticket = mDemoAppearFromHomeCameraTicket;
            const char* cameraName = "DemoMeterUp";
            if (endFrame <= halfFrame)
                al::startAnimCameraAnim(ticket, cameraName, 0, 0, 0);
            else
                al::startAnimCameraAnim(ticket, cameraName, -1, -1, -1);
        }
        mIsDemoWaitingToEnd = true;
        al::setSklAnimFrameRate(mHomeMeterActor, 1.0f, 0);
        al::tryEmitEffect(mHomeMeterActor, "MeterUp", nullptr);
        rs::setDemoInfoDemoName(this, "メーター上昇デモ");
    }

    f32 frame = mDemoMeterUpStartFrame + al::getNerveStep(this);
    f32 currentFrame = mDemoMeterUpEndFrame;
    if (frame < currentFrame)
        currentFrame = frame;

    if (al::isNear(currentFrame, mDemoMeterUpEndFrame, 0.001f) ||
        currentFrame > mDemoMeterUpEndFrame) {
        al::tryDeleteEffect(mHomeMeterActor, "MeterUp");
        al::LiveActor* meter = mHomeMeterActor;
        f32 endFrame = mDemoMeterUpEndFrame;
        al::startSklAnim(meter, "Meter");
        al::setSklAnimFrame(meter, endFrame, 0);
        al::setSklAnimFrameRate(meter, 0.0f, 0);
        rs::setDemoInfoDemoName(this, "メーター上昇終了デモ");
        al::setNerve(this, &DemoMeterUpPost);
    }
}

void ShineTowerRocket::exeDemoSelectGoOtherWorld() {
    al::EventFlowExecutor** eventFlowExecutorPtr;
    if (al::isFirstStep(this)) {
        if (al::isActiveCamera(mDemoAppearFromHomeCameraTicket))
            al::endCamera(this, mDemoAppearFromHomeCameraTicket, -1, false);
        if (!al::isActiveCamera(mWorldMapFadeCameraTicket))
            al::startCamera(this, mWorldMapFadeCameraTicket, -1);

        const char* eventFlowName = "SelectGoOtherWorld";
        bool isExistKoopaShip = rs::isExistKoopaShip(this);
        al::EventFlowExecutor* eventFlowExecutor = mEventFlowExecutor;
        eventFlowExecutorPtr = &mEventFlowExecutor;
        if (isExistKoopaShip) {
            GameDataHolderAccessor accessor(this);
            al::StringTmp<128> koopaShipEventFlowName(
                "SelectGoOtherWorldExistKoopa_%s",
                GameDataFunction::getWorldDevelopNameCurrent(accessor));
            eventFlowName = koopaShipEventFlowName.cstr();
        }
        rs::startEventFlow(eventFlowExecutor, eventFlowName);
        rs::setDemoInfoDemoName(this, "ワールドマップデモ");
    } else {
        eventFlowExecutorPtr = &mEventFlowExecutor;
    }

    rs::updateEventFlow(*eventFlowExecutorPtr);
}

void ShineTowerRocket::exeDemoAwardMoon() {
    if (al::isFirstStep(this)) {
        const al::IUseSceneObjHolder* holder = this;
        s32 currentWorld = GameDataFunction::getCurrentWorldId(GameDataHolderAccessor(holder));
        if (currentWorld == GameDataFunction::getWorldIndexSpecial1()) {
            rs::buyCloth(holder, "MarioKing");
            rs::buyCap(holder, "MarioKing");
        } else if (GameDataFunction::getCurrentWorldId(GameDataHolderAccessor(holder)) ==
                   GameDataFunction::getWorldIndexSpecial2()) {
            rs::buyCap(holder, "MarioInvisible");
        }

        if (!rs::isActiveDemo(this)) {
            tryStartDemo();
            al::setNerve(this, &NrvShineTowerRocket.DemoAwardMoon);
            return;
        }
    }

    if (al::isStep(this, 10)) {
        s32 currentWorld = GameDataFunction::getCurrentWorldId(GameDataHolderAccessor(this));
        if (currentWorld == GameDataFunction::getWorldIndexSpecial1())
            rs::startEventFlow(mEventFlowExecutor, "AwardSpecial1");
        else
            rs::startEventFlow(mEventFlowExecutor, "AwardSpecial2");
        rs::setDemoInfoDemoName(this, "コスチューム取得メッセージデモ");
    }

    if (al::isGreaterEqualStep(this, 10) && rs::updateEventFlow(mEventFlowExecutor)) {
        al::endCamera(this, mDemoReturnToHomeCameraTicket, 0, false);
        al::setNerve(this, &NrvShineTowerRocket.WaitIgnoreLockOn);
    }
}

void ShineTowerRocket::exeDemoUpLevelCamera() {
    if (al::isFirstStep(this)) {
        if (al::isActiveCamera(mDemoAppearFromHomeCameraTicket))
            al::endCamera(this, mDemoAppearFromHomeCameraTicket, -1, false);

        calcCameraMtxLevelUp();
        al::startAnimCameraWithStartStepAndEndStepAndPlayStep(this, mDemoAppearFromHomeCameraTicket,
                                                              "DemoChangeHome", 363, 363, 0, 0);

        al::LiveActor* meter = mHomeMeterActor;
        if (meter) {
            f32 frameMax = al::getSklAnimFrameMax(meter, "Meter");
            al::startSklAnim(meter, "Meter");
            al::setSklAnimFrame(meter, frameMax, 0);
            al::setSklAnimFrameRate(meter, 0.0f, 0);
        }
    }

    if (al::isGreaterEqualStep(this, 15))
        al::setNerve(this, &DemoUpLevel);
}

void ShineTowerRocket::calcCameraMtxLevelUp() {
    al::makeMtxSRT(&mDemoPlayerMtx, this);

    f32 m00 = mDemoPlayerMtx.a[0];
    f32 m01 = mDemoPlayerMtx.a[1];
    f32 m02 = mDemoPlayerMtx.a[2];
    sead::Vector3f trans;
    trans.x = mDemoPlayerMtx.a[3];
    f32 m10 = mDemoPlayerMtx.a[4];
    f32 m11 = mDemoPlayerMtx.a[5];
    f32 m12 = mDemoPlayerMtx.a[6];
    trans.y = mDemoPlayerMtx.a[7];
    f32 m20 = mDemoPlayerMtx.a[8];
    f32 m21 = mDemoPlayerMtx.a[9];
    f32 m22 = mDemoPlayerMtx.a[10];
    trans.z = mDemoPlayerMtx.a[11];
    f32 scaleValue = mShineTowerCommonKeeper->calcScale();
    f32 scale = al::lerpValue(scaleValue, 0.4f, 2.0f, 0.4f, 1.3f);
    f32 rootOffsetScale = al::lerpValue(scaleValue, 0.4f, 2.0f, -0.6f, -0.45f);
    al::lerpValue(scaleValue, 0.4f, 2.0f, 0.0f, 1.0f);
    mDemoPlayerMtx.a[0] = m00 * scale;
    mDemoPlayerMtx.a[4] = m10 * scale;
    mDemoPlayerMtx.a[8] = m20 * scale;
    mDemoPlayerMtx.a[1] = m01 * scale;
    mDemoPlayerMtx.a[5] = m11 * scale;
    mDemoPlayerMtx.a[9] = m21 * scale;
    mDemoPlayerMtx.a[2] = m02 * scale;
    mDemoPlayerMtx.a[6] = m12 * scale;
    mDemoPlayerMtx.a[10] = m22 * scale;
    mDemoPlayerMtx.setTranslation(trans);

    al::LiveActor* actor = this;
    const al::IUseSceneObjHolder* holder = actor ? actor : nullptr;
    f32 angle = 0.0f;
    {
        GameDataHolderAccessor accessor(holder);
        if (GameDataFunction::isWorldSea(accessor)) {
            angle = -90.0f;
        } else {
            GameDataHolderAccessor accessor2(holder);
            if (GameDataFunction::isWorldLake(accessor2))
                angle = 90.0f;
        }
    }
    al::rotateMtxYDirDegree(&mDemoPlayerMtx, mDemoPlayerMtx, angle);

    sead::Vector3f scaleRoot = sead::Vector3f::zero;
    al::getJointLocalTrans(&scaleRoot, getModelKeeper(), "ScaleRoot");
    f32 scaleRootXValue = scaleRoot.x;
    f32 scaleRootYValue = scaleRoot.y;
    f32 scaleRootY = scaleRootYValue * rootOffsetScale;
    f32 scaleRootX = scaleRootXValue * rootOffsetScale;
    f32 transY = mDemoPlayerMtx.a[7];
    mDemoPlayerMtx.a[3] -= scaleRootX;
    f32 scaleRootZ = scaleRoot.z * rootOffsetScale;
    f32 transZ = mDemoPlayerMtx.a[11];
    mDemoPlayerMtx.a[7] = transY - scaleRootY;
    mDemoPlayerMtx.a[11] = transZ - scaleRootZ;
}

void ShineTowerRocket::exeDemoUpLevel() {
    if (al::isFirstStep(this)) {
        al::LiveActor* actor = this;
        const al::IUseSceneObjHolder* holder = actor ? actor : nullptr;
        s32 currentWorld = GameDataFunction::getCurrentWorldId(GameDataHolderAccessor(holder));
        if (currentWorld != GameDataFunction::getWorldIndexClash()) {
            s32 bossWorld = GameDataFunction::getWorldIndexBoss();
            if (currentWorld != 1 && currentWorld != bossWorld &&
                !GameDataFunction::isGameClear(GameDataHolderAccessor(holder))) {
                GameDataFunction::upHomeLevel(GameDataHolderWriter(this));
            }
        }
    }

    if (al::isEndAnimCamera(mDemoAppearFromHomeCameraTicket) || al::isGreaterEqualStep(this, 10)) {
        mIsWorldMap = true;
        al::setNerve(this, &DemoInformPowerUp);
    }
}

void ShineTowerRocket::exeDemoKoopaShipFade() {
    if (al::isFirstStep(this))
        mWorldMapWipe->startClose(rs::getKoopaShipWipeDemoHomeFlyAwayStep());

    if (mWorldMapWipe->isCloseEnd() && rs::isEnableEndKoopaShipDemoHomeFlyAway(this)) {
        rs::endKoopaShipDemoHomeFlyAway(this);
        if (al::isActiveCamera(mDemoAppearFromHomeCameraTicket))
            al::endCamera(this, mDemoAppearFromHomeCameraTicket, -1, false);
        if (al::isActiveCamera(mWorldMapFadeCameraTicket))
            al::endCamera(this, mWorldMapFadeCameraTicket, -1, false);
        mIsWorldMapCamera = false;
        mWorldMapWipe->startOpen(-1);
        al::setNerve(this, &DemoMeterUp);
    }
}

void ShineTowerRocket::exeDemoUpLevelCloseFade() {
    if (al::isFirstStep(this)) {
        al::StringTmp<32> cameraName;
        cameraName.format("DemoStartUpHome");
        if (al::isActiveCamera(mWorldMapFadeCameraTicket))
            al::endCamera(this, mWorldMapFadeCameraTicket, -1, false);
        if (al::isActiveCamera(mDemoAppearFromHomeCameraTicket))
            al::endCamera(this, mDemoAppearFromHomeCameraTicket, -1, false);

        if (mDirtyModel) {
            al::startHitReaction(mDirtyModel, "ホームきれいになる");
            al::hideModelIfShow(this);
            mDirtyModel->makeActorAlive();
            mHomeActor->makeActorDead();
            mDoorAreaChange->makeActorDead();
            mHomeSubActor->makeActorDead();
            al::startAction(mShineTowerRock, cameraName.cstr());
            rs::setDemoInfoDemoName(this, "ホーム初起動デモ");
        }

        if (GameDataFunction::isWorldSky(GameDataHolderAccessor(this))) {
            cameraName.format("DemoChangeHomeFullMoon");
            rs::setDemoInfoDemoName(this, "ホーム最終変化デモ");
            al::startHitReaction(this, "ホーム最終変化デモ");
        }

        if (mClashModel) {
            al::startHitReaction(mClashModel, "ホーム復活");
            rs::setDemoInfoDemoName(this, "ホーム修復デモ");
            al::tryOffStageSwitch(this, "SwitchClashOn");
        }

        rs::hideDemoCapSilhouette(this);
        al::makeMtxSRT(&mDemoPlayerMtx, this);
        al::startAnimCamera(this, mDemoAppearFromHomeCameraTicket, cameraName.cstr(), 0);
    }

    if (al::isStep(this, 390))
        mDemoWipe->startClose(60);

    if (mDemoWipe->isCloseEnd())
        al::setNerve(this, &DemoUpLevelWaitFade);
}

void ShineTowerRocket::exeDemoUpLevelWaitFade() {
    if (al::isFirstStep(this)) {
        al::showModelIfHide(this);
        al::validateCollisionParts(this);

        if (mDirtyModel) {
            mDirtyModel->kill();
            mHomeActor->makeActorAlive();
            mDoorAreaChange->makeActorAlive();
            mDoorAreaChange->start();
            mHomeSubActor->makeActorAlive();
            al::getSubActor(this, "フレーム")->makeActorAlive();
            al::startAction(this, "WaitWaterfallDemo");
            al::resetPosition(this);
            al::getSubActor(this, "地球儀")->makeActorAlive();
            setupCapTargetInfoForHomeGlobe(&mCapTargetOffset, mCapTargetInfo, this, mDirtyModel);
            GameDataFunction::activateHome(GameDataHolderWriter(this));
            GameDataFunction::upHomeLevel(GameDataHolderWriter(this));
        }

        if (GameDataFunction::isWorldSky(GameDataHolderAccessor(this))) {
            if (mDamageModel)
                mDamageModel->kill();
            al::tryStartActionIfNotPlaying(this, "WaitDemo");
            al::showModelIfHide(this);
            GameDataFunction::upHomeLevel(GameDataHolderWriter(this));
        }

        if (mClashModel) {
            mHomeActor->makeActorAlive();
            mDoorAreaChange->makeActorAlive();
            mDoorAreaChange->start();
            al::getSubActor(this, "フレーム")->makeActorAlive();
            al::getSubActor(this, "ステッカー")->makeActorAlive();
            al::tryStartActionIfNotPlaying(this, "WaitDemo");

            if (GameDataFunction::isCrashHome(GameDataHolderAccessor(this))) {
                GameDataFunction::repairHome(GameDataHolderWriter(this));
                GameDataFunction::unlockWorld(GameDataHolderWriter(this),
                                              GameDataFunction::getWorldIndexCity());
                mWorldId = GameDataFunction::getWorldIndexCity();
            } else {
                GameDataFunction::repairHomeByCrashedBoss(GameDataHolderWriter(this));
                mWorldId = GameDataFunction::getWorldIndexSky();
                GameDataFunction::unlockWorld(GameDataHolderWriter(this),
                                              GameDataFunction::getWorldIndexSky());
            }

            mClashModel->kill();
        }

        mShineTowerCommonKeeper->update();
        mShineTowerCommonKeeper->updateSensor();
    }

    if (al::isGreaterEqualStep(this, 60))
        al::setNerve(this, &DemoUpLevelOpenFade);
}

void ShineTowerRocket::exeDemoUpLevelOpenFade() {
    if (al::isFirstStep(this)) {
        al::startHitReaction(this, "ホーム変化後フェード明け");
        mDemoWipe->startOpen(60);
        rs::showDemoCapSilhouette(this);
    }

    if (al::isEndAnimCamera(mDemoAppearFromHomeCameraTicket)) {
        if (!mDemoWipe || al::isDead(mDemoWipe)) {
            mIsWorldMap = true;
            al::setNerve(this, &DemoInformNewHome);
        }
    }
}

bool ShineTowerRocket::isActiveClashModel() const {
    return mClashModel && al::isAlive(mClashModel);
}

bool ShineTowerRocket::isWorldMap() const {
    return al::isNerve(this, &WorldMap);
}

void rs::setupHomeMeter(al::LiveActor* actor) {
    if (rs::isModeDiverRom() || rs::isModeE3Rom() || rs::isModeE3MovieRom() ||
        rs::isModeE3LiveRom() || rs::isModeEpdMovieRom()) {
        for (s32 i = 1; i != 10; i++) {
            al::LiveActor* meterFrame = al::getSubActor(actor, "フレーム");
            al::StringTmp<32> name("帆%d", i);
            al::LiveActor* meter = al::getSubActor(meterFrame, name.cstr());
            if (i >= 4) {
                meter->makeActorDead();
            } else {
                al::startSklAnim(meter, "Meter");
                al::setSklAnimFrame(meter, al::getSklAnimFrameMax(meter, "Meter"), 0);
                al::setSklAnimFrameRate(meter, 0.0f, 0);
                al::startMclAnimAndSetFrameAndStop(meter, "Color", 0.0f);
                al::startMtpAnimAndSetFrameAndStop(meter, "Color", 0.0f);
                meter->makeActorAlive();
            }
        }
        al::LiveActor* meterFrame = al::getSubActor(actor, "フレーム");
        al::LiveActor* centerMeter = al::getSubActor(meterFrame, "帆真ん中");
        centerMeter->makeActorDead();
        meterFrame = al::getSubActor(actor, "フレーム");
        al::startMclAnimAndSetFrameAndStop(meterFrame, "FrameColor", 0.0f);
        return;
    }

    const al::IUseSceneObjHolder* holder = actor ? actor : nullptr;
    if (!GameDataFunction::isActivateHome(GameDataHolderAccessor(holder)) ||
        (!GameDataFunction::isPlayDemoWorldWarp(GameDataHolderAccessor(holder)) &&
         (GameDataFunction::isCrashHome(GameDataHolderAccessor(holder)) ||
          GameDataFunction::isBossAttackedHome(GameDataHolderAccessor(holder))))) {
        for (s32 i = 1; i != 10; i++) {
            al::LiveActor* meterFrame = al::getSubActor(actor, "フレーム");
            al::StringTmp<32> name("帆%d", i);
            al::getSubActor(meterFrame, name.cstr())->makeActorDead();
        }
        al::LiveActor* meterFrame = al::getSubActor(actor, "フレーム");
        al::LiveActor* centerMeter = al::getSubActor(meterFrame, "帆真ん中");
        centerMeter->makeActorDead();
        return;
    }

    bool isAllPay = GameDataFunction::isPayShineAllInAllWorld(GameDataHolderAccessor(holder));
    if (isAllPay) {
        al::startMclAnimAndSetFrameAndStop(al::getSubActor(actor, "フレーム"), "FrameColor",
                                           2.0f);
    } else {
        s32 homeLevel = GameDataFunction::getHomeLevel(GameDataHolderAccessor(holder));
        if (homeLevel == 9) {
            al::startMclAnimAndSetFrameAndStop(al::getSubActor(actor, "フレーム"), "FrameColor",
                                               1.0f);
        } else {
            al::startMclAnimAndSetFrameAndStop(al::getSubActor(actor, "フレーム"), "FrameColor",
                                               0.0f);
        }
    }

    for (s32 i = 1; i != 10; i++) {
        if (GameDataFunction::getHomeLevel(GameDataHolderAccessor(holder)) == 9) {
            al::LiveActor* meterFrame = al::getSubActor(actor, "フレーム");
            al::StringTmp<32> name("帆%d", i);
            al::LiveActor* meter = al::getSubActor(meterFrame, name.cstr());
            meter->makeActorAlive();
            al::startSklAnim(meter, "Meter");
            al::setSklAnimFrame(meter, al::getSklAnimFrameMax(meter, "Meter"), 0);
            al::setSklAnimFrameRate(meter, 0.0f, 0);
            if (isAllPay)
                al::startMclAnimAndSetFrameAndStop(meter, "Color", 10.0f);
            else
                al::startMclAnimAndSetFrameAndStop(meter, "Color", 9.0f);
            al::startMtpAnimAndSetFrameAndStop(meter, "Color", 1.0f);
            continue;
        }

        s32 homeLevel = GameDataFunction::getHomeLevel(GameDataHolderAccessor(holder));
        al::LiveActor* meterFrame = al::getSubActor(actor, "フレーム");
        al::StringTmp<32> name("帆%d", i);
        al::LiveActor* meter = al::getSubActor(meterFrame, name.cstr());
        if (i <= homeLevel) {
            meter->makeActorAlive();
            al::startSklAnim(meter, "Meter");
            al::setSklAnimFrame(meter, al::getSklAnimFrameMax(meter, "Meter"), 0);
            al::setSklAnimFrameRate(meter, 0.0f, 0);
            al::startMclAnimAndSetFrameAndStop(meter, "Color", 0.0f);
            al::startMtpAnimAndSetFrameAndStop(meter, "Color", 0.0f);
        } else {
            meter->makeActorDead();
            if (isHomeMeterComplete(meter)) {
                al::startAction(meter, "Wait");
            } else {
                f32 frame = calcHomeMeterAnimFrame(meter, 0);
                al::startSklAnim(meter, "Meter");
                al::setSklAnimFrame(meter, frame, 0);
                al::setSklAnimFrameRate(meter, 0.0f, 0);
            }
            s32 worldId = GameDataFunction::getLatestUnlockWorldIdForShineTowerMeter(actor);
            s32 frame = rs::getStageShineAnimFrame(actor, worldId);
            al::startMclAnimAndSetFrameAndStop(meter, "Color", frame > 0 ? frame : 0);
            al::startMtpAnimAndSetFrameAndStop(meter, "Color", 0.0f);
        }
    }

    if (GameDataFunction::getHomeLevel(GameDataHolderAccessor(holder)) < 9 &&
        GameDataFunction::getPayShineNum(
            GameDataHolderAccessor(holder),
            GameDataFunction::getLatestUnlockWorldIdForShineTowerMeter(actor)) != 0) {
        if (!isHomeMeterComplete(actor)) {
            s32 homeLevel = GameDataFunction::getHomeLevel(GameDataHolderAccessor(holder));
            al::LiveActor* meterFrame = al::getSubActor(actor, "フレーム");
            s32 nextMeterIndex = homeLevel + 1;
            if (nextMeterIndex > 9)
                nextMeterIndex = 9;
            al::StringTmp<32> name("帆%d", nextMeterIndex);
            al::LiveActor* nextMeter = al::getSubActor(meterFrame, name.cstr());
            nextMeter->makeActorAlive();
            al::startSklAnim(nextMeter, "Meter");
            if (isHomeMeterComplete(nextMeter)) {
                al::startAction(nextMeter, "Wait");
            } else {
                f32 frame = calcHomeMeterAnimFrame(nextMeter, 0);
                al::startSklAnim(nextMeter, "Meter");
                al::setSklAnimFrame(nextMeter, frame, 0);
                al::setSklAnimFrameRate(nextMeter, 0.0f, 0);
            }
            s32 worldId = GameDataFunction::getLatestUnlockWorldIdForShineTowerMeter(actor);
            s32 frame = rs::getStageShineAnimFrame(actor, worldId);
            al::startMclAnimAndSetFrameAndStop(nextMeter, "Color", frame > 0 ? frame : 0);
            al::startMtpAnimAndSetFrameAndStop(nextMeter, "Color", 0.0f);
        }
    }

    if (GameDataFunction::getHomeLevel(GameDataHolderAccessor(holder)) <= 8 &&
        GameDataFunction::getPayShineNum(
            GameDataHolderAccessor(holder),
            GameDataFunction::getLatestUnlockWorldIdForShineTowerMeter(actor)) == 0 &&
        (GameDataFunction::isPlayDemoWorldWarp(GameDataHolderAccessor(holder)) ||
         (!GameDataFunction::isCrashHome(GameDataHolderAccessor(holder)) &&
          !GameDataFunction::isBossAttackedHome(GameDataHolderAccessor(holder))))) {
        al::LiveActor* nextMeter =
            al::getSubActor(al::getSubActor(actor, "フレーム"), "帆真ん中");
        nextMeter->makeActorAlive();
        al::startSklAnim(nextMeter, "Meter");
        al::setSklAnimFrame(nextMeter, 0.0f, 0);
        al::setSklAnimFrameRate(nextMeter, 0.0f, 0);
        s32 worldId = GameDataFunction::getLatestUnlockWorldIdForShineTowerMeter(actor);
        s32 frame = rs::getStageShineAnimFrame(actor, worldId);
        al::startMclAnimAndSetFrameAndStop(nextMeter, "Color", frame > 0 ? frame : 0);
        al::startMtpAnimAndSetFrameAndStop(nextMeter, "Color", 0.0f);
    } else {
        al::getSubActor(al::getSubActor(actor, "フレーム"), "帆真ん中")->makeActorDead();
    }
}

void rs::setupHomeMeterDitherParam(al::LiveActor* actor, ShineTowerCommonKeeper* keeper) {
    f32 scale = keeper->calcScale();
    f32 radius = scale * 1250.0f;
    f32 offsetXZ = scale * 0.0f;
    f32 offsetY = scale * 1125.0f;
    const sead::Vector3f offset(offsetXZ, offsetY, offsetXZ);

    s32 i = 1;
    do {
        al::LiveActor* meterFrame = al::getSubActor(actor, "フレーム");
        al::StringTmp<32> name("帆%d", i);
        al::LiveActor* meter = al::getSubActor(meterFrame, name.cstr());
        if (al::isExistDitherAnimator(meter)) {
            al::setDitherAnimSphereRadius(meter, radius);
            al::setDitherAnimClippingJudgeLocalOffset(meter, offset);
        }
        i++;
    } while (i != 10);

    al::LiveActor* meterFrame = al::getSubActor(actor, "フレーム");
    al::LiveActor* lastMeter = al::getSubActor(meterFrame, "帆真ん中");
    if (al::isExistDitherAnimator(lastMeter)) {
        al::setDitherAnimSphereRadius(lastMeter, radius);
        al::setDitherAnimClippingJudgeLocalOffset(lastMeter, offset);
    }

    al::LiveActor* meterFrameDither = al::getSubActor(actor, "フレーム");
    al::LiveActor* meterFrameForSail = al::getSubActor(actor, "フレーム");
    al::LiveActor* firstMeter = nullptr;
    {
        al::StringTmp<32> name("帆%d", 1);
        firstMeter = al::getSubActor(meterFrameForSail, name.cstr());
    }
    if (al::isExistDitherAnimator(meterFrameDither)) {
        sead::Vector3f frameOffset;
        frameOffset.setSub(al::getTrans(firstMeter), al::getTrans(meterFrameDither));
        al::setDitherAnimSphereRadius(meterFrameDither, radius);
        frameOffset.add(offset);
        al::setDitherAnimClippingJudgeLocalOffset(meterFrameDither, frameOffset);
    }
}

void rs::setupHomeSticker(al::LiveActor* actor) {
    al::LiveActor* stickerActor = al::getSubActor(actor, "ステッカー");
    const al::IUseSceneObjHolder* holder = actor;
    const sead::PtrArray<ShopItem::ItemInfo>& stickerList =
        rs::getStickerList(GameDataHolderAccessor(holder));
    s32 lastIdx = stickerList.size() - 1;
    if (lastIdx >= 0) {
        const al::IUseSceneObjHolder* stickerHolder = stickerActor;
        for (s32 i = 0;; i++) {
            if (al::isExistJoint(stickerActor, stickerList[i]->name)) {
                bool isHave = rs::isHaveSticker(GameDataHolderAccessor(stickerHolder), i);
                if (isHave)
                    al::setJointVisibility(stickerActor, stickerList[i]->name, true);
                else
                    al::setJointVisibility(stickerActor, stickerList[i]->name, false);
            }
            if (al::isExistJoint(actor, stickerList[i]->name)) {
                bool isHave = rs::isHaveSticker(GameDataHolderAccessor(holder), i);
                if (isHave)
                    al::setJointVisibility(actor, stickerList[i]->name, true);
                else
                    al::setJointVisibility(actor, stickerList[i]->name, false);
            }
            if (lastIdx == i)
                break;
        }
    }
}

void rs::setupHomeCompLight(al::LiveActor* actor) {
    al::LiveActor* compLightActor = al::getSubActor(actor, "コンプライト");
    const al::IUseSceneObjHolder* holder = actor;
    for (s32 i = 0; i < GameDataFunction::getWorldNum(GameDataHolderAccessor(holder)); i++) {
        al::StringTmp<64> name("CompLight%s",
                               GameDataFunction::getWorldDevelopName(GameDataHolderAccessor(holder),
                                                                      i));
        if (al::isExistJoint(compLightActor, name.cstr())) {
            bool isComplete = GameDataFunction::calcIsGetShineAllInWorld(
                GameDataHolderAccessor(compLightActor), i);
            if (isComplete)
                al::setJointVisibility(compLightActor, name.cstr(), true);
            else
                al::setJointVisibility(compLightActor, name.cstr(), false);
        }
        if (al::isExistJoint(actor, name.cstr())) {
            bool isComplete =
                GameDataFunction::calcIsGetShineAllInWorld(GameDataHolderAccessor(holder), i);
            if (isComplete)
                al::setJointVisibility(actor, name.cstr(), true);
            else
                al::setJointVisibility(actor, name.cstr(), false);
        }
    }
}

const char* rs::getHomeArchiveName(const al::LiveActor* actor) {
    if (isDamageHomeModel(actor))
        return "ShineTowerDamage";
    return "ShineTower";
}

void DamageModel::control() {
    if (mIsFollowDamage) {
        ShineTowerRocket* rocket = mShineTowerRocket;
        al::resetPosition(rocket->getDamageModel());
        al::resetMtxPosition(rocket, *al::getJointMtxPtr(rocket->getDamageModel(), "ShineTower"));
        rocket->updateParts();
        resetSubActorPositionAll(rocket);
    }
}

void ShineTowerGlobeAnimCtrl::init(const al::ActorInitInfo& info) {
    al::initActorSceneInfo(this, info);
    al::initActorPoseTFSV(this);
    al::copyPose(this, mGlobeActor);
    al::initActorClipping(this, info);
    al::setClippingInfo(this, 3500.0f, nullptr);
    al::initExecutorMapObjMovement(this, info);
    al::initActorSeKeeper(this, info, "ShineTowerGlobe");
    al::initActorBgmKeeper(this, info, nullptr);

    al::LiveActor* actor = this;
    actor->makeActorAlive();
}

void ShineTowerGlobeAnimCtrl::startClipped() {
    mWorldMapCameraAnimRate = 1.0f;
    mMusicBoxTimer = -1;
    al::LiveActor::startClipped();
}

void ShineTowerGlobeAnimCtrl::control() {
    f32 frameRate = mWorldMapCameraAnimRate;
    s32 step = mBrakeTimer - 1;
    if (step < 0)
        step = 0;
    mBrakeTimer = step;

    if (step == 0) {
        f32 nextRate = frameRate * 0.995f;
        f32 absRate = nextRate > 0.0f ? nextRate : -nextRate;
        frameRate = 1.0f;
        mWorldMapCameraAnimRate = nextRate;
        if (absRate < 1.0f) {
            if (nextRate > 0.0f) {
                mWorldMapCameraAnimRate = 1.0f;
            } else {
                frameRate = -1.0f;
                mWorldMapCameraAnimRate = -1.0f;
            }
        } else {
            frameRate = nextRate;
        }
    }

    al::setActionFrameRate(mGlobeActor, frameRate);
    updateMusicBox();
}

void ShineTowerGlobeAnimCtrl::updateMusicBox() {
    f32 rate = mWorldMapCameraAnimRate;
    f32 absRate = rate > 0.0f ? rate : -rate;
    s32 timer = mMusicBoxTimer;
    bool shouldHold = true;

    if (absRate <= 1.0f) {
        if (timer < 0) {
            shouldHold = false;
        } else {
            timer--;
            mMusicBoxTimer = timer;
        }
    }

    bool isHoldingMusicBox = false;
    if (shouldHold && timer >= 0) {
        s32 param = sead::Mathi::clampMax((s32)(absRate * 10.0f), 170);
        sead::SafeString musicBoxName(mMusicBoxName);
        al::holdSeWithParam(this, musicBoxName, param, "");
        al::setLifeTimeForHoldCall(this, mMusicBoxName, 5, nullptr);
        isHoldingMusicBox = true;
    }
    mIsHoldingMusicBox = isHoldingMusicBox;

    if (absRate > 1.8f) {
        s32 param = absRate * 10.0f;
        al::holdSeWithParam(this, sead::SafeString("Roll"), param, "");
    }
}

void DemoShine::startDemo(s32 index) {
    al::LiveActor* actor = this;
    actor->makeActorAlive();

    const al::IUseSceneObjHolder* holder = actor;
    bool isLaunchHome = GameDataFunction::isLaunchHome(GameDataHolderAccessor(holder));
    s32 actionIndex;
    if (index + 1 < 10)
        actionIndex = index + 1;
    else
        actionIndex = 10;

    al::startAction(this, !isLaunchHome ?
                              al::StringTmp<64>("DemoPayToHome%02dWaterfall", actionIndex).cstr() :
                              al::StringTmp<64>("DemoPayToHome%02d", actionIndex).cstr());

    if (index >= 11)
        al::rotateQuatYDirDegree(this, (index - 10) * -15.0f);

    mStep = 0;
    mIsReactionStarted = false;
}

void DemoShine::control() {
    s32 step = mStep;
    const char* actionName = al::getActionName(this);
    if (step == (s32)al::getActionFrameMax(this, actionName)) {
        al::startHitReaction(this, "タンクに入った");
        mIsReactionStarted = true;
    }

    if (al::isActionEnd(this)) {
        mRumbleCalculator->start(0);
        al::tryStartSe(this, "HomeIn");

        al::LiveActor* actor = this;
        actor->kill();
    }

    mStep++;
}

void ShineTowerRocket::exeDemoWorldTakeoffForDebug() {
    al::setNerve(this, &NrvShineTowerRocket.DemoWorldTakeoff);
}

void ShineTowerRocket::exeWorldMap() {
    if (al::isFirstStep(this))
        rs::setDemoInfoDemoName(this, "ワールドマップデモ");

    if (mShineTowerLight && al::isGreaterStep(this, 2))
        mShineTowerLight->setLightingWorldMap(true);
}

void ShineTowerRocket::exeGoToWorldMapWithFade() {
    if (al::isFirstStep(this))
        mWorldMapWipe->startClose(-1);

    if (mWorldMapWipe->isCloseEnd()) {
        if (al::isActiveCamera(mDemoAppearFromHomeCameraTicket))
            al::endCamera(this, mDemoAppearFromHomeCameraTicket, -1, false);
        if (al::isActiveCamera(mWorldMapFadeCameraTicket))
            al::endCamera(this, mWorldMapFadeCameraTicket, -1, false);

        mIsWorldMapCamera = false;
        mWorldMapWipe->startOpen(-1);
        al::setNerve(this, &WorldMap);
    }
}

void ShineTowerRocket::cancelWorldMap() {
    mIsWorldMap = false;
    calcPlayerPoseForPayDemo();
    tryStartEntranceCamera(-1);

    if (al::isActiveCamera(mWorldMapCameraTicket))
        al::endCamera(this, mWorldMapCameraTicket, -1, false);

    al::setNerve(this, &NrvShineTowerRocket.WaitIgnoreLockOn);
}

void ShineTowerRocket::decideWorldMap(s32 worldId) {
    mIsWorldMap = false;
    mWorldId = worldId;

    if (al::isActiveCamera(mWorldMapCameraTicket))
        al::endCamera(this, mWorldMapCameraTicket, -1, false);

    mDoorAreaChange->setHomeDoor(false);
    al::setNerve(this, &NrvShineTowerRocket.DemoWorldTakeoff);
}

void ShineTowerRocket::startDemoAppearPlayerFromHome() {
    al::startAnimCamera(this, mDemoAppearFromHomeCameraTicket, "DemoAppearFromHome", 0);
    rs::addDemoSubActor(this);
    al::setNerve(this, &DemoAppearPlayerFromHome);
}

void ShineTowerRocket::startDemoReturnToHome() {
    if (GameDataFunction::isWorldWaterfall(GameDataHolderAccessor(this)))
        rs::replaceDemoPlayer(this, mDemoReturnPlayerTrans, mDemoReturnPlayerQuat);

    al::makeMtxQuatPos(&mDemoReturnToHomeMtx, rs::getDemoPlayerQuat(this),
                       rs::getDemoPlayerTrans(this));
    rs::startActionDemoPlayer(this, "DemoReturnToHome");
    al::startAnimCamera(this, mDemoReturnToHomeCameraTicket, "DemoReturnToHome", 0);
    al::setNerve(this, &DemoReturnToHome);
}

void ShineTowerRocket::updatePartsByDamage() {
    al::resetPosition(mDamageModel);
    al::resetMtxPosition(this, *al::getJointMtxPtr(mDamageModel, "ShineTower"));
    updateParts();
    resetSubActorPositionAll(this);
}

void ShineTowerRocket::exeDemoAppearPlayerFromHome() {
    al::LiveActor* actor = this;
    if (mDamageModel && al::isAlive(mDamageModel)) {
        actor = mDamageModel;
        mDamageModel->setFollowDamage(true);
    }

    if (al::isFirstStep(this)) {
        if (mShineTowerLight)
            mShineTowerLight->setLightingWorldMap(true);
        al::showModelIfHide(actor);
        const char* actionName = "DemoAppearFromHome";
        al::startAction(actor, actionName);
        rs::startActionDemoPlayer(this, actionName);
        rs::forcePutOnDemoCap(this);
        const sead::Vector3f& trans = al::getTrans(this);
        const sead::Quatf& quat = al::getQuat(this);
        rs::replaceDemoPlayer(this, trans, quat);
        rs::hideDemoPlayerSilhouette(this);
        mShineTowerCommonKeeper->setMeterRotateForWorld(false);
        al::getSubActor(this, "地球儀")->makeActorAlive();

        s32 birdNum = mBirdGroup->getActorCount();
        if (birdNum >= 1) {
            Bird* bird = mBirdGroup->getDeriveActor(0);
            bird->makeActorAlive();
            rs::addDemoActor(bird, false);
            for (s32 i = 1; i != birdNum; i++) {
                bird = mBirdGroup->getDeriveActor(i);
                bird->makeActorAlive();
                rs::addDemoActor(bird, false);
            }
        }

        al::addDemoActorFromAddDemoInfo(this, mAddDemoInfo);
        rs::setMarioGroundDepthShadowMapLength(this, 280.0f);
        al::hideShadowMask(this);
    }

    if (al::isStep(this, 10)) {
        s32 birdNum = mBirdGroup->getActorCount();
        if (birdNum >= 1) {
            mBirdGroup->getDeriveActor(0)->startFlyAwayHomeLanding();
            for (s32 i = 1; i != birdNum; i++)
                mBirdGroup->getDeriveActor(i)->startFlyAwayHomeLanding();
        }
    }

    updateParts();

    if (al::isActionEnd(actor) && al::isEndAnimCamera(mDemoAppearFromHomeCameraTicket)) {
        if (mDamageModel && al::isAlive(mDamageModel))
            mDamageModel->setFollowDamage(false);
        al::requestCaptureScreenCover(this, 2);
        al::showShadowMask(this);
        al::setNerve(this, &DemoAppearPlayerFromHomeAfter);
    }
}

void ShineTowerRocket::exeDemoReturnToHome() {
    if (al::isFirstStep(this)) {
        if ((!mDirtyModel || !al::isAlive(mDirtyModel)) &&
            (!mClashModel || !al::isAlive(mClashModel))) {
            al::startAction(this, "WaitNormal");
            al::setActionFrameRate(this, 0.0f);
        }
    }

    s32 cameraStep = al::getAnimCameraStep(mDemoReturnToHomeCameraTicket);
    if (cameraStep == al::getAnimCameraStepMax(mDemoReturnToHomeCameraTicket) - 1) {
        const char* stageName =
            GameDataFunction::getCurrentStageName(GameDataHolderAccessor(this));
        if (stageName)
            al::isEqualString(stageName, "CityWorldHomeStage");
        al::requestCaptureScreenCover(this, 5);
    }

    if (al::isEndAnimCamera(mDemoReturnToHomeCameraTicket) && rs::isActionEndDemoPlayer(this)) {
        rs::requestEndDemoReturnToHome(this);
        if (mIsPlayDemoAwardSpecial) {
            tryStartDemo();
            al::setNerve(this, &NrvShineTowerRocket.DemoAwardMoon);
        } else {
            al::endCamera(this, mDemoReturnToHomeCameraTicket, 0, false);
            if ((!mDirtyModel || !al::isAlive(mDirtyModel)) &&
                (!mClashModel || !al::isAlive(mClashModel)))
                al::setActionFrameRate(this, 1.0f);
            al::setNerve(this, &NrvShineTowerRocket.WaitAfterReturnToHome);
        }
    }
}

void ShineTowerRocket::exeDemoWorldTakeoff() {
    const al::IUseSceneObjHolder* holder = this;
    bool isTakeoffMoonFromOther = false;
    if (GameDataFunction::isWorldTypeMoon(GameDataHolderAccessor(holder), mWorldId)) {
        isTakeoffMoonFromOther = !GameDataFunction::isWorldTypeMoon(
            GameDataHolderAccessor(holder),
            GameDataFunction::getCurrentWorldId(GameDataHolderAccessor(holder)));
    }

    if (al::isFirstStep(this)) {
        if (mShineTowerLight)
            mShineTowerLight->setLightingWorldMap(true);
        rs::disableOpenWipeForSkipDemo(holder);
        al::hideShadowMask(this);
        mMeterRotateDegree = 0.0f;
        mRopeRootRotateDegree = 0.0f;
        mShineTowerCommonKeeper->setMeterRotateForWorld(false);
        al::invalidateClipping(this);

        sead::Quatf quat = sead::Quatf::unit;
        al::calcQuat(&quat, this);

        const char* actionName;
        const char* demoName = nullptr;
        bool isInvalidateCameraAngleSwing = false;
        bool isStartDemoPlayer = true;
        if (GameDataFunction::isWorldTypeMoon(GameDataHolderAccessor(holder), mWorldId)) {
            if (GameDataFunction::isWorldTypeMoon(
                    GameDataHolderAccessor(holder),
                    GameDataFunction::getCurrentWorldId(GameDataHolderAccessor(holder)))) {
                actionName = "DemoWorldTakeoff";
                al::LiveActor* poseCopyActor = mPoseCopyActor;
                al::startAction(poseCopyActor, actionName);
                poseCopyActor->makeActorAlive();
                al::copyPose(poseCopyActor, this);
            } else if (mDemoPlayerActor && GameDataFunction::getWorldIndexMoon() == mWorldId &&
                       !GameDataFunction::isAlreadyGoWorld(GameDataHolderAccessor(holder),
                                                            mWorldId)) {
                const char* tuxedoName = "MarioTuxedo";
                rs::buyCloth(holder, tuxedoName);
                GameDataFunction::wearCostume(GameDataHolderWriter(holder), tuxedoName);
                rs::buyCap(holder, tuxedoName);
                GameDataFunction::wearCap(GameDataHolderWriter(holder), tuxedoName);

                actionName = "DemoWorldTakeoffForMoonFirst";
                al::LiveActor* poseCopyActor = mPoseCopyActor;
                al::startAction(poseCopyActor, actionName);
                poseCopyActor->makeActorAlive();
                al::copyPose(poseCopyActor, this);
                al::LiveActor* capManHeroEyesActor = mCapManHeroEyesActor;
                al::startAction(capManHeroEyesActor, actionName);
                capManHeroEyesActor->makeActorAlive();
                al::copyPose(capManHeroEyesActor, this);
                al::LiveActor* demoMarioCapActor = mDemoMarioCapActor;
                al::startAction(demoMarioCapActor, actionName);
                demoMarioCapActor->makeActorAlive();
                al::copyPose(demoMarioCapActor, this);

                const char* tuxedoActionName = "DemoWorldTakeoffForMoonFirstTuxedo";
                al::LiveActor* demoPlayerActor = mDemoPlayerActor;
                al::startAction(demoPlayerActor, tuxedoActionName);
                demoPlayerActor->makeActorAlive();
                al::copyPose(demoPlayerActor, this);
                al::tryStartActionSubActorAll(mDemoPlayerActor, tuxedoActionName);
                al::hideModel(mDemoPlayerActor);

                al::LiveActor* demoPlayerActor2 = mDemoPlayerActor2;
                al::startAction(demoPlayerActor2, actionName);
                demoPlayerActor2->makeActorAlive();
                al::copyPose(demoPlayerActor2, this);
                al::tryStartActionSubActorAll(mDemoPlayerActor2, actionName);
                al::copyPose(mDemoPlayerActor, this);
                al::copyPose(mDemoPlayerActor2, this);
                rs::addDemoActor(mDemoPlayerActor, true);
                rs::addDemoActor(mDemoPlayerActor2, true);
                rs::hideDemoPlayer(this);

                isStartDemoPlayer = false;
                isInvalidateCameraAngleSwing = true;
                mIsFixDoorwayCamera = true;
                demoName = "ホーム離陸デモ(月へ初回)";
            } else {
                actionName = "DemoWorldTakeoffForMoon";
                al::LiveActor* poseCopyActor = mPoseCopyActor;
                al::startAction(poseCopyActor, actionName);
                poseCopyActor->makeActorAlive();
                al::copyPose(poseCopyActor, this);
                demoName = "ホーム離陸デモ(月へ)";
            }
        } else if (mShineTowerRock) {
            const char* capName = "MarioCaptain";
            rs::buyCap(holder, capName);
            GameDataFunction::wearCap(GameDataHolderWriter(holder), capName);

            actionName = "DemoStartUpHomeSky";
            al::startAction(mShineTowerRock, actionName);
            al::LiveActor* poseCopyActor = mPoseCopyActor;
            al::startAction(poseCopyActor, actionName);
            poseCopyActor->makeActorAlive();
            al::copyPose(poseCopyActor, this);
            al::LiveActor* capManHeroEyesActor = mCapManHeroEyesActor;
            al::startAction(capManHeroEyesActor, actionName);
            capManHeroEyesActor->makeActorAlive();
            al::copyPose(capManHeroEyesActor, this);
            al::LiveActor* demoMarioCapActor = mDemoMarioCapActor;
            al::startAction(demoMarioCapActor, actionName);
            demoMarioCapActor->makeActorAlive();
            al::copyPose(demoMarioCapActor, this);
            al::hideSilhouetteModel(mDemoMarioCapActor);
            al::LiveActor* demoPlayerActor = mDemoPlayerActor;
            al::startAction(demoPlayerActor, actionName);
            demoPlayerActor->makeActorAlive();
            al::copyPose(demoPlayerActor, this);
            al::tryStartActionSubActorAll(mDemoPlayerActor, actionName);
            rs::hideDemoPlayer(this);
            rs::addDemoActor(mDemoPlayerActor, true);
            al::startAction(mWaterfallWorldDemoStepActor, actionName);
            mWaterfallWorldHomeRockBreakActor->makeActorAlive();

            isInvalidateCameraAngleSwing = true;
            demoName = "ホーム初離陸デモ";
        } else {
            al::copyPose(mPoseCopyActor, this);
            actionName = "DemoWorldTakeoff";
            al::startAction(mPoseCopyActor, actionName);
            mPoseCopyActor->makeActorAlive();
        }

        rs::hideDemoPlayerSilhouette(this);
        rs::replaceDemoPlayer(this, al::getTrans(this), quat);
        if (isStartDemoPlayer)
            rs::startActionDemoPlayer(this, actionName);
        rs::hideDemoCap(this);
        if (al::isActiveCamera(mDemoAppearFromHomeCameraTicket))
            al::endCamera(this, mDemoAppearFromHomeCameraTicket, -1, false);
        al::makeMtxSRT(&mDemoPlayerMtx, this);
        al::startAnimCamera(this, mDemoAppearFromHomeCameraTicket, actionName, 0);
        if (isInvalidateCameraAngleSwing)
            al::invalidateAnimCameraAngleSwing(mDemoAppearFromHomeCameraTicket);
        al::startAction(this, actionName);

        GameDataFunction::launchHome(GameDataHolderWriter(holder));
        GameDataFunction::tryChangeNextStageWithDemoWorldWarp(
            GameDataHolderWriter(holder),
            GameDataFunction::getMainStageName(GameDataHolderAccessor(holder), mWorldId));

        if (!demoName) {
            demoName = GameDataFunction::isForwardWorldWarpDemo(GameDataHolderAccessor(holder)) ?
                           "ホーム離陸デモ(東へ)" :
                           "ホーム離陸デモ(西へ)";
        }
        rs::setDemoInfoDemoName(this, demoName);
    }

    updateParts();

    if (mShineTowerRock && al::isStep(this, 3090))
        rs::requestWipeClose(holder, "ホーム離陸");

    if (isTakeoffMoonFromOther) {
        s32 wipeStep = al::getActionFrameMax(this);
        if (al::isStep(this, wipeStep - 60))
            rs::requestWipeClose(holder, "ホーム離陸");
    }

    if (mIsFixDoorwayCamera) {
        if (al::isStep(this, 431))
            al::requestCaptureScreenCover(this, 2);
        if (mIsFixDoorwayCamera && al::isStep(this, 867)) {
            al::hideModel(mDemoPlayerActor2);
            al::showModel(mDemoPlayerActor);
        }
    }

    if (mShineTowerRock && al::isStep(this, 1330))
        al::requestCaptureScreenCover(this, 2);

    if (al::isActionEnd(this) && al::isEndAnimCamera(mDemoAppearFromHomeCameraTicket)) {
        if ((isTakeoffMoonFromOther & 1) | (mShineTowerRock != nullptr)) {
            al::setNerve(this, &NrvShineTowerRocket.DemoWarpWorld);
        } else {
            al::requestCaptureScreenCover(this, 2);
            al::setNerve(this, &NrvShineTowerRocket.DemoWorldTakeoffNext);
        }
    }
}

void ShineTowerRocket::exeDemoWorldTakeoffNext() {
    if (al::isFirstStep(this)) {
        al::hideShadowMask(mDoorAreaChange);
        bool isForwardWorldWarpDemo =
            GameDataFunction::isForwardWorldWarpDemo(GameDataHolderAccessor(this));
        al::setAnimCameraRotateBaseUp(mDemoAppearFromHomeCameraTicket);
        al::invalidateAnimCameraAngleSwing(mDemoAppearFromHomeCameraTicket);

        const char* actionName;
        if (isForwardWorldWarpDemo) {
            sead::Matrix34f rotateTransMtx = sead::Matrix34f::ident;
            al::makeMtxRotateTrans(&rotateTransMtx, mCameraRotateEastTrans, mCameraRotateEastRot);
            mDemoPlayerMtx.setMul(mDokanDemoMtx, rotateTransMtx);
            al::resetMtxPosition(this, mDemoPlayerMtx);

            sead::Quatf quat = sead::Quatf::unit;
            al::calcQuat(&quat, this);
            rs::replaceDemoPlayer(this, al::getTrans(this), quat);
            updateParts();
            actionName = "DemoWorldTakeoffEast";
        } else {
            sead::Matrix34f rotateTransMtx = sead::Matrix34f::ident;
            al::makeMtxRotateTrans(&rotateTransMtx, mCameraRotateWestTrans, mCameraRotateWestRot);
            mDemoPlayerMtx.setMul(mDokanDemoMtx, rotateTransMtx);
            al::resetMtxPosition(this, mDemoPlayerMtx);

            sead::Quatf quat = sead::Quatf::unit;
            al::calcQuat(&quat, this);
            rs::replaceDemoPlayer(this, al::getTrans(this), quat);
            updateParts();
            actionName = "DemoWorldTakeoffWest";
        }

        if (al::isActiveCamera(mDemoAppearFromHomeCameraTicket))
            al::endCamera(this, mDemoAppearFromHomeCameraTicket, -1, false);
        al::startAnimCamera(this, mDemoAppearFromHomeCameraTicket, actionName, 0);
        al::startAction(this, actionName);
        rs::startActionDemoPlayer(this, actionName);
        al::startAction(mPoseCopyActor, actionName);
        rs::hideDemoPlayerSilhouette(this);
    }

    updateParts();

    if (al::isStep(this, 340))
        rs::requestWipeClose(this, "ホーム離陸");

    if (al::isActionEnd(this) && al::isEndAnimCamera(mDemoAppearFromHomeCameraTicket))
        al::setNerve(this, &NrvShineTowerRocket.DemoWarpWorld);
}

void ShineTowerRocket::setupWorldTakeoffPose(bool isNext) {
    al::setAnimCameraRotateBaseUp(mDemoAppearFromHomeCameraTicket);
    al::invalidateAnimCameraAngleSwing(mDemoAppearFromHomeCameraTicket);

    sead::Matrix34f rotateTransMtx = sead::Matrix34f::ident;
    if (isNext)
        al::makeMtxRotateTrans(&rotateTransMtx, mCameraRotateEastTrans, mCameraRotateEastRot);
    else
        al::makeMtxRotateTrans(&rotateTransMtx, mCameraRotateWestTrans, mCameraRotateWestRot);

    mDemoPlayerMtx.setMul(mDokanDemoMtx, rotateTransMtx);
    al::resetMtxPosition(this, mDemoPlayerMtx);

    sead::Quatf quat = sead::Quatf::unit;
    al::calcQuat(&quat, this);
    rs::replaceDemoPlayer(this, al::getTrans(this), quat);
    updateParts();
}

void ShineTowerRocket::exeDemoAppearFromEntrance() {
    if (al::isFirstStep(this)) {
        sead::Vector3f front = mEntranceCameraFront;
        sead::Vector3f up = sead::Vector3f::zero;
        al::calcUpDir(&up, this);
        sead::Quatf quat = sead::Quatf::unit;
        al::makeQuatFrontUp(&quat, front, up);

        sead::Vector3f trans = al::getTrans(this);
        trans += front * 540.0f;
        trans += up * 190.0f;

        al::validateEndEntranceCamera(this);
        rs::replaceDemoPlayer(this, trans, quat);
        rs::requestEndDemoWithPlayer(this);
        if (!al::isPlayingEntranceCamera(this, 0))
            tryStartEntranceCamera(0);
        al::setNerve(this, &NrvShineTowerRocket.Wait);
    }
}

void ShineTowerRocket::exeDemoMeterUpPost() {
    if (al::isGreaterEqualStep(this, 45))
        al::setNerve(this, &DemoScaleUp);
}

void ShineTowerRocket::exeDemoTutorialShine() {
    if (al::isFirstStep(this))
        rs::startEventFlow(mEventFlowExecutor, "TutorialShine");

    if (rs::updateEventFlow(mEventFlowExecutor))
        al::setNerve(this, &NrvShineTowerRocket.WaitIgnoreLockOn);
}

void ShineTowerRocket::exeDemoInformPowerUp() {
    if (al::isFirstStep(this))
        rs::setDemoInfoDemoName(this, "ホームメッセージデモ");

    if (al::isGreaterEqualStep(this, 0))
        al::setNerve(this, &DemoInformPowerUpMessage);
}

void ShineTowerRocket::exeDemoInformPowerUpMessage() {
    if (al::isFirstStep(this))
        rs::startEventFlow(mEventFlowExecutor, "PowerUp");

    if (rs::updateEventFlow(mEventFlowExecutor)) {
        if (GameDataFunction::isWorldSand(GameDataHolderAccessor(this)) &&
            GameDataFunction::getScenarioNo(this) == 1 &&
            rs::tryStartKoopaShipDemoHomeFlyAway(this)) {
            al::endCamera(this, mDemoAppearFromHomeCameraTicket, -1, false);
            if (al::isActiveCamera(mWorldMapFadeCameraTicket))
                al::endCamera(this, mWorldMapFadeCameraTicket, -1, false);
            al::setNerve(this, &NrvShineTowerRocket.DemoKoopaShip);
        } else if (mIsCompleteShine) {
            al::setNerve(this, &NrvShineTowerRocket.DemoInformPeachCastleCap);
        } else {
            al::setNerve(this, &NrvShineTowerRocket.GoToWorldMapWithFade);
        }
    }
}

void ShineTowerRocket::exeDemoKoopaShip() {
    if (al::isFirstStep(this))
        rs::setDemoInfoDemoName(this, "砂ワールドクッパ戦艦デモ");

    if (rs::isEnableStartWipeKoopaShipDemoHomeFlyAway(this))
        al::setNerve(this, &DemoKoopaShipFade);
}

void ShineTowerRocket::exeDemoInformRepairHome() {
    if (al::isFirstStep(this))
        rs::startEventFlow(mDemoEventFlowExecutor, "InformRepairHome");

    if (rs::updateEventFlow(mDemoEventFlowExecutor)) {
        al::requestCaptureScreenCover(this, 4);
        al::setNerve(this, &NrvShineTowerRocket.DemoWorldTakeoff);
    }
}

void ShineTowerRocket::exeDemoInformNewHome() {
    if (al::isFirstStep(this))
        rs::setDemoInfoDemoName(this, "ホームメッセージデモ");

    if (al::isGreaterEqualStep(this, 0)) {
        if (GameDataFunction::isWorldClash(GameDataHolderAccessor(this)) ||
            GameDataFunction::isWorldBoss(GameDataHolderAccessor(this)))
            al::setNerve(this, &NrvShineTowerRocket.DemoInformRepairHome);
        else
            al::setNerve(this, &NrvShineTowerRocket.DemoInformNewHomeMessage);
    }
}

void ShineTowerRocket::exeDemoInformNewHomeMessage() {
    al::EventFlowExecutor** executor;
    if (al::isFirstStep(this)) {
        bool isWorldSky = GameDataFunction::isWorldSky(GameDataHolderAccessor(this));
        al::EventFlowExecutor* eventFlowExecutor = mDemoEventFlowExecutor;
        executor = &mDemoEventFlowExecutor;
        if (isWorldSky)
            rs::startEventFlow(eventFlowExecutor, "InformCompleteHome");
        else
            rs::startEventFlow(eventFlowExecutor, "InformNewHome");
    } else {
        executor = &mDemoEventFlowExecutor;
    }

    if (rs::updateEventFlow(*executor))
        al::setNerve(this, &NrvShineTowerRocket.GoToWorldMapWithFade);
}

void ShineTowerRocket::exeDemoInformPeachCastleCap() {
    if (al::isFirstStep(this))
        rs::startEventFlow(mDemoEventFlowExecutor, "InformPeachCastleCap");

    if (rs::updateEventFlow(mDemoEventFlowExecutor)) {
        if (mDemoPeachCastleCapActor)
            mDemoPeachCastleCapActor->appear();

        if (mIsWorldMap || mIsDemoPeachCastleCap) {
            al::setNerve(this, &NrvShineTowerRocket.GoToWorldMapWithFade);
        } else {
            if (al::isActiveCamera(mWorldMapFadeCameraTicket))
                al::endCamera(this, mWorldMapFadeCameraTicket, -1, false);
            if (al::isActiveCamera(mDemoAppearFromHomeCameraTicket))
                al::endCamera(this, mDemoAppearFromHomeCameraTicket, -1, false);
            tryStartEntranceCamera(0);
            al::resumeActiveBgm(this, 180);
            al::setNerve(this, &NrvShineTowerRocket.WaitIgnoreLockOn);
        }
    }
}

void ShineTowerRocket::exeDemoInformNewItem() {
    if (al::isFirstStep(this)) {
        rs::startEventFlow(mDemoEventFlowExecutor, "InformNewItem");
        rs::setDemoInfoDemoName(this, "ショップに新しい品物入荷デモ");
    }

    if (rs::updateEventFlow(mDemoEventFlowExecutor) && !tryLevelUp()) {
        if (mIsDemoPeachCastleCap) {
            al::setNerve(this, &NrvShineTowerRocket.DemoInformCompleteShineFadeIn);
        } else if (mIsCompleteShine) {
            al::setNerve(this, &NrvShineTowerRocket.DemoInformPeachCastleCap);
        } else {
            calcPlayerPoseForPayDemo();
            if (al::isActiveCamera(mWorldMapFadeCameraTicket))
                al::endCamera(this, mWorldMapFadeCameraTicket, -1, false);
            if (al::isActiveCamera(mDemoAppearFromHomeCameraTicket))
                al::endCamera(this, mDemoAppearFromHomeCameraTicket, -1, false);
            tryStartEntranceCamera(0);
            al::resumeActiveBgm(this, 180);
            al::setNerve(this, &NrvShineTowerRocket.WaitIgnoreLockOn);
        }
    }
}

void ShineTowerRocket::exeDemoInformCompleteShineFadeIn() {
    if (al::isFirstStep(this))
        mDemoWipe->startClose(60);

    if (mDemoWipe->isCloseEnd())
        al::setNerve(this, &DemoInformCompleteShineFadeWait);
}

void ShineTowerRocket::exeDemoInformCompleteShineFadeOut() {
    if (al::isFirstStep(this))
        mDemoWipe->startOpen(60);

    if (al::isGreaterEqualStep(this, 75))
        al::setNerve(this, &DemoInformCompleteShine);
}

void ShineTowerRocket::exeDemoInformCompleteShineFadeWait() {
    if (al::isFirstStep(this)) {
        mShineTowerCommonKeeper->update();
        mShineTowerCommonKeeper->updateSensor();
        if (al::isActiveCamera(mDemoAppearFromHomeCameraTicket))
            al::endCamera(this, mDemoAppearFromHomeCameraTicket, -1, false);
        calcCameraMtxLevelUp();
        al::startAnimCameraWithStartStepAndEndStepAndPlayStep(this, mDemoAppearFromHomeCameraTicket,
                                                              "DemoChangeHome", 363, 363, 0, 0);
    }

    if (al::isGreaterEqualStep(this, 60))
        al::setNerve(this, &DemoInformCompleteShineFadeOut);
}

void ShineTowerRocket::exeDemoInformCompleteShine() {
    al::EventFlowExecutor** executor;
    if (al::isFirstStep(this)) {
        rs::setDemoInfoDemoName(this, "ホームメッセージデモ");
        rs::startEventFlow(mDemoEventFlowExecutor, "InformCompleteShine");
        executor = &mDemoEventFlowExecutor;
    } else {
        executor = &mDemoEventFlowExecutor;
    }

    if (rs::updateEventFlow(*executor)) {
        if (mIsCompleteShine)
            al::setNerve(this, &NrvShineTowerRocket.DemoInformPeachCastleCap);
        else if (mIsWorldMap)
            al::setNerve(this, &NrvShineTowerRocket.DemoUpLevelCamera);
        else
            al::setNerve(this, &NrvShineTowerRocket.GoToWorldMapWithFade);
    }
}

void ShineTowerRocket::exeDemoWarpWorld() {
    if (al::isFirstStep(this)) {
        GameDataHolderAccessor accessor(this);
        accessor->setStageChanging(true);
    }
}

void ShineTowerRocket::exeDemoAppearPlayerFromHomeAfter() {
    al::endCamera(this, mDemoAppearFromHomeCameraTicket, 0, false);
    rs::showDemoPlayerSilhouette(this);
    rs::replaceDemoPlayer(this, mDemoReturnPlayerTrans, mDemoReturnPlayerQuat);
    rs::startActionDemoPlayer(this, "Wait");
    rs::clearDemoAnimInterpolatePlayer(this);
    rs::requestEndDemoAppearFromHome(this);
    al::validateEndEntranceCamera(this);
    mShineTowerCommonKeeper->setMeterRotateForWorld(true);
    al::setNerve(this, &NrvShineTowerRocket.WaitIgnoreLockOn);
}

namespace {
__attribute__((used, noinline)) void setupCapTargetInfoForHomeGlobe(sead::Vector3f* offset,
                                                                    CapTargetInfo* capTargetInfo,
                                                                    al::LiveActor* actor,
                                                                    al::LiveActor* damageModel) {
    if (damageModel && al::isAlive(damageModel))
        al::calcJointPos(offset, damageModel, "GlobeCapPoint");
    else
        al::calcJointPos(offset, actor, "GlobeCapPoint");

    al::multVecInvPose(offset, actor, *offset);
    capTargetInfo->setFollowLockOnMtx(nullptr, *offset, sead::Vector3f::zero);

    offset->x += 0.0f;
    offset->y -= 70.0f;
    offset->z += 0.0f;
    al::setSensorFollowPosOffset(actor, "Body", *offset);
}

void invalidateDitherAnimAll(al::LiveActor* actor) {
    if (!al::isExistSubActorKeeper(actor))
        return;

    for (s32 i = 0; i < al::getSubActorNum(actor); i++) {
        al::LiveActor* subActor = al::getSubActor(actor, i);
        if (al::isExistDitherAnimator(subActor)) {
            al::invalidateDitherAnim(subActor);
            al::setModelAlphaMask(subActor, 1.0f);
        }
        invalidateDitherAnimAll(subActor);
    }
}

void invalidateOcclusionQueryAll(al::LiveActor* actor) {
    al::invalidateOcclusionQuery(actor);
    if (!al::isExistSubActorKeeper(actor))
        return;

    for (s32 i = 0; i < al::getSubActorNum(actor); i++)
        invalidateOcclusionQueryAll(al::getSubActor(actor, i));
}

void validateDitherAnimAll(al::LiveActor* actor) {
    if (!al::isExistSubActorKeeper(actor))
        return;

    for (s32 i = 0; i < al::getSubActorNum(actor); i++) {
        al::LiveActor* subActor = al::getSubActor(actor, i);
        if (al::isExistDitherAnimator(subActor))
            al::validateDitherAnim(subActor);
        validateDitherAnimAll(subActor);
    }
}

void validateOcclusionQueryAll(al::LiveActor* actor) {
    al::validateOcclusionQuery(actor);
    if (!al::isExistSubActorKeeper(actor))
        return;

    for (s32 i = 0; i < al::getSubActorNum(actor); i++)
        validateOcclusionQueryAll(al::getSubActor(actor, i));
}

__attribute__((noinline)) s32 calcRestShineNum(const al::LiveActor* actor) {
    bool isGameClear = false;
    const al::IUseSceneObjHolder* holder = actor;
    s32 unlockShineNum =
        GameDataFunction::findUnlockShineNum(&isGameClear, GameDataHolderAccessor(holder));
    s32 payShineNum = GameDataFunction::getPayShineNum(GameDataHolderAccessor(holder));
    if (isGameClear)
        payShineNum = GameDataFunction::getTotalPayShineNum(GameDataHolderAccessor(holder));

    s32 rest = unlockShineNum - payShineNum;
    if (rest < 0)
        rest = 0;
    return rest;
}

__attribute__((noinline)) bool isDamageHomeModel(const al::LiveActor* actor) {
    const al::IUseSceneObjHolder* holder = actor;

    GameDataHolderAccessor accessorCrashHome(holder);
    if (!GameDataFunction::isCrashHome(accessorCrashHome)) {
        if (!GameDataFunction::isRepairHome(GameDataHolderAccessor(holder)))
            return false;
    }

    return GameDataFunction::getHomeLevel(GameDataHolderAccessor(holder)) < 9;
}

__attribute__((noinline)) bool isNeedShowHomeSkyMessage(const ShineTowerRocket* actor) {
    u8 isNeedShow = 0;

    if (rs::isExistKoopaShipInSky(actor)) {
        isNeedShow = 1;
    } else {
        const al::IUseSceneObjHolder* holder = actor;
        if (GameDataFunction::getCurrentShineNum(GameDataHolderAccessor(holder)) == 0) {
            if (GameDataFunction::isWorldSky(GameDataHolderAccessor(holder))) {
                if (GameDataFunction::getScenarioNo(actor) == 2) {
                    if (!GameDataFunction::isAlreadyGoWorld(GameDataHolderAccessor(holder),
                                                            GameDataFunction::getWorldIndexMoon()))
                        isNeedShow = 1;
                }
            }
        }
    }
    return isNeedShow & 1;
}

__attribute__((used, noinline)) bool isPayShineEnoughForUnlock(const al::LiveActor* actor) {
    bool isGameClear = false;
    const al::IUseSceneObjHolder* holder = actor;
    s32 unlockShineNum =
        GameDataFunction::findUnlockShineNum(&isGameClear, GameDataHolderAccessor(holder));
    s32 payShineNum = GameDataFunction::getPayShineNum(GameDataHolderAccessor(holder));
    if (isGameClear)
        payShineNum = GameDataFunction::getTotalPayShineNum(GameDataHolderAccessor(holder));

    return payShineNum >= unlockShineNum;
}

__attribute__((noinline)) bool isNeedDemoWalkPlayerToPoint(const al::LiveActor* actor) {
    const al::IUseSceneObjHolder* holder = actor;
    if (GameDataFunction::getCurrentWorldId(GameDataHolderAccessor(holder)) == 1 &&
        !GameDataFunction::isUnlockedWorld(GameDataHolderAccessor(holder), 2))
        return true;
    if (GameDataFunction::isCrashHome(GameDataHolderAccessor(holder)))
        return true;
    return GameDataFunction::isBossAttackedHome(GameDataHolderAccessor(holder));
}

__attribute__((noinline)) bool isEnableUnlockWorldByPayShine(const al::LiveActor* actor,
                                                             s32 worldId, s32 payShineNum) {
    if (GameDataFunction::getWorldIndexMoon() == worldId &&
        !GameDataFunction::isGameClear(GameDataHolderAccessor(actor)))
        return false;

    if (GameDataFunction::checkEnableUnlockWorldSpecial1(actor))
        return true;
    if (GameDataFunction::checkEnableUnlockWorldSpecial2(actor))
        return true;

    bool isGameClear = false;
    const al::IUseSceneObjHolder* holder = actor;
    s32 unlockShineNum =
        GameDataFunction::findUnlockShineNum(&isGameClear, GameDataHolderAccessor(holder));
    s32 currentPayShineNum = GameDataFunction::getPayShineNum(GameDataHolderAccessor(holder));
    if (isGameClear)
        currentPayShineNum = GameDataFunction::getTotalPayShineNum(GameDataHolderAccessor(holder));

    if (currentPayShineNum < unlockShineNum)
        return currentPayShineNum + payShineNum >= unlockShineNum;

    s32 unlockWorldNum = GameDataFunction::getUnlockWorldNumForWorldMap(actor);
    if (unlockWorldNum == GameDataFunction::getWorldNum(GameDataHolderAccessor(holder)))
        return false;

    s32 unlockWorldId = GameDataFunction::getUnlockWorldIdForWorldMap(actor, unlockWorldNum - 1);
    return GameDataFunction::getCurrentWorldId(GameDataHolderAccessor(holder)) == unlockWorldId;
}

__attribute__((used, noinline)) f32 calcHomeMeterAnimFrame(const al::LiveActor* actor, s32 worldId) {
    bool isGameClear = false;
    const al::IUseSceneObjHolder* holder = actor;
    s32 unlockShineNum =
        GameDataFunction::findUnlockShineNumByWorldId(&isGameClear, GameDataHolderAccessor(holder),
                                                      GameDataFunction::getLatestUnlockWorldIdForShineTowerMeter(actor));
    s32 payShineNum = GameDataFunction::getPayShineNum(
        GameDataHolderAccessor(holder),
        GameDataFunction::getLatestUnlockWorldIdForShineTowerMeter(actor));
    if (isGameClear)
        payShineNum = GameDataFunction::getTotalPayShineNum(GameDataHolderAccessor(holder));

    s32 totalPayShineNum = payShineNum + worldId;
    if (totalPayShineNum >= unlockShineNum)
        return al::getSklAnimFrameMax(actor, "Meter");

    f32 totalPayShineNumRate = totalPayShineNum;
    f32 unlockShineNumRate = unlockShineNum;
    if (unlockShineNum == 0)
        unlockShineNumRate = 1.0f;

    f32 frame = totalPayShineNumRate / unlockShineNumRate;
    frame *= al::getSklAnimFrameMax(actor, "Meter");

    f32 maxFrame = al::getSklAnimFrameMax(actor, "Meter");
    if (frame < maxFrame)
        return frame;
    return maxFrame;
}

__attribute__((used, noinline)) bool isHomeMeterComplete(const al::LiveActor* actor) {
    bool isGameClear = false;
    const al::IUseSceneObjHolder* holder = actor;
    s32 unlockShineNum =
        GameDataFunction::findUnlockShineNumByWorldId(&isGameClear, GameDataHolderAccessor(holder),
                                                      GameDataFunction::getLatestUnlockWorldIdForShineTowerMeter(actor));
    s32 payShineNum = GameDataFunction::getPayShineNum(
        GameDataHolderAccessor(holder),
        GameDataFunction::getLatestUnlockWorldIdForShineTowerMeter(actor));
    if (isGameClear)
        payShineNum = GameDataFunction::getTotalPayShineNum(GameDataHolderAccessor(holder));
    return payShineNum >= unlockShineNum;
}

void resetSubActorPositionAll(al::LiveActor* actor) {
    if (!al::isExistSubActorKeeper(actor))
        return;

    for (s32 i = 0; i < al::getSubActorNum(actor); i++) {
        resetSubActorPositionAll(al::getSubActor(actor, i));
        al::resetPosition(al::getSubActor(actor, i));
    }
}

void setModelAlphaMaskSubActorAll(al::LiveActor* actor, f32 alpha) {
    if (!al::isExistSubActorKeeper(actor))
        return;

    for (s32 i = 0; i < al::getSubActorNum(actor); i++) {
        al::LiveActor* subActor = al::getSubActor(actor, i);
        if (al::isExistDitherAnimator(subActor))
            al::setModelAlphaMask(subActor, alpha);
        setModelAlphaMaskSubActorAll(subActor, alpha);
    }
}
}  // namespace

namespace {
void ShineTowerRocketNrvNoStartEnter::execute(al::NerveKeeper* keeper) const {
    ShineTowerRocket* rocket = keeper->getParent<ShineTowerRocket>();
    if (rocket->isNearPlayerEntrance() && rocket->getDoorAreaChange()->isOpen())
        al::setNerve(rocket, &NrvShineTowerRocket.NoStartEnter);
    else if (al::isGreaterEqualStep(rocket, 60))
        al::setNerve(rocket, &NrvShineTowerRocket.Wait);
}

void ShineTowerRocketNrvDemoInformCompleteShineFadeIn::execute(al::NerveKeeper* keeper) const {
    ShineTowerRocket* rocket = keeper->getParent<ShineTowerRocket>();
    if (al::isFirstStep(rocket))
        rocket->getDemoWipe()->startClose(60);

    if (rocket->getDemoWipe()->isCloseEnd())
        al::setNerve(rocket, &DemoInformCompleteShineFadeWait);
}

void ShineTowerRocketNrvDemoMeterUpPost::execute(al::NerveKeeper* keeper) const {
    ShineTowerRocket* rocket = keeper->getParent<ShineTowerRocket>();
    if (al::isGreaterEqualStep(rocket, 45))
        al::setNerve(rocket, &DemoScaleUp);
}

void ShineTowerRocketNrvDemoInformPowerUp::execute(al::NerveKeeper* keeper) const {
    ShineTowerRocket* rocket = keeper->getParent<ShineTowerRocket>();
    if (al::isFirstStep(rocket))
        rs::setDemoInfoDemoName(rocket, "ホームメッセージデモ");

    if (al::isGreaterEqualStep(rocket, 0))
        al::setNerve(rocket, &DemoInformPowerUpMessage);
}

void ShineTowerRocketNrvDemoKoopaShip::execute(al::NerveKeeper* keeper) const {
    ShineTowerRocket* rocket = keeper->getParent<ShineTowerRocket>();
    if (al::isFirstStep(rocket))
        rs::setDemoInfoDemoName(rocket, "砂ワールドクッパ戦艦デモ");

    if (rs::isEnableStartWipeKoopaShipDemoHomeFlyAway(rocket))
        al::setNerve(rocket, &DemoKoopaShipFade);
}

void ShineTowerRocketNrvDemoInformRepairHome::execute(al::NerveKeeper* keeper) const {
    ShineTowerRocket* rocket = keeper->getParent<ShineTowerRocket>();
    if (al::isFirstStep(rocket))
        rs::startEventFlow(rocket->getDemoEventFlowExecutor(), "InformRepairHome");

    if (rs::updateEventFlow(rocket->getDemoEventFlowExecutor())) {
        al::requestCaptureScreenCover(rocket, 4);
        al::setNerve(rocket, &NrvShineTowerRocket.DemoWorldTakeoff);
    }
}

void ShineTowerRocketNrvDemoInformCompleteShineFadeOut::execute(al::NerveKeeper* keeper) const {
    ShineTowerRocket* rocket = keeper->getParent<ShineTowerRocket>();
    if (al::isFirstStep(rocket))
        rocket->getDemoWipe()->startOpen(60);

    if (al::isGreaterEqualStep(rocket, 75))
        al::setNerve(rocket, &DemoInformCompleteShine);
}

void ShineTowerRocketNrvDemoAppearPlayerFromHomeAfter::execute(al::NerveKeeper* keeper) const {
    ShineTowerRocket* rocket = keeper->getParent<ShineTowerRocket>();
    al::IUseCamera* cameraUser = rocket->getIUseCamera();
    al::endCamera(cameraUser, rocket->getDemoAppearFromHomeCameraTicket(), 0, false);
    rs::showDemoPlayerSilhouette(rocket);
    rs::replaceDemoPlayer(rocket, rocket->getDemoReturnPlayerTrans(),
                          rocket->getDemoReturnPlayerQuat());
    rs::startActionDemoPlayer(rocket, "Wait");
    rs::clearDemoAnimInterpolatePlayer(rocket);
    rs::requestEndDemoAppearFromHome(rocket);
    al::validateEndEntranceCamera(cameraUser);
    rocket->getShineTowerCommonKeeper()->setMeterRotateForWorld(true);
    al::setNerve(rocket, &NrvShineTowerRocket.WaitIgnoreLockOn);
}

void ShineTowerRocketNrvDemoWarpWorld::execute(al::NerveKeeper* keeper) const {
    ShineTowerRocket* rocket = keeper->getParent<ShineTowerRocket>();
    if (al::isFirstStep(rocket)) {
        GameDataHolderAccessor accessor(rocket->getSceneObjHolderBase());
        accessor->setStageChanging(true);
    }
}
}  // namespace
