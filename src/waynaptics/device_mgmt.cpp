#include "synshared.h"
#include "options.h"
#include "config_socket.h"
#include <glib.h>
#include <map>
#include <string>
#include <fcntl.h>
#include <unistd.h>

static std::map<int, guint> fd_source_map;

static gboolean io_watch_callback(GIOChannel *source, GIOCondition condition, gpointer data)
{
    auto pInfo = static_cast<InputInfoPtr>(data);
    pInfo->read_input(pInfo);
    waynaptics_broadcast_touches(pInfo->dev);
    return G_SOURCE_CONTINUE;
}

extern "C" int xf86OpenSerial(XF86OptionPtr options)
{
    const char *device = xf86CheckStrOption(options, "Device", nullptr);
    if (!device) {
        return -1;
    }
    return open(device, O_RDWR | O_NONBLOCK);
}

extern "C" int xf86CloseSerial(int fd)
{
    return close(fd);
}

extern "C" void xf86AddEnabledDevice(InputInfoPtr pInfo)
{
    GIOChannel *channel = g_io_channel_unix_new(pInfo->fd);
    guint source_id = g_io_add_watch(channel, G_IO_IN, io_watch_callback, pInfo);
    g_io_channel_unref(channel);
    fd_source_map[pInfo->fd] = source_id;
}

extern "C" void xf86RemoveEnabledDevice(InputInfoPtr pInfo)
{
    auto it = fd_source_map.find(pInfo->fd);
    if (it != fd_source_map.end()) {
        g_source_remove(it->second);
        fd_source_map.erase(it);
    }
}

extern "C" void xf86DeleteInput(InputInfoPtr pInfo, int flags)
{
    // No-op: we only have one device
}

extern "C" void xf86AddInputDriver(InputDriverPtr driver, void *module, int flags)
{
    // No-op: module setup not used
}
