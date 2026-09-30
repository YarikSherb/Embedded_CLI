/*
 * misc.h
 *
 *  Created on: Jul 23, 2026
 *      Author: YarikSherb
 */

#ifndef MISC_H_
#define MISC_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if MICROSH_CFG_CONSOLE_SESSIONS
enum {
    /* Login type 0x00 reserved by library as _LOGIN_TYPE_LOGGED_OUT type */
    _LOGIN_TYPE_DEBUG = 0x01,
    _LOGIN_TYPE_ADMIN
};
#endif /* MICROSH_CFG_CONSOLE_SESSIONS */

#if MICROSH_CFG_CONSOLE_SESSIONS
microshr_t register_auth_commands(microsh_t* msh);
#endif /* MICROSH_CFG_CONSOLE_SESSIONS */
microshr_t register_all_commands(microsh_t* msh);
int        microrl_print(microrl_t* mrl, const char* str);
char       get_char(void);

#if MICRORL_CFG_USE_COMPLETE
char**     complet(microrl_t* mrl, int argc, const char* const *argv);
#endif /* MICRORL_CFG_USE_COMPLETE */

#if MICRORL_CFG_USE_CTRL_C
void       sigint(microrl_t* mrl);
#endif /* MICRORL_CFG_USE_CTRL_C */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MISC_H_ */
