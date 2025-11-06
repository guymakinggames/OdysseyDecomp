#include "Library/Screen/ScreenFunction.h"

#include <gfx/seadProjection.h>

#include "Library/Camera/CameraUtil.h"
#include "Library/Camera/CameraViewInfo.h"
#include "Library/Camera/SceneCameraInfo.h"
#include "Library/Math/MathUtil.h"
#include "Library/Projection/Projection.h"
#include "Project/Screen/ScreenCapture.h"

namespace al {

u32 getDisplayWidth() {
    return 1280;
}

u32 getDisplayHeight() {
    return 720;
}

u32 getSubDisplayWidth() {
    return 0;
}

u32 getSubDisplayHeight() {
    return 0;
}

u32 getLayoutDisplayWidth() {
    return 1280;
}

u32 getLayoutDisplayHeight() {
    return 720;
}

u32 getVirtualDisplayWidth() {
    return 1280;
}

u32 getVirtualDisplayHeight() {
    return 720;
}

bool isInScreen(const sead::Vector2f& screenPos, f32 tolerance) {
    if (screenPos.x < -tolerance || screenPos.y < -tolerance)
        return false;

    if (screenPos.x < getDisplayWidth() + tolerance && screenPos.y < getDisplayHeight() + tolerance)
        return true;

    return false;
}

void calcScreenPosFromLayoutPos(sead::Vector2f* outScreenPos, const sead::Vector2f& layoutPos) {
    outScreenPos->x = layoutPos.x + getLayoutDisplayWidth() / 2.0f;
    outScreenPos->y = -layoutPos.y + getLayoutDisplayHeight() / 2.0f;
}

inline void calcScreenPosFromLayoutPosSub(sead::Vector2f* outScreenPos,
                                          const sead::Vector2f& layoutPos) {
    outScreenPos->x = layoutPos.x + getSubDisplayWidth() / 2.0f;
    outScreenPos->y = -layoutPos.y + getSubDisplayHeight() / 2.0f;
}

void calcLayoutPosFromScreenPos(sead::Vector2f* outLayoutPos, const sead::Vector2f& screenPos) {
    outLayoutPos->x = screenPos.x - getLayoutDisplayWidth() / 2.0f;
    outLayoutPos->y = -screenPos.y + getLayoutDisplayHeight() / 2.0f;
}

bool calcWorldPosFromScreen(sead::Vector3f* outWorldPos, const sead::Vector2f& screenPos,
                            const sead::Matrix34f& viewMtx, f32 zPos) {
    // NONMATCHING: 10 attempts exhausted.
    // Root cause: clang folds the negated z/layout-y terms into subtract chains,
    // while the target materializes fneg/fnmul and schedules the matrix column
    // loads in a fixed IDA-pseudocode order. Tried flat a[] access, m[row][col]
    // column access, cached translation members, y/z/x offset temporaries,
    // explicit depth temporaries, vector layout locals, and final set()/component
    // stores; each either preserved the same sign-folding diff or worsened load
    // scheduling. Revisit if a neighboring math/source pattern reveals how to
    // keep the negated z value materialized without non-source tricks.
    f32 screenX = screenPos.x;
    f32 screenY = screenPos.y;
    f32 screenScale = 360.0f * sead::Mathf::tan(sead::Mathf::pi() / 8.0f);
    zPos = zPos >= 0.0f ? zPos : screenScale;

    if (!outWorldPos)
        return true;

    f32 rate = zPos / screenScale;
    f32 negZ = -zPos;
    sead::Vector2f layoutPos;
    layoutPos.x = rate * (screenX + -640.0f);
    layoutPos.y = -(rate * (screenY + -360.0f));

    f32 worldZ = ((layoutPos.x * viewMtx.a[2] + viewMtx.a[6] * layoutPos.y) +
                  viewMtx.a[10] * negZ) +
                 ((-(viewMtx.a[2] * viewMtx.a[3]) - viewMtx.a[6] * viewMtx.a[7]) -
                  viewMtx.a[10] * viewMtx.a[11]);
    f32 worldY = ((layoutPos.x * viewMtx.a[1] + viewMtx.a[5] * layoutPos.y) +
                  viewMtx.a[9] * negZ) +
                 ((-(viewMtx.a[1] * viewMtx.a[3]) - viewMtx.a[5] * viewMtx.a[7]) -
                  viewMtx.a[9] * viewMtx.a[11]);

    f32 worldX = ((layoutPos.x * viewMtx.a[0] + viewMtx.a[4] * layoutPos.y) +
                  viewMtx.a[8] * negZ) +
                 ((-(viewMtx.a[0] * viewMtx.a[3]) - viewMtx.a[4] * viewMtx.a[7]) -
                  viewMtx.a[8] * viewMtx.a[11]);
    outWorldPos->set(worldX, worldY, worldZ);
    return true;
}

void calcWorldPosFromScreenPos(sead::Vector3f* outWorldPos, const IUseCamera* camera,
                               const sead::Vector2f& screenPos, f32 zPos) {
    sead::Vector2f layoutPos = {screenPos.x - getDisplayWidth() / 2.0f,
                                -(screenPos.y - getDisplayHeight() / 2.0f)};
    calcWorldPosFromLayoutPos(outWorldPos, camera, layoutPos, zPos);
}

inline void normalizeCamera(sead::Vector3f* cameraPos, f32 zz) {
    if (zz < -0.0f) {
        const f32 inv_len = zz / cameraPos->z;
        cameraPos->x *= inv_len;
        cameraPos->y *= inv_len;
        cameraPos->z *= inv_len;
    }
}

void calcWorldPosFromLayoutPos(sead::Vector3f* outWorldPos, const IUseCamera* camera,
                               const sead::Vector2f& layoutPos, f32 zPos) {
    sead::Vector2f screenPos;
    sead::Vector3f cameraPos;
    sead::Viewport viewPort(0.0f, 0.0f, getProjectionSead(camera, 0).getAspect() * 720.0f, 720.0f);

    const sead::LookAtCamera& lookAt = getLookAtCamera(camera, 0);
    const sead::Projection& projection = getProjectionSead(camera, 0);

    screenPos = layoutPos;
    screenPos.x /= viewPort.getHalfSizeX();
    screenPos.y /= viewPort.getHalfSizeY();
    projection.screenPosToCameraPos(&cameraPos, screenPos);

    normalizeCamera(&cameraPos, -zPos);

    lookAt.cameraPosToWorldPosByMatrix(outWorldPos, cameraPos);
}

void calcWorldPosFromScreenPos(sead::Vector3f* outWorldPos, const IUseCamera* camera,
                               const sead::Vector2f& screenPos, const sead::Vector3f& worldPos) {
    sead::Vector2f layoutPos = {screenPos.x - getDisplayWidth() / 2.0f,
                                -(screenPos.y - getDisplayHeight() / 2.0f)};
    calcWorldPosFromLayoutPos(outWorldPos, camera, layoutPos, worldPos);
}

void calcWorldPosFromLayoutPos(sead::Vector3f* outWorldPos, const IUseCamera* camera,
                               const sead::Vector2f& layoutPos, const sead::Vector3f& worldPos) {
    sead::Vector2f screenPos;
    sead::Vector3f cameraPos;
    sead::Vector3f worldCameraPos;
    sead::Viewport viewPort(0.0f, 0.0f, getProjectionSead(camera, 0).getAspect() * 720.0f, 720.0f);

    const sead::LookAtCamera& lookAt = getLookAtCamera(camera, 0);
    const sead::Projection& projection = getProjectionSead(camera, 0);

    lookAt.worldPosToCameraPosByMatrix(&worldCameraPos, worldPos);
    f32 cameraZ = worldCameraPos.z;

    screenPos = layoutPos;
    screenPos.x /= viewPort.getHalfSizeX();
    screenPos.y /= viewPort.getHalfSizeY();
    projection.screenPosToCameraPos(&cameraPos, screenPos);

    normalizeCamera(&cameraPos, cameraZ);

    lookAt.cameraPosToWorldPosByMatrix(outWorldPos, cameraPos);
}

void calcWorldPosFromScreenPosSub(sead::Vector3f* outWorldPos, const IUseCamera* camera,
                                  const sead::Vector2f& screenPos, f32 zPos) {
    sead::Vector2f layoutPos = {screenPos.x, -screenPos.y};
    calcWorldPosFromLayoutPosSub(outWorldPos, camera, layoutPos, zPos);
}

void calcWorldPosFromLayoutPosSub(sead::Vector3f* outWorldPos, const IUseCamera* camera,
                                  const sead::Vector2f& layoutPos, f32 zPos) {
    sead::Vector2f screenPos;
    sead::Vector3f cameraPos;
    sead::Vector3f worldCameraPos;
    sead::Viewport viewPort(0.0f, 0.0f, 0.0f, 0.0f);

    const sead::LookAtCamera& lookAt = getLookAtCamera(camera, getViewNumMax(camera) - 1);
    const sead::Projection& projection = getProjectionSead(camera, getViewNumMax(camera) - 1);

    screenPos = layoutPos;
    screenPos.x /= viewPort.getHalfSizeX();
    screenPos.y /= viewPort.getHalfSizeY();
    projection.screenPosToCameraPos(&cameraPos, screenPos);

    normalizeCamera(&cameraPos, -zPos);

    lookAt.cameraPosToWorldPosByMatrix(outWorldPos, cameraPos);
}

void calcWorldPosFromScreenPosSub(sead::Vector3f* outWorldPos, const IUseCamera* camera,
                                  const sead::Vector2f& screenPosSub,
                                  const sead::Vector3f& worldPos) {
    sead::Vector2f layoutPosSub = {screenPosSub.x, -screenPosSub.y};
    calcWorldPosFromLayoutPosSub(outWorldPos, camera, layoutPosSub, worldPos);
}

void calcWorldPosFromLayoutPosSub(sead::Vector3f* outWorldPos, const IUseCamera* camera,
                                  const sead::Vector2f& layoutPos, const sead::Vector3f& worldPos) {
    sead::Vector2f screenPos;
    sead::Vector3f cameraPos;
    sead::Vector3f worldCameraPos;
    sead::Viewport viewPort(0.0f, 0.0f, 0.0f, 0.0f);

    const sead::LookAtCamera& lookAt = getLookAtCamera(camera, getViewNumMax(camera) - 1);
    const sead::Projection& projection = getProjectionSead(camera, getViewNumMax(camera) - 1);

    lookAt.worldPosToCameraPosByMatrix(&worldCameraPos, worldPos);
    f32 cameraZ = worldCameraPos.z;

    screenPos = layoutPos;
    screenPos.x /= viewPort.getHalfSizeX();
    screenPos.y /= viewPort.getHalfSizeY();
    projection.screenPosToCameraPos(&cameraPos, screenPos);

    normalizeCamera(&cameraPos, cameraZ);

    lookAt.cameraPosToWorldPosByMatrix(outWorldPos, cameraPos);
}

}  // namespace al

namespace ScreenFunction {

inline void normalizeCamera(sead::Vector3f* cameraPos, f32 zz) {
    if (zz < -0.0f) {
        const f32 inv_len = zz / cameraPos->z;
        cameraPos->x *= inv_len;
        cameraPos->y *= inv_len;
        cameraPos->z *= inv_len;
    }
}

void calcWorldPositionFromCenterScreen(sead::Vector3f* outWorldPos, const sead::Vector2f& layoutPos,
                                       const sead::Vector3f& worldPos, const sead::Camera& camera,
                                       const sead::Projection& projection,
                                       const sead::Viewport& viewPort) {
    sead::Vector2f screenPos;
    sead::Vector3f cameraPos;
    sead::Vector3f worldCameraPos;

    camera.worldPosToCameraPosByMatrix(&worldCameraPos, worldPos);
    f32 cameraZ = worldCameraPos.z;

    screenPos = layoutPos;
    screenPos.x /= viewPort.getHalfSizeX();
    screenPos.y /= viewPort.getHalfSizeY();

    projection.screenPosToCameraPos(&cameraPos, screenPos);

    normalizeCamera(&cameraPos, cameraZ);

    camera.cameraPosToWorldPosByMatrix(outWorldPos, cameraPos);
}

}  // namespace ScreenFunction

// Note: New file?

namespace al {
void calcScreenPosFromWorldPos(sead::Vector2f* outScreenPos, const IUseCamera* camera,
                               const sead::Vector3f& worldPos) {
    calcLayoutPosFromWorldPos(outScreenPos, camera, worldPos);
    calcScreenPosFromLayoutPos(outScreenPos, *outScreenPos);
}

void calcLayoutPosFromWorldPos(sead::Vector2f* outLayoutPos, const IUseCamera* camera,
                               const sead::Vector3f& worldPos) {
    sead::Viewport viewPort(0.0f, 0.0f, getProjectionSead(camera, 0).getAspect() * 720.0f, 720.0f);

    getLookAtCamera(camera, 0).projectByMatrix(outLayoutPos, worldPos, getProjectionSead(camera, 0),
                                               viewPort);
}

void calcScreenPosFromWorldPosSub(sead::Vector2f* outScreenPos, const IUseCamera* camera,
                                  const sead::Vector3f& worldPosSub) {
    calcLayoutPosFromWorldPosSub(outScreenPos, camera, worldPosSub);
    calcScreenPosFromLayoutPosSub(outScreenPos, *outScreenPos);
}

void calcLayoutPosFromWorldPosSub(sead::Vector2f* outLayoutPos, const IUseCamera* camera,
                                  const sead::Vector3f& worldPosSub) {
    sead::Viewport viewPort(0.0f, 0.0f, 0.0f, 0.0f);

    getLookAtCamera(camera, getViewNumMax(camera) - 1)
        .projectByMatrix(outLayoutPos, worldPosSub,
                         getProjectionSead(camera, getViewNumMax(camera) - 1), viewPort);
}

void calcLayoutPosFromWorldPos(sead::Vector3f* outLayoutPos, const IUseCamera* camera,
                               const sead::Vector3f& worldPos) {
    sead::Viewport viewPort(0.0f, 0.0f, getProjectionSead(camera, 0).getAspect() * 720.0f, 720.0f);

    sead::Vector3f cameraPos;
    getLookAtCamera(camera, 0).worldPosToCameraPosByMatrix(&cameraPos, worldPos);

    sead::Vector2f layoutPos;
    getProjectionSead(camera, 0).project(&layoutPos, cameraPos, viewPort);

    outLayoutPos->set(layoutPos.x, layoutPos.y, cameraPos.z);
}

void calcLayoutPosFromWorldPosWithClampOutRange(sead::Vector3f* outLayoutPos,
                                                const IUseCamera* camera,
                                                const sead::Vector3f& worldPos, f32 range,
                                                s32 viewIdx) {
    calcLayoutPosFromWorldPosWithClampOutRange(outLayoutPos, getSceneCameraInfo(camera), worldPos,
                                               range, viewIdx);
}

void calcLayoutPosFromWorldPosWithClampOutRange(sead::Vector3f* outLayoutPos,
                                                const SceneCameraInfo* camera,
                                                const sead::Vector3f& worldPos, f32 range,
                                                s32 viewIdx) {
    sead::Viewport viewPort(0.0f, 0.0f, 1280.0f, 720.0f);

    sead::Vector3f cameraPos;
    getLookAtCamera(camera, viewIdx).worldPosToCameraPosByMatrix(&cameraPos, worldPos);

    if (-range < cameraPos.z && range > cameraPos.z) {
        if (cameraPos.z > 0.0f)
            cameraPos.z = range;
        else
            cameraPos.z = -range;
    }

    sead::Vector2f layoutPos;
    getProjectionSead(camera, viewIdx).project(&layoutPos, cameraPos, viewPort);

    outLayoutPos->set(layoutPos.x, layoutPos.y, cameraPos.z);
}

void calcLayoutPosFromWorldPosWithClampByScreen(sead::Vector3f* outLayoutPos,
                                                const IUseCamera* camera,
                                                const sead::Vector3f& worldPos) {
    calcLayoutPosFromWorldPos(outLayoutPos, getSceneCameraInfo(camera), worldPos, 0);

    outLayoutPos->x = sead::Mathf::clamp(outLayoutPos->x, -640.0f, 640.0f);
    outLayoutPos->y = sead::Mathf::clamp(outLayoutPos->y, -360.0f, 360.0f);
}

void calcLayoutPosFromWorldPos(sead::Vector3f* outLayoutPos, const SceneCameraInfo* cameraInfo,
                               const sead::Vector3f& worldPos, s32 viewIdx) {
    sead::Viewport viewPort(0.0f, 0.0f, 1280.0f, 720.0f);

    sead::Vector3f cameraPos;
    getLookAtCamera(cameraInfo, viewIdx).worldPosToCameraPosByMatrix(&cameraPos, worldPos);

    sead::Vector2f layoutPos;
    getProjectionSead(cameraInfo, viewIdx).project(&layoutPos, cameraPos, viewPort);

    outLayoutPos->set(layoutPos.x, layoutPos.y, cameraPos.z);
}

f32 calcScreenRadiusFromWorldRadius(const sead::Vector3f& worldPos, const IUseCamera* camera,
                                    f32 worldRadius) {
    sead::Viewport viewPort(0.0f, 0.0f, 1280.0f, 720.0f);

    sead::Vector3f cameraPos;
    getLookAtCamera(camera, 0).worldPosToCameraPosByMatrix(&cameraPos, worldPos);

    cameraPos.x = worldRadius;
    cameraPos.y = 0.0f;

    sead::Vector2f layoutPos;
    getProjectionSead(camera, 0).project(&layoutPos, cameraPos, viewPort);
    return layoutPos.x;
}

f32 calcScreenRadiusFromWorldRadiusSub(const sead::Vector3f& worldPos, const IUseCamera* camera,
                                       f32 worldRadius) {
    sead::Viewport viewPort(0.0f, 0.0f, 0.0f, 0.0f);

    sead::Vector3f cameraPos;
    getLookAtCamera(camera, getViewNumMax(camera) - 1)
        .worldPosToCameraPosByMatrix(&cameraPos, worldPos);

    cameraPos.x = worldRadius;
    cameraPos.y = 0.0f;

    sead::Vector2f layoutPos;
    getProjectionSead(camera, getViewNumMax(camera) - 1).project(&layoutPos, cameraPos, viewPort);
    return layoutPos.x;
}

f32 calcLayoutRadiusFromWorldRadius(const sead::Vector3f& worldPos, const IUseCamera* camera,
                                    f32 worldRadius) {
    sead::Vector3f cameraPos;
    getLookAtCamera(camera, 0).worldPosToCameraPosByMatrix(&cameraPos, worldPos);

    cameraPos.x = worldRadius;
    cameraPos.y = 0.0f;

    sead::Viewport viewPort(0.0f, 0.0f, getProjectionSead(camera, 0).getAspect() * 720.0f, 720.0f);

    sead::Vector2f layoutPos;
    getProjectionSead(camera, 0).project(&layoutPos, cameraPos, viewPort);
    return layoutPos.x;
}

bool calcCameraPosToWorldPosDirFromScreenPos(sead::Vector3f* outCameraPos, const IUseCamera* camera,
                                             const sead::Vector2f& screenPos, f32 zPos) {
    return calcCameraPosToWorldPosDirFromScreenPos(outCameraPos, getSceneCameraInfo(camera),
                                                   screenPos, zPos, 0);
}

bool calcCameraPosToWorldPosDirFromScreenPos(sead::Vector3f* outCameraPos,
                                             const SceneCameraInfo* cameraInfo,
                                             const sead::Vector2f& screenPos, f32 zPos,
                                             s32 viewIdx) {
    sead::Vector2f layoutPos = {screenPos.x - (viewIdx == 1 ? 0.0f : 640.0f),
                                -(screenPos.y - (viewIdx == 1 ? 0.0f : 360.0f))};
    sead::Vector3f worldPos;
    calcWorldPosFromLayoutPos(&worldPos, cameraInfo, layoutPos, zPos, viewIdx);

    outCameraPos->setSub(worldPos, getCameraPos(cameraInfo, viewIdx));
    return tryNormalizeOrZero(outCameraPos);
}

bool calcCameraPosToWorldPosDirFromScreenPos(sead::Vector3f* outCameraPos, const IUseCamera* camera,
                                             const sead::Vector2f& screenPos,
                                             const sead::Vector3f& zPos) {
    return calcCameraPosToWorldPosDirFromScreenPos(outCameraPos, getSceneCameraInfo(camera),
                                                   screenPos, zPos, 0);
}

bool calcCameraPosToWorldPosDirFromScreenPos(sead::Vector3f* outCameraPos,
                                             const SceneCameraInfo* camera,
                                             const sead::Vector2f& screenPos,
                                             const sead::Vector3f& zPos, s32 viewIdx) {
    sead::Vector2f layoutPos = {screenPos.x - (viewIdx == 1 ? 0.0f : 640.0f),
                                -(screenPos.y - (viewIdx == 1 ? 0.0f : 360.0f))};

    sead::Vector3f worldPos;
    calcWorldPosFromLayoutPos(&worldPos, camera, layoutPos, zPos, viewIdx);

    outCameraPos->setSub(worldPos, getCameraPos(camera, viewIdx));
    return tryNormalizeOrZero(outCameraPos);
}

void calcCameraPosToWorldPosDirFromScreenPosSub(sead::Vector3f* outCameraPos,
                                                const IUseCamera* camera,
                                                const sead::Vector2f& screenPos, f32 zPos) {
    sead::Vector2f layoutPos = {screenPos.x, -screenPos.y};

    sead::Vector3f worldPos;
    calcWorldPosFromLayoutPosSub(&worldPos, camera, layoutPos, zPos);

    outCameraPos->setSub(worldPos, getCameraPos(camera, getViewNumMax(camera) - 1));
    tryNormalizeOrZero(outCameraPos);
}

void calcCameraPosToWorldPosDirFromScreenPosSub(sead::Vector3f* outCameraPos,
                                                const IUseCamera* camera,
                                                const sead::Vector2f& screenPos,
                                                const sead::Vector3f& zPos) {
    sead::Vector2f layoutPos = {screenPos.x, -screenPos.y};

    sead::Vector3f worldPos;
    calcWorldPosFromLayoutPosSub(&worldPos, camera, layoutPos, zPos);

    outCameraPos->setSub(worldPos, getCameraPos(camera, getViewNumMax(camera) - 1));
    tryNormalizeOrZero(outCameraPos);
}

void calcLineCameraToWorldPosFromScreenPos(sead::Vector3f* outLineCamera,
                                           sead::Vector3f* outWorldPos, const IUseCamera* camera,
                                           const sead::Vector2f& _screenPos, f32 near, f32 far) {
    const SceneCameraInfo* cameraInfo = getSceneCameraInfo(camera);
    f32 layoutX = _screenPos.x + -640.0f;
    f32 layoutY = -(_screenPos.y + -360.0f);
    sead::Vector3f cameraPos;
    sead::Vector2f projectionPos;
    const sead::LookAtCamera* lookAt = nullptr;

    {
        sead::Viewport viewPort(0.0f, 0.0f,
                                cameraInfo->getViewAt(0)->getProjection().getAspect() * 720.0f,
                                720.0f);

        lookAt = &getLookAtCamera(cameraInfo, 0);
        const sead::Projection& projection = getProjectionSead(cameraInfo, 0);

        projectionPos.x = layoutX / viewPort.getHalfSizeX();
        projectionPos.y = layoutY / viewPort.getHalfSizeY();
        projection.screenPosToCameraPos(&cameraPos, projectionPos);
    }

    sead::Vector3f dir;
    sead::Vector3f worldPos;
    lookAt->cameraPosToWorldPosByMatrix(&worldPos, cameraPos);
    dir.setSub(worldPos, getCameraPos(cameraInfo, 0));
    tryNormalizeOrZero(&dir);

    outLineCamera->setScaleAdd(near, dir, getCameraPos(camera, 0));
    outWorldPos->set((far - near) * dir);
}

void calcLineCameraToWorldPosFromScreenPos(sead::Vector3f* outLineCamera,
                                           sead::Vector3f* outWorldPos, const IUseCamera* camera,
                                           const sead::Vector2f& screenPos) {
    calcLineCameraToWorldPosFromScreenPos(outLineCamera, outWorldPos, camera, screenPos,
                                          getNear(camera, 0), getFar(camera, 0));
}

void calcLineCameraToWorldPosFromScreenPosSub(sead::Vector3f* outLineCamera,
                                              sead::Vector3f* outWorldPos, const IUseCamera* camera,
                                              const sead::Vector2f& _screenPos, f32 near, f32 far) {
    f32 layoutX = _screenPos.x;
    f32 layoutY = -_screenPos.y;
    sead::Vector2f projectionPos;
    sead::Vector3f cameraPos;
    const sead::LookAtCamera* lookAt = nullptr;

    {
        sead::Viewport viewPort(0.0f, 0.0f, 0.0f, 0.0f);

        lookAt = &getLookAtCamera(camera, getViewNumMax(camera) - 1);
        const sead::Projection& projection = getProjectionSead(camera, getViewNumMax(camera) - 1);

        projectionPos.x = layoutX / viewPort.getHalfSizeX();
        projectionPos.y = layoutY / viewPort.getHalfSizeY();
        projection.screenPosToCameraPos(&cameraPos, projectionPos);
    }

    sead::Vector3f dir;
    sead::Vector3f worldPos;
    lookAt->cameraPosToWorldPosByMatrix(&worldPos, cameraPos);
    dir.setSub(worldPos, getCameraPos(camera, getViewNumMax(camera) - 1));
    tryNormalizeOrZero(&dir);

    outLineCamera->setScaleAdd(near, dir, getCameraPos(camera, getViewNumMax(camera) - 1));
    outWorldPos->set((far - near) * dir);
}

void calcLineCameraToWorldPosFromScreenPosSub(sead::Vector3f* outLineCamera,
                                              sead::Vector3f* outWorldPos, const IUseCamera* camera,
                                              const sead::Vector2f& screenPos) {
    calcLineCameraToWorldPosFromScreenPosSub(outLineCamera, outWorldPos, camera, screenPos,
                                             getNear(camera, 0), getFar(camera, 0));
}

void calcWorldPosFromLayoutPos(sead::Vector3f* outWorldPos, const SceneCameraInfo* cameraInfo,
                               const sead::Vector2f& layoutPos, f32 zPos, s32 viewIdx) {
    sead::Vector2f screenPos;
    sead::Vector3f cameraPos;
    f32 viewPortHeight = (viewIdx == 1 ? 0.0f : 720.0f);
    sead::Viewport viewPort(
        0.0f, 0.0f, cameraInfo->getViewAt(viewIdx)->getProjection().getAspect() * viewPortHeight,
        viewPortHeight);

    const sead::LookAtCamera& lookAt = getLookAtCamera(cameraInfo, viewIdx);
    const sead::Projection& projection = getProjectionSead(cameraInfo, viewIdx);

    screenPos = layoutPos;
    screenPos.x /= viewPort.getHalfSizeX();
    screenPos.y /= viewPort.getHalfSizeY();
    projection.screenPosToCameraPos(&cameraPos, screenPos);

    normalizeCamera(&cameraPos, -zPos);

    lookAt.cameraPosToWorldPosByMatrix(outWorldPos, cameraPos);
}

void calcWorldPosFromLayoutPos(sead::Vector3f* outWorldPos, const SceneCameraInfo* cameraInfo,
                               const sead::Vector2f& layoutPos, const sead::Vector3f& worldPos,
                               s32 viewIdx) {
    sead::Vector2f screenPos;
    sead::Vector3f cameraPos;
    f32 viewPortHeight = (viewIdx == 1 ? 0.0f : 720.0f);
    sead::Vector3f worldCameraPos;
    sead::Viewport viewPort(
        0.0f, 0.0f, cameraInfo->getViewAt(viewIdx)->getProjection().getAspect() * viewPortHeight,
        viewPortHeight);

    const sead::LookAtCamera& lookAt = getLookAtCamera(cameraInfo, viewIdx);
    const sead::Projection& projection = getProjectionSead(cameraInfo, viewIdx);

    lookAt.worldPosToCameraPosByMatrix(&worldCameraPos, worldPos);
    f32 cameraZ = worldCameraPos.z;

    screenPos = layoutPos;
    screenPos.x /= viewPort.getHalfSizeX();
    screenPos.y /= viewPort.getHalfSizeY();
    projection.screenPosToCameraPos(&cameraPos, screenPos);

    normalizeCamera(&cameraPos, cameraZ);

    lookAt.cameraPosToWorldPosByMatrix(outWorldPos, cameraPos);
}

void calcWorldPosFromScreenPos(sead::Vector3f* outWorldPos, const SceneCameraInfo* cameraInfo,
                               const sead::Vector2f& screenPos, f32 zPos, s32 viewIdx) {
    sead::Vector2f layoutPos = {screenPos.x - (viewIdx == 1 ? 0.0f : 640.0f),
                                -(screenPos.y - (viewIdx == 1 ? 0.0f : 360.0f))};

    calcWorldPosFromLayoutPos(outWorldPos, cameraInfo, layoutPos, zPos, viewIdx);
}

void calcWorldPosFromScreenPos(sead::Vector3f* outWorldPos, const SceneCameraInfo* cameraInfo,
                               const sead::Vector2f& screenPos, const sead::Vector3f& zPos,
                               s32 viewIdx) {
    sead::Vector2f layoutPos = {screenPos.x - (viewIdx == 1 ? 0.0f : 640.0f),
                                -(screenPos.y - (viewIdx == 1 ? 0.0f : 360.0f))};

    calcWorldPosFromLayoutPos(outWorldPos, cameraInfo, layoutPos, zPos, viewIdx);
}

void calcLayoutPosFromWorldPos(sead::Vector2f* outLayoutPos, const SceneCameraInfo* cameraInfo,
                               const sead::Vector3f& worldPos, s32 viewIdx) {
    sead::Viewport viewPort(0.0f, 0.0f, 1280.0f, 720.0f);
    getLookAtCamera(cameraInfo, viewIdx)
        .projectByMatrix(outLayoutPos, worldPos, getProjectionSead(cameraInfo, viewIdx), viewPort);
}

void calcLineCameraToWorldPosFromScreenPos(sead::Vector3f* outLineCamera,
                                           sead::Vector3f* outWorldPos,
                                           const SceneCameraInfo* cameraInfo,
                                           const sead::Vector2f& screenPos, f32 near, f32 far,
                                           s32 viewIdx) {
    sead::Vector3f cameraPos;
    sead::Vector2f projectionPos;
    const sead::LookAtCamera* lookAt = nullptr;
    f32 layoutX = screenPos.x - (viewIdx == 1 ? 0.0f : 640.0f);
    f32 layoutY = -(screenPos.y - (viewIdx == 1 ? 0.0f : 360.0f));
    f32 viewPortHeight = viewIdx == 1 ? 0.0f : 720.0f;

    {
        sead::Viewport viewPort(0.0f, 0.0f,
                                cameraInfo->getViewAt(viewIdx)->getProjection().getAspect() *
                                    viewPortHeight,
                                viewPortHeight);

        lookAt = &getLookAtCamera(cameraInfo, viewIdx);
        const sead::Projection& projection = getProjectionSead(cameraInfo, viewIdx);

        projectionPos.x = layoutX / viewPort.getHalfSizeX();
        projectionPos.y = layoutY / viewPort.getHalfSizeY();

        projection.screenPosToCameraPos(&cameraPos, projectionPos);
    }

    sead::Vector3f dir;
    sead::Vector3f worldPos;
    lookAt->cameraPosToWorldPosByMatrix(&worldPos, cameraPos);
    dir.setSub(worldPos, getCameraPos(cameraInfo, viewIdx));
    tryNormalizeOrZero(&dir);

    outLineCamera->setScaleAdd(near, dir, getCameraPos(cameraInfo, viewIdx));
    outWorldPos->set((far - near) * dir);
}

void calcLineCameraToWorldPosFromScreenPos(sead::Vector3f* outLineCamera,
                                           sead::Vector3f* outWorldPos,
                                           const SceneCameraInfo* cameraInfo,
                                           const sead::Vector2f& screenPos, s32 viewIdx) {
    calcLineCameraToWorldPosFromScreenPos(outLineCamera, outWorldPos, cameraInfo, screenPos,
                                          getNear(cameraInfo, 0), getFar(cameraInfo, 0), viewIdx);
}

ScreenCaptureExecutor::CaptureInfo::CaptureInfo()
    : screenCapture(nullptr), isCaptureRequested(false), isDraw(false) {}

ScreenCaptureExecutor::ScreenCaptureExecutor(s32 maxCaptures) {
    mArray.allocBuffer(maxCaptures, nullptr, 8);

    for (s32 i = 0; i < mArray.capacity(); i++)
        mArray.pushBack(new CaptureInfo);
}

ScreenCaptureExecutor::~ScreenCaptureExecutor() {
    // NONMATCHING: 10 attempts exhausted.
    // Root cause: the target keeps a distinct branch from the virtual
    // ScreenCapture delete to the CaptureInfo delete block, then reloads the
    // PtrArray size with LDR W8 and compares with `cmp x21, w8, sxtw`.
    // Straight `delete info->screenCapture; delete info;`, explicit null
    // checks, else-if block shaping, cached size variants, and wider/narrower
    // index locals all either preserved the missing branch or changed the loop
    // compare into an ldrsw/cmp-x form. The current source is the cleanest form
    // that preserves behavior while leaving the codegen mismatch documented.
    for (s32 i = 0; i < mArray.size(); i++) {
        CaptureInfo* info = mArray(i);
        delete info->screenCapture;
        delete info;
    }
}

void ScreenCaptureExecutor::createScreenCapture(s32 width, s32 height, s32 screenCaptureIndex) {
    mArray(screenCaptureIndex)->screenCapture = new ScreenCapture(width, height);
}

void ScreenCaptureExecutor::tryCaptureAndDraw(agl::DrawContext* drawContext,
                                              const agl::RenderBuffer* renderBuffer,
                                              s32 screenCaptureIndex) {
    CaptureInfo* info = mArray(screenCaptureIndex);
    if (info->isDraw)
        info->screenCapture->drawCaptureImage(drawContext, renderBuffer);

    CaptureInfo* captureInfo = nullptr;
    if (u32(screenCaptureIndex) < u32(mArray.size()))
        captureInfo = mArray(screenCaptureIndex);

    if (captureInfo->isCaptureRequested) {
        captureInfo->screenCapture->copyImageFromFrameBuffer(drawContext, renderBuffer);
        captureInfo->isCaptureRequested = false;
        captureInfo->isDraw = true;
        mIsCaptured = true;
    }
}

bool ScreenCaptureExecutor::isDraw(s32 screenCaptureIndex) const {
    return mArray(screenCaptureIndex)->isDraw;
}

void ScreenCaptureExecutor::draw(agl::DrawContext* drawContext, const agl::RenderBuffer* renderBuffer,
                                 s32 screenCaptureIndex) const {
    mArray(screenCaptureIndex)->screenCapture->drawCaptureImage(drawContext, renderBuffer);
}

bool ScreenCaptureExecutor::tryCapture(agl::DrawContext* drawContext,
                                       const agl::RenderBuffer* renderBuffer,
                                       s32 screenCaptureIndex) {
    CaptureInfo* info = nullptr;
    if (u32(screenCaptureIndex) < u32(mArray.size()))
        info = mArray(screenCaptureIndex);

    if (!info->isCaptureRequested)
        return false;

    info->screenCapture->copyImageFromFrameBuffer(drawContext, renderBuffer);
    info->isCaptureRequested = false;
    info->isDraw = true;
    mIsCaptured = true;
    return true;
}

void ScreenCaptureExecutor::requestCapture(bool isOffDraw, s32 screenCaptureIndex) {
    mArray(screenCaptureIndex)->isCaptureRequested = true;
    if (isOffDraw)
        mArray(screenCaptureIndex)->isDraw = false;
}

void ScreenCaptureExecutor::onDraw(s32 screenCaptureIndex) {
    mArray(screenCaptureIndex)->isDraw = true;
}

void ScreenCaptureExecutor::offDraw(s32 screenCaptureIndex) {
    mArray(screenCaptureIndex)->isDraw = false;
}

void ScreenCaptureExecutor::offDraw() {
    mArray(0)->isDraw = false;
    mIsCaptured = false;
}

}  // namespace al
