#include "Scene/StageSceneStateOption.h"

#include "Library/Base/StringUtil.h"
#include "Library/Layout/LayoutActionFunction.h"
#include "Library/Layout/LayoutActorUtil.h"
#include "Library/Layout/LayoutInitInfo.h"
#include "Library/Message/LanguageUtil.h"
#include "Library/Message/MessageHolder.h"
#include "Library/Nerve/NerveSetupUtil.h"
#include "Library/Nerve/NerveUtil.h"
#include "Library/Play/Layout/RollParts.h"
#include "Library/Play/Layout/SimpleLayoutAppearWaitEnd.h"
#include "Library/Play/Layout/WindowConfirm.h"
#include "Library/Scene/Scene.h"

#include "Layout/CommonVerticalList.h"
#include "Layout/FooterParts.h"
#include "Layout/SimpleLayoutMenu.h"
#include "Layout/WindowConfirmData.h"
#include "System/GameConfigData.h"
#include "System/GameDataFile.h"
#include "System/GameDataFunction.h"
#include "System/GameDataHolder.h"
#include "System/GameDataHolderAccessor.h"
#include "System/GameDataUtil.h"
#include "System/SaveDataAccessFunction.h"
#include "Util/InputSeparator.h"
#include "Util/ScenePrepoFunction.h"
#include "Util/StageInputFunction.h"

namespace {
NERVE_IMPL(StageSceneStateOption, ModeSelectSelecting);
NERVE_IMPL(StageSceneStateOption, DataManager);
NERVE_END_IMPL(StageSceneStateOption, Config);
NERVE_IMPL(StageSceneStateOption, LanguageSetting);
NERVE_IMPL(StageSceneStateOption, SaveDataSelecting);
NERVE_IMPL(StageSceneStateOption, LoadDataSelecting);
NERVE_IMPL(StageSceneStateOption, DeleteDataSelecting);
NERVE_IMPL(StageSceneStateOption, OptionTop);
NERVE_IMPL(StageSceneStateOption, ModeSelectSelectingByHelp);
NERVE_IMPL(StageSceneStateOption, LanguageSettingConfirmYesNo);
NERVE_IMPL(StageSceneStateOption, ModeSelectConfirmEnd);
NERVE_IMPL(StageSceneStateOption, Close);
NERVE_IMPL(StageSceneStateOption, ModeSelectConfirmYesNo);
NERVE_IMPL(StageSceneStateOption, SaveDataConfirmYesNo);
NERVE_IMPL(StageSceneStateOption, SaveDataSaving);
NERVE_IMPL(StageSceneStateOption, SaveDataSaved);
NERVE_IMPL(StageSceneStateOption, LoadDataConfirmNg);
NERVE_IMPL(StageSceneStateOption, LoadDataConfirmYesNo);
NERVE_IMPL(StageSceneStateOption, LoadDataSaving);
NERVE_IMPL(StageSceneStateOption, DeleteDataConfirmNg);
NERVE_IMPL(StageSceneStateOption, DeleteDataConfirmYesNo);
NERVE_IMPL(StageSceneStateOption, DeleteDataDeleting);
NERVE_IMPL(StageSceneStateOption, DeleteDataDeleted);
NERVE_IMPL(StageSceneStateOption, WaitEndAutoSave);
NERVE_IMPL(StageSceneStateOption, WaitEndDecideAnimAndAutoSave);
NERVE_IMPL(StageSceneStateOption, WaitEndDecideAnim);

NERVES_MAKE_NOSTRUCT(StageSceneStateOption, SaveDataSaved, DeleteDataDeleted, WaitEndAutoSave,
                     LanguageSetting);
NERVES_MAKE_STRUCT(StageSceneStateOption, ModeSelectSelecting, DataManager, SaveDataSelecting,
                   LoadDataSelecting, DeleteDataSelecting, OptionTop, ModeSelectSelectingByHelp,
                   LanguageSettingConfirmYesNo, ModeSelectConfirmEnd, Close, ModeSelectConfirmYesNo,
                   SaveDataConfirmYesNo, SaveDataSaving, LoadDataConfirmNg, LoadDataConfirmYesNo,
                   LoadDataSaving, DeleteDataConfirmNg, DeleteDataConfirmYesNo, DeleteDataDeleting,
                   WaitEndAutoSave, WaitEndDecideAnim, WaitEndDecideAnimAndAutoSave, Config,
                   LanguageSetting);

const char16 cSeparatorText[] = u"---";
const char16 cEmptyText[] = {0};
const char* cSaveDataListPane = "TxtContent";

const s32 cConfigLevelTable[] = {-2, -1, 0, 1, 2};

const char* cLanguageNames[] = {"USen", "EUfr", "USfr", "EUde", "EUes", "USes",
                                "EUit", "EUnl", "EUru", "JPja", "CNzh", "TWzh"};

const char* cOptionTopLabels[] = {"PlayMode", "Data", "Config", "Language"};
const char* cModeLabels[] = {"PlayMode_Normal", "PlayMode_Kids"};
const char* cModeExplainLabels[] = {"PlayMode_Normal_Explain", "PlayMode_Kids_Explain"};
const char* cDataLabels[] = {"Data_Save", "Data_Load", "Data_Delete"};
const char* cConfigLabels[] = {"Config_Stick", "Config_Stick_Sensitivity",
                               "Config_Stick", "Config_Stick",
                               "Config_Stick", "Config_Gyro_Sensitivity",
                               "Config_Stick", "Config_Stick",
                               "Config_Stick", "Config_Stick",
                               "Config_Stick"};

struct OptionTopTransition {
    const char* label;
    const al::Nerve* nerve;
};

const OptionTopTransition cOptionTopTransitions[] = {
    {"PlayMode", &NrvStageSceneStateOption.ModeSelectSelecting},
    {"Data", &NrvStageSceneStateOption.DataManager},
    {"Config", &NrvStageSceneStateOption.Config},
    {"Language", &NrvStageSceneStateOption.LanguageSetting},
};

const OptionTopTransition cDataManagerTransitions[] = {
    {"Save", &NrvStageSceneStateOption.SaveDataSelecting},
    {"Load", &NrvStageSceneStateOption.LoadDataSelecting},
    {"Delete", &NrvStageSceneStateOption.DeleteDataSelecting},
};

const char16* emptyText() {
    return cEmptyText;
}

const char16** makeRollTextArray(const al::IUseMessageSystem* user, const char* group,
                                 const char* const* labels, s32 count) {
    const char16** textArray = new const char16*[count];
    for (s32 i = 0; i < count; i++)
        textArray[i] = al::getSystemMessageString(user, group, labels[i]);
    return textArray;
}

sead::WFixedSafeString<512>* makeMessageList(const al::IUseMessageSystem* user, const char* group,
                                             const char* const* labels, s32 count) {
    sead::WFixedSafeString<512>* strings = new sead::WFixedSafeString<512>[count];
    for (s32 i = 0; i < count; i++) {
        const char16* text = al::getSystemMessageString(user, group, labels[i]);
        al::copyMessageWithTag(strings[i].getBuffer(), strings[i].getBufferSize(), text);
    }
    return strings;
}

sead::WFixedSafeString<512>* makePrefixedMessageList(const al::IUseMessageSystem* user,
                                                     const char* group, const char* prefix,
                                                     const char* const* labels, s32 count) {
    sead::WFixedSafeString<512>* strings = new sead::WFixedSafeString<512>[count];
    for (s32 i = 0; i < count; i++) {
        sead::FormatFixedSafeString<64> label("%s_%s", prefix, labels[i]);
        const char16* text = al::getSystemMessageString(user, group, label.cstr());
        al::copyMessageWithTag(strings[i].getBuffer(), strings[i].getBufferSize(), text);
    }
    return strings;
}

void addMessageList(CommonVerticalList* list, sead::WFixedSafeString<512>* strings,
                    const char* paneName) {
    list->addStringData(strings, paneName);
}

s32 gyroLevelToRollIdx(s32 value) {
    if (value == -2)
        return -1;
    if (value == -1)
        return 0;
    if (value == 0)
        return 1;
    if (value == 1)
        return 2;
    if (value == 2)
        return 3;
    return -2;
}

void updateVerticalListInput(CommonVerticalList* list, const al::IUseSceneObjHolder* user) {
    if (rs::isTriggerUiUp(user) && list->getSelectedIdx() == list->getVisibleTopIdx()) {
        list->jumpBottom();
        return;
    }

    if (rs::isTriggerUiDown(user) && list->getSelectedIdx() == list->getDataNum() - 1) {
        list->jumpTop();
        return;
    }

    if (rs::isHoldUiUp(user)) {
        if (rs::isRepeatUiUp(user))
            list->up();
    } else if (rs::isHoldUiDown(user)) {
        if (rs::isRepeatUiDown(user))
            list->down();
    }
}

void updateConfirmInput(al::WindowConfirm* confirm, const al::IUseSceneObjHolder* user) {
    if (rs::isTriggerUiDecide(user)) {
        confirm->tryDecide();
        return;
    }
    if (rs::isTriggerUiCancel(user)) {
        confirm->tryCancel();
        return;
    }
    if (rs::isRepeatUiDown(user)) {
        confirm->tryDown();
        return;
    }
    if (rs::isRepeatUiUp(user))
        confirm->tryUp();
}

bool isConfirmYes(al::WindowConfirm* confirm) {
    return confirm->getPrevSelectionType() == al::WindowConfirm::SelectionType::List00;
}

s32 findCurrentLanguageIndex() {
    const char* language = al::getLanguage();
    for (s32 i = 0; i < 12; i++)
        if (al::isEqualString(cLanguageNames[i], language))
            return i;
    return 0;
}

nn::ui2d::TextureInfo* tryGetSelectedTexture(nn::ui2d::TextureInfo** textures, s32 selected) {
    if (!textures)
        return nullptr;
    if (selected < 0)
        return nullptr;
    if (selected >= 5)
        return nullptr;
    return textures[selected];
}

void setupDataWindow(WindowConfirmData* window, CommonVerticalList* list,
                     nn::ui2d::TextureInfo** textures, s32 selected, const char16* message,
                     const char16* confirm, const char16* cancel) {
    window->setConfirmData(list->getSelectedParts(), tryGetSelectedTexture(textures, selected));
    window->setConfirmMessage(message, confirm, cancel);
}
}  // namespace

StageSceneStateOption::StageSceneStateOption(const char* name, al::Scene* scene,
                                             const al::LayoutInitInfo& info,
                                             FooterParts* footerParts,
                                             GameDataHolder* gameDataHolder, bool isTitle)
    : al::HostStateBase<al::Scene>(name, scene), field_28(nullptr), field_30(nullptr),
      field_38(nullptr), field_40(nullptr), field_48(nullptr), field_50(isTitle), field_51(false),
      mFooterParts(footerParts), field_60(nullptr), field_68(nullptr), field_70(nullptr),
      field_78(nullptr), field_80(nullptr), field_88(nullptr), field_90(nullptr), field_98(nullptr),
      field_a0(nullptr), field_a8(nullptr), mCtrlSettingsList(nullptr), field_b8(nullptr),
      field_c0(nullptr), field_c8(nullptr), field_d0(false), field_d1{}, field_d8(nullptr),
      field_e0(nullptr), field_e8(nullptr), field_f0(nullptr), field_f8(nullptr),
      field_100(nullptr), field_108(nullptr), field_110(nullptr), field_118(nullptr),
      field_120(nullptr), field_128(nullptr), field_130(nullptr), field_138(nullptr), field_140(0),
      field_144(0), field_148(nullptr), field_150(nullptr), field_158(nullptr), field_160(nullptr),
      mLanguage(al::getLanguage()), mScene(scene), mGameDataHolder(gameDataHolder),
      mIsLoadData(false), mMessageSystem(info.getMessageSystem()), mInputSeperator(nullptr) {
    // NONMATCHING: 35 attempts exhausted.
    // Tried setup-order reshaping, helper splitting, string cleanup, and nerve-layout tuning;
    // remaining diff is the large constructor allocation/init sequence.
    field_60 = new al::WindowConfirm(info, "WindowConfirm", "");
    field_60->kill();

    field_68 = new SimpleLayoutMenu("", "OptionSelect", info, nullptr, false);
    field_70 = new CommonVerticalList(field_68, info, true);
    al::setPaneSystemMessage(field_68, "TxtOption", "MenuOption", "OptionTop");
    field_70->initDataNoResetSelected(4);
    addMessageList(field_70,
                   makePrefixedMessageList(this, "MenuOption", "OptionTop", cOptionTopLabels, 4),
                   cSaveDataListPane);

    field_78 = new SimpleLayoutMenu("", "OptionMode", info, nullptr, false);
    field_88 = new SimpleLayoutMenu("", "OptionMode", info, "ByHelp", false);
    field_80 = new CommonVerticalList(field_78, info, true);
    field_90 = new CommonVerticalList(field_88, info, true);
    field_80->initDataNoResetSelected(2);
    field_90->initDataNoResetSelected(2);
    al::setPaneSystemMessage(field_78, "TxtOption", "MenuOption", "PlayMode");
    al::setPaneSystemMessage(field_88, "TxtOption", "MenuOption", "PlayMode");
    sead::WFixedSafeString<512>* modeNames = makeMessageList(this, "MenuOption", cModeLabels, 2);
    sead::WFixedSafeString<512>* modeExplains =
        makeMessageList(this, "MenuOption", cModeExplainLabels, 2);
    addMessageList(field_80, modeNames, "TxtContent00");
    addMessageList(field_90, modeNames, "TxtContent00");
    addMessageList(field_80, modeExplains, "TxtContent01");
    addMessageList(field_90, modeExplains, "TxtContent01");

    field_98 = new al::SimpleLayoutAppearWaitEnd("", "MenuGuide", info, "ByHelp", false);
    field_a0 = new FooterParts(field_98, info,
                               al::getSystemMessageString(this, "Footer", "Choice_Back_Decide"),
                               "TxtGuide", "ParFooter");

    field_a8 = new SimpleLayoutMenu("", "OptionConfig", info, nullptr, false);
    mCtrlSettingsList = new CommonVerticalList(field_a8, info, true);
    al::setPaneSystemMessage(field_a8, "TxtOption", "MenuOption", "Config");
    mCtrlSettingsList->initDataNoResetSelected(11);
    addMessageList(mCtrlSettingsList, makeMessageList(this, "MenuOption", cConfigLabels, 11),
                   cSaveDataListPane);

    bool* enableConfig = new bool[11];
    RollPartsData* rollPartsData = new RollPartsData[11];
    const char* onOffLabels[] = {"On", "Off"};
    const char* levelLabels[] = {"On", "Off", "On", "Off", "On"};
    for (s32 i = 0; i < 11; i++) {
        enableConfig[i] = true;
        rollPartsData[i].textNum = 1;
        rollPartsData[i].texts = new const char16*[1];
        rollPartsData[i].texts[0] = cSeparatorText;
        rollPartsData[i].selectedIdx = 0;
        rollPartsData[i].isLoop = false;
    }
    rollPartsData[1].textNum = 5;
    rollPartsData[1].texts = makeRollTextArray(this, "MenuOption", levelLabels, 5);
    rollPartsData[2].textNum = 2;
    rollPartsData[2].texts = makeRollTextArray(this, "MenuOption", onOffLabels, 2);
    rollPartsData[3].textNum = 2;
    rollPartsData[3].texts = makeRollTextArray(this, "MenuOption", onOffLabels, 2);
    rollPartsData[4].textNum = 2;
    rollPartsData[4].texts = makeRollTextArray(this, "MenuOption", onOffLabels, 2);
    rollPartsData[5].textNum = 4;
    rollPartsData[5].texts = makeRollTextArray(this, "MenuOption", &levelLabels[1], 4);
    rollPartsData[7].textNum = 2;
    rollPartsData[7].texts = makeRollTextArray(this, "MenuOption", onOffLabels, 2);
    rollPartsData[8].textNum = 4;
    rollPartsData[8].texts = makeRollTextArray(this, "MenuOption", &levelLabels[1], 4);
    rollPartsData[10].textNum = 2;
    rollPartsData[10].texts = makeRollTextArray(this, "MenuOption", onOffLabels, 2);
    mCtrlSettingsList->setEnableData(enableConfig);
    mCtrlSettingsList->startLoopActionAll("Loop", "Loop");
    mCtrlSettingsList->setRollPartsData(rollPartsData);
    updateConfigDataInfo(rs::getGameConfigData(field_a8));

    mInputSeperator = new InputSeparator(scene, true);

    field_c8 = new SimpleLayoutMenu("", "OptionProcess", info, nullptr, false);
    field_b8 = new SimpleLayoutMenu("", "OptionSelect", info, nullptr, false);
    field_c0 = new CommonVerticalList(field_b8, info, true);
    al::setPaneSystemMessage(field_b8, "TxtOption", "MenuOption", "Data");
    field_c0->initDataNoResetSelected(3);
    addMessageList(field_c0, makeMessageList(this, "MenuOption", cDataLabels, 3),
                   cSaveDataListPane);

    field_d8 = new WindowConfirmData(info, "WindowConfirmData", "", true);
    field_e0 = new SimpleLayoutMenu("", "OptionData", info, nullptr, false);
    field_e8 = new CommonVerticalList(field_e0, info, true);
    field_e8->initDataNoResetSelected(5);
    field_f0 = new sead::WFixedSafeString<512>[5];
    field_f8 = new sead::WFixedSafeString<512>[5];
    field_100 = new sead::WFixedSafeString<512>[5];
    field_108 = new sead::WFixedSafeString<512>[5];
    field_110 = new sead::WFixedSafeString<512>[5];
    field_118 = new nn::ui2d::TextureInfo*[5];
    for (s32 i = 0; i < 5; i++)
        field_118[i] = nullptr;
    addMessageList(field_e8, field_f0, "TxtNumber");
    addMessageList(field_e8, field_f8, "TxtWorld");
    addMessageList(field_e8, field_100, "TxtShine");
    addMessageList(field_e8, field_108, "TxtDay");
    addMessageList(field_e8, field_110, "TxtPlay");
    field_e8->setImageData(field_118, "PicDummy");
    field_120 = al::createTextureInfo("ObjectData/TextureSaveData", "TextureSaveData", "Empty");
    updateSaveDataInfo(false);

    field_148 = new SimpleLayoutMenu("", "OptionLanguage", info, nullptr, false);
    field_150 = new FooterParts(field_148, info, emptyText(), "TxtGuide", "ParFooter");
    field_158 = new CommonVerticalList(field_148, info, true);
    field_160 = new al::WindowConfirm(info, "WindowConfirmLanguage", "");
    field_158->initDataNoResetSelected(12);
    sead::WFixedSafeString<512>* languageNames = new sead::WFixedSafeString<512>[12];
    for (s32 i = 0; i < 12; i++) {
        sead::FormatFixedSafeString<64> label("%s_%s", "Language", cLanguageNames[i]);
        const char16* text = al::getSystemMessageString(this, "LanguageSetting", label.cstr());
        al::copyMessageWithTag(languageNames[i].getBuffer(), languageNames[i].getBufferSize(),
                               text);
    }
    addMessageList(field_158, languageNames, cSaveDataListPane);

    field_78->kill();
    field_88->kill();
    field_98->kill();
    field_a8->kill();
    field_b8->kill();
    field_d8->kill();
    field_e0->kill();
    field_c8->kill();
    field_148->kill();
    field_150->kill();
    field_160->kill();

    initNerve(&NrvStageSceneStateOption.OptionTop, 0);
}

StageSceneStateOption::~StageSceneStateOption() = default;

void StageSceneStateOption::updateConfigDataInfo(const GameConfigData* config) {
    CommonVerticalList* list = mCtrlSettingsList;
    s32 stickLevel = config->getCameraStickSensitivityLevel();
    s32 stickSelected;
    if (stickLevel != -2) {
        s32 selectedM1 = 1;
        s32 selected0 = 2;
        s32 selected1 = 3;
        s32 selected = stickLevel == 2 ? 4 : -1;
        selected = stickLevel == 1 ? selected1 : selected;
        selected = stickLevel == 0 ? selected0 : selected;
        stickSelected = stickLevel == -1 ? selectedM1 : selected;
    } else {
        stickSelected = 0;
    }
    list->setRollPartsSelected(stickSelected, 1);

    list = mCtrlSettingsList;
    list->setRollPartsSelected(config->isCameraReverseInputH(), 2);
    list = mCtrlSettingsList;
    list->setRollPartsSelected(config->isCameraReverseInputV(), 3);
    list = mCtrlSettingsList;
    list->setRollPartsSelected(!config->isValidCameraGyro(), 4);

    list = mCtrlSettingsList;
    s32 gyroLevel = config->getCameraGyroSensitivityLevel();
    s32 gyroSelected;
    if (gyroLevel == -2)
        gyroSelected = -1;
    else if (gyroLevel == -1)
        gyroSelected = 0;
    else if (gyroLevel == 0)
        gyroSelected = 1;
    else if (gyroLevel == 1)
        gyroSelected = 2;
    else if (gyroLevel == 2)
        gyroSelected = 3;
    else
        gyroSelected = -2;
    list->setRollPartsSelected(gyroSelected, 5);

    list = mCtrlSettingsList;
    list->setRollPartsSelected(!config->isValidPadRumble(), 7);

    list = mCtrlSettingsList;
    s32 rumbleLevel = config->getPadRumbleLevel();
    s32 rumbleSelected;
    if (rumbleLevel == -2)
        rumbleSelected = -1;
    else if (rumbleLevel == -1)
        rumbleSelected = 0;
    else if (rumbleLevel == 0)
        rumbleSelected = 1;
    else if (rumbleLevel == 1)
        rumbleSelected = 2;
    else if (rumbleLevel == 2)
        rumbleSelected = 3;
    else
        rumbleSelected = -2;
    list->setRollPartsSelected(rumbleSelected, 8);

    list = mCtrlSettingsList;
    list->setRollPartsSelected(!config->isUseOpenListAdditionalButton(), 10);
}

void StageSceneStateOption::killAllLayouts() {
    field_60->kill();
    field_68->kill();
    field_78->kill();
    field_88->kill();
    field_a8->kill();
    field_b8->kill();
    field_d8->kill();
    field_e0->kill();
    field_c8->kill();
    field_148->kill();
    field_150->kill();
    field_160->kill();
}

void StageSceneStateOption::init() {
    initNerve(&NrvStageSceneStateOption.OptionTop, 0);
}

void StageSceneStateOption::appear() {
    al::NerveStateBase::appear();

    if (field_51) {
        field_88->startAppear("Appear");
        field_98->appear();
        field_a0->appear();
        al::setNerve(this, &NrvStageSceneStateOption.ModeSelectSelectingByHelp);
        return;
    }

    field_68->startAppear("Appear");
    al::setNerve(this, &NrvStageSceneStateOption.OptionTop);
}

void StageSceneStateOption::kill() {
    if (!al::isNerve(this, &NrvStageSceneStateOption.LanguageSettingConfirmYesNo)) {
        rs::trySavePrepoSettingsState(
            mGameDataHolder->getGameDataFile()->isKidsMode(), al::getLanguage(),
            *mGameDataHolder->getGameConfigData(),
            GameDataFunction::getSaveDataIdForPrepo(GameDataHolderAccessor(getHost())),
            GameDataFunction::getPlayTimeAcrossFile(GameDataHolderAccessor(getHost())));
    }

    al::NerveStateBase::kill();
    field_28 = nullptr;
    field_30 = nullptr;
    field_38 = nullptr;
    field_40 = nullptr;
    field_48 = nullptr;
    if (field_51)
        field_51 = false;
}

bool StageSceneStateOption::isModeSelectEnd() const {
    return al::isNerve(this, &NrvStageSceneStateOption.ModeSelectConfirmEnd);
}

s32 StageSceneStateOption::getSelectedFileId() const {
    return field_e8->getSelectedIdx();
}

bool StageSceneStateOption::isChangeLanguage() const {
    return isDead() && al::isNerve(this, &NrvStageSceneStateOption.LanguageSettingConfirmYesNo);
}

void StageSceneStateOption::exeOptionTop() {
    // NONMATCHING: 35 attempts exhausted.
    // Tried input helper reshaping, branch ordering, and nerve-layout tuning; remaining diff is
    // menu-state control-flow codegen.
    if (al::isFirstStep(this)) {
        if (field_30) {
            field_30->kill();
            field_30 = nullptr;
        }
        if (!field_68->isWait() && field_40) {
            field_68->startAppear(field_40);
            field_40 = nullptr;
        }
        field_70->activate();
        mFooterParts->tryChangeTextFade(
            al::getSystemMessageString(this, "Footer", "Choice_Back_Decide"));
    }

    field_70->update();
    if (al::isStep(this, 6))
        field_70->appearCursor();

    if (!al::isLessStep(this, 6)) {
        updateVerticalListInput(field_70, getHost());
        if (rs::isTriggerUiDecide(getHost())) {
            al::startHitReaction(field_68, "決定");
            field_70->endCursor();
            SimpleLayoutMenu* layout = field_68;
            CommonVerticalList* list = field_70;
            const al::Nerve* nerve = cOptionTopTransitions[list->getSelectedIdx()].nerve;
            field_40 = "RightIn";
            field_48 = "RightOut";
            list->decide();
            field_28 = nerve;
            field_30 = layout;
            field_38 = list;
            al::setNerve(this, &NrvStageSceneStateOption.WaitEndDecideAnim);
            return;
        }
        if (rs::isTriggerUiCancel(getHost())) {
            al::startHitReaction(field_68, "キャンセル");
            mFooterParts->tryChangeTextFade(emptyText());
            al::setNerve(this, &NrvStageSceneStateOption.Close);
        }
    }
}

void StageSceneStateOption::decide(const al::Nerve* nerve, SimpleLayoutMenu* layout,
                                   CommonVerticalList* list) {
    field_40 = "RightIn";
    field_48 = "RightOut";
    list->decide();
    field_28 = nerve;
    field_30 = layout;
    field_38 = list;
    bool isDataNerve = nerve == &NrvStageSceneStateOption.LoadDataSelecting ||
                       nerve == &NrvStageSceneStateOption.DeleteDataSelecting ||
                       nerve == &NrvStageSceneStateOption.SaveDataSelecting;
    if (isDataNerve && !SaveDataAccessFunction::isDoneSave(mGameDataHolder))
        al::setNerve(this, &NrvStageSceneStateOption.WaitEndAutoSave);
    else
        al::setNerve(this, &NrvStageSceneStateOption.WaitEndDecideAnim);
}

void StageSceneStateOption::exeModeSelectSelecting() {
    if (al::isFirstStep(this)) {
        if (field_30) {
            field_30->kill();
            field_30 = nullptr;
        }
        if (al::isDead(field_78) && field_40) {
            field_78->startAppear(field_40);
            field_40 = nullptr;
        }
        field_80->activate();
        bool isKidsMode = rs::isKidsMode(field_78);
        al::startAction(field_80->getParts(0), isKidsMode ? "Off" : "On", "State");
        al::startAction(field_80->getParts(1), isKidsMode ? "On" : "Off", "State");
        field_80->setSelectedIdx(isKidsMode, 0);
    } else if (al::isActionPlaying(field_80->getParts(field_80->getSelectedIdx()), "CheckOn",
                                   "Main")) {
        al::RollParts* parts = field_80->getParts(field_80->getSelectedIdx());
        if (!al::isActionEnd(parts, "Main"))
            return;
        al::startAction(parts, "Select");
        al::setActionFrame(parts, al::getActionFrameMax(parts, "Select"));
    }

    field_80->update();
    if (al::isStep(this, 6))
        field_80->appearCursor();

    if (!al::isLessStep(this, 6)) {
        updateVerticalListInput(field_80, getHost());
        if (rs::isTriggerUiDecide(getHost())) {
            s32 selected = field_80->getSelectedIdx();
            if ((selected == 1) != rs::isKidsMode(field_78)) {
                al::startHitReaction(field_78, "モード切替");
                field_80->endCursor();
                al::startAction(field_80->getParts(0), selected == 1 ? "Off" : "On", "State");
                al::startAction(field_80->getParts(1), selected == 1 ? "On" : "Off", "State");
                openConfirm(&NrvStageSceneStateOption.ModeSelectConfirmYesNo, field_78, field_80);
            } else {
                al::startHitReaction(field_78, "選択中のモード");
                al::startAction(field_80->getParts(selected), "CheckOn", "Main");
            }
            return;
        }
        if (rs::isTriggerUiCancel(getHost())) {
            al::startHitReaction(field_78, "キャンセル");
            field_80->hideCursor();
            cancel(&NrvStageSceneStateOption.OptionTop, field_78, field_80);
        }
    }
}

void StageSceneStateOption::openConfirm(const al::Nerve* nerve, SimpleLayoutMenu* layout,
                                        CommonVerticalList* list) {
    field_40 = nullptr;
    field_48 = nullptr;
    list->decide();
    list->update();
    field_28 = nerve;
    field_30 = layout;
    field_38 = list;
    bool isDataNerve = nerve == &NrvStageSceneStateOption.LoadDataSelecting ||
                       nerve == &NrvStageSceneStateOption.DeleteDataSelecting ||
                       nerve == &NrvStageSceneStateOption.SaveDataSelecting;
    if (isDataNerve && !SaveDataAccessFunction::isDoneSave(mGameDataHolder))
        al::setNerve(this, &NrvStageSceneStateOption.WaitEndAutoSave);
    else
        al::setNerve(this, &NrvStageSceneStateOption.WaitEndDecideAnim);
}

void StageSceneStateOption::cancel(const al::Nerve* nerve, SimpleLayoutMenu* layout,
                                   CommonVerticalList* list) {
    field_40 = "LeftIn";
    field_48 = "LeftOut";
    list->deactivate();
    field_28 = nerve;
    field_30 = layout;
    field_38 = list;
    bool isDataNerve = nerve == &NrvStageSceneStateOption.LoadDataSelecting ||
                       nerve == &NrvStageSceneStateOption.DeleteDataSelecting ||
                       nerve == &NrvStageSceneStateOption.SaveDataSelecting;
    if (isDataNerve && !SaveDataAccessFunction::isDoneSave(mGameDataHolder))
        al::setNerve(this, &NrvStageSceneStateOption.WaitEndAutoSave);
    else
        al::setNerve(this, &NrvStageSceneStateOption.WaitEndDecideAnim);
}

void StageSceneStateOption::exeModeSelectSelectingByHelp() {
    if (al::isFirstStep(this)) {
        field_90->activate();
        bool isKidsMode = rs::isKidsMode(field_88);
        al::startAction(field_90->getParts(0), isKidsMode ? "Off" : "On", "State");
        al::startAction(field_90->getParts(1), isKidsMode ? "On" : "Off", "State");
        field_90->setSelectedIdx(isKidsMode, 0);
    } else if (al::isActionPlaying(field_90->getParts(field_90->getSelectedIdx()), "CheckOn",
                                   "Main")) {
        al::RollParts* parts = field_90->getParts(field_90->getSelectedIdx());
        if (!al::isActionEnd(parts, "Main"))
            return;
        al::startAction(parts, "Select");
        al::setActionFrame(parts, al::getActionFrameMax(parts, "Select"));
    }

    field_90->update();
    if (al::isStep(this, 6))
        field_90->appearCursor();

    if (!al::isLessStep(this, 6)) {
        updateVerticalListInput(field_90, getHost());
        if (rs::isTriggerUiDecide(getHost())) {
            s32 selected = field_90->getSelectedIdx();
            if ((selected == 1) != rs::isKidsMode(field_88)) {
                al::startHitReaction(field_88, "モード切替");
                field_90->endCursor();
                al::startAction(field_90->getParts(0), selected == 1 ? "Off" : "On", "State");
                al::startAction(field_90->getParts(1), selected == 1 ? "On" : "Off", "State");
                openConfirm(&NrvStageSceneStateOption.ModeSelectConfirmYesNo, field_88, field_90);
            } else {
                al::startHitReaction(field_88, "選択中のモード");
                al::startAction(field_90->getParts(selected), "CheckOn", "Main");
            }
            return;
        }
        if (rs::isTriggerUiCancel(getHost())) {
            al::startHitReaction(field_78, "キャンセル");
            al::setNerve(this, &NrvStageSceneStateOption.Close);
        }
    }
}

void StageSceneStateOption::exeModeSelectConfirmYesNo() {
    CommonVerticalList* list = field_51 ? field_90 : field_80;
    if (al::isFirstStep(this)) {
        field_60->setListNum(2);
        field_60->setTxtMessage(al::getSystemMessageString(
            this, "ConfirmMessage",
            list->getSelectedIdx() == 0 ? "PlayMode_Confirm_Normal" : "PlayMode_Confirm_Kids"));
        field_60->setTxtList(
            0, al::getSystemMessageString(this, "ConfirmMessage", "PlayMode_Confirm_Yes"));
        field_60->setTxtList(
            1, al::getSystemMessageString(this, "ConfirmMessage", "PlayMode_Confirm_No"));
        field_60->appear();
    }

    if (field_60->isNerveEnd()) {
        if (field_60->getPrevSelectionType() == field_60->getCancelIdx()) {
            if (field_51)
                al::setNerve(this, &NrvStageSceneStateOption.ModeSelectSelectingByHelp);
            else
                al::setNerve(this, &NrvStageSceneStateOption.ModeSelectSelecting);
            return;
        }

        if (list->getSelectedIdx() == 0)
            GameDataFunction::setKidsModeOff(getHost());
        else
            GameDataFunction::setKidsModeOn(getHost());

        rs::trySavePrepoSettingsState(
            mGameDataHolder->getGameDataFile()->isKidsMode(), al::getLanguage(),
            *mGameDataHolder->getGameConfigData(),
            GameDataFunction::getSaveDataIdForPrepo(GameDataHolderAccessor(getHost())),
            GameDataFunction::getPlayTimeAcrossFile(GameDataHolderAccessor(getHost())));
        mGameDataHolder->getGameDataFile()->changeWipeType("FadeBlack");
        al::setNerve(this, &NrvStageSceneStateOption.ModeSelectConfirmEnd);
        return;
    }

    if (rs::isTriggerUiDecide(getHost())) {
        if (field_60->getCancelIdx() != field_60->getPrevSelectionType())
            field_60->tryDecideWithoutEnd();
        else
            field_60->tryCancel();
        return;
    }

    if (rs::isTriggerUiCancel(getHost())) {
        field_60->tryCancel();
        return;
    }

    if (rs::isRepeatUiDown(getHost())) {
        field_60->tryDown();
        return;
    }

    if (rs::isRepeatUiUp(getHost()))
        field_60->tryUp();
}

void StageSceneStateOption::exeModeSelectConfirmEnd() {}

void StageSceneStateOption::exeConfig() {
    if (al::isFirstStep(this)) {
        if (field_30) {
            field_30->kill();
            field_30 = nullptr;
        }
        mInputSeperator->reset();
        mFooterParts->tryChangeTextFade(
            al::getSystemMessageString(this, "Footer", "Choice_Reset_Back"));
        field_a8->startAppear(field_40);
        field_40 = nullptr;
        mCtrlSettingsList->activate();
    }

    mCtrlSettingsList->update();
    if (al::isStep(this, 6))
        mCtrlSettingsList->appearCursor();

    if (!al::isLessStep(this, 6)) {
        mInputSeperator->update();
        if (mInputSeperator->isTriggerUiUp() &&
            mCtrlSettingsList->getSelectedIdx() == mCtrlSettingsList->getVisibleTopIdx()) {
            mCtrlSettingsList->jumpBottom();
        } else if (mInputSeperator->isTriggerUiDown() &&
                   mCtrlSettingsList->getSelectedIdx() == mCtrlSettingsList->getDataNum() - 1) {
            mCtrlSettingsList->jumpTop();
        } else if (mInputSeperator->isHoldUiUp()) {
            if (mInputSeperator->isRepeatUiUp())
                mCtrlSettingsList->up();
        } else if (mInputSeperator->isHoldUiDown()) {
            if (mInputSeperator->isRepeatUiDown())
                mCtrlSettingsList->down();
        } else if (mInputSeperator->isTriggerUiLeft()) {
            mCtrlSettingsList->rollLeft();
        } else if (mInputSeperator->isTriggerUiRight()) {
            mCtrlSettingsList->rollRight();
        }

        if (rs::isTriggerUiY(getHost())) {
            al::startHitReaction(field_a8, "リセット");
            GameConfigData* config = rs::getGameConfigData(field_a8);
            config->init();
            updateConfigDataInfo(config);
        }

        if (rs::isTriggerUiCancel(getHost())) {
            al::startHitReaction(field_a8, "キャンセル");
            mCtrlSettingsList->hideCursor();
            cancel(&NrvStageSceneStateOption.OptionTop, field_a8, mCtrlSettingsList);
        }
    }
}

void StageSceneStateOption::endConfig() {
    GameConfigData* config = rs::getGameConfigData(field_a8);
    config->setCameraStickSensitivityLevel(
        cConfigLevelTable[mCtrlSettingsList->getRollPartsSelected(1)]);

    if (mCtrlSettingsList->getRollPartsSelected(2) == 1)
        config->onCameraReverseInputH();
    else
        config->offCameraReverseInputH();

    if (mCtrlSettingsList->getRollPartsSelected(3) == 1)
        config->onCameraReverseInputV();
    else
        config->offCameraReverseInputV();

    if (mCtrlSettingsList->getRollPartsSelected(4))
        config->invalidateCameraGyro();
    else
        config->validateCameraGyro();

    if (mCtrlSettingsList->getRollPartsSelected(7))
        config->invalidatePadRumble();
    else
        config->validatePadRumble();

    if (mCtrlSettingsList->getRollPartsSelected(10))
        config->offUseOpenListAdditionalButton();
    else
        config->onUseOpenListAdditionalButton();

    config->setCameraGyroSensitivityLevel(
        cConfigLevelTable[mCtrlSettingsList->getRollPartsSelected(5) + 1]);
    config->setPadRumbleLevel(cConfigLevelTable[mCtrlSettingsList->getRollPartsSelected(8) + 1]);
    rs::applyGameConfigData(mScene, config);
    rs::saveGameConfigData(field_a8);
}

void StageSceneStateOption::exeDataManager() {
    CommonVerticalList** dataList;
    if (al::isFirstStep(this)) {
        if (al::isActive(field_c8))
            field_c8->kill();
        if (field_30) {
            field_30->kill();
            field_30 = nullptr;
        }
        if (!field_b8->isWait()) {
            field_b8->startAppear(field_40);
            field_40 = nullptr;
        }
        dataList = &field_c0;
        (*dataList)->activate();
        CommonVerticalList* fileList = field_e8;
        s32 playingFile = mGameDataHolder->getPlayingFileId();
        fileList->setSelectedIdx(playingFile < 0 ? 0 : playingFile, 0);
    } else {
        dataList = &field_c0;
    }

    field_c0->update();
    if (al::isStep(this, 6))
        (*dataList)->appearCursor();

    if (!al::isLessStep(this, 6)) {
        updateVerticalListInput(field_c0, getHost());
        if (rs::isTriggerUiDecide(getHost())) {
            field_c0->endCursor();
            al::startHitReaction(field_b8, "決定");
            SimpleLayoutMenu* layout = field_b8;
            CommonVerticalList* list = field_c0;
            s32 selected = list->getSelectedIdx();
            const al::Nerve* nerve = cDataManagerTransitions[selected].nerve;
            field_40 = "RightIn";
            field_48 = "RightOut";
            list->decide();
            field_28 = nerve;
            field_30 = layout;
            field_38 = list;
            if ((u32)selected <= 2 && !SaveDataAccessFunction::isDoneSave(mGameDataHolder))
                al::setNerve(this, &NrvStageSceneStateOption.WaitEndAutoSave);
            else
                al::setNerve(this, &NrvStageSceneStateOption.WaitEndDecideAnim);
            return;
        }
        if (rs::isTriggerUiCancel(getHost())) {
            al::startHitReaction(field_b8, "キャンセル");
            field_c0->hideCursor();
            cancel(&NrvStageSceneStateOption.OptionTop, field_b8, field_c0);
        }
    }
}

void StageSceneStateOption::exeSaveDataSelecting() {
    // NONMATCHING: 35 attempts exhausted.
    // Tried target confirm setup ordering, save-window helper reshaping, and input flow ordering;
    // remaining diff is save-select state codegen.
    CommonVerticalList** dataList;
    if (al::isFirstStep(this)) {
        if (al::isActive(field_c8))
            field_c8->kill();
        if (field_30) {
            field_30->kill();
            field_30 = nullptr;
        }
        if (al::isDead(field_e0) && field_40) {
            field_e0->startAppear(field_40);
            field_40 = nullptr;
        }
        al::setPaneSystemMessage(field_e0, "TxtOption", "MenuOption", "Data_Save");
        dataList = &field_e8;
        (*dataList)->activate();
        updateSaveDataInfo(false);
    } else {
        dataList = &field_e8;
    }

    field_e8->update();
    if (al::isStep(this, 6))
        (*dataList)->appearCursor();

    if (!al::isLessStep(this, 6)) {
        updateVerticalListInput(field_e8, getHost());
        if (rs::isTriggerUiDecide(getHost())) {
            s32 selected = getSelectedFileId();
            al::startHitReaction(field_e0, "決定");
            field_e8->endCursor();
            const char* message = GameDataFunction::isNewSaveDataByFileId(field_e0, selected) ?
                                      "Data_Save_Create_Confirm" :
                                      "Data_Save_Override_Confirm";
            setupDataWindow(
                field_d8, field_e8, field_118, selected,
                al::getSystemMessageString(this, "ConfirmMessage", message),
                al::getSystemMessageString(this, "ConfirmMessage", "Data_Save_Confirm_Yes"),
                al::getSystemMessageString(this, "ConfirmMessage", "Data_Save_Confirm_No"));
            openConfirm(&NrvStageSceneStateOption.SaveDataConfirmYesNo, field_e0, field_e8);
            return;
        }
        if (rs::isTriggerUiCancel(getHost())) {
            al::startHitReaction(field_e0, "キャンセル");
            field_e8->hideCursor();
            cancel(&NrvStageSceneStateOption.DataManager, field_e0, field_e8);
        }
    }
}

void StageSceneStateOption::updateSaveDataInfo(bool) {
    // NONMATCHING: 35 attempts exhausted.
    // Tried message/string helper reshaping, texture lookup setup, and target loop ordering;
    // remaining diff is the large save-data formatting/texture loop.
    for (s32 i = 0; i < 5; i++) {
        field_f0[i].format(u"%d", i + 1);
        if (GameDataFunction::isNewSaveDataByFileId(field_e0, i)) {
            al::copyMessageWithTag(field_f8[i].getBuffer(), field_f8[i].getBufferSize(),
                                   cSeparatorText);
            al::copyMessageWithTag(field_100[i].getBuffer(), field_100[i].getBufferSize(),
                                   cSeparatorText);
            al::copyMessageWithTag(field_108[i].getBuffer(), field_108[i].getBufferSize(),
                                   cSeparatorText);
            al::copyMessageWithTag(field_110[i].getBuffer(), field_110[i].getBufferSize(),
                                   cSeparatorText);
            field_118[i] = field_120;
        } else {
            const char16* worldName = GameDataFunction::tryGetWorldNameByFileId(field_e0, i);
            if (worldName)
                al::copyMessageWithTag(field_f8[i].getBuffer(), field_f8[i].getBufferSize(),
                                       worldName);
            else
                al::copyMessageWithTag(field_f8[i].getBuffer(), field_f8[i].getBufferSize(),
                                       cSeparatorText);
            field_100[i].format(u"%d", 0);
            field_108[i].format(u"%d", 0);
            field_110[i].format(u"%d", 0);
        }
    }
}

void StageSceneStateOption::exeSaveDataConfirmYesNo() {
    if (al::isFirstStep(this))
        field_d8->appearWithChoicingCancel();

    if (field_d8->isDisable()) {
        al::setNerve(this, &NrvStageSceneStateOption.SaveDataSelecting);
        return;
    }

    field_e8->update();
    field_d8->updateNerve();
    field_d8->updateConfirmDataDate();
    if (field_d8->isEndSelect()) {
        if (field_d8->isDecided()) {
            s32 selectedFile = field_e8->getSelectedIdx();
            s32 playingFile = mGameDataHolder->getPlayingFileId();
            if (selectedFile == playingFile)
                SaveDataAccessFunction::startSaveDataWriteWithWindow(mGameDataHolder);
            else
                SaveDataAccessFunction::startSaveDataCopyWithWindow(mGameDataHolder, playingFile,
                                                                    selectedFile);
            al::setNerve(this, &NrvStageSceneStateOption.SaveDataSaving);
            return;
        }
        if (field_d8->isCanceled())
            field_d8->end();
    }
}

void StageSceneStateOption::exeSaveDataSaving() {
    if (SaveDataAccessFunction::updateSaveDataAccess(mGameDataHolder, false))
        al::setNerve(this, &SaveDataSaved);
}

void StageSceneStateOption::exeSaveDataSaved() {
    // NONMATCHING: 35 attempts exhausted.
    // Tried target post-save update ordering and direct nerve layout fixes; remaining diff is
    // saved-window animation/update codegen.
    if (al::isFirstStep(this)) {
        updateSaveDataInfo(true);
        field_d8->end();
    }

    if (field_d8->isDisable())
        al::setNerve(this, &NrvStageSceneStateOption.SaveDataSelecting);
}

void StageSceneStateOption::exeLoadDataSelecting() {
    // NONMATCHING: 35 attempts exhausted.
    // Tried target confirm setup ordering, NG/yes-no branch ordering, and input flow ordering;
    // remaining diff is load-select state codegen.
    CommonVerticalList** dataList;
    if (al::isFirstStep(this)) {
        if (al::isActive(field_c8))
            field_c8->kill();
        if (field_30) {
            field_30->kill();
            field_30 = nullptr;
        }
        if (al::isDead(field_e0) && field_40) {
            field_e0->startAppear(field_40);
            field_40 = nullptr;
        }
        al::setPaneSystemMessage(field_e0, "TxtOption", "MenuOption", "Data_Load");
        dataList = &field_e8;
        (*dataList)->activate();
        updateSaveDataInfo(false);
    } else {
        dataList = &field_e8;
    }

    field_e8->update();
    if (al::isStep(this, 6))
        (*dataList)->appearCursor();

    if (!al::isLessStep(this, 6)) {
        updateVerticalListInput(field_e8, getHost());
        if (rs::isTriggerUiDecide(getHost())) {
            s32 selected = getSelectedFileId();
            al::startHitReaction(field_e0, "決定");
            field_e8->endCursor();
            if (GameDataFunction::isNewSaveDataByFileId(field_e0, selected)) {
                field_d8->setConfirmMessage(
                    al::getSystemMessageString(this, "ConfirmMessage",
                                               "Data_Load_NG_PlayingData_Confirm"),
                    al::getSystemMessageString(this, "ConfirmMessage", "Data_Load_Confirm_Yes"),
                    al::getSystemMessageString(this, "ConfirmMessage", "Data_Load_Confirm_No"));
                openConfirm(&NrvStageSceneStateOption.LoadDataConfirmNg, field_e0, field_e8);
            } else {
                setupDataWindow(
                    field_d8, field_e8, field_118, selected,
                    al::getSystemMessageString(this, "ConfirmMessage", "Data_Load_Confirm"),
                    al::getSystemMessageString(this, "ConfirmMessage", "Data_Load_Confirm_Yes"),
                    al::getSystemMessageString(this, "ConfirmMessage", "Data_Load_Confirm_No"));
                openConfirm(&NrvStageSceneStateOption.LoadDataConfirmYesNo, field_e0, field_e8);
            }
            return;
        }
        if (rs::isTriggerUiCancel(getHost())) {
            al::startHitReaction(field_e0, "キャンセル");
            field_e8->hideCursor();
            cancel(&NrvStageSceneStateOption.DataManager, field_e0, field_e8);
        }
    }
}

void StageSceneStateOption::exeLoadDataConfirmNg() {
    if (al::isFirstStep(this))
        field_60->appear();
    field_e8->update();
    if (al::isDead(field_60)) {
        al::setNerve(this, &NrvStageSceneStateOption.LoadDataSelecting);
        return;
    }
    if (rs::isTriggerUiDecide(getHost()))
        field_60->tryDecide();
    if (rs::isTriggerUiCancel(getHost()))
        field_60->tryCancel();
}

void StageSceneStateOption::exeLoadDataConfirmYesNo() {
    if (al::isFirstStep(this))
        field_d8->appear();
    if (field_d8->isDisable()) {
        al::setNerve(this, &NrvStageSceneStateOption.LoadDataSelecting);
        return;
    }
    field_e8->update();
    field_d8->updateNerve();
    if (field_d8->isEndSelect()) {
        if (field_d8->isDecided()) {
            al::setNerve(this, &NrvStageSceneStateOption.LoadDataSaving);
            return;
        }
        if (field_d8->isCanceled())
            field_d8->end();
    }
}

void StageSceneStateOption::exeLoadDataSaving() {
    if (al::isFirstStep(this)) {
        mGameDataHolder->requestSetPlayingFileId(getSelectedFileId());
        mIsLoadData = true;
        field_d8->end();
    }
}

void StageSceneStateOption::exeDeleteDataSelecting() {
    // NONMATCHING: 35 attempts exhausted.
    // Tried target confirm setup ordering, NG/yes-no branch ordering, and input flow ordering;
    // remaining diff is delete-select state codegen.
    CommonVerticalList** dataList;
    if (al::isFirstStep(this)) {
        if (al::isActive(field_c8))
            field_c8->kill();
        if (field_30) {
            field_30->kill();
            field_30 = nullptr;
        }
        if (al::isDead(field_e0) && field_40) {
            field_e0->startAppear(field_40);
            field_40 = nullptr;
        }
        al::setPaneSystemMessage(field_e0, "TxtOption", "MenuOption", "Data_Delete");
        dataList = &field_e8;
        (*dataList)->activate();
        updateSaveDataInfo(false);
    } else {
        dataList = &field_e8;
    }

    field_e8->update();
    if (al::isStep(this, 6))
        (*dataList)->appearCursor();

    if (!al::isLessStep(this, 6)) {
        updateVerticalListInput(field_e8, getHost());
        if (rs::isTriggerUiDecide(getHost())) {
            s32 selected = getSelectedFileId();
            al::startHitReaction(field_e0, "決定");
            field_e8->endCursor();
            if (selected == mGameDataHolder->getPlayingFileId() ||
                GameDataFunction::isNewSaveDataByFileId(field_e0, selected)) {
                field_d8->setConfirmMessage(
                    al::getSystemMessageString(this, "ConfirmMessage",
                                               selected == mGameDataHolder->getPlayingFileId() ?
                                                   "Data_Delete_NG_PlayingData_Confirm" :
                                                   "Data_Delete_NG_EmptyData_Confirm"),
                    al::getSystemMessageString(this, "ConfirmMessage", "Data_Delete_Confirm_Yes"),
                    al::getSystemMessageString(this, "ConfirmMessage", "Data_Delete_Confirm_No"));
                openConfirm(&NrvStageSceneStateOption.DeleteDataConfirmNg, field_e0, field_e8);
            } else {
                setupDataWindow(
                    field_d8, field_e8, field_118, selected,
                    al::getSystemMessageString(this, "ConfirmMessage", "Data_Delete_Confirm"),
                    al::getSystemMessageString(this, "ConfirmMessage", "Data_Delete_Confirm_Yes"),
                    al::getSystemMessageString(this, "ConfirmMessage", "Data_Delete_Confirm_No"));
                openConfirm(&NrvStageSceneStateOption.DeleteDataConfirmYesNo, field_e0, field_e8);
            }
            return;
        }
        if (rs::isTriggerUiCancel(getHost())) {
            al::startHitReaction(field_e0, "キャンセル");
            field_e8->hideCursor();
            cancel(&NrvStageSceneStateOption.DataManager, field_e0, field_e8);
        }
    }
}

void StageSceneStateOption::exeDeleteDataConfirmNg() {
    if (al::isFirstStep(this))
        field_60->appear();
    field_e8->update();
    if (al::isDead(field_60)) {
        al::setNerve(this, &NrvStageSceneStateOption.DeleteDataSelecting);
        return;
    }
    if (rs::isTriggerUiDecide(getHost()))
        field_60->tryDecide();
    if (rs::isTriggerUiCancel(getHost()))
        field_60->tryCancel();
}

void StageSceneStateOption::exeDeleteDataConfirmYesNo() {
    if (al::isFirstStep(this))
        field_d8->appearWithChoicingCancel();
    if (field_d8->isDisable()) {
        al::setNerve(this, &NrvStageSceneStateOption.DeleteDataSelecting);
        return;
    }
    field_e8->update();
    field_d8->updateNerve();
    if (field_d8->isEndSelect()) {
        if (field_d8->isDecided()) {
            al::setNerve(this, &NrvStageSceneStateOption.DeleteDataDeleting);
            return;
        }
        if (field_d8->isCanceled())
            field_d8->end();
    }
}

void StageSceneStateOption::exeDeleteDataDeleting() {
    if (al::isFirstStep(this))
        SaveDataAccessFunction::startSaveDataDeleteWithWindow(mGameDataHolder, getSelectedFileId());
    if (SaveDataAccessFunction::updateSaveDataAccess(mGameDataHolder, false))
        al::setNerve(this, &DeleteDataDeleted);
}

void StageSceneStateOption::exeDeleteDataDeleted() {
    // NONMATCHING: 35 attempts exhausted.
    // Tried target post-delete update ordering and direct nerve layout fixes; remaining diff is
    // deleted-window animation/update codegen.
    if (al::isFirstStep(this)) {
        updateSaveDataInfo(true);
        field_d8->end();
    }
    if (field_d8->isDisable())
        al::setNerve(this, &NrvStageSceneStateOption.DeleteDataSelecting);
}

void StageSceneStateOption::exeLanguageSetting() {
    // NONMATCHING: 35 attempts exhausted.
    // Tried language-index helpers, message-label construction, and input flow ordering; remaining
    // diff is language list/confirm setup codegen.
    if (al::isFirstStep(this)) {
        if (field_30)
            field_30->kill();
        if (field_40)
            field_148->startAppear(field_40);
        field_158->setSelectedIdx(findCurrentLanguageIndex(), -1);
        field_158->activate();
        mFooterParts->changeText(emptyText());
        field_150->changeTextFade(al::getSystemMessageString(this, "Footer", "Choice_Back_Decide"));
    }

    field_158->update();
    if (al::isStep(this, 6))
        field_158->appearCursor();

    if (!al::isLessStep(this, 6)) {
        updateVerticalListInput(field_158, getHost());
        if (rs::isTriggerUiDecide(getHost())) {
            s32 selected = field_158->getSelectedIdx();
            al::startHitReaction(field_148, "決定");
            field_158->endCursor();
            mLanguage = cLanguageNames[selected];
            field_160->setListNum(2);
            sead::FormatFixedSafeString<64> yesLabel("%s_%s_YES", "Language", mLanguage);
            sead::FormatFixedSafeString<64> noLabel("%s_%s_NO", "Language", mLanguage);
            sead::FormatFixedSafeString<64> confirmLabel("%s_%s_Confirm", "Language", mLanguage);
            field_160->setTxtMessage(
                al::getSystemMessageString(this, "LanguageSetting", confirmLabel.cstr()));
            field_160->setTxtList(
                0, al::getSystemMessageString(this, "LanguageSetting", yesLabel.cstr()));
            field_160->setTxtList(
                1, al::getSystemMessageString(this, "LanguageSetting", noLabel.cstr()));
            field_160->setCancelIdx(1);
            al::setNerve(this, &NrvStageSceneStateOption.LanguageSettingConfirmYesNo);
            return;
        }
        if (rs::isTriggerUiCancel(getHost())) {
            al::startHitReaction(field_148, "キャンセル");
            field_158->hideCursor();
            cancel(&NrvStageSceneStateOption.OptionTop, field_148, field_158);
        }
    }
}

void StageSceneStateOption::exeLanguageSettingConfirmYesNo() {
    // NONMATCHING: 35 attempts exhausted.
    // Tried confirm input helper reshaping, yes/no branch ordering, and language side-effect
    // ordering; remaining diff is WindowConfirm control-flow codegen.
    if (al::isFirstStep(this))
        field_160->appearWithChoicingCancel();

    if (field_160->isNerveEnd()) {
        if (field_160->getPrevSelectionType() == field_160->getCancelIdx()) {
            al::setNerve(this, &LanguageSetting);
            return;
        } else {
            kill();
            return;
        }
    }

    if (rs::isTriggerUiDecide(getHost())) {
        if (field_160->getCancelIdx() != field_160->getPrevSelectionType())
            field_160->tryDecideWithoutEnd();
        else
            field_160->tryCancel();
        return;
    }

    if (rs::isTriggerUiCancel(getHost())) {
        field_160->tryCancel();
        return;
    }

    if (rs::isRepeatUiDown(getHost())) {
        field_160->tryDown();
        return;
    }

    if (rs::isRepeatUiUp(getHost()))
        field_160->tryUp();
}

void StageSceneStateOption::exeWaitEndDecideAnim() {
    field_38->update();
    if (field_38->isDecideEnd() || field_38->isDeactive()) {
        if (field_48)
            field_30->startEnd(field_48);
        else {
            al::setNerve(this, field_28);
            field_30 = nullptr;
            field_38 = nullptr;
            return;
        }
    }

    if (field_30->isEndWait()) {
        al::setNerve(this, field_28);
        field_28 = nullptr;
        field_38 = nullptr;
        field_48 = nullptr;
    }
}

void StageSceneStateOption::exeWaitEndDecideAnimAndAutoSave() {
    al::isFirstStep(this);
    field_38->update();
    if (field_38->isDecideEnd() || field_38->isDeactive())
        field_30->startEnd(field_48);
    if (field_30->isEndWait())
        al::setNerve(this, &WaitEndAutoSave);
}

void StageSceneStateOption::exeWaitEndAutoSave() {
    if (al::isFirstStep(this)) {
        field_d0 = false;
        field_30->kill();
        field_c8->startAppear("Appear");
        al::startAction(field_c8, "Loop", "Loop");
    }

    if (rs::isHoldUiCancel(getHost()) || al::isGreaterEqualStep(field_c8, 600)) {
        field_d0 = true;
        field_c8->startEnd("End");
    }

    if (field_c8->isWait() && al::isGreaterEqualStep(field_c8, 45) &&
        SaveDataAccessFunction::isDoneSave(mGameDataHolder))
        field_c8->startEnd("End");

    if (field_c8->isEndWait()) {
        if (field_d0 || al::isGreaterEqualStep(field_c8, 600)) {
            field_38 = nullptr;
            field_28 = nullptr;
            field_30 = nullptr;
            field_48 = nullptr;
            field_40 = "LeftIn";
            return al::setNerve(this, &NrvStageSceneStateOption.DataManager);
        } else {
            al::setNerve(this, field_28);
            field_48 = nullptr;
            field_30 = nullptr;
            field_38 = nullptr;
            field_28 = nullptr;
        }
    }
}

void StageSceneStateOption::exeClose() {
    if (al::isFirstStep(this)) {
        if (field_51) {
            field_90->hideCursor();
            field_88->startEnd("End");
            field_98->end();
        } else {
            field_70->hideCursor();
            field_68->startEnd("End");
        }
    }

    if (field_51) {
        if (field_88->isEndWait()) {
            field_88->kill();
            field_98->end();
            field_a0->kill();
            kill();
        }
        return;
    }

    if (field_68->isEndWait()) {
        field_68->kill();
        kill();
    }
}

void StageSceneStateOption::changeNerve(const al::Nerve* nerve, SimpleLayoutMenu* layout,
                                        CommonVerticalList* list) {
    bool isDataNerve = nerve == &NrvStageSceneStateOption.LoadDataSelecting ||
                       nerve == &NrvStageSceneStateOption.DeleteDataSelecting ||
                       nerve == &NrvStageSceneStateOption.SaveDataSelecting;
    field_28 = nerve;
    field_30 = layout;
    field_38 = list;

    if (isDataNerve && !SaveDataAccessFunction::isDoneSave(mGameDataHolder)) {
        al::setNerve(this, &NrvStageSceneStateOption.WaitEndAutoSave);
        return;
    }

    al::setNerve(this, &NrvStageSceneStateOption.WaitEndDecideAnim);
}

const al::MessageSystem* StageSceneStateOption::getMessageSystem() const {
    return mMessageSystem;
}
