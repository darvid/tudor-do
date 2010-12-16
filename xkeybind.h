/*
    xkeybind
    ~~~~~~~~

    Provides a simple interface to binding hotkeys via Xlib.

    :copyright: (c) 2010 David 'dav' Gidwani
    :license: New BSD License. See LICENSE for details.
*/
#ifndef TUDOR_XKEYBIND_H
#define TUDOR_XKEYBIND_H
#include <glibmm.h>
#include <gtkmm.h>
#include <X11/Xlib.h>

class XKeyBind {
    public:
        Glib::Dispatcher sig_done;

        XKeyBind();
        virtual ~XKeyBind();
        void start();
        void stop();
        void bind_key(const std::string& keystring);
        static Display* get_active_display();
        static int get_keycode(const std::string& character);
        static unsigned int get_modifiermask(const std::string& modifier_str);
    protected:
        Glib::Thread* thread;
        Glib::Mutex   mutex;
        Display*      dpy;
        Window        root;
        bool          _stop;

        void run();
};

#endif /* TUDOR_XKEYBIND_H */
