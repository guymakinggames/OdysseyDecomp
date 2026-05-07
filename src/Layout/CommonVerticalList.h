#pragma once

#include <container/seadPtrArray.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>

#include "Library/Nerve/NerveExecutor.h"

namespace al {
class LayoutActor;
class LayoutInitInfo;
class RollParts;
}  // namespace al

namespace nn::ui2d {
class TextureInfo;
}

struct RollPartsData {
    s32 textNum = 0;
    u32 padding = 0;
    const char16** texts = nullptr;
    s32 selectedIdx = 0;
    bool isLoop = false;
    u8 padding14[3] = {};
};

static_assert(sizeof(RollPartsData) == 0x18);

class CommonVerticalList : public al::NerveExecutor {
public:
    CommonVerticalList(al::LayoutActor*, const al::LayoutInitInfo&, bool);

    void initData(s32);
    void initDataNoResetSelected(s32);
    void initDataWithIdx(s32, s32, s32);
    void hideAll();
    void updateCursorPos();
    void addStringData(const sead::WFixedSafeString<512>*, const char*);
    void setEnableData(const bool*);
    void addGroupAnimData(const sead::FixedSafeString<64>*, const char*);
    void setImageData(nn::ui2d::TextureInfo**, const char*);
    void setSelectedIdx(s32, s32);
    void setRollPartsData(RollPartsData*);
    void setRollPartsSelected(s32, s32);
    s32 getRollPartsSelected(s32);
    al::RollParts* getSelectedParts() const;
    al::RollParts* getParts(s32) const;
    s32 getListPartsNum() const;
    void startLoopActionAll(const char*, const char*);
    void calcCursorPos(sead::Vector2f*) const;
    bool isActive() const;
    bool isDeactive() const;
    bool isDecideEnd() const;
    bool isRejectEnd() const;
    void update();
    void up();
    void down();
    void decide();
    void updateParts();
    void reject();
    void deactivate();
    void activate();
    bool jumpTop();
    bool jumpBottom();
    void pageUp();
    void pageDown();
    void rollRight();
    void rollLeft();
    void exeActive();
    void exeDeactive();
    void exeDecide();
    void exeDecideEnd();
    void exeReject();
    void exeRejectEnd();
    void calcAnimRate();
    void appearCursor();
    void hideCursor();
    void endCursor();

    s32 getSelectedIdx() const { return mSelectedIdx; }

    s32 getTopIdx() const { return mTopIdx; }

    s32 getVisibleTopIdx() const { return mVisibleTopIdx; }

    s32 getDataNum() const { return mDataNum; }

private:
    al::LayoutActor* mParentLayout = nullptr;
    sead::PtrArray<al::RollParts> mParts;
    void* mCursorParts = nullptr;
    void* mScrollBarParts = nullptr;
    s32 mListPartsNum = 0;
    s32 mSelectedIdx = 0;
    s32 mTopIdx = 0;
    s32 mPrevSelectedIdx = 0;
    s32 mPrevTopIdx = 0;
    f32 mBaseListY = 0.0f;
    s32 mMoveStep = 0;
    s32 mScrollBarPos = 0;
    f32 mMoveRate = 0.0f;
    sead::Vector2f mCursorPos;
    u8 mField64 = 0;
    bool mIsCursorDirty = false;
    u8 mPadding66[2] = {};
    s32 mVisibleTopIdx = 0;
    const sead::WFixedSafeString<512>** mStringData = nullptr;
    sead::FixedSafeString<128>* mStringPaneNames = nullptr;
    nn::ui2d::TextureInfo** mImageData = nullptr;
    void* mImageParts = nullptr;
    const bool* mEnableData = nullptr;
    s32 mDataIndex = 0;
    s32 mDataNum = 0;
    const char* mImagePaneName = nullptr;
    void** mGroupAnimData = nullptr;
    s32 mGroupAnimDataNum = 0;
    s32 mRollUpdateFrame = -1;
    RollPartsData* mRollPartsData = nullptr;
    s32* mRollPartsSelected = nullptr;
    s32 mRollPartsWork = 0;
    bool mIsDataReady = false;
    u8 mPaddingCD[3] = {};
};

static_assert(sizeof(CommonVerticalList) == 0xd0);
