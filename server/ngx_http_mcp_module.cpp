extern "C" {
    #include <ngx_config.h>
    #include <ngx_core.h>
    #include <ngx_http.h>
}
#include <nlohmann/json/json.hpp>
#include <variant>
#include "../common/types.h"
#include "include/mcp_server.h"

extern "C" {

// 新增: 每个方法的限流配置
typedef struct {
    ngx_str_t   method;        // 方法名
    ngx_uint_t  rate_limit;    // 配额(每 interval)
    ngx_uint_t  burst;         // 突发上限
    ngx_msec_t  interval;      // 窗口大小(毫秒)
    ngx_uint_t  tokens;        // 当前令牌
    ngx_msec_t  last_refill;   // 上次填充时间
} ngx_http_mcp_method_limit_t;

// 新的 loc 配置，支持多条方法
typedef struct {
    ngx_flag_t     enabled;
    ngx_array_t   *methods;    // 元素类型: ngx_http_mcp_method_limit_t
} ngx_http_mcp_loc_conf_t;

static ngx_int_t ngx_http_mcp_handler(ngx_http_request_t *r);
static void *ngx_http_mcp_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_mcp_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static char *ngx_http_mcp_limit_method(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_mcp_postconfiguration(ngx_conf_t *cf);

// 更新: 指令定义( mcp_limit_method 现在接收 2 参数 )
static ngx_command_t ngx_http_mcp_commands[] = {
    { ngx_string("mcp_enable"),
      NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_mcp_loc_conf_t, enabled),
      NULL },

    { ngx_string("mcp_limit_method"),
      NGX_HTTP_LOC_CONF|NGX_CONF_TAKE2,
      ngx_http_mcp_limit_method,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,            // 不再使用 offsetof 直接写入，由处理函数管理
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

// 新增: 查找方法配置
static ngx_http_mcp_method_limit_t *
ngx_http_mcp_find_method(ngx_http_mcp_loc_conf_t *conf, ngx_str_t *m) {
    if (!conf || !conf->methods) return NULL;
    ngx_http_mcp_method_limit_t *arr = (ngx_http_mcp_method_limit_t*)conf->methods->elts;
    for (ngx_uint_t i = 0; i < conf->methods->nelts; ++i) {
        if (arr[i].method.len == m->len &&
            ngx_strncmp(arr[i].method.data, m->data, m->len) == 0) {
            return &arr[i];
        }
    }
    return NULL;
}

// 新增: 令牌桶刷新
static ngx_int_t
ngx_http_mcp_take_token(ngx_http_mcp_method_limit_t *ml) {
    if (ml->rate_limit == 0) return NGX_OK;

    ngx_msec_t now = ngx_current_msec;
    if (ml->last_refill == 0) {
        ml->tokens = ml->rate_limit;
        ml->last_refill = now;
    } else {
        ngx_msec_t elapsed = now - ml->last_refill;
        if (elapsed > 0) {
            // 根据经过时间补充令牌
            ngx_uint_t add = (ngx_uint_t)(( (uint64_t)elapsed * ml->rate_limit) / ml->interval);
            if (add > 0) {
                ml->tokens = ngx_min((ngx_uint_t)(ml->tokens + add), ml->burst);
                ml->last_refill = now;
            }
        }
    }
    if (ml->tokens > 0) {
        ml->tokens--;
        return NGX_OK;
    }
    return NGX_HTTP_TOO_MANY_REQUESTS;
}

// 修改: 处理函数支持多方法
static ngx_int_t ngx_http_mcp_handler(ngx_http_request_t *r) {
    ngx_http_mcp_loc_conf_t *conf =
        (ngx_http_mcp_loc_conf_t *)ngx_http_get_module_loc_conf(r, ngx_http_mcp_module);
    if (!conf || !conf->enabled) {
        return NGX_DECLINED;
    }

    bool need_body = (r->method & (NGX_HTTP_POST | NGX_HTTP_PUT | NGX_HTTP_PATCH)) != 0;
    if (need_body) {
        ngx_int_t rc_rb = ngx_http_read_client_request_body(r, NULL);
        if (rc_rb != NGX_OK && rc_rb != NGX_AGAIN) {
            return rc_rb;
        }
    }

    nlohmann::json request;
    bool has_json = false;
    if (need_body && r->request_body && r->request_body->buf &&
        r->request_body->buf->pos && r->request_body->buf->last > r->request_body->buf->pos) {
        try {
            size_t body_len = r->request_body->buf->last - r->request_body->buf->pos;
            std::string body(reinterpret_cast<char*>(r->request_body->buf->pos), body_len);
            request = nlohmann::json::parse(body);
            has_json = true;
        } catch (const std::exception &e) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "mcp json parse error: %s", e.what());
            return NGX_HTTP_BAD_REQUEST;
        } catch (...) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "mcp unknown json parse error");
            return NGX_HTTP_BAD_REQUEST;
        }
    }

    // 必须有 JSON 且包含顶层 "method" 为字符串
    if (!has_json) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "mcp missing JSON body or body empty");
        r->headers_out.status = NGX_HTTP_BAD_REQUEST;
        ngx_http_send_header(r);
        return NGX_HTTP_BAD_REQUEST;
    }

    auto it = request.find("method");
    if (it == request.end()) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "mcp missing 'method' field");
        r->headers_out.status = NGX_HTTP_BAD_REQUEST;
        ngx_http_send_header(r);
        return NGX_HTTP_BAD_REQUEST;
    }
    if (!it->is_string()) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "mcp 'method' field not a string");
        r->headers_out.status = NGX_HTTP_BAD_REQUEST;
        ngx_http_send_header(r);
        return NGX_HTTP_BAD_REQUEST;
    }

    std::string logic_method = it->get<std::string>();

    // 查找并执行限流
    ngx_str_t ms;
    ms.len = logic_method.size();
    ms.data = (u_char*)logic_method.data();
    ngx_http_mcp_method_limit_t *ml = ngx_http_mcp_find_method(conf, &ms);
    if (ml) {
        ngx_int_t rl_rc = ngx_http_mcp_take_token(ml);
        if (rl_rc != NGX_OK) {
            ngx_log_error(NGX_LOG_WARN, r->connection->log, 0,
                          "mcp rate limit exceeded for method: %V", &ms);
            r->headers_out.status = NGX_HTTP_TOO_MANY_REQUESTS;
            ngx_http_send_header(r);
            return NGX_HTTP_TOO_MANY_REQUESTS;
        }
    }

    mcp::server::McpServer::MCPRequestVariant req_variant;
    if (!mcp::server::McpServer::ngx_http_mcp_parse_request(
            request, logic_method, req_variant, r->connection->log)) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "mcp unknown or parse-failed request method: %s",
                      logic_method.c_str());
        r->headers_out.status = NGX_HTTP_BAD_REQUEST;
        ngx_http_send_header(r);
        return NGX_HTTP_BAD_REQUEST;
    }
    // std::visit 可在此处理 req_variant
    nlohmann::json response;
    response["jsonrpc"] = "2.0";
    response["result"]  = std::string("Processed method: ") + logic_method;
    // 如需调试输出具体类型，可启用（示例）：
    // std::visit([&](auto const& req){ response["request_type"] = typeid(req).name(); }, req_variant);

    std::string response_str = response.dump();
    ngx_str_t res;
    res.len = response_str.size();
    res.data = (u_char*)response_str.data();

    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = res.len;
    ngx_http_send_header(r);

    ngx_chain_t out;
    out.buf = ngx_create_temp_buf(r->pool, res.len);
    if (out.buf == NULL) return NGX_HTTP_INTERNAL_SERVER_ERROR;

    ngx_memcpy(out.buf->pos, res.data, res.len);
    out.buf->last = out.buf->pos + res.len;
    out.buf->memory = 1;
    out.buf->last_buf = 1;
    out.next = NULL;

    return ngx_http_output_filter(r, &out);
}

// 更新: create loc conf
static void *ngx_http_mcp_create_loc_conf(ngx_conf_t *cf) {
    ngx_http_mcp_loc_conf_t *conf =
        (ngx_http_mcp_loc_conf_t*)ngx_pcalloc(cf->pool, sizeof(ngx_http_mcp_loc_conf_t));
    if (conf == NULL) return NULL;
    conf->enabled = NGX_CONF_UNSET;
    conf->methods = NULL;
    return conf;
}

// 更新: merge loc conf
static char *ngx_http_mcp_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child) {
    ngx_http_mcp_loc_conf_t *prev = (ngx_http_mcp_loc_conf_t*)parent;
    ngx_http_mcp_loc_conf_t *conf = (ngx_http_mcp_loc_conf_t*)child;

    ngx_conf_merge_value(conf->enabled, prev->enabled, 0);
    if (conf->methods == NULL) {
        conf->methods = prev->methods; // 直接继承指针 (父级只读)
    }
    return NGX_CONF_OK;
}

// 更新: 指令处理(追加或更新方法)
static char *ngx_http_mcp_limit_method(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_http_mcp_loc_conf_t *mcp_conf = (ngx_http_mcp_loc_conf_t*)conf;
    ngx_str_t *value = (ngx_str_t*)cf->args->elts;
    // value[1]: method, value[2]: qps
    if (value[1].len == 0) return (char*)"invalid method name";
    if (value[2].len == 0) return (char*)"invalid qps value";

    ngx_int_t qps = ngx_atoi(value[2].data, value[2].len);
    if (qps == NGX_ERROR || qps <= 0) return (char*)"invalid qps (positive integer expected)";

    if (mcp_conf->methods == NULL) {
        mcp_conf->methods = ngx_array_create(cf->pool, 2, sizeof(ngx_http_mcp_method_limit_t));
        if (mcp_conf->methods == NULL) return (char*)"failed to allocate methods array";
    }

    // 如果已存在同名方法则更新
    ngx_http_mcp_method_limit_t *existing = ngx_http_mcp_find_method(mcp_conf, &value[1]);
    if (existing) {
        existing->rate_limit = (ngx_uint_t)qps;
        existing->burst = existing->burst ? existing->burst : existing->rate_limit;
        existing->interval = 1000;
        return NGX_CONF_OK;
    }

    ngx_http_mcp_method_limit_t *ml =
        (ngx_http_mcp_method_limit_t*)ngx_array_push(mcp_conf->methods);
    if (ml == NULL) return (char*)"failed to push method item";

    ml->method = value[1];
    ml->rate_limit = (ngx_uint_t)qps;
    ml->burst = ml->rate_limit;   // 默认突发 = qps
    ml->interval = 1000;          // 固定 1s 窗口
    ml->tokens = ml->rate_limit;  // 初始满桶
    ml->last_refill = 0;

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

} // extern "C"
