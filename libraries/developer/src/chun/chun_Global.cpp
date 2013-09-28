#include "chun_Global.h"


void chun_log(int bDisplayLog, const char *fmt, ...)
{
	if(bDisplayLog == LOG_TURN_ON)
	{
		va_list vl;
		va_start(vl, fmt);
		vfprintf(stdout, fmt, vl);
		va_end(vl);
	}
}
