#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>

#include "Library/Scene/IUseSceneObjHolder.h"

namespace al {
struct ActorInitInfo;
class PlacementInfo;
class SceneObjHolder;
}  // namespace al

class QuestInfo : public al::IUseSceneObjHolder {
public:
    QuestInfo();
    void clear();
    void init(const al::ActorInitInfo&);
    void init(const al::PlacementInfo&, const al::ActorInitInfo&);
    void init(const al::PlacementInfo&, al::SceneObjHolder*);
    void setStageName(const char*);
    void setLabel(const char*);
    void copy(const QuestInfo*);
    void end();
    bool isEqual(const QuestInfo*) const;

    al::SceneObjHolder* getSceneObjHolder() const override { return mSceneObjHolder; }

    s32 getQuestNo() const { return mQuestNo; }
    const sead::Vector3f& getTrans() const { return mTrans; }
    const sead::FixedSafeString<128>& getMapLabel() const { return mMapLabel; }

private:
    s32 mQuestNo;
    sead::Vector3f mTrans;
    bool mIsValid;
    bool mIsMainQuest;
    u8 padding[6];
    al::SceneObjHolder* mSceneObjHolder;
    sead::FixedSafeString<128> mMapLabel;
    sead::FixedSafeString<128> mObjId;
    bool mIsSingle;
    u8 padding2[7];
    sead::FixedSafeString<128> mPlacementId;
    sead::FixedSafeString<128> mStageName;
};

static_assert(sizeof(QuestInfo) == 0x290);
