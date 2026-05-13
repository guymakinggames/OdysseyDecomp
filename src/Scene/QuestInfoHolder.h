#pragma once

#include <basis/seadTypes.h>

class QuestInfo;

namespace al {
class IUseSceneObjHolder;
}  // namespace al

namespace rs {
const QuestInfo* const* getActiveQuestList(const al::IUseSceneObjHolder*);
s32 getActiveQuestNum(const al::IUseSceneObjHolder*);
s32 getActiveQuestNumForMap(const al::IUseSceneObjHolder*);
s32 getActiveQuestNo(const al::IUseSceneObjHolder*);
const char* getActiveQuestLabel(const al::IUseSceneObjHolder*);
}  // namespace rs
