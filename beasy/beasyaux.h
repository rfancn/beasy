#ifndef _BEASY_AUX_H
#define _BEASY_AUX_H

void 		beasy_signal_handler(int sig);
void 		beasy_process_singals(void );
void 		beasy_set_i18_support(void);
gboolean 	beasy_parse_cmd_options(int argc, char *argv[]);

#endif
