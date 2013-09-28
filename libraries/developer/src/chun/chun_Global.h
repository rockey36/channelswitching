#ifndef CHUN_GLOBAL_H
#define CHUN_GLOBAL_H

#include "chun_def.h"

#define		LOG_TURN_ON		0
#define		LOG_TURN_OFF	1

#define		LOG_IPC					LOG_TURN_OFF
#define		LOG_IPC_RECEIVE			LOG_TURN_OFF

void chun_log(int bDisplayLog, const char *fmt, ...);

#endif /* CHUN_GLOBAL_H*/