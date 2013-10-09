#ifndef QUIRE_H_CFG_API
#define QUIRE_H_CFG_API

struct qu_config_context *qu_config_parser();
void qu_config_parser_free(struct qu_config_context *ctx);

#endif  // QUIRE_H_CFG_API
