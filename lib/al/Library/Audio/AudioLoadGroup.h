#pragma once
#include <container/seadPtrArray.h>

#include "Library/Audio/AudioInfo.h"

namespace al {

struct AudioResourceLoadInfo {
    AudioResourceLoadInfo();

    void setName(const char* named,bool kek){
        name=named;
        isBgm=kek;
    }

    const char* name=nullptr;
    bool isBgm=false;

    static sead::PtrArray<AudioResourceLoadInfo>::CompareCallback compareInfo;
};

struct AudioLoadGroupList {
    sead::PtrArray<AudioResourceLoadInfo>* unk1;
    void* unk2;
    sead::PtrArray<AudioResourceLoadInfo>* resourceLoadInfos;
};

struct AudioResourceLoadGroupInfo {
    AudioResourceLoadGroupInfo();

    static AudioResourceLoadGroupInfo* createInfo(const ByamlIter& iter);
    static s32 compareInfo(const AudioResourceLoadGroupInfo* lhs, const AudioResourceLoadGroupInfo* rhs);

    const char* name=nullptr;
    al::AudioInfoListWithParts<al::AudioResourceLoadInfo>* userManagementGroupLoadInfoList=nullptr;
    al::AudioInfoListWithParts<al::AudioResourceLoadInfo>* addonSoundArchiveLoadInfoList=nullptr;
};

} // namespace al
