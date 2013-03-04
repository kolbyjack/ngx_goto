
/*
 * Copyright (C) Jonathan Kolb
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct {
    ngx_http_complex_value_t *target;
} ngx_http_goto_loc_conf_t;


static void *ngx_http_goto_create_loc_conf(ngx_conf_t *cf);
static ngx_int_t ngx_http_goto_init(ngx_conf_t *cf);
static char *ngx_http_goto(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);


static ngx_command_t  ngx_http_goto_commands[] = {

    { ngx_string("goto"),
      NGX_HTTP_SIF_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_TAKE1,
      ngx_http_goto,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_goto_module_ctx = {
    NULL,                                  /* preconfiguration */
    ngx_http_goto_init,                    /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    ngx_http_goto_create_loc_conf,         /* create location configuration */
    NULL                                   /* merge location configuration */
};


ngx_module_t  ngx_http_goto_module = {
    NGX_MODULE_V1,
    &ngx_http_goto_module_ctx,             /* module context */
    ngx_http_goto_commands,                /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_int_t
ngx_http_goto_handler(ngx_http_request_t *r)
{
    ngx_int_t                     index;
    ngx_http_core_srv_conf_t     *cscf;
    ngx_http_core_main_conf_t    *cmcf;
    ngx_http_goto_loc_conf_t     *glcf;
    ngx_str_t                     path, args;

    cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);
    cscf = ngx_http_get_module_srv_conf(r, ngx_http_core_module);
    index = cmcf->phase_engine.location_rewrite_index;

    if (r->phase_handler == index && r->loc_conf == cscf->ctx->loc_conf) {
        /* skipping location rewrite phase for server null location */
        return NGX_DECLINED;
    }

    glcf = ngx_http_get_module_loc_conf(r, ngx_http_goto_module);

    if (glcf->target == NULL) {
        return NGX_DECLINED;
    }

    if (NGX_OK != ngx_http_complex_value(r, glcf->target, &path)) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    if (path.data[0] == '@') {
        (void) ngx_http_named_location(r, &path);

    } else {
        ngx_http_split_args(r, &path, &args);

        (void) ngx_http_internal_redirect(r, &path, &args);
    }

    ngx_http_finalize_request(r, NGX_DONE);
    return NGX_DONE;
}


static void *
ngx_http_goto_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_goto_loc_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_goto_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    return conf;
}


static ngx_int_t
ngx_http_goto_init(ngx_conf_t *cf)
{
    ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_SERVER_REWRITE_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_goto_handler;

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_REWRITE_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_goto_handler;

    return NGX_OK;
}


static char *
ngx_http_goto(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_goto_loc_conf_t  *lcf = conf;

    ngx_str_t                         *value;
    ngx_http_compile_complex_value_t   ccv;

    lcf->target = ngx_palloc(cf->pool, sizeof(ngx_http_complex_value_t));
    if (lcf->target == NULL) {
        return NGX_CONF_ERROR;
    }

    value = cf->args->elts;

    ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = &value[1];
    ccv.complex_value = lcf->target;

    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}

