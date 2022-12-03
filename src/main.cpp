#include <cstdio>
#include <iostream>
#include <filesystem>

#include "mainwindow.h"
#include "retrieverscript.h"

#include <curl/curl.h>

int main(int argc, char* argv[])
{
    std::cout << "Initializing cURL..." << std::endl;

    if (curl_global_init(CURL_GLOBAL_ALL))
    {
        std::cerr << "Failed to initialize cURL." << std::endl;
        return EXIT_FAILURE;
    }

    const std::string appID = "cz.tefek.ymd3";
    const std::string appName = "YMD3";
    const std::string appVersion = YMD_VERSION;

    std::filesystem::path execPath(argv[0]);

    YMD::ScriptingEngine scriptingEngine(execPath);

    const std::string appFullName = appName + " v. " + appVersion;

    std::cout << "You are running " << appFullName << "." << std::endl;

    auto app = Gtk::Application::create(appID);

    auto settings = Gtk::Settings::get_default();
    settings->property_gtk_theme_name() = "Adwaita";
    settings->property_gtk_application_prefer_dark_theme() = true;
    std::cout << "Using theme: " << settings->property_gtk_theme_name() << std::endl;

    int appExitStatus = app->make_window_and_run<YMD::MainWindow>(argc, argv, appFullName);

    curl_global_cleanup();

    return appExitStatus;
}