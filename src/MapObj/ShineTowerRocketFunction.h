#pragma once

namespace al {
class LiveActor;
}  // namespace al

class ShineTowerCommonKeeper;

namespace rs {
void setupHomeMeter(al::LiveActor* actor);
void setupHomeMeterDitherParam(al::LiveActor* actor, ShineTowerCommonKeeper* keeper);
void setupHomeSticker(al::LiveActor* actor);
void setupHomeCompLight(al::LiveActor* actor);
const char* getHomeArchiveName(const al::LiveActor* actor);
}  // namespace rs
