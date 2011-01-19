/*
    tudor-do
    ~~~~~~~~

    *tudor-do* is a lightweight run dialog that has a minimal feature set and
    resource footprint. It also runs in the background, so you won't have to
    worry about terrible responsiveness when you're multi-tasking.

    :copyright: (c) 2010 David 'dav' Gidwani
    :license: New BSD License. See LICENSE for details.
*/
#ifndef TUDOR_DO_H
#define TUDOR_DO_H
#include <map>
#include <vector>
#include <glibmm.h>
#include <gtkmm.h>
#include "xkeybind.h"

class PathMonitor;

class Do : public Gtk::Window
{
    public:
        typedef std::map<std::string, std::vector<std::string> > t_path_map;

        Do();
        virtual ~Do();
        void bind_key(const std::string& keystring);
        void start_xevent_loop();
    protected:
        Glib::RefPtr<Gtk::ListStore>    m_Liststore;
        Gtk::Entry                      m_Entry;
        PathMonitor*                    m_Monitor;
        XKeyBind                        m_Xkb;

        std::vector<std::string>        m_history;
        typedef t_path_map::const_iterator t_path_iter;
        t_path_map                      m_path;

        class PathModelColumns : public Gtk::TreeModel::ColumnRecord
        {
            public:
                Gtk::TreeModelColumn<Glib::ustring> m_col_dir;
                Gtk::TreeModelColumn<Glib::ustring> m_col_file;

                PathModelColumns() {
                    this->add(this->m_col_dir);
                    this->add(this->m_col_file);
                }
        };
        PathModelColumns columns;

        void bind_signals();
        void liststore_append(const std::string& dirname,
                              const std::string& filename);
        void setup_completion();

        void update_path();

        bool on_completion_match(const Glib::ustring& key,
                                 const Gtk::TreeModel::const_iterator& iter);
        bool on_completion_match_selected(const Gtk::TreeModel::iterator& iter);
        bool on_delete_event(GdkEventAny* event);
        bool on_focus_out_event(GdkEventFocus* event);

        void on_entry_activate();
        void on_entry_changed_event();
        bool on_entry_key_pressed_event(GdkEventKey* event);
        bool on_key_pressed_event(GdkEventKey* event);
};

#endif /* TUDOR_DO_H */
