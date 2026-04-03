#include "fakeclude/shim.h"
#include <stdbool.h>
#include <synapticsstr.h>

bool waynaptics_get_dimensions(DeviceIntPtr dev, int *minx, int *maxx, int *miny, int *maxy) {
    if (!dev || !dev->public.devicePrivate)
        return false;

    InputInfoPtr pInfo = (InputInfoPtr)dev->public.devicePrivate;
    if (!pInfo->private)
        return false;

    SynapticsPrivate *priv = (SynapticsPrivate *)pInfo->private;

    *minx = priv->minx;
    *maxx = priv->maxx;
    *miny = priv->miny;
    *maxy = priv->maxy;

    return (*maxx > *minx && *maxy > *miny);
}
