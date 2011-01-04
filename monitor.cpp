/*
    monitor
    ~~~~~~~

    Handles monitoring of directories in $PATH for changes and updates
    directory listings of them.

    :copyright: (c) 2010 David 'dav' Gidwani
    :license: New BSD License. See LICENSE for details.
*/
#include <iostream>
#include <algorithm>
#include "tudor-do.h"
#include "util.h"
#include "monitor.h"

PathMonitor::PathMonitor(Do::t_path_map& path) :
m_thread(0), m_stop(false), m_path(path)
{
}

PathMonitor::~PathMonitor()
{
    {
        Glib::Mutex::Lock lock(this->m_mutex);
        this->m_stop = true;
    }
    if (this->m_thread)
        this->m_thread->join();
}

bool PathMonitor::monitor_directory(const std::string& path)
{
    if (Glib::file_test(path, Glib::FILE_TEST_IS_DIR))
    {
        this->m_watchlist.push_back(path);
        return true;
    }
    return false;
}

bool PathMonitor::update_directory_listing(const std::string& path)
{
    if (Glib::file_test(path, Glib::FILE_TEST_IS_DIR))
    {
        Glib::Dir dir(path);
        std::vector<std::string> listing(dir.begin(), dir.end());
        {
            Glib::Mutex::Lock lock(this->m_mutex);
            this->m_path[path] = listing;
        }
    }
    return false;
}

void PathMonitor::start()
{
    this->m_thread = Glib::Thread::create(sigc::mem_fun(*this,
        &PathMonitor::run), true);
}

void PathMonitor::stop()
{
    Glib::Mutex::Lock lock(this->m_mutex);
    this->m_stop = true;
}

void PathMonitor::run()
{
    Inotify notify;
    notify.SetNonBlock(true);
    for (int i = 0; i < this->m_watchlist.size(); i++)
        try
        {
            InotifyWatch* watch = new InotifyWatch(this->m_watchlist[i],
                IN_CREATE | IN_DELETE | IN_MOVE);
            watch->SetEnabled(true);
            notify.Add(watch);
        } catch (InotifyException) { }
    try
    {
        while (true) {
            {
                Glib::Mutex::Lock lock(this->m_mutex);
                if (this->m_stop)
                    break;
            }
            notify.WaitForEvents();
            size_t count = notify.GetEventCount();
            while (count > 0)
            {
                InotifyEvent event;
                bool got_event = notify.GetEvent(&event);

                if (got_event)
                {
                    const std::string directory = event.GetWatch()->GetPath();

                    Glib::Mutex::Lock lock(this->m_mutex);
                    if (event.IsType(IN_CREATE) ||
                        event.IsType(IN_MOVED_TO))
                        this->m_path[directory].push_back(event.GetName());
                    else if (event.IsType(IN_DELETE) ||
                             event.IsType(IN_MOVED_FROM))
                        this->m_path[directory].erase(
                            ++find(
                                this->m_path[directory].begin(),
                                this->m_path[directory].end(),
                                event.GetName()
                            )
                        );
                    this->sig_changed();
                }
                count--;
            }
        }
    } catch(InotifyException &e) {
        warning(e.GetMessage());
    }
}
