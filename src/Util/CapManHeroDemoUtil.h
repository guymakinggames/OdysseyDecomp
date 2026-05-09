#pragma once

namespace al {
struct ActorInitInfo;
class IUseSceneObjHolder;
class LiveActor;
class Scene;
}  // namespace al

class StageTalkDemoNpcCap;

namespace CapManHeroDemoUtil {
void initCapManHeroDemoDirector(const al::Scene* scene, const al::ActorInitInfo& info);
void initCapManHeroTailJointController(al::LiveActor* actor);
void startCapManHeroCommonSettingAfterShowModel(al::LiveActor* actor);
al::LiveActor* createDemoCapManHero(const char* name, const al::ActorInitInfo& info,
                                    const char* suffix);
void capManHeroControl(al::LiveActor* actor);
void stopTailScroll(al::LiveActor* actor);
void restartTailScroll(al::LiveActor* actor);
al::LiveActor* getCapManHero(const al::IUseSceneObjHolder* sceneObjHolder);
void setTalkDemoFirstMoonGet(StageTalkDemoNpcCap* npc);
void setTalkDemoStageStart(StageTalkDemoNpcCap* npc);
void setTalkDemoMoonRock(StageTalkDemoNpcCap* npc);
}  // namespace CapManHeroDemoUtil
