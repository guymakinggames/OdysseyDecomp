#pragma once

#include <basis/seadTypes.h>
#include <common/aglRenderBuffer.h>
#include <container/seadPtrArray.h>
#include <gfx/seadViewport.h>
#include <gfx/seadCamera.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

#include "Library/Camera/IUseCamera.h"
#include "Library/HostIO/HioNode.h"

namespace al {

class SceneCameraInfo;
class ScreenCapture;

class ScreenCaptureExecutor : public IUseHioNode {
public:
    ScreenCaptureExecutor(s32);
    virtual ~ScreenCaptureExecutor();

    void createScreenCapture(s32, s32, s32);
    void draw(agl::DrawContext*, const agl::RenderBuffer*, s32) const;
    bool tryCapture(agl::DrawContext*, const agl::RenderBuffer*, s32);
    void tryCaptureAndDraw(agl::DrawContext*, const agl::RenderBuffer*, s32);

    void requestCapture(bool, s32);
    void onDraw(s32 screenCaptureIndex);
    void offDraw(s32 screenCaptureIndex);
    void offDraw();

    bool isDraw(s32) const;

private:
    struct CaptureInfo {
        CaptureInfo();

        ScreenCapture* screenCapture;
        bool isCaptureRequested;
        bool isDraw;
    };

    sead::PtrArray<CaptureInfo> mArray;
    bool mIsCaptured = false;
};

static_assert(sizeof(ScreenCaptureExecutor) == 0x20);

u32 getDisplayWidth();
u32 getDisplayHeight();
u32 getSubDisplayWidth();
u32 getSubDisplayHeight();
u32 getLayoutDisplayWidth();
u32 getLayoutDisplayHeight();
u32 getVirtualDisplayWidth();
u32 getVirtualDisplayHeight();
bool isInScreen(const sead::Vector2f& screenPos, f32 tolerance);

void calcScreenPosFromLayoutPos(sead::Vector2f* outScreenPos, const sead::Vector2f& layoutPos);
void calcLayoutPosFromScreenPos(sead::Vector2f* outLayoutPos, const sead::Vector2f& screenPos);
bool calcWorldPosFromScreen(sead::Vector3f* outWorldPos, const sead::Vector2f& screenPos,
                            const sead::Matrix34f& viewMtx, f32 zPos);  // Always returns true
void calcWorldPosFromScreenPos(sead::Vector3f* outWorldPos, const IUseCamera* camera,
                               const sead::Vector2f& screenPos, f32 zPos);
void calcWorldPosFromLayoutPos(sead::Vector3f* outWorldPos, const IUseCamera* camera,
                               const sead::Vector2f& layoutPos, f32 zPos);
void calcWorldPosFromScreenPos(sead::Vector3f* outWorldPos, const IUseCamera* camera,
                               const sead::Vector2f& screenPos, const sead::Vector3f& worldPos);
void calcWorldPosFromLayoutPos(sead::Vector3f* outWorldPos, const IUseCamera* camera,
                               const sead::Vector2f& layoutPos, const sead::Vector3f& worldPos);
void calcWorldPosFromScreenPosSub(sead::Vector3f* outWorldPos, const IUseCamera* camera,
                                  const sead::Vector2f& screenPos, f32 zPos);
void calcWorldPosFromLayoutPosSub(sead::Vector3f* outWorldPos, const IUseCamera* camera,
                                  const sead::Vector2f& layoutPos, f32 zPos);
void calcWorldPosFromScreenPosSub(sead::Vector3f* outWorldPos, const IUseCamera* camera,
                                  const sead::Vector2f& screenPosSub, const sead::Vector3f& zPos);
void calcWorldPosFromLayoutPosSub(sead::Vector3f* outWorldPos, const IUseCamera* camera,
                                  const sead::Vector2f& layoutPos, const sead::Vector3f& worldPos);
void calcScreenPosFromWorldPos(sead::Vector2f* outScreenPos, const IUseCamera* camera,
                               const sead::Vector3f& worldPos);
void calcLayoutPosFromWorldPos(sead::Vector2f* outLayoutPos, const IUseCamera* camera,
                               const sead::Vector3f& worldPos);
void calcScreenPosFromWorldPosSub(sead::Vector2f* outScreenPos, const IUseCamera* camera,
                                  const sead::Vector3f& worldPosSub);
void calcLayoutPosFromWorldPosSub(sead::Vector2f* outLayoutPos, const IUseCamera* camera,
                                  const sead::Vector3f& worldPosSub);
void calcLayoutPosFromWorldPos(sead::Vector3f* outLayoutPos, const IUseCamera* camera,
                               const sead::Vector3f& worldPos);
void calcLayoutPosFromWorldPosWithClampOutRange(sead::Vector3f* outLayoutPos,
                                                const IUseCamera* camera,
                                                const sead::Vector3f& worldPos, f32 range,
                                                s32 viewIdx);
void calcLayoutPosFromWorldPosWithClampOutRange(sead::Vector3f* outLayoutPos,
                                                const SceneCameraInfo* camera,
                                                const sead::Vector3f& worldPos, f32 range,
                                                s32 viewIdx);
void calcLayoutPosFromWorldPosWithClampByScreen(sead::Vector3f* outLayoutPos,
                                                const IUseCamera* camera,
                                                const sead::Vector3f& worldPos);
void calcLayoutPosFromWorldPos(sead::Vector3f* outLayoutPos, const SceneCameraInfo* cameraInfo,
                               const sead::Vector3f& worldPos, s32 viewIdx);
f32 calcScreenRadiusFromWorldRadius(const sead::Vector3f& worldPos, const IUseCamera* camera,
                                    f32 worldRadius);
f32 calcScreenRadiusFromWorldRadiusSub(const sead::Vector3f& worldPos, const IUseCamera* camera,
                                       f32 worldRadius);
f32 calcLayoutRadiusFromWorldRadius(const sead::Vector3f& worldPos, const IUseCamera* camera,
                                    f32 worldRadius);
bool calcCameraPosToWorldPosDirFromScreenPos(sead::Vector3f* outCameraPos, const IUseCamera* camera,
                                             const sead::Vector2f& screenPos, f32 zPos);
bool calcCameraPosToWorldPosDirFromScreenPos(sead::Vector3f* outCameraPos,
                                             const SceneCameraInfo* cameraInfo,
                                             const sead::Vector2f& screenPos, f32 zPos,
                                             s32 viewIdx);
bool calcCameraPosToWorldPosDirFromScreenPos(sead::Vector3f* outCameraPos, const IUseCamera* camera,
                                             const sead::Vector2f& screenPos,
                                             const sead::Vector3f& zPos);
bool calcCameraPosToWorldPosDirFromScreenPos(sead::Vector3f* outCameraPos,
                                             const SceneCameraInfo* camera,
                                             const sead::Vector2f& screenPos,
                                             const sead::Vector3f& zPos, s32 viewIdx);
void calcCameraPosToWorldPosDirFromScreenPosSub(sead::Vector3f* outCameraPos,
                                                const IUseCamera* camera,
                                                const sead::Vector2f& screenPos, f32 zPos);
void calcCameraPosToWorldPosDirFromScreenPosSub(sead::Vector3f* outCameraPos,
                                                const IUseCamera* camera,
                                                const sead::Vector2f& screenPos,
                                                const sead::Vector3f& zPos);
void calcLineCameraToWorldPosFromScreenPos(sead::Vector3f* outLineCamera,
                                           sead::Vector3f* outWorldPos, const IUseCamera* camera,
                                           const sead::Vector2f& screenPos, f32 a, f32 b);
void calcLineCameraToWorldPosFromScreenPos(sead::Vector3f* outLineCamera,
                                           sead::Vector3f* outWorldPos, const IUseCamera* camera,
                                           const sead::Vector2f& screenPos);
void calcLineCameraToWorldPosFromScreenPosSub(sead::Vector3f* outLineCamera,
                                              sead::Vector3f* outWorldPos, const IUseCamera* camera,
                                              const sead::Vector2f& _screenPos, f32 near, f32 far);
void calcLineCameraToWorldPosFromScreenPosSub(sead::Vector3f* outLineCamera,
                                              sead::Vector3f* outWorldPos, const IUseCamera* camera,
                                              const sead::Vector2f& screenPos);
void calcWorldPosFromLayoutPos(sead::Vector3f* outWorldPos, const SceneCameraInfo* cameraInfo,
                               const sead::Vector2f& layoutPos, f32 zPos, s32 viewIdx);
void calcWorldPosFromLayoutPos(sead::Vector3f* outWorldPos, const SceneCameraInfo* cameraInfo,
                               const sead::Vector2f& layoutPos, const sead::Vector3f& worldPos,
                               s32 viewIdx);
void calcWorldPosFromScreenPos(sead::Vector3f* outWorldPos, const SceneCameraInfo* cameraInfo,
                               const sead::Vector2f& screenPos, f32 zPos, s32 viewIdx);
void calcWorldPosFromScreenPos(sead::Vector3f* outWorldPos, const SceneCameraInfo* cameraInfo,
                               const sead::Vector2f& screenPos, const sead::Vector3f& zPos,
                               s32 viewIdx);
void calcLayoutPosFromWorldPos(sead::Vector2f* outLayoutPos, const SceneCameraInfo* cameraInfo,
                               const sead::Vector3f& worldPos, s32 viewIdx);
void calcLineCameraToWorldPosFromScreenPos(sead::Vector3f* outLineCamera,
                                           sead::Vector3f* outWorldPos,
                                           const SceneCameraInfo* cameraInfo,
                                           const sead::Vector2f& screenPos, f32 near, f32 far,
                                           s32 viewIdx);
void calcLineCameraToWorldPosFromScreenPos(sead::Vector3f* outLineCamera,
                                           sead::Vector3f* outWorldPos,
                                           const SceneCameraInfo* cameraInfo,
                                           const sead::Vector2f& screenPos, s32 viewIdx);
}  // namespace al

namespace ScreenFunction {
void calcWorldPositionFromCenterScreen(sead::Vector3f*, const sead::Vector2f&,
                                       const sead::Vector3f&, const sead::Camera&,
                                       const sead::Projection&, const sead::Viewport&);
}  // namespace ScreenFunction
