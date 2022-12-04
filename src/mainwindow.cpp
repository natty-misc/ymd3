//
// Created by Natty on 25.02.2021.
//

#include "mainwindow.h"
#include "youtuberetriever.h"

#include <iostream>
#include <thread>

YMD::MainWindow::MainWindow(const std::string& name)
{
    this->set_title(name);
    this->set_default_size(800, 600);
    this->property_resizable() = true;
    this->property_modal() = false;

    auto mainBox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 5);
    mainBox->set_margin(10.0);
    this->set_child(*mainBox);

    auto hBox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 5);
    hBox->set_expand(false);
    mainBox->append(*hBox);

    auto urlField = Gtk::make_managed<Gtk::Entry>();
    urlField->set_expand();
    hBox->append(*urlField);

    auto downloadButton = Gtk::make_managed<Gtk::Button>("Download");
    hBox->append(*downloadButton);

    this->taskList = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 5);
    this->taskList->set_margin(10.0);

    auto taskListWrapper = Gtk::make_managed<Gtk::ScrolledWindow>();
    taskListWrapper->set_child(*this->taskList);
    taskListWrapper->set_expand();
    taskListWrapper->set_kinetic_scrolling();
    taskListWrapper->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    mainBox->append(*taskListWrapper);

    this->taskAddDispatcher.connect([this]() -> void {
        std::lock_guard<std::mutex> lock(this->taskQueueMutex);

        while (!this->taskQueue.empty())
        {
            auto& task = this->taskQueue.front();
            this->addTask(task);
            this->taskQueue.pop();
        }
    });

    downloadButton->signal_clicked().connect([urlField, this]() -> void {
        std::string text = urlField->get_text();
        this->asyncRetrieve(text);
    });

    this->show();
}

void YMD::MainWindow::asyncRetrieve(const std::string& url)
{
    std::thread backgroundWorker([url, this]() -> void {
        try
        {
            YouTubeRetriever retrieverInstance(url);
            const std::string& videoID = retrieverInstance.getVideoID();
            RetrieverScript retrieverScript("youtube");
            std::optional<RetrieverResult> videoData = retrieverScript.run(videoID);

            {
                std::lock_guard<std::mutex> lock(this->taskQueueMutex);

                this->taskQueue.emplace(
                        TaskState::WAITING,
                        std::move(videoData)
                );
            }
        }
        catch (RetrieveFailure& e)
        {
            std::lock_guard<std::mutex> lock(this->taskQueueMutex);

            this->taskQueue.emplace(
                    TaskState::FAILURE,
                    std::optional<RetrieverResult>(),
                    e.what()
            );
        }

        this->taskAddDispatcher.emit();
    });

    backgroundWorker.detach();
}

void YMD::MainWindow::addTask(const Task& taskIn)
{
    const auto& videoData = taskIn.retrieverResult;

    if (!videoData)
    {
        this->showError("Error while retrieving video...", "Failed to retrieve the video, please check the logs for more info.");
        return;
    }

    auto task = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 5);
    task->set_margin(5.0);
    task->set_hexpand();
    task->set_halign(Gtk::Align::START);

    std::stringstream labelHTML;
    labelHTML << "<a href=\"" << videoData->originalURL << "\">";
    labelHTML << videoData->videoName;
    labelHTML << "</a>";

    auto videoNameLabel = Gtk::make_managed<Gtk::Label>();
    videoNameLabel->set_markup(labelHTML.str());
    videoNameLabel->set_halign(Gtk::Align::START);
    task->append(*videoNameLabel);

    auto videoAuthorLabel = Gtk::make_managed<Gtk::Label>(videoData->videoAuthor.value_or("<unknown author>"));
    videoAuthorLabel->set_halign(Gtk::Align::START);
    task->append(*videoAuthorLabel);

    auto configURLLabel = Gtk::make_managed<Gtk::LinkButton>(videoData->downloadURLs[0], "Download audio");
    configURLLabel->set_halign(Gtk::Align::START);
    task->append(*configURLLabel);

    this->taskList->append(*task);
}

void YMD::MainWindow::showError(const std::string& title, const std::string& text)
{
    this->dialog = std::make_shared<Gtk::MessageDialog>(*this, title, false, Gtk::MessageType::ERROR);
    this->dialog->set_secondary_text(text);
    this->dialog->set_modal();
    this->dialog->set_transient_for(*this);
    this->dialog->set_hide_on_close();
    this->dialog->signal_response().connect(sigc::hide(sigc::mem_fun(*this->dialog, &Gtk::Widget::hide)));
    this->dialog->show();
}