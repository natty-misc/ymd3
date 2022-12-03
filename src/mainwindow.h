#ifndef YMD3_MAINWINDOW_H
#define YMD3_MAINWINDOW_H

#include "shared.h"
#include "retrieverscript.h"

#include <mutex>
#include <queue>

namespace YMD
{
    enum class TaskState
    {
            SUCCESS,
            WAITING,
            FAILURE,
            DOWNLOADING
    };

    struct Task
    {
        TaskState state;
        std::optional<RetrieverResult> retrieverResult;
        std::string failureReason;
    };

    class MainWindow : public Gtk::Window
    {
        public:
            explicit MainWindow(const std::string& name);

        private:
            void addTask(const Task& taskIn);
            void showError(const std::string& title, const std::string& text);
            void asyncRetrieve(const std::string& url);

            mutable std::mutex taskQueueMutex;
            std::queue<Task> taskQueue;
            Glib::Dispatcher taskAddDispatcher;

            Gtk::Box* taskList;

            std::shared_ptr<Gtk::MessageDialog> dialog;
    };
}

#endif //YMD3_MAINWINDOW_H
