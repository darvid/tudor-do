/*
    monitor
    ~~~~~~~

    Handles monitoring of directories in $PATH for changes and updates
    directory listings of them.

    :copyright: (c) 2010 David 'dav' Gidwani
    :license: New BSD License. See LICENSE for details.
*/
#ifndef TUDOR_DO_MONITOR_H
#define TUDOR_DO_MONITOR_H
#include <string>
#include <glibmm.h>
#include "inotify-cxx.h"
#include "tudor-do.h"

class PathMonitor
{
    public:
        Glib::Dispatcher sig_changed;

        PathMonitor(Do::t_path_map& path);
        virtual ~PathMonitor();
        bool monitor_directory(const std::string& path);
        bool update_directory_listing(const std::string& path);
        void start();
        void stop();
    protected:
        Do::t_path_map&              m_path;
        std::vector<std::string>     m_watchlist;

        Glib::Thread*                m_thread;
        Glib::Mutex                  m_mutex;

        bool                         m_stop;

        void run();
};

#endif /* TUDOR_DO_MONITOR_H */
