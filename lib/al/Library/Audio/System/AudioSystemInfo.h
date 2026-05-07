#pragma once

#include <heap/seadHeap.h>

namespace al {
class CollisionCodeList;
class AudioEffectDataBase;
class AudioSoundArchiveInfo;
class BgmDataBase;
class SeDataBase;

struct AudioSystemInfo {
    AudioSystemInfo();

    void* _0;
    AudioEffectDataBase* audioEffectDataBase;
    AudioSoundArchiveInfo* audioSoundArchiveInfo;
    SeDataBase* seDataBase;
    BgmDataBase* bgmDataBase;
};

struct AudioSystemInitInfo {
void setHeapSize(sead::Heap* heap){
    if(!heap){
        systemHeapSize=0;
        return;
    }
    systemHeapSize=heap->getFreeSize() ;
}
    void setMaterialCode(CollisionCodeList* a,CollisionCodeList*b){
        materialCodeList=a;
        materialCodePrefixList=b;
    }

    const char* seDataName= nullptr;
    const char* seBgmName= nullptr;
    bool unk1 = true;
    bool unk2 = true;//af
    bool isBgmOnSameMixIndex = false;//ae
    f32 masterVolume = 1.0f;//aa
    f32 dockedVolume = 1.0f;//a6
    f32 undockedVolume= 1.0f;//a2
    s32 systemHeapSize=-1;//9e
    s32 unk4=0;//9a
    bool useAudioMaximizer= false;//99
    bool changeInputBgmChannelVolume= false;//98
    f32 monoVolume= 1.0f;
    f32 stereoVolume= 1.0f;
    CollisionCodeList* materialCodeList = nullptr;
    CollisionCodeList* materialCodePrefixList = nullptr;
    s32 cacheSizePerSound=0;
};

}  // namespace al
