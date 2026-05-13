#pragma once

#include <container/seadPtrArray.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

#include "Library/Layout/LayoutActor.h"
#include "Library/Scene/ISceneObj.h"

#include "Scene/SceneObjFactory.h"

namespace al {
class IUseSceneObjHolder;
class LayoutInitInfo;
class LiveActor;
class PlayerHolder;
class SimpleLayoutAppearWaitEnd;
}  // namespace al

struct MapIconInfo;
class DecideIconLayout;
class MapTerrainLayout;
class TalkMessage;

enum IconType : u32 {
    Flag,
    FlagDisable,
    Home,
    HomeDisable,
    Hint1,
    Hint2,
    Shop,
    ShopSold,
    Race,
    Start,
    Goal,
    Scenario1,
    ScenarioHome,
    HintRock1,
    HintRock2,
    Cap,
    Luigi,
    Poet,
    Scenario2,
    MaxValue = 0x13,
};

struct MapIconLayout {
    MapIconLayout() = default;
    MapIconLayout(s32 a, s32 b, al::LayoutActor* c)
        : fieldA(a), fieldB(b), layout(c) {}

    s32 fieldA;
    s32 fieldB;
    al::LayoutActor* layout;
};

struct MapIconInfo {
    MapIconLayout* iconLayout;
    bool isActive;
    IconType iconType;
    sead::Vector3f position;
    s32 action;
    sead::Vector2f position2;
    bool value;
};

static_assert(sizeof(MapIconInfo) == 0x30);

struct HintAmiibo {
    sead::Vector3f position;
    bool isValid;
};

class MapLayout : public al::LayoutActor, public al::ISceneObj {
public:
    static constexpr s32 sSceneObjId = SceneObjID_MapLayout;

    MapLayout(const al::LayoutInitInfo&, const al::PlayerHolder*, s32 worldId);

    void appear() override;
    void control() override;
    const char* getSceneObjName() const override;

    void changePrintWorld(s32 worldId);
    void loadTexture();
    void reset();
    void moveFocusLayout(const sead::Vector3f&, const sead::Vector2f&);
    void updateST();
    void addAmiiboHint();
    void appearAmiiboHint();
    void end();
    // sub_71001F1B04
    void updatePlayerPosLayout();
    void appearWithHint();
    void appearMoonRockDemo(s32);
    void appearCollectionList();
    bool isEnd() const;
    bool isEnableCheckpointWarp() const;
    void changeOut(bool);
    void changeIn(bool isRightIn);
    void updateLine(al::LayoutActor*);
    void appearParts(bool);
    void startNumberAction();
    void calcSeaOfTreeIconPos(sead::Vector3f*);
    void setLocalTransAndAppear(MapIconLayout*, MapIconInfo*, const sead::Vector3f&, IconType,
                                bool);
    void calcMapTransAndAppear(MapIconLayout*, MapIconInfo*, const sead::Vector3f&, IconType, bool);
    void scroll(const sead::Vector2f& scrollDistance);
    void addSize(const sead::Vector2f& size);
    bool isAppear() const;
    const sead::Matrix44f& getViewProjMtx() const;
    const sead::Matrix44f& getProjMtx() const;
    void updateIconLine(al::LayoutActor*, const sead::Vector3f&, const sead::Vector2f&);
    void focusIcon(const MapIconInfo*);
    void lostFocusIcon(MapIconLayout*);
    bool tryCalcNorthDir(sead::Vector3f* northDir);

    void exeAppear();
    void exeWait();
    void exeHintInitWait();
    void exeHintAppear();
    void exeHintDecideIconAppear();
    void exeHintDecideIconWait();
    void exeHintPressDecide();
    void exeEnd();
    void exeChangeOut();

    MapTerrainLayout* getMapTerrainLayout() const { return mMapTerrainLayout; }

private:
    al::SimpleLayoutAppearWaitEnd* mWaitEndMapBg = nullptr;
    al::SimpleLayoutAppearWaitEnd* mWaitEndMapCursor = nullptr;
    al::SimpleLayoutAppearWaitEnd* mWaitEndMapPlayer = nullptr;
    al::SimpleLayoutAppearWaitEnd* mWaitEndMapGuide = nullptr;
    al::SimpleLayoutAppearWaitEnd* mWaitEndMapLine = nullptr;
    DecideIconLayout* mDecideIconLayout = nullptr;
    MapTerrainLayout* mMapTerrainLayout = nullptr;
    TalkMessage* mTalkMessage = nullptr;

    MapIconLayout* mCheckpointIconLayouts = nullptr;

    s32 mCheckpointNumMaxInWorld = 0;
    MapIconLayout* mHomeIconLayout = nullptr;
    MapIconLayout* mHintIconLayouts = nullptr;

    s32 mHintNumMax = 0;
    MapIconLayout* mShopIconLayouts = nullptr;

    s32 mCalcShopNum = 0;
    MapIconLayout* mMiniGameIconLayouts = nullptr;

    s32 mMiniGameNumMax = 0;
    MapIconLayout* mRaceStartIconLayout = nullptr;
    MapIconLayout* mRaceGoalIconLayout = nullptr;
    MapIconLayout* mScenarioIconLayouts = nullptr;

    s32 mMainScenarioNumMax = 0;
    MapIconLayout* mMoonRockIconLayouts = nullptr;

    s32 mHintMoonRockNumMax = 0;
    MapIconLayout* mJangoIconLayout = nullptr;
    MapIconLayout* mTimeBalloonIconLayout = nullptr;
    MapIconLayout* mPoetterIconLayout = nullptr;
    MapIconLayout* mUnusedIconLayout = nullptr;
    MapIconInfo* mMapIconInfo = nullptr;

    s32 mMapIconInfoSize = 0;
    sead::PtrArray<al::LayoutActor> mArray;
    sead::Vector3f mAAA = sead::Vector3f::zero;
    const al::PlayerHolder* mPlayerHolder = nullptr;
    bool mIsPrintWorldChanged = true;
    bool mIsHelp = true;
    sead::Vector2f mScrollPosition = sead::Vector2f::zero;
    sead::Vector2f mMinScrollPosition = {-2300.0f, -1200.0f};
    sead::Vector2f mMaxScrollPosition = {2300.0f, 1200.0f};
    sead::Vector2f mPanelSize = {0.0f, 0.0f};
    sead::Vector2f mPanelLocalScale = {0.0f, 0.0f};
    s32 mWorldId = -1;
    s32 mCurrentFontType = 3;
    s32 mDoesntexist;

    u32 mHintAmiiboSizer = 0;
    HintAmiibo* mHintAmiibo = nullptr;
    s32 mHintDecideIconAmiiboSize;

    bool mIsFreezeAction;
    bool mIsResetTransform;
    bool mIsSharila;
};

static_assert(sizeof(MapLayout) == 0x298);

void setPanelName(al::LayoutActor* layoutActor, const char* name, s32 id);
bool trySetBalloon(al::LayoutActor* layoutActor, const al::LiveActor* actor,
                   const sead::Matrix44f& matrix);
void setPanelFont(f32 scale, s32* newType, al::LayoutActor* layoutActor, s32 currentType);

namespace rs {
void calcTransOnMap(sead::Vector2f*, const sead::Vector3f&, const sead::Matrix44f&,
                    const sead::Vector2f&, f32, f32);
bool tryCalcMapNorthDir(sead::Vector3f*, const al::IUseSceneObjHolder* objHolder);
const sead::Matrix44f& getMapViewProjMtx(const al::IUseSceneObjHolder* objHolder);
const sead::Matrix44f& getMapProjMtx(const al::IUseSceneObjHolder* objHolder);
void appearMapWithHint(const al::IUseSceneObjHolder* objHolder);
void addAmiiboHintToMap(const al::IUseSceneObjHolder* objHolder);
void appearMapWithAmiiboHint(const al::IUseSceneObjHolder* objHolder);
void appearMapMoonRockDemo(const al::IUseSceneObjHolder* objHolder, s32);
void endMap(const al::IUseSceneObjHolder* objHolder);
bool isEndMap(const al::IUseSceneObjHolder* objHolder);
bool isEnableCheckpointWarp(const al::IUseSceneObjHolder* objHolder);
}  // namespace rs

namespace StageMapFunction {
f32 getStageMapScaleMin();
f32 getStageMapScaleMax();
}  // namespace StageMapFunction
