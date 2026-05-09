#pragma once

#include <basis/seadTypes.h>

namespace al {
struct ActorInitInfo;
class LiveActorGroup;
}  // namespace al

class LinkObjAliveDeadCtrl {
public:
    LinkObjAliveDeadCtrl(const al::ActorInitInfo& info);

    al::LiveActorGroup* off();
    al::LiveActorGroup* on();

private:
    al::LiveActorGroup* mAliveGroup = nullptr;
    al::LiveActorGroup* mDeadGroup = nullptr;
};

static_assert(sizeof(LinkObjAliveDeadCtrl) == 0x10);
