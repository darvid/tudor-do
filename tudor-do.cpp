/*
    tudor-do
    ~~~~~~~~

    *tudor-do* is a lightweight run dialog that has a minimal feature set and
    resource footprint. It also runs in the background, so you won't have to
    worry about terrible responsiveness when you're multi-tasking.

    :copyright: (c) 2010 David 'dav' Gidwani
    :license: New BSD License. See LICENSE for details.
*/
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <glibmm.h>
#include <glibmm/fileutils.h>
#include <unistd.h>
#include "tudor-do.h"
#include "monitor.h"
#include "util.h"

Do::Do() : m_Xkb(), m_Entry()
{
    this->m_Monitor = new PathMonitor(this->m_path);
    this->update_path();
    this->bind_signals();
    this->setup_completion();

    this->m_Monitor->start();

    this->add(this->m_Entry);
    this->show_all_children();
    this->set_icon_name("applications-system");
    this->set_position(Gtk::WIN_POS_CENTER);
}

Do::~Do()
{
}

void Do::bind_signals()
{
    this->signal_delete_event().connect(sigc::mem_fun(*this,
        &Do::on_delete_event));
    this->signal_key_press_event().connect(sigc::mem_fun(*this,
        &Do::on_key_pressed_event), false);
    this->m_Entry.signal_activate().connect(sigc::mem_fun(*this,
        &Do::on_entry_activate));
    this->m_Entry.signal_changed().connect(sigc::mem_fun(*this,
        &Do::on_entry_changed_event));
    this->m_Entry.signal_key_press_event().connect(sigc::mem_fun(*this,
        &Do::on_entry_key_pressed_event), false);
}

void Do::setup_completion()
{
    Glib::RefPtr<Gtk::EntryCompletion> completion;
    completion = Gtk::EntryCompletion::create();

    this->m_Entry.set_completion(completion);
    this->m_Liststore = Gtk::ListStore::create(this->columns);

    completion->set_model(this->m_Liststore);
    completion->set_inline_completion(true);
    completion->set_popup_single_match(true);
    completion->set_popup_completion(true);
    completion->set_text_column(this->columns.m_col_file);
    completion->set_match_func(sigc::mem_fun(*this, &Do::on_completion_match));
    completion->signal_match_selected().connect(sigc::mem_fun(*this,
        &Do::on_completion_match_selected), false);
}

void Do::bind_key(const std::string& keystring)
{
    this->m_Xkb.bind_key(keystring);
}

void Do::liststore_append(const std::string& dirname,
                          const std::string& filename)
{
    Gtk::TreeModel::Row row = *(this->m_Liststore->append());
    row[this->columns.m_col_dir]  = dirname;
    row[this->columns.m_col_file] = filename;
}

void Do::start_xevent_loop()
{
    this->m_Xkb.start();
    this->m_Xkb.sig_done.connect(sigc::mem_fun(*this, &Gtk::Window::show));
}

void Do::update_path()
{
    std::vector<std::string> dirs;
    std::string path = Glib::getenv("PATH");
    if ((dirs = split(path, ':')).empty())
        fatal_error("missing PATH");
    for (int i=0; i < dirs.size(); i++)
    {
        this->m_Monitor->update_directory_listing(dirs[i]);
        this->m_Monitor->monitor_directory(dirs[i]);
    }
}

void Do::on_entry_activate()
{
    std::string text = this->m_Entry.get_text();
    if (text.substr(0, 1) == "/" || text.substr(0, 7) == "http://")
    {
        if (Glib::file_test(text, Glib::FILE_TEST_IS_REGULAR)
            && 0 == access(text.c_str(), X_OK))
            popen(text.c_str(), "r");
        else
            popen(("xdg-open " + text).c_str(), "r");
    }
    else
        popen(text.c_str(), "r");
    this->m_Entry.set_text("");
    this->hide();
}

bool Do::on_completion_match(const Glib::ustring& key,
                             const Gtk::TreeModel::const_iterator& iter)
{
    if (iter)
    {
        Gtk::TreeModel::Row row = *iter;
        Glib::ustring filename  = row[this->columns.m_col_file];
        filename = filename.lowercase();

        int space_pos;
        Glib::ustring match_string = key;
        if (std::string::npos != (space_pos = find_last_space_pos(key)))
            match_string = key.substr(++space_pos, -1);
        if ((!match_string.empty())
            && (match_string.length() > 2)
            && (filename.substr(0, match_string.size()) == match_string))
            return true;
    }
    return false;
}

bool Do::on_completion_match_selected(const Gtk::TreeModel::iterator& iter)
{
    Gtk::TreeModel::Row row = *iter;
    Glib::ustring text = this->m_Entry.get_text();

    int space_pos;
    if (std::string::npos != (space_pos = find_last_space_pos(text)))
    {
        text.replace(++space_pos, text.length(),
                     row[this->columns.m_col_file]);
        this->m_Entry.set_text(text);
        this->m_Entry.set_position(-1);
        return true;
    }
    return false;
}

bool Do::on_delete_event(GdkEventAny*)
{
    this->hide();
    return false;
}

void Do::on_entry_changed_event()
{
    this->m_Liststore->clear();

    std::string text = this->m_Entry.get_text();
    if (text.length() < 2) return;

    int space_pos = find_last_space_pos(text);
    if (std::string::npos != space_pos)
        text = text.substr(++space_pos, -1);
    if (text.empty() || text.length() <= 2)
        return;

    if (text.substr(0, 1) == "/")
    {
        std::string dir_name, base_name;
        bool walk;

        dir_name  = Glib::path_get_dirname(text);
        base_name = Glib::path_get_basename(text);

        find_and_replace(dir_name, "\\ ", " ");
        if (!Glib::file_test(dir_name, Glib::FILE_TEST_EXISTS)) return;

        Glib::Dir dir(dir_name);
        for (Glib::DirIterator i = dir.begin(); i != dir.end(); i++)
        {
            std::string name = *i;
            if ((text.substr(text.length() - 1, -1) == "/")
                || (name.substr(0, base_name.length()) == base_name))
            {
                std::string full_path = Glib::build_filename(dir_name, name);
                find_and_replace(full_path, " ", "\\ ");
                this->liststore_append(dir_name, full_path);
            }
        }
    }
    Do::t_path_iter map_iter = this->m_path.begin();
    while (map_iter != this->m_path.end())
    {
        for (int i=0; i<(map_iter->second).size(); i++)
        {
            std::string filename = map_iter->second[i];
            if ((text.length() <= filename.length())
                && (filename.compare(0, text.length(), text) == 0))
                this->liststore_append(map_iter->first, filename);
        }
        map_iter++;
    }
}

bool Do::on_entry_key_pressed_event(GdkEventKey* event)
{
    if (event->keyval == GDK_Tab)
    {
        Glib::ustring text;
        text = this->m_Entry.get_text();
        if ((text.length() != 0)
            && (text.substr(text.length() - 1, 1) == "~")
            && (this->m_Entry.get_position() == text.length()))
        {
            std::string home = Glib::getenv("HOME");
            text.replace(text.length() - 1, home.length(), home);
            this->m_Entry.set_text(text);
            this->m_Entry.set_position(-1);
            return true;
        }
        else if (text.empty() || text.length() <= 2) return true;

        Gtk::TreeModel::Row row;
        row  = *(this->m_Liststore->children().begin());
        if (!row)
        {
            this->m_Entry.set_position(-1);
            return true;
        }

        std::string buf = text;
        std::string::size_type pos;

        for (pos = buf.find_last_of(' ');
             pos != std::string::npos;)
        {
            if (buf.substr(pos - 1, 1) == "\\")
            {
                --pos;
                buf = buf.substr(0, pos);
                pos = buf.find_last_of(' ');
            }
            break;
        }

        int sel_start, sel_end;
        this->m_Entry.get_selection_bounds(sel_start, sel_end);

        if ((std::string::npos != pos) && (sel_start == sel_end))
        {
            int cursor_pos;
            cursor_pos = this->m_Entry.get_position();

            text.replace(++pos, text.length(),
                         row[this->columns.m_col_file]);
            this->m_Entry.set_text(text);
            this->m_Entry.select_region(cursor_pos, -1);
        }
        else
            this->m_Entry.set_position(-1);
        return true;
    }
    return false;
}

bool Do::on_key_pressed_event(GdkEventKey* event)
{
    if (event->keyval == GDK_Escape)
    {
        this->m_Entry.set_text("");
        this->hide();
    }
    else if (event->keyval == GDK_Tab)
        this->m_Entry.set_text(this->m_Entry.get_text());
    return false;
}

int main(int argc, char* argv[])
{
    Glib::OptionGroup options("tudor-do", "tudor-do options");
    Glib::OptionEntry entry;

    Glib::ustring hotkey = "Alt+F2";
    entry.set_long_name("hotkey");
    entry.set_short_name('h');
    entry.set_description("Set hotkey string");
    options.add_entry(entry, hotkey);

    bool undecorated(false);
    entry.set_long_name("undecorated");
    entry.set_short_name('u');
    entry.set_description("Undecorate window");
    options.add_entry(entry, undecorated);

    Glib::ustring title = " ";
    entry.set_long_name("title");
    entry.set_short_name('t');
    entry.set_description("Set the title of the window");
    options.add_entry(entry, title);

    bool version(false);
    entry.set_long_name("version");
    entry.set_description("Print version information and exit");
    options.add_entry(entry, version);

    Glib::OptionContext context("");
    context.add_group(options);

    if(!Glib::thread_supported()) Glib::thread_init();
    Gtk::Main kit(argc, argv, context);
    if (version)
    {
        std::cout << "tudor-do 0.1.2" << std::endl;
        return 0;
    }

    Do main_window;
    main_window.bind_key(hotkey);
    main_window.set_decorated(!undecorated);
    main_window.set_title(title);

    main_window.start_xevent_loop();

    kit.run();
    return 0;
}
