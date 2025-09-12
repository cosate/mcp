extern "C" {
    #include <ngx_config.h>
    #include <ngx_core.h>
    #include <ngx_http.h>
}

#include <nlohmann/json/json.hpp>

using json = nlohmann::json;

extern "C" {

typedef struct {
    ngx_str_t method;
    ngx_uint_t rate_limit;
    ngx_uint_t burst;
    ngx_msec_t rate_interval;
} ngx_http_mcp_loc_conf_t;

static ngx_int_t ngx_http_mcp_handler(ngx_http_request_t *r);
static void *ngx_http_mcp_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_mcp_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static char *ngx_http_mcp_limit_method(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_mcp_postconfiguration(ngx_conf_t *cf);

static ngx_command_t ngx_http_mcp_commands[] = {
    { ngx_string("mcp_limit_method"),
      NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_mcp_limit_method,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_mcp_loc_conf_t, method),
      NULL },
      
    ngx_null_command
};

static ngx_http_module_t ngx_http_mcp_module_ctx = {
    nullptr,                          /* preconfiguration */
    ngx_http_mcp_postconfiguration,   /* postconfiguration */
    nullptr,                          /* create main configuration */
    nullptr,                          /* init main configuration */
    nullptr,                          /* create server configuration */
    nullptr,                          /* merge server configuration */
    ngx_http_mcp_create_loc_conf,
    ngx_http_mcp_merge_loc_conf
};

ngx_module_t ngx_http_mcp_module = {
    NGX_MODULE_V1,
    &ngx_http_mcp_module_ctx,
    ngx_http_mcp_commands,
    NGX_HTTP_MODULE,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    NGX_MODULE_V1_PADDING
};

// 简单的令牌桶限流结构
typedef struct {
    ngx_atomic_t  tokens;
    ngx_msec_t    last_update;
} ngx_http_mcp_rate_limit_t;

static ngx_http_mcp_rate_limit_t rate_limit_bucket = {0, 0};

static ngx_int_t ngx_http_mcp_check_rate_limit(ngx_http_mcp_loc_conf_t *conf) {
    ngx_msec_t now = ngx_current_msec;
    
    if (conf->rate_limit == 0) {
        return NGX_OK; // 没有设置限流
    }

    ngx_msec_t elapsed = now - rate_limit_bucket.last_update;
    
    // 更新令牌
    if (elapsed > conf->rate_interval) {
        ngx_uint_t new_tokens = (elapsed * conf->rate_limit) / conf->rate_interval;
        rate_limit_bucket.tokens = ngx_min(new_tokens + rate_limit_bucket.tokens, conf->burst);
        rate_limit_bucket.last_update = now;
    }

    // 检查是否有足够的令牌
    if (rate_limit_bucket.tokens > 0) {
        rate_limit_bucket.tokens--;
        return NGX_OK;
    }

    return NGX_HTTP_TOO_MANY_REQUESTS;
}

static ngx_int_t ngx_http_mcp_handler(ngx_http_request_t *r) {
    if (r->method != NGX_HTTP_POST) {
        return NGX_HTTP_NOT_ALLOWED;
    }

    ngx_http_mcp_loc_conf_t *conf = (ngx_http_mcp_loc_conf_t *)ngx_http_get_module_loc_conf(r, ngx_http_mcp_module);
    
    ngx_int_t rc = ngx_http_read_client_request_body(r, nullptr);
    if (rc != NGX_OK) {
        return rc;
    }

    try {
        size_t body_len = r->request_body->buf->last - r->request_body->buf->pos;
        std::string body(reinterpret_cast<char*>(r->request_body->buf->pos), body_len);

        json request = json::parse(body);

        // 检查method字段并进行限流
        if (request.contains("method")) {
            std::string method = request["method"];
            
            // 如果配置了method限流，检查是否匹配
            if (conf->method.len > 0) {
                std::string configured_method((char*)conf->method.data, conf->method.len);
                
                if (method == configured_method) {
                    // 检查限流
                    rc = ngx_http_mcp_check_rate_limit(conf);
                    if (rc != NGX_OK) {
                        ngx_log_error(NGX_LOG_WARN, r->connection->log, 0, 
                                     "Rate limit exceeded for method: %s", method.c_str());
                        
                        r->headers_out.status = NGX_HTTP_TOO_MANY_REQUESTS;
                        ngx_http_send_header(r);
                        return NGX_HTTP_TOO_MANY_REQUESTS;
                    }
                }
            }
        }

        json response;
        response["jsonrpc"] = "2.0";
        if (request.contains("method")) {
            response["result"] = "Processed method: " + std::string(request["method"]);
        } else {
            response["error"] = "Missing 'method' field";
        }

        std::string response_str = response.dump();
        ngx_str_t res = ngx_string(response_str.c_str());

        r->headers_out.status = NGX_HTTP_OK;
        r->headers_out.content_length_n = res.len;
        ngx_http_send_header(r);

        ngx_chain_t out;
        out.buf = ngx_create_temp_buf(r->pool, res.len);
        if (out.buf == NULL) {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
        
        ngx_memcpy(out.buf->pos, res.data, res.len);
        out.buf->last = out.buf->pos + res.len;
        out.buf->memory = 1;
        out.buf->last_buf = 1;
        out.next = nullptr;

        return ngx_http_output_filter(r, &out);
    } catch (const std::exception& e) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "JSON parse error: %s", e.what());
        return NGX_HTTP_BAD_REQUEST;
    } catch (...) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Unknown JSON parse error");
        return NGX_HTTP_BAD_REQUEST;
    }
}

static char *ngx_http_mcp_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child) {
    ngx_http_mcp_loc_conf_t *prev = (ngx_http_mcp_loc_conf_t *)parent;
    ngx_http_mcp_loc_conf_t *conf = (ngx_http_mcp_loc_conf_t *)child;

    if (conf->method.len == 0) {
        conf->method = prev->method;
    }
    if (conf->rate_limit == 0) {
        conf->rate_limit = prev->rate_limit;
    }
    if (conf->burst == 0) {
        conf->burst = prev->burst;
    }
    if (conf->rate_interval == 0) {
        conf->rate_interval = prev->rate_interval;
    }

    return NGX_CONF_OK;
}

static void *ngx_http_mcp_create_loc_conf(ngx_conf_t *cf) {
    ngx_http_mcp_loc_conf_t *conf;

    conf = (ngx_http_mcp_loc_conf_t *)ngx_pcalloc(cf->pool, sizeof(ngx_http_mcp_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->method.len = 0;
    conf->method.data = NULL;
    conf->rate_limit = 0;
    conf->burst = 0;
    conf->rate_interval = 1000; // 默认1秒

    return conf;
}

static char *ngx_http_mcp_limit_method(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_http_mcp_loc_conf_t *mcp_conf = (ngx_http_mcp_loc_conf_t *)conf;

    ngx_str_t *value;

    value = (ngx_str_t *)cf->args->elts;

    if (value[1].len == 0) {
        return (char *)"invalid method name";
    }

    mcp_conf->method = value[1];

    return NGX_CONF_OK;
}

static ngx_int_t ngx_http_mcp_postconfiguration(ngx_conf_t *cf) {
    ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = (ngx_http_core_main_conf_t *)ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
    if (cmcf == NULL) {
        return NGX_ERROR;
    }

    h = (ngx_http_handler_pt *)ngx_array_push(&cmcf->phases[NGX_HTTP_CONTENT_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_mcp_handler;

    return NGX_OK;
}

}
