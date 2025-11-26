#include "http_server.h"

static const char* SERVER_TAG = "http server";

uint8_t image_buf[IMAGE_SIZE];

// Declaring URI Handlers
static esp_err_t image_handler(httpd_req_t*);
static esp_err_t screensaver_handler(httpd_req_t*);

// Server and Config
static httpd_handle_t server = NULL;
static httpd_config_t server_config = HTTPD_DEFAULT_CONFIG();

// URIs
static httpd_uri_t image_uri = {
    .uri = "/image",
    .method = HTTP_POST,
    .handler = image_handler,
    .user_ctx = NULL
};
static httpd_uri_t screensaver_uri = {
    .uri = "/screensaver",
    .method = HTTP_POST,
    .handler = screensaver_handler,
    .user_ctx = NULL
};

// URI Handlers
static esp_err_t image_handler(httpd_req_t* req)
{
    if (req->content_len != IMAGE_SIZE) {
        ESP_LOGE(SERVER_TAG, "Request content length does not match IMAGE_SIZE. Expected %d bytes but received %d", IMAGE_SIZE, req->content_len);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid image size");
        return ESP_FAIL;
    }

    uint16_t received = 0;
    int ret;
    while (received < IMAGE_SIZE) {
        ret = httpd_req_recv(req, (char*)(image_buf + received), (IMAGE_SIZE - received));
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            ESP_LOGE(SERVER_TAG, "Failed to receive data");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive data");
            return ESP_FAIL;
        }
        received += ret;
    }

    ESP_LOGI(SERVER_TAG, "Received %u bytes of image data", received);
    // TODO: Use buffer to activate panel

    httpd_resp_sendstr(req, "Successfully uploaded image");
    return ESP_OK;
}

static esp_err_t screensaver_handler(httpd_req_t* req)
{
    // TODO: Go to screensaver in panel
    
    ESP_LOGI(SERVER_TAG, "Received screensaver intent");
    return ESP_OK;
}

// Helpers
static void start_server()
{
    ESP_ERROR_CHECK(httpd_start(&server, &server_config));
    ESP_LOGI(SERVER_TAG, "HTTP server started");
}

static void register_uri_handlers()
{
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &image_uri));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &screensaver_uri));
    ESP_LOGI(SERVER_TAG, "URI handlers registered");
}

void http_server_init()
{
    start_server();
    register_uri_handlers();
}
