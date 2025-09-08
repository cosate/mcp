extern "C" {
    #include <ngx_config.h>
    #include <ngx_core.h>
    #include <ngx_http.h>
}

#include <nlohmann/json/json.hpp>

extern "C" {
    ngx_module_t ngx_http_mcp_module;
}

using json = nlohmann::json;

static ngx_int_t ngx_http_mcp_handler(ngx_http_request_t *r) {
    if (r->method != NGX_HTTP_POST) {
        return NGX_HTTP_NOT_ALLOWED;
    }

    ngx_int_t rc = ngx_http_read_client_request_body(r, nullptr);
    if (rc != NGX_OK) {
        return rc;
    }

    try {
        size_t body_len = r->request_body->buf->last - r->request_body->buf->pos;
        std::string body(reinterpret_cast<char*>(r->request_body->buf->pos), body_len);

        json request = json::parse(body);

        json response;
        response["jsonrpc"] = "2.0";
        if (request.contains("message")) {
            response["result"] = "Received: " + request["message"];
        } else {
            response["error"] = "Missing 'message' field";
        }

        std::string response_str = response.dump();
        ngx_str_t res = ngx_string(response_str.c_str());

        r->headers_out.status = NGX_HTTP_OK;
        r->headers_out.content_length_n = res.len;
        ngx_http_send_header(r);

        ngx_chain_t out;
        out.buf = ngx_create_temp_buf(r->pool, res.len);
        out.buf->pos = res.data;
        out.buf->last = res.data + res.len;
        out.buf->memory = 1;
        out.buf->last_buf = 1;
        out.next = nullptr;

        return ngx_http_output_filter(r, &out);
    } catch (...) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "JSON parse error");
        return NGX_HTTP_BAD_REQUEST;
    }
}

static char *ngx_http_mcp_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child) {
    return NGX_CONF_OK;
}

static void *ngx_http_mcp_create_loc_conf(ngx_conf_t *cf) {
    return nullptr;
}

static ngx_http_module_t ngx_http_mcp_module_ctx = {
    nullptr,                  /* preconfiguration */
    nullptr,                  /* postconfiguration */
    nullptr,                  /* create main configuration */
    nullptr,                  /* init main configuration */
    nullptr,                  /* create server configuration */
    nullptr,                  /* merge server configuration */
    ngx_http_mcp_create_loc_conf,
    ngx_http_mcp_merge_loc_conf
};

ngx_module_t ngx_http_mcp_module = {
    NGX_MODULE_V1,
    &ngx_http_mcp_module_ctx,
    nullptr,
    NGX_HTTP_MODULE,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    NGX_MODULE_V1_PADDING
};