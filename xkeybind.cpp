/*
    xkeybind
    ~~~~~~~~

    Provides a simple interface to binding hotkeys via Xlib.

    :copyright: (c) 2010 David 'dav' Gidwani
    :license: New BSD License. See LICENSE for details.
*/
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
        unsigned int keycode, modifiers, numlock_mask;
        numlock_mask = this->get_numlock_mask();
        keycode = XKeyBind::get_keycode(keystring.substr(pos + 1, -1));
        modifiers = XKeyBind::get_modifiermask(keystring.substr(0, pos));

        XGrabKey(this->dpy, keycode, modifiers, this->root, True,
                 GrabModeAsync, GrabModeAsync);
        XGrabKey(this->dpy, keycode, modifiers|LockMask, this->root, True,
                 GrabModeAsync, GrabModeAsync);
        XGrabKey(this->dpy, keycode, modifiers|numlock_mask,
                 this->root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(this->dpy, keycode, modifiers|numlock_mask|LockMask,
                 this->root, True, GrabModeAsync, GrabModeAsync);
    }
}

Display* XKeyBind::get_active_display()
{
    Display* dpy = XOpenDisplay(NULL);
    if (!dpy) fatal_error("unable to open display");
    return dpy;
}

unsigned int XKeyBind::get_keycode(const std::string& character)
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

// Borrowed from dwm: <http://hg.suckless.org/dwm/file/tip/dwm.c#l1848>
unsigned int XKeyBind::get_numlock_mask()
{
    unsigned int i, j, mask(0), numlock;
    Display* dpy;
    XModifierKeymap* modmap;

    dpy = XKeyBind::get_active_display();
    modmap = XGetModifierMapping(dpy);
    numlock = XKeysymToKeycode(dpy, XK_Num_Lock);
    for (i = 0; i < 8; i++)
        for (j = 0; j < modmap->max_keypermod; j++)
            if (modmap->modifiermap[i*modmap->max_keypermod+j] == numlock)
                mask = (1 << i);
    XFreeModifiermap(modmap);
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
