#pragma once

#include <basis/seadTypes.h>

namespace al {
struct ActorInitInfo;
class LiveActor;
}  // namespace al

class DokanInfo {
public:
    DokanInfo(al::LiveActor* actor, const al::ActorInitInfo& info);

private:
    al::LiveActor* mActor = nullptr;
    u8 _8[0x10] = {};
};

static_assert(sizeof(DokanInfo) == 0x18);
