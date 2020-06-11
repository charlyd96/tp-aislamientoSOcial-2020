/*
 * logger.h
 *
 *  Created on: 2 jun. 2020
 *      Author: utnso
 */

#ifndef LOGGER_H_
#define LOGGER_H_


#include <commons/log.h>

void iniciarLogger(const char* path, const char* program_name, bool is_active_console, t_log_level level);
void destruirLogger(void);

void iniciarLoggerAux(const char* path, const char* program_name, bool is_active_console, t_log_level level);
void destruirLoggerAux(void);

t_log* getLogger();
t_log* getLoggerAux();

#define logInfo(...)        log_info(getLogger(), __VA_ARGS__);
#define logInfoAux(...)     log_warning(getLoggerAux(), __VA_ARGS__);


#endif /* LOGGER_H_ */
