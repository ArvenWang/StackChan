/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "hal_aida.h"
#include <apps/common/reminder/reminder.h>
#include <application.h>
#include <board.h>
#include <cJSON.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

static constexpr const char* TAG = "HAL-AIDA";

namespace {

std::string trim(std::string value)
{
    auto start = value.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, end - start + 1);
}

std::string bridgeBaseUrl()
{
    std::string url = trim(CONFIG_AIDA_BRIDGE_BASE_URL);
    while (!url.empty() && url.back() == '/') {
        url.pop_back();
    }
    if (url.empty()) {
        throw std::runtime_error("Aida bridge URL is not configured");
    }
    return url;
}

std::string bridgeToken()
{
    return trim(CONFIG_AIDA_BRIDGE_AUTH_TOKEN);
}

std::string notificationToken()
{
    return trim(CONFIG_AIDA_NOTIFY_SERVER_TOKEN);
}

std::string readHeader(httpd_req_t* req, const char* key)
{
    auto len = httpd_req_get_hdr_value_len(req, key);
    if (len <= 0) {
        return "";
    }

    std::vector<char> buffer(len + 1, '\0');
    if (httpd_req_get_hdr_value_str(req, key, buffer.data(), buffer.size()) != ESP_OK) {
        return "";
    }
    return std::string(buffer.data());
}

esp_err_t sendJson(httpd_req_t* req, int status, const std::string& body)
{
    switch (status) {
        case 200:
            httpd_resp_set_status(req, "200 OK");
            break;
        case 400:
            httpd_resp_set_status(req, "400 Bad Request");
            break;
        case 401:
            httpd_resp_set_status(req, "401 Unauthorized");
            break;
        case 413:
            httpd_resp_set_status(req, "413 Payload Too Large");
            break;
        default:
            httpd_resp_set_status(req, "500 Internal Server Error");
            break;
    }
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, body.c_str(), body.size());
}

std::string composeNotificationMessage(const cJSON* root)
{
    auto title = cJSON_GetObjectItem(root, "title");
    auto message = cJSON_GetObjectItem(root, "message");
    auto status = cJSON_GetObjectItem(root, "status");

    std::string result;
    if (cJSON_IsString(title) && title->valuestring[0] != '\0') {
        result += title->valuestring;
    }
    if (cJSON_IsString(message) && message->valuestring[0] != '\0') {
        if (!result.empty()) {
            result += "：";
        }
        result += message->valuestring;
    }
    if (result.empty() && cJSON_IsString(status) && status->valuestring[0] != '\0') {
        result = status->valuestring;
    }
    return trim(result);
}

class NotifyServer {
public:
    bool Start()
    {
#if !CONFIG_AIDA_NOTIFY_SERVER_ENABLED
        return false;
#else
        if (server_ != nullptr) {
            return true;
        }

        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.server_port = CONFIG_AIDA_NOTIFY_SERVER_PORT;
        config.max_open_sockets = 4;
        config.ctrl_port = 32770;

        httpd_uri_t health_uri = {
            .uri = "/aida/health",
            .method = HTTP_GET,
            .handler = &NotifyServer::HandleHealth,
            .user_ctx = this,
        };
        httpd_uri_t notify_uri = {
            .uri = "/aida/notify",
            .method = HTTP_POST,
            .handler = &NotifyServer::HandleNotify,
            .user_ctx = this,
        };

        if (httpd_start(&server_, &config) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to start Aida notify server");
            server_ = nullptr;
            return false;
        }

        httpd_register_uri_handler(server_, &health_uri);
        httpd_register_uri_handler(server_, &notify_uri);
        ESP_LOGI(TAG, "Aida notify server started on port %d", CONFIG_AIDA_NOTIFY_SERVER_PORT);
        return true;
#endif
    }

private:
    static esp_err_t HandleHealth(httpd_req_t* req)
    {
        return sendJson(req, 200, R"({"ok":true})");
    }

    static esp_err_t HandleNotify(httpd_req_t* req)
    {
        auto* self = static_cast<NotifyServer*>(req->user_ctx);
        return self->handleNotify(req);
    }

    esp_err_t handleNotify(httpd_req_t* req)
    {
        auto token = notificationToken();
        if (!token.empty()) {
            auto auth = readHeader(req, "Authorization");
            auto expected = std::string("Bearer ") + token;
            if (auth != expected) {
                ESP_LOGW(TAG, "Rejected Aida notification: invalid bearer token");
                return sendJson(req, 401, R"({"ok":false,"error":"unauthorized"})");
            }
        }

        if (req->content_len <= 0) {
            return sendJson(req, 400, R"({"ok":false,"error":"empty body"})");
        }
        if (req->content_len > 4096) {
            return sendJson(req, 413, R"({"ok":false,"error":"payload too large"})");
        }

        std::string payload;
        payload.resize(req->content_len);
        int received = 0;
        while (received < req->content_len) {
            int ret = httpd_req_recv(req, payload.data() + received, req->content_len - received);
            if (ret <= 0) {
                return sendJson(req, 400, R"({"ok":false,"error":"failed to read body"})");
            }
            received += ret;
        }

        cJSON* root = cJSON_Parse(payload.c_str());
        if (root == nullptr) {
            return sendJson(req, 400, R"({"ok":false,"error":"invalid json"})");
        }

        auto message = composeNotificationMessage(root);
        cJSON_Delete(root);

        if (message.empty()) {
            return sendJson(req, 400, R"({"ok":false,"error":"missing message"})");
        }

        Application::GetInstance().Schedule([message = std::move(message)]() {
            tools::create_reminder(1, message, false);
        });

        ESP_LOGI(TAG, "Accepted Aida notification");
        return sendJson(req, 200, R"({"ok":true})");
    }

    httpd_handle_t server_ = nullptr;
};

NotifyServer& notifyServer()
{
    static NotifyServer instance;
    return instance;
}

std::string performBridgeRequest(const char* method, const std::string& path, std::string payload = "")
{
    auto& board = Board::GetInstance();
    auto network = board.GetNetwork();
    auto http = network->CreateHttp(3);
    if (!http) {
        throw std::runtime_error("Failed to create HTTP client");
    }

    auto url = bridgeBaseUrl() + path;
    if (!payload.empty() || std::string(method) != "GET") {
        http->SetHeader("Content-Type", "application/json");
    }

    auto token = bridgeToken();
    if (!token.empty()) {
        http->SetHeader("Authorization", "Bearer " + token);
    }

    if (!payload.empty()) {
        http->SetContent(std::move(payload));
    }

    if (!http->Open(method, url)) {
        throw std::runtime_error("Failed to connect to Aida bridge");
    }

    auto status = http->GetStatusCode();
    auto response = trim(http->ReadAll());
    if (status < 200 || status >= 300) {
        if (response.empty()) {
            response = "HTTP " + std::to_string(status);
        }
        throw std::runtime_error("Aida bridge request failed: " + response);
    }

    if (response.empty()) {
        return "{}";
    }
    return response;
}

std::string createTaskPayload(std::string_view prompt, std::string_view workspace, std::string_view title, bool notify)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "prompt", std::string(prompt).c_str());
    if (!workspace.empty()) {
        cJSON_AddStringToObject(root, "workspace", std::string(workspace).c_str());
    }
    if (!title.empty()) {
        cJSON_AddStringToObject(root, "title", std::string(title).c_str());
    }
    cJSON_AddBoolToObject(root, "notify", notify);

    auto raw = cJSON_PrintUnformatted(root);
    std::string payload = raw ? raw : "{}";
    if (raw != nullptr) {
        cJSON_free(raw);
    }
    cJSON_Delete(root);
    return payload;
}

}  // namespace

namespace aida {

void startNotifyServer()
{
#if CONFIG_AIDA_NOTIFY_SERVER_ENABLED
    notifyServer().Start();
#endif
}

std::string createCodexTask(std::string_view prompt, std::string_view workspace, std::string_view title, bool notify)
{
    return performBridgeRequest("POST", "/v1/tasks", createTaskPayload(prompt, workspace, title, notify));
}

std::string getCodexTask(std::string_view taskId)
{
    auto task = trim(std::string(taskId));
    if (task.empty()) {
        throw std::runtime_error("task_id is required");
    }
    return performBridgeRequest("GET", "/v1/tasks/" + task);
}

std::string cancelCodexTask(std::string_view taskId)
{
    auto task = trim(std::string(taskId));
    if (task.empty()) {
        throw std::runtime_error("task_id is required");
    }
    return performBridgeRequest("POST", "/v1/tasks/" + task + "/cancel", "{}");
}

}  // namespace aida
