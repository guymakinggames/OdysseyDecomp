#pragma once

#include <basis/seadTypes.h>
#include <math/seadMatrix.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

#include "Library/Event/IEventFlowEventReceiver.h"
#include "Library/LiveActor/LiveActor.h"

#include "Demo/IUseDemoSkip.h"
#include "MapObj/LinkIdPair.h"

namespace al {
struct ActorInitInfo;
class AreaObj;
class EventFlowExecutor;
class HitSensor;
class SensorMsg;
template <class T>
class DeriveActorGroup;
class AddDemoInfo;
class CameraTicket;
class IUseCamera;
class IUseSceneObjHolder;
class RateParamV3f;
class RumbleCalculator;
class WipeSimple;
class AreaShapeCube;
class LiveActorGroup;
class MtxConnector;
}  // namespace al

class CapHanger;
class DoorAreaChange;
class CapTargetInfo;
class DokanInfo;
class DokanPuppetController;
class GoalMark;
class LinkObjAliveDeadCtrl;
class Bird;
class ShineTowerBackDoor;
class ShineTowerCommonKeeper;
class ShineTowerLight;
class ShineTowerRocket;
class ShineTowerGlobeAnimCtrl;
class DamageModel;
class DemoShine;

class IUsePlayerPuppet;

class ShineTowerRocket : public al::LiveActor,
                         public al::IEventFlowEventReceiver,
                         public IUseDemoSkip {
public:
    ShineTowerRocket(const char*);
    void init(const al::ActorInitInfo& info) override;
    void onSwitchDither();
    void offSwitchDither();
    void makeActorDead() override;
    void makeActorAlive() override;
    void initAfterPlacement() override;
    void startClipped() override;
    void control() override;
    bool isActiveDirtyModel() const;
    void calcAnim() override;
    bool receiveMsg(const al::SensorMsg* message, al::HitSensor* other,
                    al::HitSensor* self) override;
    void attackSensor(al::HitSensor* self, al::HitSensor* other) override;
    bool receiveEvent(const al::EventFlowEventData* event) override;
    __attribute__((noinline)) void tryStartEntranceCamera(s32);
    bool isFirstDemo() const override;
    bool isEnableSkipDemo() const override;
    void skipDemo() override;
    __attribute__((noinline)) void exeWait();
    __attribute__((noinline)) void updateParts();
    __attribute__((noinline)) bool isNearPlayerEntrance() const;
    __attribute__((noinline)) void exeReaction();
    __attribute__((noinline)) void exeDemoPrepare();
    __attribute__((noinline)) bool tryStartDemo();
    __attribute__((noinline)) void tryEndEntranceCamera();
    __attribute__((noinline)) void exeDemoWalkPlayerToPoint();
    __attribute__((noinline)) void calcPlayerPoseForPayDemo();
    __attribute__((noinline)) void tryStartHitReactionDemoStart();
    __attribute__((noinline)) void exeDemoAppearShine();
    __attribute__((noinline)) void exeDemoWaitAfterAppearShine();
    __attribute__((noinline)) bool tryLevelUp();
    __attribute__((noinline)) void exeDemoWaitBeforeScaleUpDirect();
    __attribute__((noinline)) void calcCameraMtxMeterUpPrev();
    __attribute__((noinline)) void exeDemoScaleUp();
    __attribute__((noinline)) void exeDemoMeterRotate();
    __attribute__((noinline)) void calcCameraMtx();
    __attribute__((noinline)) void setupRotateMeter();
    __attribute__((noinline)) void exeDemoMeterUpPrev();
    __attribute__((noinline)) void exeDemoMeterUp();
    __attribute__((noinline)) void exeDemoMeterUpPost();
    __attribute__((noinline)) void exeDemoTutorialShine();
    __attribute__((noinline)) void exeDemoSelectGoOtherWorld();
    __attribute__((noinline)) void exeDemoAwardMoon();
    __attribute__((noinline)) void exeDemoUpLevelCamera();
    __attribute__((noinline)) void calcCameraMtxLevelUp();
    __attribute__((noinline)) void exeDemoUpLevel();
    __attribute__((noinline)) void exeDemoInformPowerUp();
    __attribute__((noinline)) void exeDemoInformPowerUpMessage();
    __attribute__((noinline)) void exeDemoKoopaShip();
    __attribute__((noinline)) void exeDemoKoopaShipFade();
    __attribute__((noinline)) void exeDemoUpLevelCloseFade();
    __attribute__((noinline)) void exeDemoUpLevelWaitFade();
    __attribute__((noinline)) void exeDemoUpLevelOpenFade();
    __attribute__((noinline)) void exeDemoInformNewHome();
    __attribute__((noinline)) void exeDemoInformNewHomeMessage();
    __attribute__((noinline)) void exeDemoInformPeachCastleCap();
    __attribute__((noinline)) void exeDemoInformRepairHome();
    __attribute__((noinline)) void exeDemoInformNewItem();
    __attribute__((noinline)) void exeDemoInformCompleteShineFadeIn();
    __attribute__((noinline)) void exeDemoInformCompleteShineFadeWait();
    __attribute__((noinline)) void exeDemoInformCompleteShineFadeOut();
    __attribute__((noinline)) void exeDemoInformCompleteShine();
    __attribute__((noinline)) void exeDemoWarpWorld();
    __attribute__((noinline)) void exeWaitDemo();
    __attribute__((noinline)) void exeDemoAppearPlayerFromHome();
    bool isActiveDamageModel() const;
    __attribute__((noinline)) void exeDemoAppearPlayerFromHomeAfter();
    __attribute__((noinline)) void exeDemoReturnToHome();
    bool isActiveDirtyOrClashModel() const;
    __attribute__((noinline)) void exeDemoWorldTakeoff();
    __attribute__((noinline)) void exeDemoWorldTakeoffNext();
    __attribute__((noinline)) void setupWorldTakeoffPose(bool);
    __attribute__((noinline)) void exeDemoAppearFromEntrance();
    __attribute__((noinline)) void exeDemoWorldTakeoffForDebug();
    __attribute__((noinline)) void exeNoStart();
    __attribute__((noinline)) void exeNoStartEarth();
    __attribute__((noinline)) void exeNoStartEnter();
    __attribute__((noinline)) void exeBackDoor();
    __attribute__((noinline)) void exeNoStartAndCoin();
    __attribute__((noinline)) void exeGoToWorldMapWithCamera();
    __attribute__((noinline)) void setupWorldMapCameraParam();
    __attribute__((noinline)) void exeGoToWorldMapWithFade();
    __attribute__((noinline)) void exeWorldMap();
    __attribute__((noinline)) void cancelWorldMap();
    __attribute__((noinline)) void decideWorldMap(s32);
    bool isWorldMap() const;
    __attribute__((noinline)) void startDemoAppearPlayerFromHome();
    __attribute__((noinline)) void startDemoReturnToHome();
    __attribute__((noinline)) void updatePartsByDamage();
    bool isActiveClashModel() const;

    DoorAreaChange* getDoorAreaChange() const { return mDoorAreaChange; }

    DamageModel* getDamageModel() const { return mDamageModel; }

    ShineTowerLight* getShineTowerLight() const { return mShineTowerLight; }

    al::WipeSimple* getWorldMapWipe() const { return mWorldMapWipe; }

    al::WipeSimple* getDemoWipe() const { return mDemoWipe; }

    al::EventFlowExecutor* getEventFlowExecutor() const { return mEventFlowExecutor; }

    al::EventFlowExecutor* getDemoEventFlowExecutor() const { return mDemoEventFlowExecutor; }

    al::CameraTicket* getDemoAppearFromHomeCameraTicket() const {
        return mDemoAppearFromHomeCameraTicket;
    }

    const sead::Vector3f& getDemoReturnPlayerTrans() const { return mDemoReturnPlayerTrans; }

    const sead::Quatf& getDemoReturnPlayerQuat() const { return mDemoReturnPlayerQuat; }

    ShineTowerCommonKeeper* getShineTowerCommonKeeper() const { return mShineTowerCommonKeeper; }

    al::IUseCamera* getIUseCamera() { return this; }

    const al::IUseSceneObjHolder* getSceneObjHolderBase() const { return this; }

private:
    CapTargetInfo* mCapTargetInfo = nullptr;
    al::DeriveActorGroup<DemoShine>* mDemoShineGroup = nullptr;
    bool mIsDemoShineInEffectEmitted = false;
    char _129[0x3];
    s32 mDemoShineNum = 0;
    s32 mDemoShineIndex = 0;
    bool mIsWorldMap = false;
    bool mIsWorldMapCamera = false;
    char _136[2];
    s32 mWorldId = 0;
    char _13c[0x4];
    al::CameraTicket* mWorldMapCameraTicket = nullptr;
    sead::Vector3f mWorldMapCameraPos = sead::Vector3f::zero;
    sead::Vector3f mWorldMapCameraAt = sead::Vector3f::zero;
    al::CameraTicket* mDemoAppearFromHomeCameraTicket = nullptr;
    sead::Matrix34f mDemoPlayerMtx = sead::Matrix34f::ident;
    f32 mMeterRotateDegree = 0.0f;
    sead::Matrix34f mScaleRootMtx = sead::Matrix34f::ident;
    char _1cc[0x4];
    ShineTowerCommonKeeper* mShineTowerCommonKeeper = nullptr;
    al::MtxConnector* mMtxConnector = nullptr;
    sead::Vector3f mDemoReturnPlayerTrans = sead::Vector3f::zero;
    sead::Quatf mDemoReturnPlayerQuat = sead::Quatf::unit;
    char _1fc[0x4];
    al::EventFlowExecutor* mDemoEventFlowExecutor = nullptr;
    al::EventFlowExecutor* mDemoEventFlowExecutorSub = nullptr;
    al::EventFlowExecutor* mEventFlowExecutor = nullptr;
    DoorAreaChange* mDoorAreaChange = nullptr;
    al::LiveActor* mHomeMeterActor = nullptr;
    ShineTowerBackDoor* mHomeActor = nullptr;
    al::LiveActor* mDirtyModel = nullptr;
    al::LiveActor* mClashModel = nullptr;
    DamageModel* mDamageModel = nullptr;
    al::LiveActor* mShineTowerRock = nullptr;
    al::LiveActor* mWaterfallWorldDemoStepActor = nullptr;
    al::LiveActor* mWaterfallWorldHomeRockBreakActor = nullptr;
    al::WipeSimple* mDemoWipe = nullptr;
    al::WipeSimple* mWorldMapWipe = nullptr;
    al::CameraTicket* mWorldMapFadeCameraTicket = nullptr;
    sead::Vector3f mPayCameraPos = sead::Vector3f::zero;
    sead::Vector3f mPayCameraAt = sead::Vector3f::zero;
    f32 mPayCameraAngle = 0.0f;
    sead::Vector3f mDemoPayPlayerTrans = sead::Vector3f::zero;
    sead::Quatf mDemoPayPlayerQuat = sead::Quatf::unit;
    al::LiveActor* mHomeSubActor = nullptr;
    al::CameraTicket* mDemoReturnToHomeCameraTicket = nullptr;
    sead::Matrix34f mDemoReturnToHomeMtx = sead::Matrix34f::ident;
    sead::Vector3f mPlayerRestartTrans = sead::Vector3f::zero;
    sead::Quatf mPlayerRestartQuat = sead::Quatf::unit;
    s32 mPlayerRestartId = 0;
    bool mIsTryStartDemoStarted = false;
    char _311[0x3];
    f32 mDemoMeterUpStartFrame = 0.0f;
    f32 mDemoMeterUpEndFrame = 0.0f;
    bool mIsDemoWaitingToEnd = false;
    char _31d[0x3];
    al::RumbleCalculator* mDemoShineRumbleCalculator = nullptr;
    sead::Vector3f mPlayerPuppetScale = sead::Vector3f::ones;
    sead::Vector3f mCapTargetOffset = sead::Vector3f::zero;
    IUsePlayerPuppet* mPlayerPuppet = nullptr;
    GoalMark* mBackDoor = nullptr;
    al::LiveActor* mPoseCopyActor = nullptr;
    al::LiveActor* mDemoPlayerActor = nullptr;
    al::LiveActor* mDemoPlayerActor2 = nullptr;
    al::LiveActor* mDemoMarioCapActor = nullptr;
    al::LiveActor* mCapManHeroEyesActor = nullptr;
    al::CameraTicket* mEntranceCameraTicket = nullptr;
    al::RateParamV3f* mWorldMapCameraAtRateParam = nullptr;
    al::RateParamV3f* mWorldMapCameraPosRateParam = nullptr;
    sead::Vector3f mEntranceCameraFront = sead::Vector3f::zero;
    LinkIdPair mPlayerRestartLinkId;
    char _39e[0x2];
    al::AreaObj* mDokanActor = nullptr;
    DokanPuppetController* mDokanPuppetController = nullptr;
    DokanInfo* mDokanInfo = nullptr;
    al::LiveActor* mDokanDemoActor = nullptr;
    LinkIdPair mDokanDemoLinkId;
    char _3c2[0x6];
    al::LiveActor* mDemoPeachCastleCapActor = nullptr;
    bool mIsCompleteShine = false;
    bool mIsDemoPeachCastleCap = false;
    bool mIsFirstPayNotEnough = false;
    bool mIsPlayDemoAwardSpecial = false;
    char _3d4[0x4];
    al::DeriveActorGroup<Bird>* mBirdGroup = nullptr;
    al::DeriveActorGroup<CapHanger>* mHomeDemoActorGroup = nullptr;
    LinkObjAliveDeadCtrl* mLinkObjAliveDeadCtrl = nullptr;
    bool mIsAppearCoin = false;
    char _3f1[0x3];
    s32 mMeterReactionCoolTime = 0;
    sead::Vector3f mHomeGlobeCapPoint = sead::Vector3f::zero;
    sead::Vector3f mClippingOffset = sead::Vector3f::zero;
    al::AreaShapeCube* mEntranceCameraAreaShape = nullptr;
    sead::Matrix34f mEntranceCameraAreaMtx = sead::Matrix34f::ident;
    al::CameraTicket* mFixDoorwayCameraTicket = nullptr;
    bool mIsFixDoorwayCamera = false;
    char _451[0x7];
    ShineTowerGlobeAnimCtrl* mShineTowerGlobeAnimCtrl = nullptr;
    f32 mRopeRootRotateDegree = 0.0f;
    char _464[0x4];
    ShineTowerLight* mShineTowerLight = nullptr;
    al::AddDemoInfo* mAddDemoInfo = nullptr;
    sead::Matrix34f mDokanDemoMtx = sead::Matrix34f::ident;
    sead::Vector3f mCameraRotateEastTrans = sead::Vector3f::zero;
    sead::Vector3f mCameraRotateWestTrans = sead::Vector3f::zero;
    sead::Vector3f mCameraRotateEastRot = sead::Vector3f::zero;
    sead::Vector3f mCameraRotateWestRot = sead::Vector3f::zero;
    bool mIsStartHitReactionDemoStart = false;
    char _4d9[0x7];
};

static_assert(sizeof(ShineTowerRocket) == 0x4e0);
