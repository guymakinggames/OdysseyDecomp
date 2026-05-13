#include "Layout/MapLayout.h"

#include "Library/Area/AreaObj.h"
#include "Library/Area/AreaObjUtil.h"
#include "Library/Base/StringUtil.h"
#include "Library/Camera/CameraUtil.h"
#include "Library/Layout/LayoutActionFunction.h"
#include "Library/Layout/LayoutActorUtil.h"
#include "Library/Layout/LayoutInitInfo.h"
#include "Library/Layout/LayoutKeeper.h"
#include "Library/LiveActor/ActorPoseUtil.h"
#include "Library/Math/MathUtil.h"
#include "Library/Message/MessageHolder.h"
#include "Library/Matrix/MatrixUtil.h"
#include "Library/Nerve/NerveSetupUtil.h"
#include "Library/Nerve/NerveUtil.h"
#include "Library/Placement/PlacementFunction.h"
#include "Library/Play/Layout/SimpleLayoutAppearWaitEnd.h"
#include "Library/Player/PlayerHolder.h"
#include "Library/Player/PlayerUtil.h"
#include "Library/Scene/SceneObjUtil.h"
#include "Library/Se/SeFunction.h"

#include "Layout/DecideIconLayout.h"
#include "Layout/MapTerrainLayout.h"
#include "Layout/TalkMessage.h"
#include "Scene/QuestInfo.h"
#include "Scene/QuestInfoHolder.h"
#include "Scene/SceneObjFactory.h"
#include "Sequence/GameSequenceInfo.h"
#include "System/GameDataFunction.h"
#include "System/GameDataUtil.h"
#include "System/MapDataHolder.h"
#include "Util/PlayerUtil.h"
#include "Util/StageLayoutFunction.h"

#include <euiFontMgr.h>
#include <euiScalableFontMgr.h>
#include <euiTextBoxEx.h>
#include <nn/ui2d/Layout.h>
#include <nn/ui2d/Pane.h>

namespace {
NERVE_IMPL(MapLayout, Appear)
NERVE_IMPL_(MapLayout, AppearWithHint, Appear)
NERVE_IMPL_(MapLayout, AppearMoonRockDemo, Appear)
NERVE_IMPL_(MapLayout, HintAppearNpc, HintAppear)
NERVE_IMPL_(MapLayout, HintAppearMoonRock, HintAppear)
NERVE_IMPL_(MapLayout, HintAppearAmiibo, HintAppear)
NERVE_IMPL_(MapLayout, ChangeIn, Appear)
NERVE_IMPL(MapLayout, Wait)
NERVE_IMPL_(MapLayout, HintInitWaitAmiibo, HintInitWait)
NERVE_IMPL_(MapLayout, HintInitWaitNpc, HintInitWait)
NERVE_IMPL_(MapLayout, HintInitWaitMoonRock, HintInitWait)
NERVE_IMPL(MapLayout, HintAppear)
NERVE_IMPL_(MapLayout, HintDecideIconAppearNpc, HintDecideIconAppear)
NERVE_IMPL_(MapLayout, HintDecideIconAppearAmiibo, HintDecideIconAppear)
NERVE_IMPL_(MapLayout, HintDecideIconAppearMoonRock, HintDecideIconAppear)
NERVE_IMPL(MapLayout, HintDecideIconWait)
NERVE_IMPL(MapLayout, HintPressDecide)
NERVE_IMPL(MapLayout, End)
NERVE_IMPL(MapLayout, ChangeOut)

NERVES_MAKE_NOSTRUCT(MapLayout, HintAppear, HintPressDecide, ChangeOut)
NERVES_MAKE_STRUCT(MapLayout, Appear, HintInitWaitAmiibo, HintInitWaitNpc, HintInitWaitMoonRock,
                   End, ChangeIn, Wait, AppearWithHint, AppearMoonRockDemo, HintAppearNpc,
                   HintAppearMoonRock, HintAppearAmiibo, HintDecideIconAppearNpc,
                   HintDecideIconAppearAmiibo, HintDecideIconAppearMoonRock, HintDecideIconWait)
}  // namespace

static const char* sPicImage[8] = {"PicImage00", "PicImage01", "PicImage02", "PicImage03",
                                   "PicImage04", "PicImage05", "PicImage06", "PicImage07"};

static const char* sPanelNames[21] = {"TxtCaption00",
                                      "TxtRegionCaption",
                                      "TxtData01",
                                      "TxtData02",
                                      "TxtTitle01",
                                      "TxtContents01",
                                      "TxtCaption01",
                                      "TxtTitle02",
                                      "TxtContents02",
                                      "TxtCaption02",
                                      "TxtTitle03"
                                      "TxtContents03",
                                      "TxtCaption03",
                                      "TxtTitle04",
                                      "TxtContents04",
                                      "TxtCaption04",
                                      "TxtTitle05",
                                      "TxtContents05",
                                      "TxtCaption05",
                                      "TxtCaption06",
                                      "TxtContents07"};

static const char* sIconType[19] = {
    "Flag",     "FlagDisable", "Home",  "HomeDisable", "Hint",     "Hint",         "Shop",
    "ShopSold", "Race",        "Start", "Goal",        "Scenario", "ScenarioHome", "HintRock",
    "HintRock", "Cap",         "Luigi", "Poet",        "Scenario"};

inline f32 modDegree(f32 value) {
    return al::modf(value + 360.0f, 360.0f) + 0.0f;
}

MapLayout::MapLayout(const al::LayoutInitInfo& initInfo, const al::PlayerHolder* playerHolder,
                     s32 worldId)
    : al::LayoutActor("マップ"), mPlayerHolder(playerHolder) {
    al::initLayoutActor(this, initInfo, "Map", nullptr);
    initNerve(&NrvMapLayout.Appear, 0);
    al::startAction(this, "Appear", nullptr);

    HintAmiibo* hint = new HintAmiibo[3];
    if (hint != nullptr) {
        mHintAmiiboSizer = 3;
        mHintAmiibo = hint;
    }

    mWaitEndMapBg =
        new al::SimpleLayoutAppearWaitEnd("[マップ]背景", "MapBG", initInfo, nullptr, false);
    mWaitEndMapPlayer = new al::SimpleLayoutAppearWaitEnd("[マップ]プレイヤー位置", "MapPlayer",
                                                          initInfo, nullptr, false);
    mWaitEndMapCursor = new al::SimpleLayoutAppearWaitEnd("[マップ]カーソル", "MapCursor", initInfo,
                                                          nullptr, false);
    mWaitEndMapGuide =
        new al::SimpleLayoutAppearWaitEnd("[マップ]ガイド", "MapGuide", initInfo, nullptr, false);
    mWaitEndMapLine = new al::SimpleLayoutAppearWaitEnd("[マップ]カーソルライン", "MapLine",
                                                        initInfo, nullptr, false);

    mTalkMessage = new TalkMessage("[マップ]会話メッセージ");
    mTalkMessage->initLayoutOver(initInfo, "StageMap");
    mDecideIconLayout = new DecideIconLayout("[マップ]決定ボタン", initInfo);

    s32 checkpointNumMax = GameDataFunction::getCheckpointNumMaxInWorld(this);
    s32 hintNumMax = GameDataFunction::getHintNumMax(this);
    s32 shopNpcIconNumMax = GameDataFunction::getShopNpcIconNumMax(this);
    s32 miniGameNumMax = GameDataFunction::getMiniGameNumMax(this);
    s32 mainScenarioNumMax = GameDataFunction::getMainScenarioNumMax(this);
    s32 hintMoonRockNumMax = GameDataFunction::getHintMoonRockNumMax(this);

    mCheckpointNumMaxInWorld = checkpointNumMax;
    mHintNumMax = hintNumMax;

    s32 mapIconSizeNumMax = checkpointNumMax + hintNumMax + shopNpcIconNumMax + miniGameNumMax +
                            mainScenarioNumMax + hintMoonRockNumMax * 2 + 7;
    MapIconInfo* iconInfo = new MapIconInfo[mapIconSizeNumMax];
    for (s32 i = 0; i < mapIconSizeNumMax; i++) {
        iconInfo[i] = {
            .iconLayout = nullptr,
            .isActive = false,
            .iconType = IconType::MaxValue,
            .position = sead::Vector3f::zero,
            .action = 0,
            .position2 = sead::Vector2f::zero,
            .value = false,
        };
    }
    mMapIconInfo = iconInfo;
    mMapIconInfoSize = mapIconSizeNumMax;
    mMainScenarioNumMax = mainScenarioNumMax;
    mHintMoonRockNumMax = hintMoonRockNumMax;
    mMiniGameNumMax = miniGameNumMax;

    s32 mapIconIndex = 0;

    mCheckpointIconLayouts = new MapIconLayout[checkpointNumMax]();
    for (s32 i = 0; i < checkpointNumMax; i++, mapIconIndex++) {
        mCheckpointIconLayouts[i].fieldA = mapIconIndex;
        mCheckpointIconLayouts[i].fieldB = i;
        mCheckpointIconLayouts[i].layout = new al::LayoutActor("[マップ]アイコン");
        al::initLayoutActor(mCheckpointIconLayouts[i].layout, initInfo, "MapIcon", "CheckPoint");
        mMapIconInfo[mapIconIndex].iconLayout = &mCheckpointIconLayouts[i];
    }

    mHomeIconLayout = new MapIconLayout(mapIconIndex, 0, nullptr);
    mHomeIconLayout->layout = new al::LayoutActor("[マップ]アイコン");
    al::initLayoutActor(mHomeIconLayout->layout, initInfo, "MapIcon", nullptr);
    mMapIconInfo[mapIconIndex].iconLayout = mHomeIconLayout;
    mapIconIndex++;

    mHintIconLayouts = new MapIconLayout[hintNumMax]();
    for (s32 i = 0; i < hintNumMax; i++, mapIconIndex++) {
        mHintIconLayouts[i].fieldA = mapIconIndex;
        mHintIconLayouts[i].fieldB = i;
        mHintIconLayouts[i].layout = new al::LayoutActor("[マップ]アイコン");
        al::initLayoutActor(mHintIconLayouts[i].layout, initInfo, "MapIcon", "Hint");
        mMapIconInfo[mapIconIndex].iconLayout = &mHintIconLayouts[i];
    }

    mShopIconLayouts = new MapIconLayout[shopNpcIconNumMax]();
    for (s32 i = 0; i < shopNpcIconNumMax; i++, mapIconIndex++) {
        mShopIconLayouts[i].fieldA = mapIconIndex;
        mShopIconLayouts[i].fieldB = 0;
        mShopIconLayouts[i].layout = new al::LayoutActor("[マップ]アイコン");
        al::initLayoutActor(mShopIconLayouts[i].layout, initInfo, "MapIcon", nullptr);
        mMapIconInfo[mapIconIndex].iconLayout = &mShopIconLayouts[i];
    }

    mMiniGameIconLayouts = new MapIconLayout[miniGameNumMax]();
    for (s32 i = 0; i < miniGameNumMax; i++, mapIconIndex++) {
        mMiniGameIconLayouts[i].fieldA = mapIconIndex;
        mMiniGameIconLayouts[i].fieldB = i;
        mMiniGameIconLayouts[i].layout = new al::LayoutActor("[マップ]アイコン");
        al::initLayoutActor(mMiniGameIconLayouts[i].layout, initInfo, "MapIcon", nullptr);
        mMapIconInfo[mapIconIndex].iconLayout = &mMiniGameIconLayouts[i];
    }

    mRaceStartIconLayout = new MapIconLayout(mapIconIndex, 0, nullptr);
    mRaceStartIconLayout->layout = new al::LayoutActor("[マップ]アイコン");
    al::initLayoutActor(mRaceStartIconLayout->layout, initInfo, "MapIcon", nullptr);
    mMapIconInfo[mapIconIndex].iconLayout = mRaceStartIconLayout;
    mapIconIndex++;

    mRaceGoalIconLayout = new MapIconLayout(mapIconIndex, 0, nullptr);
    mRaceGoalIconLayout->layout = new al::LayoutActor("[マップ]アイコン");
    al::initLayoutActor(mRaceGoalIconLayout->layout, initInfo, "MapIcon", nullptr);
    mMapIconInfo[mapIconIndex].iconLayout = mRaceGoalIconLayout;
    mapIconIndex++;

    mScenarioIconLayouts = new MapIconLayout[mainScenarioNumMax]();
    for (s32 i = 0; i < mainScenarioNumMax; i++, mapIconIndex++) {
        mScenarioIconLayouts[i].fieldA = mapIconIndex;
        mScenarioIconLayouts[i].fieldB = i;
        mScenarioIconLayouts[i].layout = new al::LayoutActor("[マップ]アイコン");
        al::initLayoutActor(mScenarioIconLayouts[i].layout, initInfo, "MapIcon", "Aiming");
        mMapIconInfo[mapIconIndex].iconLayout = &mScenarioIconLayouts[i];
    }

    mMoonRockIconLayouts = new MapIconLayout[hintMoonRockNumMax]();
    for (s32 i = 0; i < hintMoonRockNumMax; i++, mapIconIndex++) {
        mMoonRockIconLayouts[i].fieldA = mapIconIndex;
        mMoonRockIconLayouts[i].fieldB = i;
        mMoonRockIconLayouts[i].layout = new al::LayoutActor("[マップ]アイコン");
        al::initLayoutActor(mMoonRockIconLayouts[i].layout, initInfo, "MapIcon", "Hint");
        mMapIconInfo[mapIconIndex].iconLayout = &mMoonRockIconLayouts[i];
    }

    mJangoIconLayout = new MapIconLayout(mapIconIndex, 0, nullptr);
    mJangoIconLayout->layout = new al::LayoutActor("[マップ]アイコン");
    al::initLayoutActor(mJangoIconLayout->layout, initInfo, "MapIcon", nullptr);
    mMapIconInfo[mapIconIndex].iconLayout = mJangoIconLayout;
    mapIconIndex++;

    mTimeBalloonIconLayout = new MapIconLayout(mapIconIndex, 0, nullptr);
    mTimeBalloonIconLayout->layout = new al::LayoutActor("[マップ]アイコン");
    al::initLayoutActor(mTimeBalloonIconLayout->layout, initInfo, "MapIcon", nullptr);
    mMapIconInfo[mapIconIndex].iconLayout = mTimeBalloonIconLayout;
    mapIconIndex++;

    mPoetterIconLayout = new MapIconLayout(mapIconIndex, 0, nullptr);
    mPoetterIconLayout->layout = new al::LayoutActor("[マップ]アイコン");
    al::initLayoutActor(mPoetterIconLayout->layout, initInfo, "MapIcon", nullptr);
    mMapIconInfo[mapIconIndex].iconLayout = mPoetterIconLayout;
    mapIconIndex++;

    mUnusedIconLayout = new MapIconLayout(mapIconIndex, 0, nullptr);
    mUnusedIconLayout->layout = new al::LayoutActor("[マップ]アイコン");
    al::initLayoutActor(mUnusedIconLayout->layout, initInfo, "MapIcon", nullptr);
    mMapIconInfo[mapIconIndex].iconLayout = mUnusedIconLayout;

    setPanelName(this, GameDataFunction::getWorldDevelopName(this, worldId), worldId);
    mMapTerrainLayout = new MapTerrainLayout("[マップ]地形");
    al::initLayoutPartsActor(mMapTerrainLayout, this, initInfo, "ParMap", nullptr);

    al::startAction(mMapTerrainLayout, "Wait", nullptr);
    if (GameDataFunction::isWorldSnow(this)) {
        mArray.allocBuffer(5, nullptr);
        for (s32 i = 0; i < 5; i++) {
            al::LayoutActor* layActor = new al::LayoutActor("[マップ]アイコンライン");
            al::initLayoutActor(layActor, initInfo, "MapIconLine", nullptr);
            mArray.pushBack(layActor);
        }
    }

    changePrintWorld(worldId);
    loadTexture();
    appear();
}

void MapLayout::appear() {
    s32 size = mMapIconInfoSize;
    MapIconInfo* iconInfo = mMapIconInfo;
    for (s32 i = 0; i < size; i++) {
        iconInfo->isActive = false;
        iconInfo->iconType = IconType::MaxValue;
        iconInfo->position = sead::Vector3f::zero;
        iconInfo->action = 0;
        iconInfo->position2 = sead::Vector2f::zero;
        iconInfo->value = false;
        iconInfo++;
    }

    if (mIsPrintWorldChanged) {
        sead::Vector2f prevLocalSize = mPanelLocalScale;
        sead::Vector2f prevPanelSize = mPanelSize;
        reset();

        if (!mIsResetTransform) {
            mPanelLocalScale = prevLocalSize;
            mPanelSize = prevPanelSize;
        }
        mIsResetTransform = false;

        al::LiveActor* playerActor = al::tryGetPlayerActor(mPlayerHolder, 0);
        sead::Vector3f position = {0.0f, 0.0f, 0.0f};

        if (playerActor == nullptr) {
            moveFocusLayout(position, sead::Vector2f::zero);
        } else {
            position.set(al::getTrans(playerActor));
            al::AreaObj* areaObj =
                al::tryFindAreaObj(playerActor, "InvalidateStageMapArea", position);
            if (areaObj != nullptr) {
                al::getLinksQT(nullptr, &position, *areaObj->getPlacementInfo(), "PlayerPoint");
            } else {
                mScrollPosition = sead::Vector2f::zero;
                if (!GameDataFunction::isMainStage(this))
                    position.set(GameDataFunction::getStageMapPlayerPos(playerActor));
            }
        }

        moveFocusLayout(position, sead::Vector2f::zero);
        updateST();
    }

    mMapTerrainLayout->appear();
    al::LayoutActor::appear();
    al::setNerve(this, &NrvMapLayout.Appear);
    al::startAction(this, "Appear", nullptr);
    al::startAction(this, "LeftIn", "Change");
    al::setActionFrame(this, al::getActionFrameMax(this, "LeftIn", "Change"), "Change");

    if (mIsHelp)
        mWaitEndMapBg->appear();
}

void MapLayout::control() {
    if (al::isActive(mWaitEndMapLine))
        updateLine(mScenarioIconLayouts->layout);
}

const char* MapLayout::getSceneObjName() const {
    return "マップレイアウト";
}

void setPanelName(al::LayoutActor* layoutActor, const char* message, s32 id) {
    al::setPaneString(layoutActor, "TxtWorld", GameDataFunction::tryGetWorldName(layoutActor, id),
                      0);
    al::setPaneString(layoutActor, "TxtRegion",
                      GameDataFunction::tryGetRegionNameCurrent(layoutActor), 0);
    for (s32 i = 0; i < 21; i++) {
        al::StringTmp<64> stageMapMessage = {"%s_%s", sPanelNames[i], message};
        rs::trySetPaneSystemMessageIfExist(layoutActor, sPanelNames[i], "StageMapMessage",
                                           stageMapMessage.cstr());
    }
}

void MapLayout::changePrintWorld(s32 worldId) {
    mIsPrintWorldChanged = mMapTerrainLayout->tryChangePrintWorld(worldId);
    mWorldId = worldId;
    reset();
}

void MapLayout::loadTexture() {
    const char* worldDevelopName = GameDataFunction::getWorldDevelopName(this, mWorldId);
    for (s32 i = 0; i < 8; i++) {
        al::StringTmp<128> texturePath = {"ObjectData/TextureMapLayout%s", worldDevelopName};
        al::StringTmp<128> textureFolder = {"TextureMapLayout%s", worldDevelopName};
        al::StringTmp<128> textureName = {"%s%s", worldDevelopName, sPicImage[i]};
        nn::ui2d::TextureInfo* textureInfo =
            al::createTextureInfo(texturePath.cstr(), textureFolder.cstr(), textureName.cstr());
        al::setPaneTexture(this, sPicImage[i], textureInfo);
    }
}

void MapLayout::reset() {
    mPanelLocalScale.y = 0.4f;
    mPanelLocalScale.x = 0.4f;
    f32 map = mMapTerrainLayout->getPaneSize();
    mPanelSize.x = map * mPanelLocalScale.x;
    mPanelSize.y = map * mPanelLocalScale.y;
    mScrollPosition = sead::Vector2f::zero;
}

// TODO: Move to sead
inline void mul(sead::Vector2f& o, const sead::Matrix44f& m, const sead::Vector3f& a) {
    const sead::Vector3f tmp = a;
    o.x = m.m[0][0] * tmp.x + m.m[0][1] * tmp.y + m.m[0][2] * tmp.z + m.m[0][3];
    o.y = m.m[1][0] * tmp.x + m.m[1][1] * tmp.y + m.m[1][2] * tmp.z + m.m[1][3];
}

void MapLayout::moveFocusLayout(const sead::Vector3f& pos3D, const sead::Vector2f& pos2D) {
    if (!mIsPrintWorldChanged)
        return;

    MapData* mapData = mMapTerrainLayout->getMapData();
    const sead::Matrix44f& mtx = mapData->viewProjMatrix;
    sead::Vector2f proj2;
    mul(proj2, mtx, pos3D);

    f32 scale = mMapTerrainLayout->getPaneSize() * 0.5f;
    sead::Vector2f proj = proj2 * scale;

    bool isYClamped = false;
    if (proj.y > 955.0f) {
        proj.y = 955.0f;
        isYClamped = true;
    } else if (proj.y < -985.0f) {
        proj.y = -985.0f;
        isYClamped = true;
    }

    if (proj.x > 985.0f) {
        proj.x = 985.0f;
        isYClamped = true;
    } else if (proj.x < -955.0f) {
        proj.x = -955.0f;
        isYClamped = true;
    }

    sead::Vector3f direction = sead::Vector3f::zero;
    if (isYClamped) {
        f32 angle = al::calcAngleDegree(sead::Vector2f::ey, sead::Vector2f(-proj.x, -proj.y));
        al::rotateVectorDegreeZ(&direction, modDegree(angle));
    }

    // TODO: Implement -vec2f
    mScrollPosition.x = (-proj.x - pos2D.x) - (direction.x * 2.5f);
    mScrollPosition.y = (-proj.y - pos2D.y) - (direction.y * 2.5f);
}

static const sead::Vector2f sIconLineOffset0 = {50.0f, 50.0f};
static const sead::Vector2f sIconLineOffset1 = {-50.0f, 50.0f};
static const sead::Vector2f sIconLineOffset4 = {50.0f, 0.0f};

inline const sead::Vector2f& getOffset(s32 i) {
    const sead::Vector2f* offset = &sIconLineOffset0;
    if (i != 0) {
        offset = &sIconLineOffset1;
        if (i != 1)
            offset = i == 4 ? &sIconLineOffset4 : &sead::Vector2f::zero;
    }

    return *offset;
}

inline sead::Vector2f calcMapProjection(const sead::Vector3f& position,
                                        const sead::Matrix44f& viewProjMatrix,
                                        const sead::Vector2f& scrollPosition, f32 scale) {
    const f32 projectedX =
        (viewProjMatrix(0, 3) + position.x * viewProjMatrix(0, 0) +
         position.y * viewProjMatrix(0, 1) + position.z * viewProjMatrix(0, 2)) *
        1024.0;
    const f32 projectedY =
        (viewProjMatrix(1, 3) + position.x * viewProjMatrix(1, 0) +
         position.y * viewProjMatrix(1, 1) + position.z * viewProjMatrix(1, 2)) *
        1024.0;

    return {scale * projectedX + scale * scrollPosition.x,
            scale * scrollPosition.y + scale * projectedY};
}

inline bool isMapProjectionInFrame(const sead::Vector3f& position,
                                   const sead::Matrix44f& viewProjMatrix, f32 scale) {
    const sead::Vector2f projected =
        calcMapProjection(position, viewProjMatrix, sead::Vector2f::zero, scale);
    return projected.x >= scale * -955.0f && projected.x <= scale * 985.0f &&
           projected.y >= scale * -985.0f && projected.y <= scale * 955.0f;
}

inline bool isNearMapIconPosition(const MapIconInfo& iconInfo, const sead::Vector3f& position) {
    return (iconInfo.position - position).length() < 10.0f;
}

void MapLayout::updateST() {
    if (!mIsPrintWorldChanged)
        return;

    al::setPaneLocalScale(this, "All", mPanelLocalScale);
    sead::Vector2f localTrans(mScrollPosition.x * mPanelLocalScale.x,
                              mScrollPosition.y * mPanelLocalScale.y);
    al::setPaneLocalTrans(this, "All", localTrans);

    MapData* mapData = mMapTerrainLayout->getMapData();
    if (al::tryGetPlayerActor(mPlayerHolder, 0))
        updatePlayerPosLayout();

    for (s32 i = 0; i < mMapIconInfoSize; i++) {
        if (!mMapIconInfo[i].isActive)
            continue;

        if (mMapIconInfo[i].value) {
            sead::Vector3f projectedPos = sead::Vector3f::zero;
            calcSeaOfTreeIconPos(&projectedPos);
            al::setLocalTrans(mMapIconInfo[i].iconLayout->layout, projectedPos);
        } else {
            sead::Vector2f projectedPos;
            rs::calcTransOnMap(&projectedPos, mMapIconInfo[i].position, mapData->viewProjMatrix,
                               mScrollPosition, mPanelLocalScale.x, 2048.0f);
            projectedPos.x += mMapIconInfo[i].position2.x * mPanelLocalScale.x;
            projectedPos.y = mMapIconInfo[i].position2.y * mPanelLocalScale.x + projectedPos.y;
            al::setLocalTrans(mMapIconInfo[i].iconLayout->layout, projectedPos);
        }
    }

    setPanelFont(mPanelLocalScale.x, &mCurrentFontType, this, mCurrentFontType);

    s32 size = mArray.size();
    for (s32 i = 0; i < size; i++)
        if (al::isActive(mArray[i]))
            updateIconLine(mArray[i], mAAA, getOffset(i));
}

HintAmiibo& getHintAmiibo(s32 i, u32 maxSize, HintAmiibo* mHintAmiibo) {
    if (maxSize > (u32)i)
        return mHintAmiibo[i];
    return mHintAmiibo[0];
}

void MapLayout::addAmiiboHint() {
    getHintAmiibo(mHintDecideIconAmiiboSize, mHintAmiiboSizer, mHintAmiibo).position =
        GameDataFunction::getLatestHintTrans(this);
    getHintAmiibo(mHintDecideIconAmiiboSize, mHintAmiiboSizer, mHintAmiibo).isValid =
        GameDataFunction::checkLatestHintSeaOfTree(this);
    mHintDecideIconAmiiboSize++;
}

void MapLayout::appearAmiiboHint() {
    if (-1 < mWorldId) {
        s32 size = mMapIconInfoSize;
        MapIconInfo* iconInfo = mMapIconInfo;
        for (s32 i = 0; i < size; i++) {
            iconInfo->isActive = false;
            iconInfo->iconType = IconType::MaxValue;
            iconInfo->position = sead::Vector3f::zero;
            iconInfo->action = 0;
            iconInfo->position2 = sead::Vector2f::zero;
            iconInfo->value = false;
            iconInfo++;
        }
        f32 map = mMapTerrainLayout->getPaneSize();
        reset();
        mPanelSize = {map * 0.4f, map * 0.4f};
        mPanelLocalScale.y = StageMapFunction::getStageMapScaleMin();
        mPanelLocalScale.x = StageMapFunction::getStageMapScaleMin();
        updateST();
        al::LayoutActor::appear();
        al::setNerve(this, &NrvMapLayout.HintInitWaitAmiibo);
        al::LayoutActor* layout = this;
        al::startAction(layout, "Appear", nullptr);
        al::startAction(layout, "LeftIn", "Change");
        al::setActionFrame(layout, al::getActionFrameMax(layout, "LeftIn", "Change"), "Change");
        mWaitEndMapBg->appear();
        MapData* mapData = mMapTerrainLayout->getMapData();
        mIsFreezeAction = trySetBalloon(mWaitEndMapPlayer, al::getPlayerActor(mPlayerHolder, 0),
                                        mapData->viewProjMatrix);
        updatePlayerPosLayout();
        mWaitEndMapPlayer->startWait();
        mIsResetTransform = true;
        return;
    }
    end();
}

void MapLayout::end() {
    al::setNerve(this, &NrvMapLayout.End);
}

bool trySetBalloon(al::LayoutActor* layoutActor, const al::LiveActor* actor,
                   const sead::Matrix44f& matrix) {
    sead::Vector3f outsidePosition = al::getTrans(actor);
    const sead::Vector4f row1 = matrix.getRow(1);
    const sead::Vector4f row0 = matrix.getRow(0);
    const sead::Vector2f& zeroRef = sead::Vector2f::zero;
    const sead::Vector2f zero = zeroRef;

    const sead::Vector3f checkPosition = al::getTrans(actor);
    const f64 checkPosX = checkPosition.x;
    const f64 checkPosY = checkPosition.y;
    const f64 checkPosZ = checkPosition.z;
    const f64 checkM00 = matrix(0, 0);
    const f64 checkM01 = matrix(0, 1);
    const f64 checkM02 = matrix(0, 2);
    const f64 checkM03 = matrix(0, 3);
    const f64 checkX =
        (checkPosX * checkM00 + checkPosY * checkM01 + checkPosZ * checkM02 + checkM03) *
        1024.0;
    const f32 projectedX = zeroRef.x * 0.4f + checkX * 0.4000000059604645;
    if (projectedX >= -382.0f && projectedX <= 394.0f) {
        const f64 checkM10 = matrix(1, 0);
        const f64 checkM11 = matrix(1, 1);
        const f64 checkM12 = matrix(1, 2);
        const f64 checkM13 = matrix(1, 3);
        const f64 checkY =
            (checkPosX * checkM10 + checkPosY * checkM11 + checkPosZ * checkM12 + checkM13) *
            1024.0;
        const f32 projectedY = zeroRef.y * 0.4f + checkY * 0.4000000059604645;
        if (projectedY >= -394.0f && projectedY <= 382.0f)
            return false;
    }

    const f64 outsidePosX = outsidePosition.x;
    const f64 outsidePosY = outsidePosition.y;
    const f64 outsidePosZ = outsidePosition.z;
    const f64 m00D = row0.x;
    const f64 m01D = row0.y;
    const f64 m02D = row0.z;
    const f64 m03D = row0.w;
    const f64 m10D = row1.x;
    const f64 m11D = row1.y;
    const f64 m12D = row1.z;
    const f64 m13D = row1.w;
    const f64 outsideX =
        (outsidePosX * m00D + outsidePosY * m01D + outsidePosZ * m02D + m03D) * 1024.0;
    const f64 outsideY =
        (outsidePosX * m10D + outsidePosY * m11D + outsidePosZ * m12D + m13D) * 1024.0;
    const f32 unclampedX = zero.x * 0.4f + outsideX * 0.4000000059604645;
    const f32 unclampedY = zero.y * 0.4f + outsideY * 0.4000000059604645;

    f32 clampedX = -382.0f;
    if (unclampedX >= -382.0f) {
        clampedX = unclampedX;
        if (unclampedX > 394.0f)
            clampedX = 394.0f;
    }

    f32 clampedY = -394.0f;
    if (unclampedY >= -394.0f) {
        clampedY = unclampedY;
        if (unclampedY > 382.0f)
            clampedY = 382.0f;
    }

    al::startFreezeAction(layoutActor, "Outside", 0.0f, "Icon");

    sead::Vector2f balloonDir;
    balloonDir.x = -(clampedX * 0.4f);
    balloonDir.y = -(clampedY * 0.4f);
    f32 angle = modDegree(al::calcAngleDegree(sead::Vector2f::ey, balloonDir));
    f32 frameMax = al::getActionFrameMax(layoutActor, "State", "Balloon");
    al::startFreezeAction(layoutActor, "State", angle * (frameMax / 360.0f), "Balloon");
    return true;
}

void MapLayout::updatePlayerPosLayout() {
    al::LiveActor* player = al::getPlayerActor(mPlayerHolder, 0);
    f32 localScale = mPanelLocalScale.x;
    MapData* mapData = mMapTerrainLayout->getMapData();
    bool freeze = mIsFreezeAction;
    bool bVar10;
    bool bVar11;

    if (GameDataFunction::isSeaOfTreeStage(this)) {
        sead::Vector3f position;
        calcSeaOfTreeIconPos(&position);
        al::startFreezeAction(mWaitEndMapPlayer, "Stay", 0.0f, "Icon");
        al::setLocalTrans(mWaitEndMapPlayer, position);
        return;
    }

    sead::Vector2f position = sead::Vector2f::zero;
    sead::Vector3f playerPos = al::getTrans(player);

    if (!GameDataFunction::isMainStage(player))
        playerPos = GameDataFunction::getStageMapPlayerPos(this);

    al::AreaObj* areaObj = al::tryFindAreaObj(player, "InvalidateStageMapArea", playerPos);
    if (areaObj == nullptr) {
        position.x =
            localScale *
                (mapData->viewProjMatrix(3, 0) + playerPos.x * mapData->viewProjMatrix(0, 0) +
                 playerPos.y * mapData->viewProjMatrix(1, 0) +
                 playerPos.z * mapData->viewProjMatrix(2, 0)) *
                1024.0f +
            localScale * mScrollPosition.x;
        position.y =
            localScale * mScrollPosition.y +
            localScale *
                (mapData->viewProjMatrix(3, 1) + playerPos.x * mapData->viewProjMatrix(0, 1) +
                 playerPos.y * mapData->viewProjMatrix(1, 1) +
                 playerPos.z * mapData->viewProjMatrix(2, 1)) *
                1024.0f;
        if (!freeze) {
            bVar10 = false;
            bVar11 = false;
        } else {
            f32 fVar23 =
                localScale *
                    (mapData->viewProjMatrix(3, 0) + playerPos.x * mapData->viewProjMatrix(0, 0) +
                     playerPos.y * mapData->viewProjMatrix(1, 0) +
                     playerPos.z * mapData->viewProjMatrix(2, 0)) *
                    1024.0f +
                localScale * 0.0f;
            f32 fVar21 = localScale * 0.0f + localScale *
                                                 (mapData->viewProjMatrix(3, 1) +
                                                  playerPos.x * mapData->viewProjMatrix(0, 1) +
                                                  playerPos.y * mapData->viewProjMatrix(1, 1) +
                                                  playerPos.z * mapData->viewProjMatrix(2, 1)) *
                                                 1024.0f;
            f32 fVar22 = localScale * 985.0f;
            f32 fVar24 = localScale * 955.0f;
            if ((fVar24 < fVar21) || (fVar24 = localScale * -985.0f, fVar21 < fVar24))
                position.y = position.y - (fVar21 - fVar24);
            if ((fVar22 < fVar23) || (fVar22 = localScale * -955.0f, fVar23 < fVar22))
                position.x = position.x - (fVar23 - fVar22);
            bVar11 = false;
            bVar10 = true;
        }
    } else {
        sead::Vector3f pospos = sead::Vector3f::zero;
        al::getLinksQT(nullptr, &pospos, *areaObj->getPlacementInfo(), "PlayerPoint");
        position.x = localScale *
                         (mapData->viewProjMatrix(3, 0) + pospos.x * mapData->viewProjMatrix(0, 0) +
                          pospos.y * mapData->viewProjMatrix(1, 0) +
                          pospos.z * mapData->viewProjMatrix(2, 0)) *
                         1024.0f +
                     localScale * mScrollPosition.x;
        position.y = localScale * mScrollPosition.y +
                     localScale *
                         (mapData->viewProjMatrix(3, 1) + pospos.x * mapData->viewProjMatrix(0, 1) +
                          pospos.y * mapData->viewProjMatrix(1, 1) +
                          pospos.z * mapData->viewProjMatrix(2, 1)) *
                         1024.0f;
        al::startFreezeAction(mWaitEndMapPlayer, "Stay", 0.0f, "Icon");
        bVar10 = false;
        bVar11 = true;
    }
    al::setLocalTrans(mWaitEndMapPlayer, position);
    al::setLocalScale(mWaitEndMapPlayer, StageMapFunction::getStageMapScaleMax());

    sead::Vector3f pospos = sead::Vector3f::zero;
    rs::tryCalcMapNorthDir(&pospos, mWaitEndMapPlayer);

    sead::Vector3f plapplap = {0.0f, 0.0f, 0.0f};
    f32 angle;
    if (GameDataFunction::isMainStage(this)) {
        rs::calcPlayerFrontDir(&plapplap, player);
        angle = modDegree(al::calcAngleOnPlaneDegree(pospos, plapplap, sead::Vector3f::ey));
    } else {
        angle = 0.0f;
        al::startFreezeAction(mWaitEndMapPlayer, "Stay", 0.0f, "Icon");
        bVar11 = true;
    }

    al::startFreezeAction(
        mWaitEndMapPlayer, "State",
        angle * (al::getActionFrameMax(mWaitEndMapPlayer, "State", "Direct") / 360.0f), "Direct");

    sead::Vector3f cameraDif =
        al::getCameraAt(mWaitEndMapPlayer, 0) - al::getCameraPos(mWaitEndMapPlayer, 0);
    f32 popo = modDegree(al::calcAngleOnPlaneDegree(pospos, cameraDif, sead::Vector3f::ey));
    f32 max = al::getActionFrameMax(mWaitEndMapPlayer, "State", "View");
    al::startFreezeAction(mWaitEndMapPlayer, "State", popo * (max / 360.0f), "View");

    if (!bVar10 && !bVar11)
        al::startFreezeAction(mWaitEndMapPlayer, "Normal", 0.0f, "Icon");
}

void MapLayout::appearWithHint() {
    if (-1 < mWorldId) {
        s32 size = mMapIconInfoSize;
        MapIconInfo* iconInfo = mMapIconInfo;
        for (s32 i = 0; i < size; i++) {
            iconInfo->isActive = false;
            iconInfo->iconType = IconType::MaxValue;
            iconInfo->position = sead::Vector3f::zero;
            iconInfo->action = 0;
            iconInfo->position2 = sead::Vector2f::zero;
            iconInfo->value = false;
            iconInfo++;
        }
        f32 map = mMapTerrainLayout->getPaneSize();
        reset();
        mPanelSize = {map * 0.4f, map * 0.4f};
        mPanelLocalScale.y = StageMapFunction::getStageMapScaleMin();
        mPanelLocalScale.x = StageMapFunction::getStageMapScaleMin();
        updateST();
        al::LayoutActor::appear();
        al::setNerve(this, &NrvMapLayout.HintInitWaitNpc);
        al::LayoutActor* layout = this;
        al::startAction(layout, "Appear", nullptr);
        al::startAction(layout, "LeftIn", "Change");
        al::setActionFrame(layout, al::getActionFrameMax(layout, "LeftIn", "Change"), "Change");
        mWaitEndMapBg->appear();
        MapData* mapData = mMapTerrainLayout->getMapData();
        mIsFreezeAction = trySetBalloon(mWaitEndMapPlayer, al::getPlayerActor(mPlayerHolder, 0),
                                        mapData->viewProjMatrix);
        updatePlayerPosLayout();
        mWaitEndMapPlayer->startWait();
        mIsResetTransform = true;
        return;
    }
    end();
}

void MapLayout::appearMoonRockDemo(s32 sworldId) {
    s32 size = mMapIconInfoSize;
    MapIconInfo* iconInfo = mMapIconInfo;
    for (s32 i = 0; i < size; i++) {
        iconInfo->isActive = false;
        iconInfo->iconType = IconType::MaxValue;
        iconInfo->position = sead::Vector3f::zero;
        iconInfo->action = 0;
        iconInfo->position2 = sead::Vector2f::zero;
        iconInfo->value = false;
        iconInfo++;
    }
    changePrintWorld(sworldId);
    f32 map = mMapTerrainLayout->getPaneSize();
    reset();
    mPanelLocalScale.y = StageMapFunction::getStageMapScaleMin();
    mPanelLocalScale.x = StageMapFunction::getStageMapScaleMin();
    mPanelSize = {map * StageMapFunction::getStageMapScaleMin(),
                  map * StageMapFunction::getStageMapScaleMin()};
    updateST();
    al::LayoutActor::appear();
    al::setNerve(this, &NrvMapLayout.HintInitWaitMoonRock);
    al::LayoutActor* layout = this;
    al::startAction(layout, "Appear", nullptr);
    al::startAction(layout, "LeftIn", "Change");
    al::setActionFrame(layout, al::getActionFrameMax(layout, "LeftIn", "Change"), "Change");
    mWaitEndMapBg->appear();
    MapData* mapData = mMapTerrainLayout->getMapData();
    mIsFreezeAction = trySetBalloon(mWaitEndMapPlayer, al::getPlayerActor(mPlayerHolder, 0),
                                    mapData->viewProjMatrix);
    updatePlayerPosLayout();
    mWaitEndMapPlayer->startWait();
    mIsResetTransform = true;
    return;
}

void MapLayout::appearCollectionList() {
    mIsHelp = false;
    appear();
}

bool MapLayout::isEnd() const {
    return !isAlive();
}

bool MapLayout::isEnableCheckpointWarp() const {
    al::LiveActor* actor = al::tryGetPlayerActor(mPlayerHolder, 0);
    if (actor != nullptr) {
        if (!GameDataFunction::isInInvalidOpenMapStage(actor) && rs::isEnableOpenMap(actor) &&
            !GameDataFunction::isRaceStartYukimaru(actor) && GameDataFunction::isMeetCap(actor) &&
            !GameDataFunction::isRemovedCapByJango(actor)) {
            return !rs::isSceneStatusBossBattle(actor);
        }
    }
    return false;
}

void MapLayout::changeOut(bool isLeftOut) {
    al::startAction(this, isLeftOut ? "LeftOut" : "RightOut", "Change");
    al::startAction(mWaitEndMapCursor, "ChangeOut", nullptr);
    mWaitEndMapPlayer->kill();
    mWaitEndMapGuide->kill();
    s32 size = mArray.size();
    for (s32 i = 0; i < size; i++)
        mArray[i]->kill();
    for (s32 i = 0; i < mMapIconInfoSize; i++)
        if (mMapIconInfo[i].isActive && al::killLayoutIfActive(mMapIconInfo[i].iconLayout->layout))
            mMapIconInfo[i].isActive = false;
    al::setNerve(this, &ChangeOut);
}

void MapLayout::changeIn(bool isRightIn) {
    appearCollectionList();
    al::startAction(this, "Wait", nullptr);
    mWaitEndMapCursor->appear();
    mIsHelp = false;
    al::startAction(mWaitEndMapCursor, "ChangeIn", nullptr);
    al::startAction(this, isRightIn ? "RightIn" : "LeftIn", "Change");
    al::setNerve(this, &NrvMapLayout.ChangeIn);
}

void MapLayout::updateLine(al::LayoutActor* layoutActor) {
    al::Matrix43f panel = al::getPaneMtx(mWaitEndMapCursor, "LinePos");
    sead::Vector3f panelV = {panel.m[0][3], panel.m[1][3], panel.m[2][3]};

    al::Matrix43f panel2 = al::getPaneMtx(layoutActor, "LinePos");
    sead::Vector3f panelV2 = {panel2.m[0][3], panel2.m[1][3], panel2.m[2][3]};
    sead::Vector3f panelV3 = panelV2 - panelV;

    f32 angle = 0.0f;
    if (!al::isNearZero(panelV3.length(), 0.001f)) {
        sead::Vector3f direction = panelV3;
        angle = al::calcAngleOnPlaneDegree(direction, -sead::Vector3f::ex, sead::Vector3f::ez);
        angle = (360.0f - modDegree(angle)) * (3.2f / 9.0f);
    }

    al::startFreezeAction(mWaitEndMapLine, "State", angle, "Direct");

    f32 size = (panelV - panelV2).length() / 20.0f;
    al::startFreezeAction(mWaitEndMapLine, "State", size, "Length");
    sead::Vector3f newPosition = (panelV + panelV2) * 0.5f;
    al::setLocalTrans(mWaitEndMapLine, newPosition);
}

void MapLayout::appearParts(bool withMoonRockHints) {
    if (!mIsPrintWorldChanged)
        return;

    MapData* mapData = mMapTerrainLayout->getMapData();
    al::LiveActor* playerActor = al::tryGetPlayerActor(mPlayerHolder, 0);
    if (playerActor != nullptr) {
        mIsFreezeAction = trySetBalloon(mWaitEndMapPlayer, playerActor, mapData->viewProjMatrix);
        updatePlayerPosLayout();
    }
    mWaitEndMapPlayer->startWait();

    if (GameDataFunction::isExistRaceStartNpc(this) && GameDataFunction::isRaceStart(this)) {
        calcMapTransAndAppear(mRaceStartIconLayout, mMapIconInfo,
                              GameDataFunction::getRaceStartTrans(this), IconType::Start, false);
        calcMapTransAndAppear(mRaceGoalIconLayout, mMapIconInfo,
                              GameDataFunction::getRaceGoalTrans(this), IconType::Goal, false);
    }
    if (GameDataFunction::isTimeBalloonSequence(this)) {
        calcMapTransAndAppear(mRaceStartIconLayout, mMapIconInfo,
                              GameDataFunction::getTimeBalloonNpcTrans(this), IconType::Start,
                              false);
    }
    if (GameDataFunction::isRaceStart(this) || GameDataFunction::isTimeBalloonSequence(this))
        return;

    bool enableCheckpointWarp = isEnableCheckpointWarp();
    for (s32 i = 0; i < mCheckpointNumMaxInWorld; i++) {
        if (GameDataFunction::isGotCheckpointInWorld(this, i)) {
            calcMapTransAndAppear(&mCheckpointIconLayouts[i], mMapIconInfo,
                                  GameDataFunction::getCheckpointTransInWorld(this, i),
                                  enableCheckpointWarp ? IconType::Flag : IconType::FlagDisable,
                                  false);
        }
    }

    if (GameDataFunction::isExistHome(this)) {
        calcMapTransAndAppear(mHomeIconLayout, mMapIconInfo, GameDataFunction::getHomeTrans(this),
                              enableCheckpointWarp ? IconType::Home : IconType::HomeDisable,
                              false);
    } else if (rs::isSequenceGoToNextWorld(this) &&
               GameDataFunction::isUnlockedCurrentWorld(this)) {
        calcMapTransAndAppear(mHomeIconLayout, mMapIconInfo, GameDataFunction::getHomeTrans(this),
                              IconType::HomeDisable, false);
    }

    s32 hintNum = GameDataFunction::calcHintNum(this);
    MapIconLayout* seaOfTreeHintLayout = nullptr;
    for (s32 i = 0; i < hintNum; i++) {
        const sead::Vector3f& hintPosition = GameDataFunction::calcHintTrans(this, i);
        if (!isMapProjectionInFrame(hintPosition, mapData->viewProjMatrix, mPanelLocalScale.x))
            continue;

        if (GameDataFunction::checkHintSeaOfTree(this, i)) {
            if (seaOfTreeHintLayout != nullptr) {
                mMapIconInfo[seaOfTreeHintLayout->fieldA].action++;
            } else {
                sead::Vector3f seaOfTreePosition = sead::Vector3f::zero;
                calcSeaOfTreeIconPos(&seaOfTreePosition);
                seaOfTreeHintLayout = &mHintIconLayouts[i];
                setLocalTransAndAppear(seaOfTreeHintLayout, mMapIconInfo, seaOfTreePosition,
                                       IconType::Hint1, true);
            }
        } else {
            bool isMerged = false;
            for (s32 j = 0; j < i; j++) {
                MapIconInfo& info = mMapIconInfo[mHintIconLayouts[j].fieldA];
                if (isNearMapIconPosition(info, hintPosition)) {
                    info.action++;
                    isMerged = true;
                    break;
                }
            }
            if (!isMerged) {
                calcMapTransAndAppear(&mHintIconLayouts[i], mMapIconInfo, hintPosition,
                                      IconType::Hint1, false);
            }
        }
    }

    mCalcShopNum = GameDataFunction::calcShopNum(this);
    for (s32 i = 0; i < mCalcShopNum; i++) {
        calcMapTransAndAppear(&mShopIconLayouts[i], mMapIconInfo,
                              GameDataFunction::getShopNpcTrans(this, i),
                              GameDataFunction::isShopSellout(this, i) ? IconType::ShopSold :
                                                                         IconType::Shop,
                              true);
    }

    mMiniGameNumMax = GameDataFunction::getMiniGameNum(this);
    for (s32 i = 0; i < mMiniGameNumMax; i++) {
        calcMapTransAndAppear(&mMiniGameIconLayouts[i], mMapIconInfo,
                              GameDataFunction::getMiniGameTrans(this, i), IconType::Race,
                              al::isEqualString(GameDataFunction::getMiniGameName(this, i),
                                                "TitleYukimaruRace"));
    }

    if (GameDataFunction::isExistJango(this)) {
        calcMapTransAndAppear(mJangoIconLayout, mMapIconInfo, GameDataFunction::getJangoTrans(this),
                              IconType::Cap, false);
    }
    if (GameDataFunction::isExistTimeBalloonNpc(this)) {
        calcMapTransAndAppear(mTimeBalloonIconLayout, mMapIconInfo,
                              GameDataFunction::getTimeBalloonNpcTrans(this), IconType::Luigi,
                              false);
    }
    if (GameDataFunction::isExistPoetter(this)) {
        calcMapTransAndAppear(mPoetterIconLayout, mMapIconInfo,
                              GameDataFunction::getPoetterTrans(this), IconType::Poet, true);
    }

    if (rs::isSequenceGoToMoonRock(this)) {
        calcMapTransAndAppear(&mScenarioIconLayouts[0], mMapIconInfo,
                              GameDataFunction::getMoonRockTrans(this), IconType::Scenario1,
                              false);
        mWaitEndMapLine->appear();
        updateLine(mScenarioIconLayouts[0].layout);
    }

    if (GameDataFunction::isExistJango(this)) {
        calcMapTransAndAppear(&mScenarioIconLayouts[0], mMapIconInfo,
                              GameDataFunction::getJangoTrans(this), IconType::Scenario1, false);
        mWaitEndMapLine->appear();
        updateLine(mScenarioIconLayouts[0].layout);
    } else if (rs::isSequenceGoToNextWorld(this) &&
               GameDataFunction::isUnlockedCurrentWorld(this)) {
        calcMapTransAndAppear(&mScenarioIconLayouts[0], mMapIconInfo,
                              GameDataFunction::getHomeTrans(this), IconType::ScenarioHome, false);
        mWaitEndMapLine->appear();
        updateLine(mScenarioIconLayouts[0].layout);
    } else {
        s32 questNum = rs::getActiveQuestNumForMap(this);
        const QuestInfo* const* questList = rs::getActiveQuestList(this);
        for (s32 i = 0; i < questNum; i++) {
            const QuestInfo* questInfo = questList[i];
            if (rs::getActiveQuestNo(this) != 0 ||
                !al::isEqualString(
                    "Sea",
                    GameDataFunction::getWorldDevelopName(
                        this, GameDataFunction::getCurrentWorldId(this))) ||
                questInfo->getMapLabel().getStringTop()[0] == sead::SafeString::cNullChar) {
                calcMapTransAndAppear(&mScenarioIconLayouts[i], mMapIconInfo, questInfo->getTrans(),
                                      IconType::Scenario1, false);
            }
        }
        if (questNum == 1) {
            mWaitEndMapLine->appear();
            updateLine(mScenarioIconLayouts[0].layout);
        }
    }

    if (withMoonRockHints) {
        s32 moonRockHintNum = GameDataFunction::calcHintMoonRockNum(this);
        for (s32 i = 0; i < moonRockHintNum; i++) {
            const sead::Vector3f& rockPosition = GameDataFunction::calcHintMoonRockTrans(this, i);
            if (!isMapProjectionInFrame(rockPosition, mapData->viewProjMatrix, mPanelLocalScale.x))
                continue;

            bool isMerged = false;
            for (s32 j = 0; j < i; j++) {
                MapIconInfo& info = mMapIconInfo[mMoonRockIconLayouts[j].fieldA];
                if (isNearMapIconPosition(info, rockPosition)) {
                    info.action++;
                    isMerged = true;
                    break;
                }
            }
            if (!isMerged) {
                calcMapTransAndAppear(&mMoonRockIconLayouts[i], mMapIconInfo, rockPosition,
                                      IconType::HintRock1, false);
            }
        }
    }

    for (s32 i = 0; i < mMapIconInfoSize; i++) {
        if (mMapIconInfo[i].isActive)
            al::startAction(mMapIconInfo[i].iconLayout->layout, "Wait", "Main");
    }
    updateST();
}

void MapLayout::startNumberAction() {
    for (s32 i = 0; i < mMapIconInfoSize; i++) {
        if (!mMapIconInfo[i].isActive)
            continue;
        if (mMapIconInfo[i].action == 1) {
            al::startAction(mMapIconInfo[i].iconLayout->layout, "Off", "OnOff");
        } else {
            al::startAction(mMapIconInfo[i].iconLayout->layout, "On", "OnOff");
            al::WStringTmp<32> iconType = {u"%d", mMapIconInfo[i].action};
            al::setPaneString(mMapIconInfo[i].iconLayout->layout, "TxtNumber", iconType.cstr(), 0);
        }
    }
}

void MapLayout::calcSeaOfTreeIconPos(sead::Vector3f* position) {
    *position = al::getPaneLocalTrans(mMapTerrainLayout, "UnclearPos");
    position->x *= mPanelLocalScale.x;
    position->y *= mPanelLocalScale.y;
    position->x += mScrollPosition.x * mPanelLocalScale.x;
    position->y += mScrollPosition.y * mPanelLocalScale.y;
}

const char* getIconName(const IconType iconType) {
    if (iconType < IconType::MaxValue)
        return sIconType[(s32)iconType];
    return "";
}

void MapLayout::setLocalTransAndAppear(MapIconLayout* iconLayout, MapIconInfo* iconInfo,
                                       const sead::Vector3f& position, IconType iconType,
                                       bool isbool) {
    al::LayoutActor* layout = iconLayout->layout;
    if (iconType == 0xe || iconType == 0x5)
        al::startAction(layout, "HintNew", "Main");

    al::setLocalTrans(iconLayout->layout, position);
    al::startAction(iconLayout->layout, getIconName(iconType), "Icon");
    iconInfo[iconLayout->fieldA].isActive = true;
    iconInfo[iconLayout->fieldA].position = sead::Vector3f::zero;
    iconInfo[iconLayout->fieldA].iconType = iconType;
    iconInfo[iconLayout->fieldA].action = 1;
    iconInfo[iconLayout->fieldA].position2 = sead::Vector2f::zero;
    iconInfo[iconLayout->fieldA].value = isbool;
    iconLayout->layout->appear();
}

void MapLayout::calcMapTransAndAppear(MapIconLayout* iconLayout, MapIconInfo* iconInfo,
                                      const sead::Vector3f& position, IconType iconType,
                                      bool isSpecialLineIcon) {
    al::LayoutActor* layout = iconLayout->layout;
    if (iconType == IconType::HintRock2 || iconType == IconType::Hint2)
        al::startAction(layout, "HintNew", "Main");

    MapData* mapData = mMapTerrainLayout->getMapData();
    const f32 scale = mPanelLocalScale.x;
    sead::Vector2f projected =
        calcMapProjection(position, mapData->viewProjMatrix, mScrollPosition, scale);
    sead::Vector2f offset = sead::Vector2f::zero;

    if (GameDataFunction::isWorldSnow(this) && isSpecialLineIcon) {
        mAAA = position;
        if (iconType == IconType::Race) {
            offset = {-50.0f, 50.0f};
            if (mArray.size() >= 2) {
                mArray[1]->appear();
                al::startAction(mArray[1], "Appear", nullptr);
            }
        } else if (iconType == IconType::Shop || iconType == IconType::ShopSold) {
            offset = {50.0f, 50.0f};
            if (mArray.size() >= 1) {
                mArray[0]->appear();
                al::startAction(mArray[0], "Appear", nullptr);
            }
        } else if (iconType == IconType::Poet) {
            offset = {50.0f, 0.0f};
            if (mArray.size() >= 5) {
                mArray[4]->appear();
                al::startAction(mArray[4], "Appear", nullptr);
            }
        }
    }

    projected.x += offset.x * scale;
    projected.y += offset.y * scale;
    al::setLocalTrans(layout, projected);
    al::startAction(layout, getIconName(iconType), "Icon");

    iconInfo[iconLayout->fieldA].isActive = true;
    MapIconInfo& info = iconInfo[iconLayout->fieldA];
    info.position.x = position.x;
    info.position.y = position.y;
    info.position.z = position.z;
    info.iconType = iconType;
    info.action = 1;
    info.position2.x = offset.x;
    info.position2.y = offset.y;
    layout->appear();
}

void MapLayout::scroll(const sead::Vector2f& scrollDistance) {
    if (!mIsPrintWorldChanged)
        return;

    f32 scale = scrollDistance.length() / mPanelLocalScale.x;
    mScrollPosition += scrollDistance * scale;

    mScrollPosition.x =
        sead::Mathf::clamp(mScrollPosition.x, mMinScrollPosition.x, mMaxScrollPosition.x);
    mScrollPosition.y =
        sead::Mathf::clamp(mScrollPosition.y, mMinScrollPosition.y, mMaxScrollPosition.y);
}

void MapLayout::addSize(const sead::Vector2f& size) {
    mPanelSize += size;

    f32 prevLocalScaleX = mPanelLocalScale.x;
    f32 maxSize = mMapTerrainLayout->getPaneSize();
    f32 minSize = maxSize * StageMapFunction::getStageMapScaleMin();

    mPanelSize.x = sead::Mathf::clamp(mPanelSize.x, minSize, maxSize);
    mPanelSize.y = sead::Mathf::clamp(mPanelSize.y, minSize, maxSize);
    mPanelLocalScale.x = (StageMapFunction::getStageMapScaleMax() / maxSize) * mPanelSize.x;
    mPanelLocalScale.y = (StageMapFunction::getStageMapScaleMax() / maxSize) * mPanelSize.y;
    mPanelLocalScale.x =
        sead::Mathf::clamp(mPanelLocalScale.x, StageMapFunction::getStageMapScaleMin(),
                           StageMapFunction::getStageMapScaleMax());
    mPanelLocalScale.y =
        sead::Mathf::clamp(mPanelLocalScale.y, StageMapFunction::getStageMapScaleMin(),
                           StageMapFunction::getStageMapScaleMax());

    if (!al::isNearZero(mPanelLocalScale.x - prevLocalScaleX, 0.001f))
        al::holdSeWithParam(this, "Zoom", mPanelLocalScale.x, "スケール");

    setPanelFont(mPanelLocalScale.x, &mCurrentFontType, this, mCurrentFontType);
    updateST();
}

void setPanelFont(f32 scale, s32* newType, al::LayoutActor* layoutActor, s32 currentType) {
    const char* fontName;
    if (0.45f > scale) {
        if (currentType == 2)
            return;
        *newType = 2;
        fontName = "nintendo_udsg-r_std_003_10.fcpx";
    } else if (0.7f > scale) {
        if (currentType == 1)
            return;
        *newType = 1;
        fontName = "nintendo_udsg-r_std_003_40.fcpx";
    } else {
        if (currentType == 0)
            return;
        *newType = 0;
        fontName = "nintendo_udsg-r_std_003_80.fcpx";
    }
    al::setTextBoxPaneFont(layoutActor, "TxtCaption00", fontName);
    al::setTextBoxPaneFont(layoutActor, "TxtRegionCaption", fontName);
    al::setTextBoxPaneFont(layoutActor, "TxtData01", fontName);
    al::setTextBoxPaneFont(layoutActor, "TxtData02", fontName);
    al::setTextBoxPaneFont(layoutActor, "TxtTitle01", fontName);
    al::setTextBoxPaneFont(layoutActor, "TxtContents01", fontName);
    al::setTextBoxPaneFont(layoutActor, "TxtCaption01", fontName);
    al::setTextBoxPaneFont(layoutActor, "TxtTitle02", fontName);
    al::setTextBoxPaneFont(layoutActor, "TxtContents02", fontName);
    al::setTextBoxPaneFont(layoutActor, "TxtCaption02", fontName);
    al::setTextBoxPaneFont(layoutActor, "TxtTitle03", fontName);
    al::setTextBoxPaneFont(layoutActor, "TxtContents03", fontName);
    al::setTextBoxPaneFont(layoutActor, "TxtCaption03", fontName);
    al::setTextBoxPaneFont(layoutActor, "TxtTitle04", fontName);
    al::setTextBoxPaneFont(layoutActor, "TxtContents04", fontName);
    al::setTextBoxPaneFont(layoutActor, "TxtCaption04", fontName);
    al::setTextBoxPaneFont(layoutActor, "TxtTitle05", fontName);
    al::setTextBoxPaneFont(layoutActor, "TxtContents05", fontName);
    al::setTextBoxPaneFont(layoutActor, "TxtCaption05", fontName);
    al::setTextBoxPaneFont(layoutActor, "TxtCaption06", fontName);
    al::setTextBoxPaneFont(layoutActor, "TxtContents07", fontName);
    al::setTextBoxPaneFont(layoutActor, "TxtTitle07", fontName);
    al::setTextBoxPaneFont(layoutActor, "TxtElement01", fontName);
    al::setTextBoxPaneFont(layoutActor, "TxtElement02", fontName);
    al::setTextBoxPaneFont(layoutActor, "TxtIcon", fontName);
}

bool MapLayout::isAppear() const {
    return al::isNerve(this, &NrvMapLayout.Appear);
}

const sead::Matrix44f& MapLayout::getViewProjMtx() const {
    return mMapTerrainLayout->getMapData()->viewProjMatrix;
}

const sead::Matrix44f& MapLayout::getProjMtx() const {
    return mMapTerrainLayout->getMapData()->projMatrix;
}

void MapLayout::updateIconLine(al::LayoutActor* layoutActor, const sead::Vector3f& vector3,
                               const sead::Vector2f& vector2) {
    MapData* mapData = mMapTerrainLayout->getMapData();
    f32 scale = mPanelLocalScale.x;

    sead::Vector2f panelV;
    {
        const sead::Vector3f tmp = vector3;
        const sead::Matrix44f& m = mapData->viewProjMatrix;

        f64 d_m00 = m.m[0][0];
        f64 d_m01 = m.m[0][1];
        f64 d_m02 = m.m[0][2];
        f64 d_m03 = m.m[0][3];

        f64 d_m10 = m.m[1][0];
        f64 d_m11 = m.m[1][1];
        f64 d_m12 = m.m[1][2];
        f64 d_m13 = m.m[1][3];

        f64 d_v1x = (f64)tmp.x;
        f64 d_v1y = (f64)tmp.y;
        f64 d_v1z = (f64)tmp.z;

        f64 accx = (d_v1x * d_m00 + d_v1y * d_m01 + d_v1z * d_m02) + d_m03;
        f64 accy = (d_v1x * d_m10 + d_v1y * d_m11 + d_v1z * d_m12) + d_m13;
        sead::Vector2<f64> scaled = {accx, accy};
        scaled *= 2048.0f * 0.5;
        scaled *= scale;

        f64 d_v2x = mScrollPosition.x * scale + scaled.x;
        f64 d_v2y = mScrollPosition.y * scale + scaled.y;

        panelV.set(d_v2x, d_v2y);
    }

    sead::Vector2f panelV2 = {scale * vector2.x + panelV.x, scale * vector2.y + panelV.y};

    f32 angle = al::calcAngleDegree(panelV2 - panelV, sead::Vector2f::ex);
    angle = (360.0f - modDegree(angle)) * (3.2f / 9.0f);
    al::startFreezeAction(layoutActor, "State", angle, "Direct");

    f32 size = (panelV - panelV2).length() / 20.0f;
    al::startFreezeAction(layoutActor, "State", size, "Length");

    sead::Vector3f newPosition = {(panelV.x + panelV2.x) * 0.5f, (panelV.y + panelV2.y) * 0.5f,
                                  0.0f};
    al::setLocalTrans(layoutActor, newPosition);
}

void MapLayout::focusIcon(const MapIconInfo* mapIconInfo) {
    const char* selectAction = "Select";
    MapLayout* self = this;

    al::startAction(mapIconInfo->iconLayout->layout, selectAction, "Main");
    al::startAction(self->mWaitEndMapCursor, selectAction, nullptr);

    switch (mapIconInfo->iconType) {
    case IconType::Flag: {
        const char* checkpointObjId =
            GameDataFunction::getCheckpointObjIdInWorld(self, mapIconInfo->iconLayout->fieldB);
        char objName[128];
        char stageName[128];
        stageName[0] = '\0';
        objName[0] = '\0';

        const char* openParen = al::searchSubString(checkpointObjId, "(");
        const char* openBracket = al::searchSubString(checkpointObjId, "[");

        if (openParen != nullptr) {
            s32 objNameSize = openParen - checkpointObjId;
            al::extractString(objName, checkpointObjId, objNameSize, sizeof(objName));
            objName[objNameSize] = '\0';

            u32 stageNameSize = openBracket - (openParen + 1);
            al::extractString(stageName, openParen + 1, stageNameSize, sizeof(stageName));
            stageName[stageNameSize] = '\0';

            rs::trySetPaneStageMessageIfExist(self->mWaitEndMapGuide, "TxtFlagName",
                                              al::StringTmp<128>("%s_%s",
                                                                 rs::getCheckpointLabelPrefix(),
                                                                 objName)
                                                  .cstr(),
                                              stageName);
        } else {
            rs::trySetPaneStageMessageIfExist(self->mWaitEndMapGuide, "TxtFlagName",
                                              al::StringTmp<128>("%s_%s",
                                                                 rs::getCheckpointLabelPrefix(),
                                                                 checkpointObjId)
                                                  .cstr(),
                                              GameDataFunction::tryGetCurrentMainStageName(self));
        }
        al::setPaneSystemMessage(self->mWaitEndMapGuide, "TxtGuide", "StageMap",
                                 "CheckpointGuide");
        self->mWaitEndMapGuide->appear();
        al::startAction(self->mWaitEndMapGuide, "On", "OnOff");
        break;
    }
    case IconType::Home:
        rs::trySetPaneSystemMessageIfExist(self->mWaitEndMapGuide, "TxtFlagName", "GlossaryObject",
                                           "Home");
        al::setPaneSystemMessage(self->mWaitEndMapGuide, "TxtGuide", "StageMap",
                                 "CheckpointGuide");
        self->mWaitEndMapGuide->appear();
        al::startAction(self->mWaitEndMapGuide, "On", "OnOff");
        break;

    case IconType::Shop:
        if (al::isEqualSubString("Shop", "TitleYukimaruRace")) {
            rs::trySetPaneSystemMessageIfExist(self->mWaitEndMapGuide, "TxtFlagName", "StageMap",
                                               "TitleYukimaruRace");
        } else {
            rs::trySetPaneSystemMessageIfExist(self->mWaitEndMapGuide, "TxtFlagName", "StageMap",
                                               "Shop");
        }
        self->mWaitEndMapGuide->appear();
        al::startAction(self->mWaitEndMapGuide, "Off", "OnOff");
        break;
    case IconType::ShopSold:
        if (al::isEqualSubString("ShopSoldout", "TitleYukimaruRace")) {
            rs::trySetPaneSystemMessageIfExist(self->mWaitEndMapGuide, "TxtFlagName", "StageMap",
                                               "TitleYukimaruRace");
        } else {
            rs::trySetPaneSystemMessageIfExist(self->mWaitEndMapGuide, "TxtFlagName", "StageMap",
                                               "ShopSoldout");
        }
        self->mWaitEndMapGuide->appear();
        al::startAction(self->mWaitEndMapGuide, "Off", "OnOff");
        break;
    case IconType::Race: {
        const char* miniGameName =
            GameDataFunction::getMiniGameName(self, mapIconInfo->iconLayout->fieldB);
        if (al::isEqualSubString(miniGameName, "TitleYukimaruRace")) {
            rs::trySetPaneSystemMessageIfExist(self->mWaitEndMapGuide, "TxtFlagName", "MiniGame",
                                               "TitleYukimaruRace");
        } else {
            rs::trySetPaneSystemMessageIfExist(self->mWaitEndMapGuide, "TxtFlagName", "MiniGame",
                                               miniGameName);
        }

        self->mWaitEndMapGuide->appear();
        al::startAction(self->mWaitEndMapGuide, "Off", "OnOff");

        al::LiveActor* player = al::getPlayerActor(self->mPlayerHolder, 0);

        bool isExistRecord;
        s32 record;
        s32 bestRecord;
        s32 lapRecord;

        if (al::isEqualString(miniGameName, "TitleRadiconRace")) {
            record = 0;
            bestRecord = 0;
            lapRecord = 0;
            isExistRecord = false;
            rs::findRaceRecord(&isExistRecord, nullptr, &record, &bestRecord, &lapRecord, player,
                               "Radicon");
        } else if (al::isEqualString(miniGameName, "TitleJumprope")) {
            if (rs::isExistRecordJumpingRope(player)) {
                s32 count = rs::getJumpingRopeBestCount(player);
                al::WStringTmp<64> guide;
                const char16* message =
                    al::getSystemMessageString(self->mWaitEndMapGuide, "StageMap",
                                               "MinigameCountGuide");
                al::replaceMessageTagScore(&guide, self->mWaitEndMapGuide, message, count,
                                           "Score");
                al::setPaneString(self->mWaitEndMapGuide, "TxtGuide", guide.cstr(), 0);
                al::startAction(self->mWaitEndMapGuide, "On", "OnOff");
                break;
            }

            al::WStringTmp<64> guide;
            const char16* message =
                al::getSystemMessageString(self->mWaitEndMapGuide, "StageMap",
                                           "MinigameCountGuide");
            al::replaceMessageTagScore(&guide, self->mWaitEndMapGuide, message, 0, "Score");
            al::setPaneString(self->mWaitEndMapGuide, "TxtGuide", guide.cstr(), 0);
            al::startAction(self->mWaitEndMapGuide, "On", "OnOff");
            break;
        } else if (al::isEqualString(miniGameName, "TitleVolleyball")) {
            if (rs::isExistRecordVolleyball(player)) {
                s32 count = rs::getVolleyballBestCount(player);
                al::WStringTmp<64> guide;
                const char16* message =
                    al::getSystemMessageString(self->mWaitEndMapGuide, "StageMap",
                                               "MinigameCountGuide");
                al::replaceMessageTagScore(&guide, self->mWaitEndMapGuide, message, count,
                                           "Score");
                al::setPaneString(self->mWaitEndMapGuide, "TxtGuide", guide.cstr(), 0);
                al::startAction(self->mWaitEndMapGuide, "On", "OnOff");
                break;
            }

            al::WStringTmp<64> guide;
            const char16* message =
                al::getSystemMessageString(self->mWaitEndMapGuide, "StageMap",
                                           "MinigameCountGuide");
            al::replaceMessageTagScore(&guide, self->mWaitEndMapGuide, message, 0, "Score");
            al::setPaneString(self->mWaitEndMapGuide, "TxtGuide", guide.cstr(), 0);
            al::startAction(self->mWaitEndMapGuide, "On", "OnOff");
            break;
        } else if (al::isEqualString(miniGameName, "TitleRaceManRace")) {
            record = 0;
            bestRecord = 0;
            lapRecord = 0;
            isExistRecord = false;
            rs::findRaceRecordRaceManRace(&isExistRecord, nullptr, &record, &bestRecord, &lapRecord,
                                          player);
        } else if (al::isEqualString(miniGameName, "TitleYukimaruRace")) {
            record = 0;
            bestRecord = 0;
            lapRecord = 0;
            isExistRecord = false;
            rs::findRaceRecord(&isExistRecord, nullptr, &record, &bestRecord, &lapRecord, player,
                               "Yukimaru_1");
        } else if (al::isEqualString(miniGameName, "TitleYukimaruRaceMoonRock")) {
            record = 0;
            bestRecord = 0;
            lapRecord = 0;
            isExistRecord = false;
            rs::findRaceRecord(&isExistRecord, nullptr, &record, &bestRecord, &lapRecord, player,
                               "Yukimaru_2");
        } else {
            al::startAction(self->mWaitEndMapGuide, "Off", "OnOff");
            break;
        }

        if (!isExistRecord) {
            al::setPaneString(self->mWaitEndMapGuide, "TxtGuide",
                              al::getSystemMessageString(self->mWaitEndMapGuide, "StageMap",
                                                         "MinigameNoRecordGuide"),
                              0);
        } else {
            rs::replaceRaceRecordMessageCsec(
                self->mWaitEndMapGuide, "TxtGuide", record,
                al::getSystemMessageString(self->mWaitEndMapGuide, "StageMap",
                                           "MinigameRecordGuide"),
                "BestRecord");
        }
        al::startAction(self->mWaitEndMapGuide, "On", "OnOff");
        break;
    }
    case IconType::Cap:
        if (al::isEqualSubString("Cap", "TitleYukimaruRace")) {
            rs::trySetPaneSystemMessageIfExist(self->mWaitEndMapGuide, "TxtFlagName", "StageMap",
                                               "TitleYukimaruRace");
        } else {
            rs::trySetPaneSystemMessageIfExist(self->mWaitEndMapGuide, "TxtFlagName", "StageMap",
                                               "Cap");
        }
        self->mWaitEndMapGuide->appear();
        al::startAction(self->mWaitEndMapGuide, "Off", "OnOff");
        break;
    case IconType::Luigi:
        if (al::isEqualSubString("TimeBalloon", "TitleYukimaruRace")) {
            rs::trySetPaneSystemMessageIfExist(self->mWaitEndMapGuide, "TxtFlagName", "StageMap",
                                               "TitleYukimaruRace");
        } else {
            rs::trySetPaneSystemMessageIfExist(self->mWaitEndMapGuide, "TxtFlagName", "StageMap",
                                               "TimeBalloon");
        }
        self->mWaitEndMapGuide->appear();
        al::startAction(self->mWaitEndMapGuide, "Off", "OnOff");
        break;
    case IconType::Poet:
        if (al::isEqualSubString("Poetter", "TitleYukimaruRace")) {
            rs::trySetPaneSystemMessageIfExist(self->mWaitEndMapGuide, "TxtFlagName", "StageMap",
                                               "TitleYukimaruRace");
        } else {
            rs::trySetPaneSystemMessageIfExist(self->mWaitEndMapGuide, "TxtFlagName", "StageMap",
                                               "Poetter");
        }
        self->mWaitEndMapGuide->appear();
        al::startAction(self->mWaitEndMapGuide, "Off", "OnOff");
        break;
    case IconType::Scenario2:
        if (al::isEqualSubString("MoonRock", "TitleYukimaruRace")) {
            rs::trySetPaneSystemMessageIfExist(self->mWaitEndMapGuide, "TxtFlagName", "StageMap",
                                               "TitleYukimaruRace");
        } else {
            rs::trySetPaneSystemMessageIfExist(self->mWaitEndMapGuide, "TxtFlagName", "StageMap",
                                               "MoonRock");
        }
        self->mWaitEndMapGuide->appear();
        al::startAction(self->mWaitEndMapGuide, "Off", "OnOff");
        break;
    default:
        break;
    }
}

void MapLayout::lostFocusIcon(MapIconLayout* mapIconLayout) {
    al::startAction(mWaitEndMapCursor, "Wait", nullptr);
    al::startAction(mapIconLayout->layout, "Wait", "Main");
    al::killLayoutIfActive(mWaitEndMapGuide);
    al::startAction(mWaitEndMapGuide, "End", nullptr);
}

bool MapLayout::tryCalcNorthDir(sead::Vector3f* northDir) {
    MapData* mapData = mMapTerrainLayout->getMapData();
    if (mapData == nullptr)
        return false;

    const sead::Vector3f ey = sead::Vector3f::ey;
    northDir->x = mapData->viewMatrix(0, 0) * ey.x + mapData->viewMatrix(1, 0) * ey.y +
                  mapData->viewMatrix(2, 0) * ey.z;
    northDir->y = mapData->viewMatrix(0, 1) * ey.x + mapData->viewMatrix(1, 1) * ey.y +
                  mapData->viewMatrix(2, 1) * ey.z;
    northDir->z = mapData->viewMatrix(0, 2) * ey.x + mapData->viewMatrix(1, 2) * ey.y +
                  mapData->viewMatrix(2, 2) * ey.z;
    al::normalize(northDir);
    return true;
}

void MapLayout::exeAppear() {
    if (al::isFirstStep(this)) {
        sead::FixedSafeString<0x100> stringA;
        sead::FixedSafeString<0x100> stringB;
        bool isB = false;

        al::startAction(mWaitEndMapCursor, "On", "Scenario");
        if (GameDataFunction::isTimeBalloonSequence(this) ||
            GameDataFunction::isRaceStart(this) ||
            !GameDataFunction::isUnlockedCurrentWorld(this) ||
            !rs::tryGetMapMainScenarioLabel(&stringA, &stringB, &isB, this)) {
            al::startAction(mWaitEndMapCursor, "Off", "Scenario");
        } else {
            if (isB) {
                rs::trySetPaneStageMessageIfExist(mWaitEndMapCursor, "TxtScenario", stringB.cstr(),
                                                  stringB.cstr());
            } else {
                rs::trySetPaneSystemMessageIfExist(mWaitEndMapCursor, "TxtScenario", stringA.cstr(),
                                                   stringA.cstr());
            }
        }
        if (isAppear())
            mWaitEndMapCursor->appear();
        al::startHitReaction(this, "マップオープン", nullptr);
    }

    if (al::isGreaterEqualStep(this, 4) && !mIsSharila) {
        eui::ScalableFontMgr* scalableFontMgr =
            getLayoutSceneInfo()->getFontMgr()->getScalableFontMgr();

        if (!scalableFontMgr->getTextureCacheNoSpaceErrorInline()) {
            s32 worldId = mWorldId;
            scalableFontMgr->getFont("nintendo_udsg-r_std_003_10.fcpx");
            const nn::font::ScalableFont* font40 =
                scalableFontMgr->getFont("nintendo_udsg-r_std_003_40.fcpx");
            const nn::font::ScalableFont* font80 =
                scalableFontMgr->getFont("nintendo_udsg-r_std_003_80.fcpx");

            nn::ui2d::Pane* rootPane = getLayoutKeeper()->getLayout()->getRootPane();

            s32 i = 0;
            do {
                nn::ui2d::Pane* pane = rootPane->FindPaneByName(sPanelNames[i], true);
                eui::TextBoxEx* textBox = nn::ui2d::DynamicCast<eui::TextBoxEx*>(pane);
                scalableFontMgr->registerGlyphs(textBox->getTextBuffer(),
                                                textBox->getTextBufferLength(), font40,
                                                getLayoutSceneInfo()->getFontMgr(), -1);
                scalableFontMgr->registerGlyphs(textBox->getTextBuffer(),
                                                textBox->getTextBufferLength(), font80,
                                                getLayoutSceneInfo()->getFontMgr(), -1);
                i++;
            } while (i < 21);

            nn::ui2d::Pane* title07Pane = rootPane->FindPaneByName("TxtTitle07", true);
            eui::TextBoxEx* title07TextBox =
                nn::ui2d::DynamicCast<eui::TextBoxEx*>(title07Pane);
            scalableFontMgr->registerGlyphs(title07TextBox->getTextBuffer(),
                                            title07TextBox->getTextBufferLength(), font40,
                                            getLayoutSceneInfo()->getFontMgr(), -1);
            scalableFontMgr->registerGlyphs(title07TextBox->getTextBuffer(),
                                            title07TextBox->getTextBufferLength(), font80,
                                            getLayoutSceneInfo()->getFontMgr(), -1);

            nn::ui2d::Pane* element01Pane = rootPane->FindPaneByName("TxtElement01", true);
            eui::TextBoxEx* element01TextBox =
                nn::ui2d::DynamicCast<eui::TextBoxEx*>(element01Pane);
            scalableFontMgr->registerGlyphs(element01TextBox->getTextBuffer(),
                                            element01TextBox->getTextBufferLength(), font40,
                                            getLayoutSceneInfo()->getFontMgr(), -1);
            scalableFontMgr->registerGlyphs(element01TextBox->getTextBuffer(),
                                            element01TextBox->getTextBufferLength(), font80,
                                            getLayoutSceneInfo()->getFontMgr(), -1);

            nn::ui2d::Pane* element02Pane = rootPane->FindPaneByName("TxtElement02", true);
            eui::TextBoxEx* element02TextBox =
                nn::ui2d::DynamicCast<eui::TextBoxEx*>(element02Pane);
            scalableFontMgr->registerGlyphs(element02TextBox->getTextBuffer(),
                                            element02TextBox->getTextBufferLength(), font40,
                                            getLayoutSceneInfo()->getFontMgr(), -1);
            scalableFontMgr->registerGlyphs(element02TextBox->getTextBuffer(),
                                            element02TextBox->getTextBufferLength(), font80,
                                            getLayoutSceneInfo()->getFontMgr(), -1);

            nn::ui2d::Pane* iconPane = rootPane->FindPaneByName("TxtIcon", true);
            eui::TextBoxEx* iconTextBox = nn::ui2d::DynamicCast<eui::TextBoxEx*>(iconPane);
            scalableFontMgr->registerGlyphs(iconTextBox->getTextBuffer(),
                                            iconTextBox->getTextBufferLength(), font40,
                                            getLayoutSceneInfo()->getFontMgr(), -1);
            scalableFontMgr->registerGlyphs(iconTextBox->getTextBuffer(),
                                            iconTextBox->getTextBufferLength(), font80,
                                            getLayoutSceneInfo()->getFontMgr(), -1);
            mIsSharila = true;
        }
    }
    if (al::isActionEnd(this, nullptr)) {
        if (isAppear() || al::isNerve(this, &NrvMapLayout.ChangeIn)) {
            al::setNerve(this, &NrvMapLayout.Wait);
            appearParts(true);
            startNumberAction();
            return;
        }
        if (al::isNerve(this, &NrvMapLayout.AppearWithHint)) {
            al::setNerve(this, &NrvMapLayout.HintInitWaitNpc);
            appearParts(true);
            startNumberAction();
            return;
        }
        if (al::isNerve(this, &NrvMapLayout.AppearMoonRockDemo)) {
            al::setNerve(this, &NrvMapLayout.HintInitWaitMoonRock);
            appearParts(false);
        }
        startNumberAction();
    }
}

void MapLayout::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", nullptr);
        s32 size = mArray.size();
        for (s32 i = 0; i < size; i++)
            if (al::isActive(mArray[i]))
                al::startAction(mArray[i], "Wait", nullptr);
    }
}

void MapLayout::exeHintInitWait() {
    if (al::isFirstStep(this))
        al::showPaneRoot(this);

    if (al::isGreaterEqualStep(this, 30)) {
        if (al::isNerve(this, &NrvMapLayout.HintInitWaitNpc))
            al::setNerve(this, &NrvMapLayout.HintAppearNpc);
        else if (al::isNerve(this, &NrvMapLayout.HintInitWaitMoonRock))
            al::setNerve(this, &NrvMapLayout.HintAppearMoonRock);
        else if (al::isNerve(this, &NrvMapLayout.HintInitWaitAmiibo))
            al::setNerve(this, &NrvMapLayout.HintAppearAmiibo);
    }
}

void MapLayout::exeHintAppear() {
    if (al::isFirstStep(this)) {
        if (al::isNerve(this, &NrvMapLayout.HintAppearNpc)) {
            s32 hintNum = GameDataFunction::calcHintNum(this);
            s32 hintIndex = hintNum - 1;
            if (GameDataFunction::checkLatestHintSeaOfTree(this)) {
                sead::Vector3f position;
                calcSeaOfTreeIconPos(&position);
                setLocalTransAndAppear(&mHintIconLayouts[hintIndex], mMapIconInfo, position,
                                       IconType::Hint2, true);
            } else {
                calcMapTransAndAppear(&mHintIconLayouts[hintIndex], mMapIconInfo,
                                      GameDataFunction::getLatestHintTrans(this), IconType::Hint2,
                                      false);
            }
            al::startAction(mHintIconLayouts[hintIndex].layout, "HintNew", nullptr);
        } else if (al::isNerve(this, &NrvMapLayout.HintAppearAmiibo)) {
            s32 hintNum = GameDataFunction::calcHintNum(this);
            MapIconLayout* seaOfTreeHintLayout = nullptr;
            for (s32 i = 0; i < mHintDecideIconAmiiboSize; i++) {
                HintAmiibo& hintAmiibo = getHintAmiibo(i, mHintAmiiboSizer, mHintAmiibo);
                if (hintAmiibo.isValid) {
                    if (seaOfTreeHintLayout != nullptr) {
                        mMapIconInfo[seaOfTreeHintLayout->fieldA].action++;
                        continue;
                    }

                    sead::Vector3f position = sead::Vector3f::zero;
                    calcSeaOfTreeIconPos(&position);
                    seaOfTreeHintLayout = &mHintIconLayouts[i];
                    setLocalTransAndAppear(seaOfTreeHintLayout, mMapIconInfo, position,
                                           IconType::Hint2, true);
                    continue;
                }

                bool isMerged = false;
                for (s32 j = 0; j < mHintDecideIconAmiiboSize; j++) {
                    MapIconInfo& info = mMapIconInfo[mHintIconLayouts[j].fieldA];
                    if (isNearMapIconPosition(info, hintAmiibo.position)) {
                        info.action++;
                        isMerged = true;
                        break;
                    }
                }
                if (!isMerged) {
                    s32 hintIndex = hintNum - 1 - i;
                    calcMapTransAndAppear(&mHintIconLayouts[hintIndex], mMapIconInfo,
                                          hintAmiibo.position, IconType::Hint2, false);
                    al::startAction(mHintIconLayouts[hintIndex].layout, "HintNew", nullptr);
                }
            }

        } else if (al::isNerve(this, &NrvMapLayout.HintAppearMoonRock)) {
            s32 rockNum = GameDataFunction::calcHintMoonRockNum(this);
            for (s32 i = 0; i < rockNum; i++) {
                sead::Vector3f rockPosition = GameDataFunction::calcHintMoonRockTrans(this, i);
                bool isMerged = false;
                for (s32 j = 0; j < i; j++) {
                    MapIconInfo& info = mMapIconInfo[mMoonRockIconLayouts[j].fieldA];
                    if (isNearMapIconPosition(info, rockPosition)) {
                        info.action++;
                        isMerged = true;
                        break;
                    }
                }
                if (!isMerged) {
                    calcMapTransAndAppear(&mMoonRockIconLayouts[i], mMapIconInfo, rockPosition,
                                          IconType::HintRock2, false);
                    al::startAction(mMoonRockIconLayouts[i].layout, "HintNew", nullptr);
                }
            }
        }
        startNumberAction();
    }
    if (al::isGreaterEqualStep(this, 150)) {
        if (al::isNerve(this, &NrvMapLayout.HintAppearNpc))
            al::setNerve(this, &NrvMapLayout.HintDecideIconAppearNpc);
        else if (al::isNerve(this, &NrvMapLayout.HintAppearAmiibo))
            al::setNerve(this, &NrvMapLayout.HintDecideIconAppearAmiibo);
        else if (al::isNerve(this, &NrvMapLayout.HintAppearMoonRock))
            al::setNerve(this, &NrvMapLayout.HintDecideIconAppearMoonRock);
    }
}

void MapLayout::exeHintDecideIconAppear() {
    if (al::isFirstStep(this)) {
        mDecideIconLayout->appear();
        if (al::isNerve(this, &NrvMapLayout.HintDecideIconAppearMoonRock)) {
            s32 moonRockNum = GameDataFunction::calcHintMoonRockNum(this);
            for (s32 i = 0; i < moonRockNum; i++) {
                al::startAction(mMoonRockIconLayouts[i].layout, "Wait", nullptr);
                startNumberAction();
            }
        } else if (al::isNerve(this, &NrvMapLayout.HintDecideIconAppearNpc)) {
            al::startAction(mHintIconLayouts[GameDataFunction::calcHintNum(this) - 1].layout,
                            "Wait", nullptr);
            startNumberAction();
        } else if (al::isNerve(this, &NrvMapLayout.HintDecideIconAppearAmiibo)) {
            s32 hintNum = GameDataFunction::calcHintNum(this);
            for (s32 i = 0; i < mHintDecideIconAmiiboSize; i++) {
                al::startAction(mHintIconLayouts[hintNum - (i + 1)].layout, "Wait", nullptr);
                startNumberAction();
            }
            mHintDecideIconAmiiboSize = 0;
        }
    }
    mDecideIconLayout->updateNerve();
    if (mDecideIconLayout->isWait())
        al::setNerve(this, &NrvMapLayout.HintDecideIconWait);
}

void MapLayout::exeHintDecideIconWait() {
    mDecideIconLayout->updateNerve();
    if (mDecideIconLayout->isDecide())
        al::setNerve(this, &HintPressDecide);
}

void MapLayout::exeHintPressDecide() {
    mDecideIconLayout->updateNerve();
    if (mDecideIconLayout->isEnd())
        end();
}

void MapLayout::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", nullptr);
        al::startAction(mWaitEndMapPlayer, "End", nullptr);
        al::startAction(mWaitEndMapCursor, "End", nullptr);
        al::startAction(mWaitEndMapGuide, "End", nullptr);

        if (al::isActive(mWaitEndMapLine))
            al::startAction(mWaitEndMapLine, "End", nullptr);

        s32 size = mArray.size();
        for (s32 i = 0; i < size; i++)
            al::startAction(mArray[i], "End", nullptr);
        for (s32 i = 0; i < mMapIconInfoSize; i++)
            if (mMapIconInfo[i].isActive &&
                al::killLayoutIfActive(mMapIconInfo[i].iconLayout->layout))
                mMapIconInfo[i].isActive = false;
        al::startHitReaction(this, "マップクローズ", nullptr);
    }

    if (al::isActionEnd(this, nullptr)) {
        s32 size = mArray.size();
        for (s32 i = 0; i < size; i++)
            mArray[i]->kill();
        mWaitEndMapPlayer->kill();
        mWaitEndMapCursor->kill();
        mWaitEndMapGuide->kill();
        mWaitEndMapLine->kill();
        if (mIsHelp)
            mWaitEndMapBg->kill();
        mIsHelp = true;
        kill();
    }
}

void MapLayout::exeChangeOut() {
    if (al::isFirstStep(this)) {
        for (s32 i = 0; i < mMapIconInfoSize; i++)
            if (mMapIconInfo[i].isActive &&
                al::killLayoutIfActive(mMapIconInfo[i].iconLayout->layout))
                mMapIconInfo[i].isActive = false;
    }

    if (al::isActionEnd(this, "Change")) {
        mWaitEndMapPlayer->kill();
        mWaitEndMapCursor->kill();
        mWaitEndMapGuide->kill();
        mWaitEndMapLine->kill();
        if (mIsHelp)
            mWaitEndMapBg->kill();
        mIsHelp = true;
        kill();
    }
}

namespace rs {
void calcTransOnMap(sead::Vector2f* out, const sead::Vector3f& v1, const sead::Matrix44f& m,
                    const sead::Vector2f& v2, f32 f1, f32 f2) {
    const sead::Vector3f tmp = v1;

    f64 d_m00 = m.m[0][0];
    f64 d_m01 = m.m[0][1];
    f64 d_m02 = m.m[0][2];
    f64 d_m03 = m.m[0][3];

    f64 d_m10 = m.m[1][0];
    f64 d_m11 = m.m[1][1];
    f64 d_m12 = m.m[1][2];
    f64 d_m13 = m.m[1][3];

    f64 d_v1x = (f64)tmp.x;
    f64 d_v1y = (f64)tmp.y;
    f64 d_v1z = (f64)tmp.z;

    f64 accx = (d_v1x * d_m00 + d_v1y * d_m01 + d_v1z * d_m02) + d_m03;
    f64 accy = (d_v1x * d_m10 + d_v1y * d_m11 + d_v1z * d_m12) + d_m13;
    sead::Vector2<f64> scaled = {accx, accy};
    scaled *= f2 * 0.5;
    scaled *= f1;

    f64 d_v2x = v2.x * f1 + scaled.x;
    f64 d_v2y = __builtin_fma(scaled.y, 1.0, v2.y * f1);

    out->set(d_v2x, d_v2y);
}

bool tryCalcMapNorthDir(sead::Vector3f* direction, const al::IUseSceneObjHolder* objHolder) {
    MapLayout* mapLayout = al::getSceneObj<MapLayout>(objHolder);
    return mapLayout->tryCalcNorthDir(direction);
}

const sead::Matrix44f& getMapViewProjMtx(const al::IUseSceneObjHolder* objHolder) {
    MapLayout* mapLayout = al::getSceneObj<MapLayout>(objHolder);
    return mapLayout->getViewProjMtx();
}

const sead::Matrix44f& getMapProjMtx(const al::IUseSceneObjHolder* objHolder) {
    MapLayout* mapLayout = al::getSceneObj<MapLayout>(objHolder);
    return mapLayout->getProjMtx();
}

void appearMapWithHint(const al::IUseSceneObjHolder* objHolder) {
    MapLayout* mapLayout = al::getSceneObj<MapLayout>(objHolder);
    mapLayout->appearWithHint();
}

void addAmiiboHintToMap(const al::IUseSceneObjHolder* objHolder) {
    MapLayout* mapLayout = al::getSceneObj<MapLayout>(objHolder);
    mapLayout->addAmiiboHint();
}

void appearMapWithAmiiboHint(const al::IUseSceneObjHolder* objHolder) {
    MapLayout* mapLayout = al::getSceneObj<MapLayout>(objHolder);
    mapLayout->appearAmiiboHint();
}

void appearMapMoonRockDemo(const al::IUseSceneObjHolder* objHolder, s32 value) {
    MapLayout* mapLayout = al::getSceneObj<MapLayout>(objHolder);
    mapLayout->appearMoonRockDemo(value);
}

void endMap(const al::IUseSceneObjHolder* objHolder) {
    MapLayout* mapLayout = al::getSceneObj<MapLayout>(objHolder);
    mapLayout->end();
}

bool isEndMap(const al::IUseSceneObjHolder* objHolder) {
    MapLayout* mapLayout = al::getSceneObj<MapLayout>(objHolder);
    return mapLayout->isEnd();
}

bool isEnableCheckpointWarp(const al::IUseSceneObjHolder* objHolder) {
    MapLayout* mapLayout = al::getSceneObj<MapLayout>(objHolder);
    return mapLayout->isEnableCheckpointWarp();
}
}  // namespace rs

namespace StageMapFunction {
f32 getStageMapScaleMin() {
    return 0.3f;
}

f32 getStageMapScaleMax() {
    return 1.0f;
}
}  // namespace StageMapFunction
