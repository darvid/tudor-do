/*
    xkeybind
    ~~~~~~~~

    Provides a simple interface to binding hotkeys via Xlib.

    :copyright: (c) 2010 David 'dav' Gidwani
    :license: New BSD License. See LICENSE for details.
*/
#include <iostream>
#include "util.h"
#include "xkeybind.h"

XKeyBind::XKeyBind() : thread(0), _stop(false)
{
    this->dpy = XKeyBind::get_active_display();
    this->root = DefaultRootWindow(this->dpy);
}

XKeyBind::~XKeyBind()
{
    {
        Glib::Mutex::Lock lock(this->mutex);
        this->_stop = true;
    }
    if (this->thread)
        this->thread->join();
}

void XKeyBind::start()
{
    this->thread = Glib::Thread::create(sigc::mem_fun(*this,
        &XKeyBind::run), true);
}

void XKeyBind::stop()
{
    Glib::Mutex::Lock lock(this->mutex);
    this->_stop = true;
}

void XKeyBind::bind_key(const std::string& keystring)
{
    size_t pos;
    if (std::string::npos != (pos = keystring.rfind('+')))
    {
        XGrabKey(this->dpy,
                 XKeyBind::get_keycode(keystring.substr(pos + 1, -1)),
                 XKeyBind::get_modifiermask(keystring.substr(0, pos)),
                 this->root, True, GrabModeAsync, GrabModeAsync);
    }
}

Display* XKeyBind::get_active_display()
{
    Display* dpy = XOpenDisplay(NULL);
    if (!dpy) fatal_error("unable to open display");
    return dpy;
}

int XKeyBind::get_keycode(const std::string& character)
{
    return XKeysymToKeycode(XKeyBind::get_active_display(),
        XStringToKeysym(character.c_str()));
}

unsigned int XKeyBind::get_modifiermask(const std::string& modifier_str)
{
    if (modifier_str.empty()) return AnyModifier;
    unsigned int mask = 0;
    std::vector<std::string> modifiers;
    modifiers = split(lower(modifier_str), '+');
    for (int i = 0; i < modifiers.size(); i++)
    {
        if (modifiers[i] == "ctrl")
            mask = mask | ControlMask;
        else if (modifiers[i] == "alt")
            mask = mask | Mod1Mask;
        else if (modifiers[i] == "shift")
            mask = mask | ShiftMask;
        else if (modifiers[i] == "super")
            mask = mask | Mod4Mask;
    }
    return mask;
}

void XKeyBind::run()
{
    XEvent event;
    while (true) {
        {
            Glib::Mutex::Lock lock(this->mutex);
            if (this->_stop)
                break;
        }
        XNextEvent(this->dpy, &event);
        if (event.type == KeyPress)
            this->sig_done();
    }
}
