#include "../backends.h"
#include "module_backend_t.h"
#include "module_backend.h"

static void module_backend_cleanup(void *private_data)
{
	struct module_backend_t *module_backend = private_data;
	module_backend->api->destroy_handler(module_backend->api);
	destroy_dlopen_handle(&module_backend->dlopen_handle);
	destroy_module_backend_config(&module_backend->config);
}

static int dnet_module_config_init(struct dnet_config_backend *b, struct dnet_config *c)
{
	int err;
	struct module_backend_t *module_backend = b->data;
	err=create_dlopen_handle(&module_backend->dlopen_handle, module_backend->config.module_path, module_backend->config.symbol_name);
	if (0!=err) {
		dnet_backend_log(DNET_LOG_ERROR, "Fail to create dlopen handle from %s\n", module_backend->config.module_path);
		err=-ENOMEM;
		goto err;
	}
	module_constructor* constructor=module_backend->dlopen_handle.symbol;
	module_backend->api=constructor(&module_backend->config);
	if (NULL==module_backend->api) {
		dnet_backend_log(DNET_LOG_ERROR, "Fail to create module_backend from %s\n", module_backend->config.module_path);
		err=-ENOMEM;
		goto constructor_err;
	}
	c->cb = &b->cb; /// @todo NOTE need to understand this is for.
	b->cb.command_private = module_backend;
	b->cb.command_handler = module_backend->api->command_handler;
	b->cb.meta_write = module_backend->api->meta_write_handler;
	b->cb.backend_cleanup=module_backend_cleanup;
	dnet_backend_log(DNET_LOG_NOTICE, "Dynamic module_backend loaded successfully\n");
	return 0;
	constructor_err:
		destroy_dlopen_handle(&module_backend->dlopen_handle);
	err:
		return err;
}

static void dnet_module_config_cleanup(struct dnet_config_backend *b)
{
	struct module_backend_t *module_backend = b->data;
	module_backend_cleanup(module_backend);
}

static struct dnet_config_backend dnet_module_backend = {
	.name			= "module",
	.size			= sizeof(struct module_backend_t),
	.init			= dnet_module_config_init,
	.cleanup		= dnet_module_config_cleanup,
};

int dnet_module_backend_init(void)
{
	dnet_module_backend.ent=dnet_config_entries_module();
	dnet_module_backend.num=dnet_config_entries_module_size();
	return dnet_backend_register(&dnet_module_backend);
}

void dnet_module_backend_exit(void)
{
}