#pragma once

#include <basis/seadTypes.h>

namespace sead {
template <typename T>
class PtrArray;
}  // namespace sead

namespace al {
template <typename T>
class AudioInfoListWithParts;
class AudioDirector;
class SeKeeper;
class SeDemoPlayingSeNameList;
struct SeDemoListenerPoserInfo;
struct SeDemoPauseInfo;
struct SeDemoProcInfo;
struct SeDemoSituationInfo;

class SeDemoEventController {
public:
    SeDemoEventController(AudioDirector* director);

    void startEvent(AudioInfoListWithParts<SeDemoProcInfo>* demoProcInfo);
    void endEvent(AudioInfoListWithParts<SeDemoProcInfo>* demoProcInfo, bool isStopSe);
    void procDemoEvent(const SeDemoProcInfo* demoProcInfo);
    void update(s32 frame);

private:
    AudioDirector* mAudioDirector = nullptr;
    AudioInfoListWithParts<SeDemoProcInfo>* mDemoProcInfo = nullptr;
    AudioInfoListWithParts<SeDemoSituationInfo>* mSituationInfoList = nullptr;
    AudioInfoListWithParts<SeDemoPauseInfo>* mPauseInfoList = nullptr;
    AudioInfoListWithParts<SeDemoListenerPoserInfo>* mListenerPoserInfoList = nullptr;
    SeKeeper* mSeKeeper = nullptr;
    SeDemoPlayingSeNameList* mPlayingSeNameList = nullptr;
    s32 mCurFrame = 0;
    const char* mLastPlaySeName = nullptr;
    s32 mLastPlaySeFrame = 0;
};

static_assert(sizeof(SeDemoEventController) == 0x50);
}  // namespace al
