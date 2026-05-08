#include "Library/Se/SeDemoEventController.h"

#include <container/seadPtrArray.h>

#include "Library/Audio/AudioInfo.h"
#include "Library/Audio/System/AudioKeeperFunction.h"
#include "Library/Base/Macros.h"
#include "Library/Se/SeDemoProcInfo.h"
#include "Library/Se/SeFunction.h"
#include "Library/Se/SeKeeper.h"

#include "Project/Action/InitResourceDataActionAnim.h"

namespace {

s32 compareSeName(const void* lhs, const void* rhs);
void processPlaySe(const al::SeDemoPlaySeInfo* info, al::SeKeeper* seKeeper,
                   al::SeDemoPlayingSeNameList* playingSeNameList);

template <typename T>
ALWAYS_INLINE s32 getInfoNum(const al::AudioInfoListWithParts<T>* list) {
    if (!list)
        return 0;
    return list->getInfoNum();
}

template <typename T>
ALWAYS_INLINE const T* tryGetInfo(const al::AudioInfoListWithParts<T>* list, s32 index) {
    if (!list)
        return nullptr;
    return list->getInfo(index);
}

}  // namespace

namespace al {

class SeDemoPlayingSeNameList : public sead::PtrArray<const char> {
public:
    s32 binarySearchByName(const char* name) const {
        return PtrArrayImpl::binarySearch(name, compareSeName);
    }

    void heapSortByName() { PtrArrayImpl::heapSort(compareSeName); }
};

SeDemoEventController::SeDemoEventController(AudioDirector* director) : mAudioDirector(director) {
    AudioInfoListWithParts<SeDemoSituationInfo>* situationInfoList =
        new AudioInfoListWithParts<SeDemoSituationInfo>;
    situationInfoList->init(10, 0);
    mSituationInfoList = situationInfoList;

    AudioInfoListWithParts<SeDemoPauseInfo>* pauseInfoList =
        new AudioInfoListWithParts<SeDemoPauseInfo>;
    pauseInfoList->init(10, 0);
    mPauseInfoList = pauseInfoList;

    AudioInfoListWithParts<SeDemoListenerPoserInfo>* listenerPoserInfoList =
        new AudioInfoListWithParts<SeDemoListenerPoserInfo>;
    listenerPoserInfoList->init(10, 0);
    mListenerPoserInfoList = listenerPoserInfoList;

    mPlayingSeNameList = new SeDemoPlayingSeNameList();
    mPlayingSeNameList->allocBuffer(10, nullptr);
}

void SeDemoEventController::startEvent(AudioInfoListWithParts<SeDemoProcInfo>* demoProcInfo) {
    mDemoProcInfo = demoProcInfo;
    mCurFrame = 0;
    mLastPlaySeName = nullptr;
    mLastPlaySeFrame = 0;
}

void SeDemoEventController::endEvent(AudioInfoListWithParts<SeDemoProcInfo>* demoProcInfo,
                                     bool isStopSe) {
    if (demoProcInfo)
        for (s32 i = 0; i < getInfoNum(demoProcInfo); i++)
            procDemoEvent(demoProcInfo->getInfo(i));

    for (s32 i = 0; i < getInfoNum(mSituationInfoList); i++) {
        const SeDemoSituationInfo* info = mSituationInfoList->getInfo(i);
        alSeFunction::endSituation(mAudioDirector, info->situationName, -1);
    }
    if (mSituationInfoList)
        mSituationInfoList->clear();

    for (s32 i = 0; i < getInfoNum(mPauseInfoList); i++) {
        const SeDemoPauseInfo* info = mPauseInfoList->getInfo(i);
        alAudioSystemFunction::pauseAllSeForDemo(mAudioDirector, false, info->fadeInFrameNum);
    }
    if (mPauseInfoList)
        mPauseInfoList->clear();

    for (s32 i = 0; i < getInfoNum(mListenerPoserInfoList); i++) {
        const SeDemoListenerPoserInfo* info = mListenerPoserInfoList->getInfo(i);
        alSeFunction::endListenerPoser(mAudioDirector, info->poserName, info->fadeInFrameNum);
    }
    if (mListenerPoserInfoList)
        mListenerPoserInfoList->clear();

    if (isStopSe) {
        for (s32 i = 0; i < mPlayingSeNameList->size(); i++) {
            const char* name = mPlayingSeNameList->unsafeAt(i);
            SeKeeper* seKeeper = mSeKeeper;
            seKeeper->stopSe(name, 0, true, nullptr);
        }

        for (s32 i = 0; i < getInfoNum(mDemoProcInfo); i++) {
            const SeDemoProcInfo* info = mDemoProcInfo->getInfoUnsafe(i);
            if (SeDemoProcInfo::isPlaySe(info->name)) {
                const SeDemoPlaySeInfo* playInfo =
                    static_cast<const SeDemoPlaySeInfo*>(info);  // TODO: verify cast
                if (playInfo->triggerFrame < 0 || playInfo->triggerFrame > mCurFrame) {
                    if (playInfo->isStartSeEvenIfDemoSkip)
                        processPlaySe(playInfo, mSeKeeper, mPlayingSeNameList);
                }
            }
        }
    }

    mPlayingSeNameList->clear();
    mDemoProcInfo = nullptr;
    mCurFrame = 0;
}

void SeDemoEventController::procDemoEvent(const SeDemoProcInfo* demoProcInfo) {
    const char* name = demoProcInfo->name;

    if (SeDemoProcInfo::isChangeSituation(name)) {
        const SeDemoSituationInfo* info =
            static_cast<const SeDemoSituationInfo*>(demoProcInfo);  // TODO: verify cast
        AudioDirector* director = mAudioDirector;
        AudioInfoListWithParts<SeDemoSituationInfo>* situationInfoList = mSituationInfoList;
        alSeFunction::startSituation(director, info->situationName, -1);

        if (situationInfoList && situationInfoList->setInfo(info))
            situationInfoList->sort();
        return;
    } else if (SeDemoProcInfo::isPause(name)) {
        const SeDemoPauseInfo* info =
            static_cast<const SeDemoPauseInfo*>(demoProcInfo);  // TODO: verify cast
        AudioDirector* director = mAudioDirector;
        AudioInfoListWithParts<SeDemoPauseInfo>* pauseInfoList = mPauseInfoList;
        alAudioSystemFunction::pauseAllSeForDemo(director, true, info->fadeOutFrameNum);

        if (pauseInfoList && pauseInfoList->setInfo(info))
            pauseInfoList->sort();
        return;
    } else if (SeDemoProcInfo::isStopOneShot(name)) {
        const SeDemoStopOneShotInfo* info =
            static_cast<const SeDemoStopOneShotInfo*>(demoProcInfo);  // TODO: verify cast
        return alSeFunction::stopAllOneShotSe(mAudioDirector, info->fadeOutFrameNum, nullptr);
    } else if (SeDemoProcInfo::isChnageListenerPoser(name)) {
        const SeDemoListenerPoserInfo* info =
            static_cast<const SeDemoListenerPoserInfo*>(demoProcInfo);  // TODO: verify cast
        AudioDirector* director = mAudioDirector;
        AudioInfoListWithParts<SeDemoListenerPoserInfo>* listenerPoserInfoList =
            mListenerPoserInfoList;
        alSeFunction::startListenerPoser(director, info->poserName, info->fadeOutFrameNum);

        if (listenerPoserInfoList && listenerPoserInfoList->setInfo(info))
            listenerPoserInfoList->sort();
        return;
    } else if (SeDemoProcInfo::isPlaySe(name)) {
        const SeDemoPlaySeInfo* info =
            static_cast<const SeDemoPlaySeInfo*>(demoProcInfo);  // TODO: verify cast
        return processPlaySe(info, mSeKeeper, mPlayingSeNameList);
    }
}

}  // namespace al

namespace {

void processPlaySe(const al::SeDemoPlaySeInfo* info, al::SeKeeper* seKeeper,
                   al::SeDemoPlayingSeNameList* playingSeNameList) {
    if (!seKeeper)
        return;

    if (info->isStopSe) {
        seKeeper->stopSe(info->playName, info->fadeOutFrameNum, true, nullptr);
        if (playingSeNameList) {
            s32 index = playingSeNameList->binarySearchByName(info->playName);
            if (index >= 0) {
                playingSeNameList->erase(index, 1);
                playingSeNameList->heapSortByName();
            }
        }
    } else {
        seKeeper->requestPlaySe(info->playName, 0.0f, nullptr, nullptr, nullptr, false, nullptr,
                                nullptr);
        if (!info->isNotStopSeEvenIfDemoSkip) {
            s32 index = playingSeNameList->binarySearchByName(info->playName);
            if (index < 0 && playingSeNameList && !playingSeNameList->isFull()) {
                playingSeNameList->pushBack(info->playName);
                playingSeNameList->heapSortByName();
            }
        }
    }
}

}  // namespace

namespace al {

void SeDemoEventController::update(s32 frame) {
    f32 frameF = frame;
    mCurFrame = frame;

    for (s32 i = 0; i < getInfoNum(mDemoProcInfo); i++) {
        const SeDemoProcInfo* info = tryGetInfo(mDemoProcInfo, i);
        if (alAnimFunction::checkPass(frameF, -1.0f, 1.0f, false, info->triggerFrame)) {
            procDemoEvent(info);
            if (SeDemoProcInfo::isPlaySe(info->name)) {
                const SeDemoPlaySeInfo* playInfo =
                    static_cast<const SeDemoPlaySeInfo*>(info);  // TODO: verify cast
                mLastPlaySeName = playInfo->playName;
                mLastPlaySeFrame = frame;
            }
        }
    }

    if (getInfoNum(mSituationInfoList) > 0) {
        for (s32 i = getInfoNum(mSituationInfoList) - 1; i >= 0; i--) {
            const SeDemoSituationInfo* info = tryGetInfo(mSituationInfoList, i);
            if (info->endFrame >= 0 &&
                alAnimFunction::checkPass(frameF, -1.0f, 1.0f, false, info->endFrame)) {
                alSeFunction::endSituation(mAudioDirector, info->situationName, -1);
                AudioInfoListWithParts<SeDemoSituationInfo>* situationInfoList = mSituationInfoList;
                if (situationInfoList) {
                    situationInfoList->eraseInfo(i);
                    situationInfoList->sort();
                }
            }
        }
    }

    if (getInfoNum(mPauseInfoList) > 0) {
        for (s32 i = getInfoNum(mPauseInfoList) - 1; i >= 0; i--) {
            const SeDemoPauseInfo* info = tryGetInfo(mPauseInfoList, i);
            if (info->endFrame >= 0 &&
                alAnimFunction::checkPass(frameF, -1.0f, 1.0f, false, info->endFrame)) {
                alAudioSystemFunction::pauseAllSeForDemo(mAudioDirector, false,
                                                         info->fadeInFrameNum);
                AudioInfoListWithParts<SeDemoPauseInfo>* pauseInfoList = mPauseInfoList;
                if (pauseInfoList) {
                    pauseInfoList->eraseInfo(i);
                    pauseInfoList->sort();
                }
            }
        }
    }

    if (getInfoNum(mListenerPoserInfoList) > 0) {
        for (s32 i = getInfoNum(mListenerPoserInfoList) - 1; i >= 0; i--) {
            const SeDemoListenerPoserInfo* info = tryGetInfo(mListenerPoserInfoList, i);
            if (info->endFrame >= 0 &&
                alAnimFunction::checkPass(frameF, -1.0f, 1.0f, false, info->endFrame)) {
                alSeFunction::endListenerPoser(mAudioDirector, info->poserName,
                                               info->fadeInFrameNum);
                AudioInfoListWithParts<SeDemoListenerPoserInfo>* listenerPoserInfoList =
                    mListenerPoserInfoList;
                if (listenerPoserInfoList) {
                    listenerPoserInfoList->eraseInfo(i);
                    listenerPoserInfoList->sort();
                }
            }
        }
    }
}

}  // namespace al

namespace {

s32 compareSeName(const void* lhs, const void* rhs) {
    if (!lhs)
        return rhs != nullptr;
    if (rhs)
        return strcmp(static_cast<const char*>(lhs),   // TODO: verify cast
                      static_cast<const char*>(rhs));  // TODO: verify cast
    return -1;
}

}  // namespace
