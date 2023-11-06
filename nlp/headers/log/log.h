#ifndef __FLB_LOG_H__
#define __FLB_LOG_H__

#define LOG_LEVEL_EMPTY 0
#define LOG_LEVEL_CRIT 1
#define LOG_LEVEL_ERR 2
#define LOG_LEVEL_WARNING 3
#define LOG_LEVEL_DEBUG 4
#define LOG_LEVEL_TRACE 5
#define LOG_LEVEL_INFO 127

#define flb_log flb_log_handle

void flb_log_handle(int level, const char *fmt, ...);

#endif /* __FLB_LOG_H__ */