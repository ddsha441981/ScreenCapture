//
// Created by deendayal on 20/06/25.
//

//---------------------------working with dim screen and visible rectangle----------------------------------------------------

#include "stb_image_write.h"
// #include "external/screen_capture_lite/stb_image_write.h"
#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/shape.h>
#include <X11/Xatom.h>
#include <unistd.h>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <cstring>
#include <chrono>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>

Display* dpy;
Window root, overlay;
GC gc;
int start_x, start_y, end_x, end_y;
bool drawing = false;
bool capturing = false;

std::string get_screenshot_dir() {
    const char* cmd = "xdg-user-dir PICTURES";
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    std::string path;

    if (pipe) {
        char buffer[256];
        if (fgets(buffer, sizeof(buffer), pipe.get())) {
            path = buffer;
            if (!path.empty() && path.back() == '\n')
                path.pop_back();
        }
    }

    if (path.empty()) {
        const char* home = getenv("HOME");
        if (home) path = std::string(home) + "/Pictures";
        else path = ".";
    }

    return path + "/Screenshots";
}

std::string screenshots_dir = get_screenshot_dir();

void create_screenshots_dir() {
    struct stat st = {0};

    if (stat(screenshots_dir.c_str(), &st) == -1) {
        if (mkdir(screenshots_dir.c_str(), 0755) != 0) {
            std::cerr << "[WARNING] Could not create Screenshots directory: " << screenshots_dir << "\n";
            screenshots_dir = "."; 
        } else {
            std::cout << "[INFO] Created Screenshots directory: " << screenshots_dir << std::endl;
        }
    }
}

std::string get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    return ss.str();
}

void draw_rectangle(int x, int y, int w, int h) {
    XClearWindow(dpy, overlay);

    XSetForeground(dpy, gc, WhitePixel(dpy, DefaultScreen(dpy)));
    XDrawRectangle(dpy, overlay, gc, x, y, w, h);

    int corner_size = 6;
    XFillRectangle(dpy, overlay, gc, x - corner_size/2, y - corner_size/2, corner_size, corner_size);
    XFillRectangle(dpy, overlay, gc, x + w - corner_size/2, y - corner_size/2, corner_size, corner_size);
    XFillRectangle(dpy, overlay, gc, x - corner_size/2, y + h - corner_size/2, corner_size, corner_size);
    XFillRectangle(dpy, overlay, gc, x + w - corner_size/2, y + h - corner_size/2, corner_size, corner_size);

    std::string dimensions = std::to_string(w) + "x" + std::to_string(h);
    int text_width = dimensions.length() * 6; 

    XSetForeground(dpy, gc, BlackPixel(dpy, DefaultScreen(dpy)));
    XFillRectangle(dpy, overlay, gc, x + 2, y - 18, text_width + 4, 16);

    XSetForeground(dpy, gc, WhitePixel(dpy, DefaultScreen(dpy)));
    XDrawString(dpy, overlay, gc, x + 4, y - 6, dimensions.c_str(), dimensions.length());

    XFlush(dpy);
}

void show_capture_notification() {
    std::cout << "📸 Screen capture completed!" << std::endl;
}

void capture_area(int x, int y, int w, int h) {
    capturing = true;
    std::cout << "[INFO] Capturing area at (" << x << "," << y << ") size " << w << "x" << h << std::endl;
    show_capture_notification();
    XUnmapWindow(dpy, overlay);
    XFlush(dpy);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    XImage* img = XGetImage(dpy, root, x, y, w, h, AllPlanes, ZPixmap);
    if (!img) {
        std::cerr << "[ERROR] Failed to capture screen!" << std::endl;
        capturing = false;
        return;
    }
    std::vector<unsigned char> pixels(w * h * 4);

    int red_shift = __builtin_ctz(img->red_mask);
    int green_shift = __builtin_ctz(img->green_mask);
    int blue_shift = __builtin_ctz(img->blue_mask);

    for (int row = 0; row < h; ++row) {
        for (int col = 0; col < w; ++col) {
            unsigned long pixel = XGetPixel(img, col, row);
            int idx = (row * w + col) * 4;

            pixels[idx + 0] = (pixel & img->red_mask) >> red_shift;
            pixels[idx + 1] = (pixel & img->green_mask) >> green_shift;
            pixels[idx + 2] = (pixel & img->blue_mask) >> blue_shift;
            pixels[idx + 3] = 255; 
        }
    }

    XDestroyImage(img);

    std::string filename = screenshots_dir + "/screenshot_" + get_timestamp() + ".png";

    if (stbi_write_png(filename.c_str(), w, h, 4, pixels.data(), w * 4)) {
        std::cout << "✅ Screenshot saved: " << filename << " (" << w << "x" << h << ")" << std::endl;
    } else {
        std::cerr << "[ERROR] Failed to save screenshot!" << std::endl;
    }

    capturing = false;

    XDestroyWindow(dpy, overlay);
    XFreeGC(dpy, gc);
    XCloseDisplay(dpy);
    exit(0);
}

void show_help() {
    std::cout << "\n📸 Screen Capture Tool" << std::endl;
    std::cout << "======================" << std::endl;
    std::cout << "Instructions:" << std::endl;
    std::cout << "• Click and drag to select an area" << std::endl;
    std::cout << "• Release mouse to capture the selected area" << std::endl;
    std::cout << "• Press ESC to cancel" << std::endl;
    std::cout << "• Screenshots are saved to: " << screenshots_dir << std::endl;
    std::cout << "\nReady to capture..." << std::endl;
}

int main() {
    std::cout << "Starting Screen Capture Tool..." << std::endl;

    create_screenshots_dir();

    dpy = XOpenDisplay(nullptr);
    if (!dpy) {
        std::cerr << "❌ Cannot open X display" << std::endl;
        return 1;
    }

    int screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);
    int width = DisplayWidth(dpy, screen);
    int height = DisplayHeight(dpy, screen);

    Visual* visual = DefaultVisual(dpy, screen);
    int depth = DefaultDepth(dpy, screen);
    Colormap colormap = DefaultColormap(dpy, screen);
    XSetWindowAttributes attrs;
    attrs.override_redirect = True;
    attrs.background_pixel = 0;
    attrs.border_pixel = 0;
    attrs.colormap = colormap;
    attrs.event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask |
                       PointerMotionMask | KeyPressMask;

    overlay = XCreateWindow(dpy, root, 0, 0, width, height, 0, depth, InputOutput,
                           visual, CWOverrideRedirect | CWBackPixel | CWBorderPixel |
                           CWColormap | CWEventMask, &attrs);

    if (!overlay) {
        std::cerr << "❌ Failed to create overlay window" << std::endl;
        return 1;
    }

    Atom atom_above = XInternAtom(dpy, "_NET_WM_STATE_ABOVE", False);
    Atom atom_state = XInternAtom(dpy, "_NET_WM_STATE", False);
    Atom skip_taskbar = XInternAtom(dpy, "_NET_WM_STATE_SKIP_TASKBAR", False);
    Atom skip_pager = XInternAtom(dpy, "_NET_WM_STATE_SKIP_PAGER", False);

    Atom props[3] = { atom_above, skip_taskbar, skip_pager };
    XChangeProperty(dpy, overlay, atom_state, XA_ATOM, 32, PropModeReplace,
                    reinterpret_cast<unsigned char*>(props), 3);

    Atom wm_name = XInternAtom(dpy, "WM_NAME", False);
    std::string window_name = "Screen Capture Tool";
    XChangeProperty(dpy, overlay, wm_name, XA_STRING, 8, PropModeReplace,
                    reinterpret_cast<const unsigned char*>(window_name.c_str()),
                    window_name.length());

    XMapRaised(dpy, overlay);

    unsigned long opacity = 0x80000000; 
    Atom net_wm_opacity = XInternAtom(dpy, "_NET_WM_WINDOW_OPACITY", False);
    XChangeProperty(dpy, overlay, net_wm_opacity, XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char*)&opacity, 1);

    XFlush(dpy);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    XRectangle rect = { 0, 0, (unsigned short)width, (unsigned short)height };
    XserverRegion region = XFixesCreateRegion(dpy, &rect, 1);
    XFixesSetWindowShapeRegion(dpy, overlay, ShapeInput, 0, 0, region);
    XFixesDestroyRegion(dpy, region);

    gc = XCreateGC(dpy, overlay, 0, nullptr);
    XSetForeground(dpy, gc, WhitePixel(dpy, screen));

    XSetLineAttributes(dpy, gc, 2, LineSolid, CapButt, JoinMiter);

    show_help();

    XEvent ev;
    while (true) {
        XNextEvent(dpy, &ev);

        if (capturing) continue;

        switch (ev.type) {
            case KeyPress: {
                KeySym key = XLookupKeysym(&ev.xkey, 0);
                if (key == XK_Escape) {
                    std::cout << "Cancelled by user" << std::endl;
                    goto cleanup;
                }
                break;
            }
            case ButtonPress:
                if (ev.xbutton.button == Button1) {
                    start_x = ev.xbutton.x;
                    start_y = ev.xbutton.y;
                    drawing = true;
                    std::cout << "[INFO] Selection started at (" << start_x << "," << start_y << ")" << std::endl;
                }
                break;
            case MotionNotify:
                if (drawing) {
                    end_x = ev.xmotion.x;
                    end_y = ev.xmotion.y;
                    int x = std::min(start_x, end_x);
                    int y = std::min(start_y, end_y);
                    int w = abs(end_x - start_x);
                    int h = abs(end_y - start_y);
                    draw_rectangle(x, y, w, h);
                }
                break;
            case ButtonRelease:
                if (ev.xbutton.button == Button1 && drawing) {
                    drawing = false;
                    int x = std::min(start_x, end_x);
                    int y = std::min(start_y, end_y);
                    int w = abs(start_x - end_x);
                    int h = abs(start_y - end_y);

                    if (w > 10 && h > 10) {
                        std::cout << "[INFO] Capturing selection: " << w << "x" << h << std::endl;
                        std::thread([=]() { capture_area(x, y, w, h); }).detach();
                    } else {
                        std::cout << "[INFO] Selection too small, minimum 10x10 pixels" << std::endl;
                        XClearWindow(dpy, overlay);
                    }
                }
                break;
            case Expose:
                if (drawing) {
                    int x = std::min(start_x, end_x);
                    int y = std::min(start_y, end_y);
                    int w = abs(end_x - start_x);
                    int h = abs(end_y - start_y);
                    draw_rectangle(x, y, w, h);
                }
                break;
        }
    }

cleanup:
    XFreeGC(dpy, gc);
    XDestroyWindow(dpy, overlay);
    XCloseDisplay(dpy);
    return 0;
}


