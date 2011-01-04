/*
    xkeybind
    ~~~~~~~~

    Provides a simple interface to binding hotkeys via Xlib.

    :copyright: (c) 2010 David 'dav' Gidwani
    :license: New BSD License. See LICENSE for details.
*/
#include "util.h"
#include "xkeybind.h"

XKeyBind::XKeyBind() : m_thread(0), m_stop(false)
{
    this->m_dpy  = XKeyBind::get_active_display();
    this->m_root = DefaultRootWindow(this->m_dpy);
}

XKeyBind::~XKeyBind()
{
    {
        Glib::Mutex::Lock lock(this->m_mutex);
        this->m_stop = true;
    }
    if (this->m_thread)
        this->m_thread->join();
}

void XKeyBind::start()
{
    this->m_thread = Glib::Thread::create(sigc::mem_fun(*this,
        &XKeyBind::run), true);
}

void XKeyBind::stop()
{
    Glib::Mutex::Lock lock(this->m_mutex);
    this->m_stop = true;
}

void XKeyBind::bind_key(const std::string& keystring)
{
    size_t pos;
    if (std::string::npos != (pos = keystring.rfind('+')))
    {
        unsigned int keycode, modifiers, numlock_mask;
        numlock_mask = this->get_numlock_mask();
        keycode = XKeyBind::get_keycode(upper(keystring.substr(pos + 1, -1)));
        modifiers = XKeyBind::get_modifiermask(keystring.substr(0, pos));

        XGrabKey(this->m_dpy, keycode, modifiers, this->m_root, True,
                 GrabModeAsync, GrabModeAsync);
        XGrabKey(this->m_dpy, keycode, modifiers|LockMask, this->m_root, True,
                 GrabModeAsync, GrabModeAsync);
        XGrabKey(this->m_dpy, keycode, modifiers|numlock_mask,
                 this->m_root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(this->m_dpy, keycode, modifiers|numlock_mask|LockMask,
                 this->m_root, True, GrabModeAsync, GrabModeAsync);
    }
}

Display* XKeyBind::get_active_display()
{
    Display* m_dpy = XOpenDisplay(NULL);
    if (!m_dpy) fatal_error("unable to open display");
    return m_dpy;
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
    modifiers = split(upper(modifier_str), '+');
    for (int i = 0; i < modifiers.size(); i++)
    {
        if (modifiers[i] == "CTRL")
            mask = mask | ControlMask;
        else if (modifiers[i] == "ALT" || modifiers[i] == "MOD1")
            mask = mask | Mod1Mask;
        else if (modifiers[i] == "SHIFT")
            mask = mask | ShiftMask;
        else if (modifiers[i] == "SUPER" || modifiers[i] == "MOD4")
            mask = mask | Mod4Mask;
    }
    return mask;
}

// Borrowed from dwm: <http://hg.suckless.org/dwm/file/tip/dwm.c#l1848>
unsigned int XKeyBind::get_numlock_mask()
{
    unsigned int i, j, mask(0), numlock;
    Display* m_dpy;
    XModifierKeymap* modmap;

    m_dpy = XKeyBind::get_active_display();
    modmap = XGetModifierMapping(m_dpy);
    numlock = XKeysymToKeycode(m_dpy, XK_Num_Lock);
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
            Glib::Mutex::Lock lock(this->m_mutex);
            if (this->m_stop)
                break;
        }
        XNextEvent(this->m_dpy, &event);
        if (event.type == KeyPress)
            this->sig_done();
    }
}
