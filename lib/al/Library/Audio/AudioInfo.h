#pragma once

#include <basis/seadTypes.h>
#include <container/seadPtrArray.h>

#include "Library/Audio/System/System.h"

namespace al {
class ByamlIter;
class AudioInfoListCreateFunctorBase;

template <typename T>
class AudioInfoList;

class AudioInfoListCreateFunctorBase {
public:
    AudioInfoListCreateFunctorBase() = default;

    virtual bool tryCreateAudioInfoAndSetToList(const ByamlIter&) = 0;
};

template <typename T>
class AudioInfoListCreateFunctor : public AudioInfoListCreateFunctorBase {
public:
    using CreateInfoFunc = T* (*)(const ByamlIter&);

    AudioInfoListCreateFunctor(AudioInfoList<T>* list, CreateInfoFunc fun)
        : mCreateInfoFunc(fun), mAudioInfoList(list) {}

    bool tryCreateAudioInfoAndSetToList(const ByamlIter& iter) override {
        T* info = mCreateInfoFunc(iter);

        if (!info)
            return false;

        return mAudioInfoList->setInfo(info);
    }

private:
    CreateInfoFunc mCreateInfoFunc;
    AudioInfoList<T>* mAudioInfoList;
};

template <typename T>
class AudioInfoList {
public:
    static s32 compareInfoAndKey(const T* info, const char* key) { return strcmp(info->name, key); }

    AudioInfoList() = default;

    void init(s32 listSize) {
        mIsLinearSearch = false;
        mList = new sead::PtrArray<const T>();
        mList->allocBuffer((listSize == 0) ? 1 : listSize, nullptr);
    }

    bool setInfo(const T* audioInfo) const { return mList->pushBack(audioInfo); }

    sead::PtrArray<const T>* getList() const { return mList; }

    s32 getInfoNum() const { return mList->size(); }

    const T* getInfo(s32 index) const { return mList->unsafeAt(index); }

    void eraseInfo(s32 index) const { mList->erase(index, 1); }

    void clear() const { mList->clear(); }

    void sort() const {
        if (mList->size() >= 10)
            mList->heapSort(T::compareInfo);
        else
            mList->sort(T::compareInfo);
    }

protected:
    sead::PtrArray<const T>* mList;
    bool mIsLinearSearch;
};

template <typename T>
class AudioInfoListWithParts : public AudioInfoList<T> {
public:
    AudioInfoListWithParts() = default;

    s32 tryGetInfoIndex(const char* key) const;
    const T* tryFindInfo(const char* key) const;

    void init(s32 listSize, s32 maxNumParts) {
        AudioInfoList<T>::init(listSize);

        mParts = nullptr;
        if (maxNumParts != 0) {
            mParts = new sead::PtrArray<AudioInfoList<T>>();
            mParts->allocBuffer(maxNumParts, nullptr);
        }
    }

    void sort() const {
        AudioInfoList<T>::sort();

        for (s32 i = 0; i < getPartsSize(); i++)
            mParts->unsafeAt(i)->sort();
    }

    s32 getPartsSize() const {
        if (!mParts)
            return 0;
        return mParts->size();
    }

    s32 getInfoNum() const {
        const sead::PtrArray<const T>* list = AudioInfoList<T>::mList;
        sead::PtrArray<AudioInfoList<T>>* parts = mParts;
        s32 infoNum = list->size();

        if (parts) {
            s32 partsInfoNum = 0;
            for (s32 i = 0; i < getPartsSize(); i++)
                partsInfoNum += parts->unsafeAt(i)->getInfoNum();
            infoNum += partsInfoNum;
        }

        return infoNum;
    }

    const T* getInfo(s32 index) const {
        s32 listSize = AudioInfoList<T>::getInfoNum();
        if (index < listSize) {
            if (index < 0)
                return nullptr;
            return AudioInfoList<T>::getInfo(index);
        }

        if (!mParts)
            return nullptr;

        index -= listSize;
        for (s32 i = 0; i < mParts->size(); i++) {
            AudioInfoList<T>* part = mParts->unsafeAt(i);
            s32 partSize = part->getInfoNum();
            if (index < partSize) {
                if (index < 0)
                    return nullptr;
                return part->getInfo(index);
            }
            index -= partSize;
        }

        return nullptr;
    }

    const T* getInfoUnsafe(s32 index) const {
        sead::PtrArray<const T>* list = AudioInfoList<T>::mList;
        s32 listSize = list->size();
        if (index < listSize)
            return list->unsafeAt(index);

        sead::PtrArray<AudioInfoList<T>>* parts = mParts;
        index -= listSize;
        for (s32 i = 0;; i++) {
            AudioInfoList<T>* part = parts->unsafeAt(i);
            sead::PtrArray<const T>* partList = part->getList();
            s32 partSize = partList->size();
            if (index < partSize)
                return partList->unsafeAt(index);
            index -= partSize;
        }
    }

    void eraseInfo(s32 index) const {
        s32 listSize = AudioInfoList<T>::getInfoNum();
        if (index < listSize) {
            AudioInfoList<T>::eraseInfo(index);
            return;
        }

        if (!mParts)
            return;

        index -= listSize;
        for (s32 i = 0; i < mParts->size(); i++) {
            AudioInfoList<T>* part = mParts->unsafeAt(i);
            s32 partSize = part->getInfoNum();
            if (index < partSize) {
                part->eraseInfo(index);
                return;
            }
            index -= partSize;
        }
    }

    void clear() const { AudioInfoList<T>::clear(); }

private:
    sead::PtrArray<AudioInfoList<T>>* mParts;
};

template <typename T>
AudioInfoListWithParts<T>* createAudioInfoList(const ByamlIter& iter, s32 maxNumParts) {
    AudioInfoListWithParts<T>* audioInfoList = new AudioInfoListWithParts<T>;

    s32 listSize = alAudioInfoListFunction::getCreateAudioInfoListSize(iter, 0);
    audioInfoList->init(listSize, maxNumParts);

    AudioInfoListCreateFunctor<T> functor(audioInfoList, T::createInfo);
    alAudioInfoListFunction::createAudioInfoAndSetToList(&functor, iter);
    audioInfoList->sort();

    return audioInfoList;
}

template <typename T>
bool trySetAudioInfo(const AudioInfoListWithParts<T>* audioInfoList, const T* audioInfo,
                     bool isUnsorted) {
    if (!audioInfoList || !audioInfo)
        return false;

    if (!audioInfoList->setInfo(audioInfo))
        return false;

    if (!isUnsorted)
        audioInfoList->sort();

    return true;
}

template <typename T>
const T* tryFindAudioInfo(const AudioInfoListWithParts<T>* audioInfoList, const char* name) {
    if (!audioInfoList || !name)
        return nullptr;

    return audioInfoList->tryFindInfo(name);
}

}  // namespace al
