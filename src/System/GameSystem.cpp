#include "System/GameSystem.h"

#include <heap/seadExpHeap.h>
#include <nn/audio.h>
#include <nn/friends.h>

#include "Library/Audio/AudioInfo.h"
#include "Library/Audio/AudioLoadGroup.h"
#include "Library/Audio/System/AudioKeeperFunction.h"
#include "Library/Audio/System/AudioSystemInfo.h"
#include "Library/Base/StringUtil.h"
#include "Library/Collision/CollisionCodeFunction.h"
#include "Library/Controller/GamePadSystem.h"
#include "Library/Controller/GamePadWaveVibrationData.h"
#include "Library/Effect/EffectSystem.h"
#include "Library/Framework/GameFrameworkNx.h"
#include "Library/Layout/LayoutSystem.h"
#include "Library/LiveActor/ActorSensorUtil.h"
#include "Library/Memory/HeapUtil.h"
#include "Library/Message/IUseMessageSystem.h"
#include "Library/Message/MessageHolder.h"
#include "Library/Message/MessageSystem.h"
#include "Library/Nerve/NerveSetupUtil.h"
#include "Library/Network/AccountHolder.h"
#include "Library/Network/HtmlViewer.h"
#include "Library/Network/NetworkSystem.h"
#include "Library/Resource/ResourceFunction.h"
#include "Library/Se/Info/SeAudioInfo.h"
#include "Library/Se/Info/SeEmitterInfo.h"
#include "Library/Se/Info/SeShapeInfo3DPoint.h"
#include "Library/Sequence/Sequence.h"
#include "Library/System/GameSystemInfo.h"

#include "Application.h"
#include "GameConfigData.h"
#include "ProjectNfpDirector.h"
#include "Sequence/HakoniwaSequence.h"
#include "Sequence/SequenceFactory.h"
#include "System/GameDataHolder.h"

namespace {
NERVE_IMPL(GameSystem, Play);

NERVES_MAKE_STRUCT(GameSystem, Play);

class MessageSystemUser : public al::IUseMessageSystem {
public:
    MessageSystemUser(const al::MessageSystem* messageSystem) : mMessageSystem(messageSystem) {}

    const al::MessageSystem* getMessageSystem() const override { return mMessageSystem; }

private:
    const al::MessageSystem* mMessageSystem;
};
}  // namespace

GameSystem::GameSystem() : NerveExecutor("ゲームシステム") {}

GameSystem::~GameSystem() {
    alAudioSystemFunction::destroyAudioResource(
        "システム常駐", mAudioInfoList,
        alAudioSystemFunction::getSeadAudioPlayerForSe(mAudioSystem),
        alAudioSystemFunction::getSeadAudioPlayerForBgm(mAudioSystem));
    mAudioSystem->removeAudiioFrameProccess(mWaveVibrationHolder);
    mAudioSystem->finalize();
    if (mAccountHolder)
        delete (mAccountHolder);
    mSystemInfo->nfpDirector->finalize();
}

void GameSystem::init() {
    mSystemInfo = new al::GameSystemInfo;
    mSystemInfo->drawSystemInfo = Application::instance()->getDrawSystemInfo();
    mGameConfigData = new GameConfigData;
    initNerve(&NrvGameSystem.Play);
    mAccountHolder = Application::instance()->getAccountHolder();
    nn::friends::Initialize();
    mNetworkSystem = new al::NetworkSystem(mAccountHolder->getUserHandle(), true);
    mNetworkSystem->requestSystemInitialize();
    mSystemInfo->networkSystem = mNetworkSystem;
    mHtmlViewer = new al::HtmlViewer;
    mSystemInfo->htmlViewer = mHtmlViewer;
    mNfpDirector = new ProjectNfpDirector;
    mSystemInfo->nfpDirector = mNfpDirector;
    mNfpDirector->initialize();

    sead::ExpHeap* heap = sead::ExpHeap::create(
        0x4600000, "", nullptr, 8, sead::Heap::HeapDirection::cHeapDirection_Forward, false);
    al::addNamedHeap(heap, "EffectSystemHeap");

    al::EffectSystem* effectSystem =
        al::EffectSystem::createSystem(mSystemInfo->drawSystemInfo->drawContext, heap);
    mSystemInfo->effectSystem = effectSystem;

    mSystemInfo->layoutSystem = new al::LayoutSystem;
    mSystemInfo->messageSystem = new al::MessageSystem;

    al::AudioSystemInitInfo audioSystemInitInfo{
        .seBgmName = nullptr,
        .unk1 = true,
        .unk2 = true,
        .isBgmOnSameMixIndex = false,
        .masterVolume = 1.0f,
        .dockedVolume = 1.0f,
        .unk4 = 0,
        .useAudioMaximizer = false,
        .changeInputBgmChannelVolume = false,
        .cacheSizePerSound = 0,
        .undockedVolume = 1.0f,
        .systemHeapSize = -1,
        .monoVolume = 1.0f,
        .stereoVolume = 1.0f,
        .materialCodeList = nullptr,
        .materialCodePrefixList = nullptr,
    };

    al::CollisionCodeList* materialCode =
        alCollisionCodeFunction::tyrCreateCollisionCodeList("MaterialCode");
    al::CollisionCodeList* materialCodePrefix =
        alCollisionCodeFunction::tyrCreateCollisionCodeList("MaterialCodePrefix");

    mSystemInfo->effectSystem->setMaterialCodeList(materialCode);
    mSystemInfo->effectSystem->setMaterialCodePrefix(materialCodePrefix);

    audioSystemInitInfo = {
        .dockedVolume = 0.401f,
        .undockedVolume = 1.0f,
        .unk2 = true,
        .isBgmOnSameMixIndex = true,
        .changeInputBgmChannelVolume = true,
        .seDataName = "SeData",
        .seBgmName = "BgmData",
        .monoVolume = 0.3f,
        .stereoVolume = 1.061f,
        .cacheSizePerSound = 0x40000,
        .materialCodeList = materialCode,
        .materialCodePrefixList = materialCodePrefix,
    };

    sead::Heap* audioHeap = al::tryFindNamedHeap("AudioHeap");
    u64 audioHeapSize = 0;
    if (audioHeap)
        audioHeapSize = audioHeap->getSize();
    mAudioSystem = new al::AudioSystem;
    mAudioSystem->init(audioSystemInitInfo);

    al::SeadAudioPlayer* audioPlayerForSe =
        alAudioSystemFunction::getSeadAudioPlayerForSe(mAudioSystem);
    al::SeadAudioPlayer* audioPlayerForBgm =
        alAudioSystemFunction::getSeadAudioPlayerForBgm(mAudioSystem);

    al::setAudioPlayerToResourceSystem(audioPlayerForSe, audioPlayerForBgm);
    mAudioInfoList = new al::AudioInfoListWithParts<al::AudioResourceLoadGroupInfo>;

    al::AudioResourceLoadGroupInfo* audioResourceLoadGroupInfo = new al::AudioResourceLoadGroupInfo;
    audioResourceLoadGroupInfo->name = "システム常駐";
    al::AudioLoadGroupList* audioLoadGroupList = new al::AudioLoadGroupList;
    audioLoadGroupList->unk2 = nullptr;
    audioLoadGroupList->unk1 = new sead::PtrArray<al::AudioResourceLoadInfo>(0, nullptr);
    audioLoadGroupList->unk1->allocBuffer(1, nullptr, 8);
    audioLoadGroupList->resourceLoadInfos = nullptr;
    audioResourceLoadGroupInfo->unk1 = audioLoadGroupList;

    audioLoadGroupList = new al::AudioLoadGroupList;
    audioLoadGroupList->unk2 = nullptr;
    audioLoadGroupList->unk1 = new sead::PtrArray<al::AudioResourceLoadInfo>(0, nullptr);
    audioLoadGroupList->unk1->allocBuffer(1, nullptr, 8);
    audioResourceLoadGroupInfo->unk1->resourceLoadInfos = nullptr;
    audioResourceLoadGroupInfo->unk2 = audioLoadGroupList;

    al::AudioResourceLoadInfo* audioResourceLoadInfo = new al::AudioResourceLoadInfo;
    audioResourceLoadInfo->name = "SeResourceStdSystem";
    audioResourceLoadInfo->unk1 = false;
}

void GameSystem::setPadName() {
    MessageSystemUser messageSystemUser = MessageSystemUser{mSystemInfo->messageSystem};
    mGamePadSystem->setPadName(
        0, al::getSystemMessageString(&messageSystemUser, "ControllerApplet", "SeperatePlayer1"));
    mGamePadSystem->setPadName(
        1, al::getSystemMessageString(&messageSystemUser, "ControllerApplet", "SeperatePlayer2"));
}

bool GameSystem::tryChangeSequence(const char* name) {
    if (mSequence) {
        if (!mSequence->isDisposable())
            return false;
        if (mSequence)
            delete (mSequence);
        mSequence = nullptr;
        al::freeAllSequenceHeap();
    }
    setPadName();

    sead::ScopedCurrentHeapSetter heap(al::getSequenceHeap());

    al::Sequence* sequence = SequenceFactory::createSequence(name);
    if (!sequence)
        return false;

    al::SequenceInitInfo initInfo(mSystemInfo);
    sequence->init(initInfo);
    mSequence = sequence;

    if (al::isEqualString(name, "HakoniwaSequence")) {
        GameDataHolder* gameDataHolder =
            static_cast<HakoniwaSequence*>(mSequence)->getGameDataHolder();
        gameDataHolder->setSeparatePlay(mIsSinglePlay);
        if (mIsSequenceSetupIncomplete) {
            *gameDataHolder->getGameConfigData() = *mGameConfigData;
            mIsSequenceSetupIncomplete = false;
        }
    }

    return true;
}

void GameSystem::drawMain() {
    Application::instance()->getGameFramework()->clearFrameBuffer();
    mSystemInfo->layoutSystem->beginDraw();
    if (mSequence)
        mSequence->drawMain();

    mSystemInfo->layoutSystem->endDraw();
}

void GameSystem::exePlay() {
    mNfpDirector->update();
    if (mSequence)
        mSequence->update();
}
