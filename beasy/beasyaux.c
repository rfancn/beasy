#include "internal.h"

#include "getopt.h"

#include "beasyaux.h"

#include "debug.h"
#include "util.h"


static const int catch_sig_list[] = {
    SIGSEGV,
    SIGHUP,
    SIGINT,
    SIGTERM,
    SIGQUIT,
    SIGCHLD,
    #if defined(USE_GSTREAMER) && !defined(GST_CAN_DISABLE_FORKING)
    SIGALRM,
    #endif
    -1
};

static const int ignore_sig_list[] = {
    SIGPIPE,
    -1
};

static char *segfault_msg = NULL;

static void
clean_pid(void){
    int status;
    pid_t  pid;

    do{
        pid = waitpid(-1, &status, WNOHANG);
    }while(pid !=0 && pid != (pid_t)-1);

    if((pid = (pid_t)-1) && (errno != ECHILD)){
        char errmsg[BUFSIZ];
        snprintf(errmsg, BUFSIZ, "Warning: waitpid() returned %d", pid);
        perror(errmsg);
    }
}

static char*
get_segfault_msg(void){
    char *msg;

    #ifndef DEBUG
    char *segfault_msg_tmp;
    GError *error = NULL;
    #endif

    #ifndef DEBUG
        segfault_msg_tmp = g_strdup_printf(_(
            "%s %s has segfaulted and attemped to dump a core file.\n"
            "This is a bug in the software and has happened through\n"
            "no fault of your own.\n\n"
            "If you can reproduce the crash, please notify the developers.\n"
            "Please make sure to specify what you were doing at the time\n"
            "and post the backtrace from the core file. \n\n"
            "If you need further assistance, please email to rfan.cn@gmail.com\n"),
            PACKAGE_NAME, VERSION
        );

        /* we have to convert the message here,
            because after segmentation fault
            it's not a good practice to allocate memory */
        msg = g_locale_from_utf8(segfault_msg_tmp,
                        -1, NULL, NULL, &error);

        if(msg != NULL){
            g_free(segfault_msg_tmp);
        }else{
            g_warning("%s\n", error->message);
            g_error_free(error);
            return segfault_msg_tmp;
        }
    #else
        msg = g_strdup(
            "Hi, user.  We need to talk.\n"
            "I think something's gone wrong here.  It's probably my fault.\n"
            "No, really, it's not you... it's me... no no no, I think we get along well\n"
            "it's just that.... well, I want to see other people.  I... what?!?  NO!  I \n"
            "haven't been cheating on you!!  How many times do you want me to tell you?!  And\n"
            "for the last time, it's just a rash!\n"
        );

    #endif

    return msg;
}

void
signal_handler(int sig){
    switch(sig){
        case SIGHUP:
            oul_debug_warning("signal_handler", "Caught signal %d\n", sig);
            /* 2. disconnect all the connections */
            break;

        case SIGSEGV:
            if(segfault_msg != NULL)
                fprintf(stderr, "%s", segfault_msg);

            #ifdef HAVE_SIGNAL_H
            g_free(segfault_msg);
            #endif

            abort();
            break;

        #if defined(USE_GSTREAMER) && !defined(GST_CAN_DISABLE_FORKING)
        /* By default, gstreamer forks when you initialize it, and waitpids for the
         * child.  But if beasy reaps the child rather than leaving it to
         * gstreamer, gstreamer's initialization fails.  So, we wait a second before
         * reaping child processes, to give gst a chance to reap it if it wants to.
         *
         * This is not needed in later gstreamers, which let us disable the forking.
         * And, it breaks the world on some Real Unices.
         */
        case SIGCHLD:
            signal(SIGCHLD, signal_handler);
            alarm(1);
            break;

        case SIGALRM:
        #else
        case SIGCHLD:
        #endif
            clean_pid();
            signal(SIGCHLD, signal_handler);
            break;

        default:
            oul_debug_warning("signal_handler", "Caught signal %d\n", sig);

			oul_plugins_unload_all();
            if(gtk_main_level())
                gtk_main_quit();

            exit(0);
    }
}

void
beasy_process_signals(void){
    int  sig_idx;
    sigset_t sigset;
    void (*prev_sig_disp)(int);
    char errmsg[BUFSIZ];

    segfault_msg = get_segfault_msg();

    /* for some reason things like gdm like to block  *
     * useful signals like SIGCHLD, so we unblock all *
     * the ones we declare a handler for.             */
    if(sigemptyset(&sigset)){
        snprintf(errmsg, BUFSIZ, "Warning: couldn't initialize empty signal set");
        perror(errmsg);
    }

    /* process catched signals */
    for(sig_idx=0; catch_sig_list[sig_idx] != -1; ++sig_idx ){
        if((prev_sig_disp = signal(catch_sig_list[sig_idx], signal_handler)) == SIG_ERR){
            snprintf(errmsg, BUFSIZ, "Warning: couldn't set signal %d  for catching",
                catch_sig_list[sig_idx]);
            perror(errmsg);
        }

        if(sigaddset(&sigset, catch_sig_list[sig_idx]) < 0){
            snprintf(errmsg, BUFSIZ, "Warning: couldn't include signal %d  for unblocking",
                catch_sig_list[sig_idx]);
            perror(errmsg);
        }
    }

    /* process ignored signals */
    for(sig_idx=0; ignore_sig_list[sig_idx]!= -1; ++sig_idx){
        if((prev_sig_disp = signal(ignore_sig_list[sig_idx], SIG_IGN)) == SIG_ERR){
            snprintf(errmsg, BUFSIZ, "Warning: couldn't get signal %d  to ignore",
                ignore_sig_list[sig_idx]);
            perror(errmsg);
        }
    }

    if(sigprocmask(SIG_UNBLOCK, &sigset, NULL)){
        snprintf(errmsg, BUFSIZ, "Warning: couldn't unblock signals");
        perror(errmsg);
    }
}

void
beasy_set_i18_support(void){

    #ifdef ENABLE_NLS
    bindtextdomain(PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(PACKAGE, "UTF-8");
    textdomain(PACKAGE);
    #endif

    #ifdef HAVE_SETLOCALE
    /* Local initialization is not complete here */
    setlocale(LC_ALL, "");
    #endif

}

static void
show_usage(const char *name, gboolean terse)
{
    char *text;

    if (terse) {
        text = g_strdup_printf(_("%s %s. Try `%s -h' for more information.\n"), PACKAGE_NAME, VERSION, name);
    }else{
        text = g_strdup_printf(_("%s %s\n"
               "Usage: %s [OPTION]...\n\n"
               "  -d, --debug         print debugging messages to stdout\n"
               "  -h, --help          display this help and exit\n"
               "  -v, --version       display the current version and exit\n"), PACKAGE_NAME, VERSION, name);
    }

    oul_print_utf8_to_console(stdout, text);
    g_free(text);
}

gboolean
beasy_parse_cmd_options(int argc, char *argv[]){
    int opt;
    gboolean debug_enabled = FALSE;
    gboolean show_help = FALSE;
    gboolean show_version = FALSE;
 
    struct option long_options[] = {
        {"debug",   no_argument,        NULL,   'd'},
        {"help",    no_argument,        NULL,   'h'},
        {"version", no_argument,        NULL,   'v'},
        {0, 0, 0, 0}
    };

    /* scan command options */
    opterr  = 1;
    while((opt = getopt_long(argc, argv,
                "dhv", long_options, NULL)) != -1){
        switch(opt){
            case 'd':
                debug_enabled = TRUE;
                break;
            case 'h':
                show_help = TRUE;
                break;
            case 'v':
                show_version = TRUE;
                break;
            case '?':
            default:
                show_usage(argv[0], TRUE);
                goto end;
        }
    }

    if(show_help){
        show_usage(argv[0], FALSE);
        goto end;
    }

    if(show_version){
        printf("%s %s\n", PACKAGE_NAME, VERSION);
        goto end;
    }

    oul_debug_set_enabled(debug_enabled);
    return FALSE;

end:
    #ifdef HAVE_SIGNAL_H
    g_free(segfault_msg);
    #endif

    return TRUE;
}

