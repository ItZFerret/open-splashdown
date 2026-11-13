// Widescreen: the game's perspective projection with the horizontal field
// of view the port's picture shape asks for (ports/pc/settings.h).
//
// Math3d.cpp's ProjectionMatrix is compiled as ProjectionMatrix_retail
// (tools/port/build.py); every camera builds its projection, and its
// guard-band projection, through this one. Its x scale is 1 / tan(fov / 2);
// multiplying it by (4/3) / aspect keeps the vertical field of view and
// widens the horizontal one, so the 640x448 picture shown in that shape has
// the right proportions. Clipping and culling follow the matrices.
// 2D drawing (the HUD, menus) doesn't go through a projection: it is
// stretched with the picture.
#include "d3dtypes.h"

_D3DMATRIX ProjectionMatrix_retail(float nearPlane, float farPlane, float fov, float aspect);
extern "C" float port_widescreen_scale(void);

_D3DMATRIX ProjectionMatrix(float nearPlane, float farPlane, float fov, float aspect)
{
    _D3DMATRIX m = ProjectionMatrix_retail(nearPlane, farPlane, fov, aspect);
    m._11 *= port_widescreen_scale();
    return m;
}
