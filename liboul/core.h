#ifndef _OUL_CORE_H
#define _OUL_CORE_H

typedef struct _OulCoreUiOps{
	void (*ui_prefs_init)(void);
    void (*ui_init)(void);
    void (*quit)(void);

    void (*_oul_reserved1)(void);
    void (*_oul_reserved2)(void);
    void (*_oul_reserved3)(void);

}OulCoreUiOps;

#ifdef __cplusplus
extern "C" {
#endif

OulCoreUiOps*   oul_core_get_ui_ops(void);
const char*     oul_core_get_ui(void);
void 			oul_core_quit(void);


#ifdef __cplusplus
}
#endif

#endif
