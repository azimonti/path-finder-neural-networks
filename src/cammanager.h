#ifndef _CAMMANAGER_H_B31470E226E7478FAD6F572DCFD26EC8_
#define _CAMMANAGER_H_B31470E226E7478FAD6F572DCFD26EC8_

/************************/
/*     cammanager.h     */
/*    Version 1.0       */
/*     2022/07/07       */
/************************/

#include "camhandler.h"

namespace ge
{
    class Transform;
    class ScenePickData;
} // namespace ge

class CamManager
{
  public:
    CamManager();

    void DrawCamManagerUI(int rootWinW, int rootWinH);

    void OnMouseButtonCM(ge::Transform& camXF, int button, bool isDown, double x, double y,
                         const std::function<ge::ScenePickData(float, float)>& pickFn);

    void OnScrollCM(ge::Transform& camXF, double x, double y);
    void OnMotionCM(ge::Transform& camXF, double x, double y, int winW, int winH);

    void OnKeyPressCM(int key, bool isDown);

    void AnimateCM(ge::Transform& camXF);

    bool mShoWin = true;

    CH::CamHandMouse mCamHand;
};

#endif
