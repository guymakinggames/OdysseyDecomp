#pragma once

#include <basis/seadTypes.h>

namespace agl {
class DrawContext;
class RenderBuffer;
class TextureData;
}  // namespace agl

namespace al {

class ScreenCapture {
public:
    ScreenCapture(s32 width, s32 height);
    virtual ~ScreenCapture();

    void copyImageFromFrameBuffer(agl::DrawContext* drawContext,
                                  const agl::RenderBuffer* renderBuffer);
    void drawCaptureImage(agl::DrawContext* drawContext,
                          const agl::RenderBuffer* renderBuffer) const;

private:
    agl::TextureData* mTextureData = nullptr;
    u8 _10[0x28 - 0x10];
};

static_assert(sizeof(ScreenCapture) == 0x28);

}  // namespace al
