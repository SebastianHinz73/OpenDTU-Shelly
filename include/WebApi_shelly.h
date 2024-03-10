// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>

class WebApiShellyClass {
public:
    WebApiShellyClass();
    void init(AsyncWebServer& server, Scheduler& scheduler);

private:
    void onShellyAdminGet(AsyncWebServerRequest* request);
    void onShellyAdminPost(AsyncWebServerRequest* request);
};
