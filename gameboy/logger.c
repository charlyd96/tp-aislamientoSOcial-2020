#include "logger.h"

static t_log* GAMEBOY_LOGGER;
static t_log* GAMEBOY_LOGGER_AUX;

void iniciarLogger(const char* path, const char* program_name, bool is_active_console, t_log_level level)
{
	GAMEBOY_LOGGER = log_create((char*)path, (char*)program_name, is_active_console, level);
}

void destruirLogger(void)
{
    log_destroy(GAMEBOY_LOGGER);
}

t_log* getLogger(void)
{
    return GAMEBOY_LOGGER;
}

//-----------------
void iniciarLoggerAux(const char* path, const char* program_name, bool is_active_console, t_log_level level)
{
	GAMEBOY_LOGGER_AUX = log_create((char*)path, (char*)program_name, !is_active_console, level);
}
void destruirLoggerAux(void)
{
    log_destroy(GAMEBOY_LOGGER_AUX);
}

t_log* getLoggerAux(void)
{
    return GAMEBOY_LOGGER_AUX;
}
