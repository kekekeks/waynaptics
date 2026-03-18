#pragma once

struct OutputBackend {
    virtual ~OutputBackend() = default;
    virtual bool init() = 0;
    virtual void destroy() = 0;
    virtual void post_motion(int dx, int dy) = 0;
    virtual void post_button(int button, bool is_down) = 0;
    // Smooth scroll: scroll_type is SCROLL_TYPE_VERTICAL(8) or SCROLL_TYPE_HORIZONTAL(9)
    // value is the raw valuator delta from the synaptics driver
    // scroll_increment is the distance-per-click from SetScrollValuator
    virtual void post_scroll(int scroll_type, double value, double scroll_increment) = 0;
    virtual void sync() = 0;
};

// Global output backend pointer — set by main before driver starts
extern OutputBackend *g_output_backend;
