#include <gtk/gtkstock.h>

#ifndef _BEASY_STOCK_H_
#define _BEASY_STOCK_H_

/**************************************************************************/
/** @name Stock images                                                    */
/**************************************************************************/
/*@{*/
#define BEASY_STOCK_ACTION          "beasy-action"
#define BEASY_STOCK_ALIAS           "beasy-alias"
#define BEASY_STOCK_AWAY            "beasy-away"
#define BEASY_STOCK_CHAT            "beasy-chat"
#define BEASY_STOCK_CLEAR           "beasy-clear"
#define BEASY_STOCK_CLOSE_TABS      "beasy-close-tab"
#define BEASY_STOCK_DEBUG           "beasy-debug"
#define BEASY_STOCK_DISCONNECT      "beasy-disconnect"
#define BEASY_STOCK_DOWNLOAD        "beasy-download"
#define BEASY_STOCK_EDIT            "beasy-edit"
#define BEASY_STOCK_FGCOLOR         "beasy-fgcolor"
#define BEASY_STOCK_FILE_CANCELED   "beasy-file-canceled"
#define BEASY_STOCK_FILE_DONE       "beasy-file-done"
#define BEASY_STOCK_IGNORE          "beasy-ignore"
#define BEASY_STOCK_INFO            "beasy-info"
#define BEASY_STOCK_INVITE          "beasy-invite"
#define BEASY_STOCK_MODIFY          "beasy-modify"
#define BEASY_STOCK_OPEN_MAIL       "beasy-stock-open-mail"
#define BEASY_STOCK_PAUSE           "beasy-pause"
#define BEASY_STOCK_POUNCE          "beasy-pounce"
#define BEASY_STOCK_SIGN_OFF        "beasy-sign-off"
#define BEASY_STOCK_SIGN_ON         "beasy-sign-on"
#define BEASY_STOCK_TEXT_NORMAL     "beasy-text-normal"
#define BEASY_STOCK_TYPED           "beasy-typed"
#define BEASY_STOCK_UPLOAD          "beasy-upload"

/* Status icons */
#define BEASY_STOCK_STATUS_AVAILABLE  	"beasy-status-available"
#define BEASY_STOCK_STATUS_AVAILABLE_I 	"beasy-status-available-i"
#define BEASY_STOCK_STATUS_AWAY       	"beasy-status-away"
#define BEASY_STOCK_STATUS_AWAY_I     	"beasy-status-away-i"
#define BEASY_STOCK_STATUS_BUSY       	"beasy-status-busy"
#define BEASY_STOCK_STATUS_BUSY_I     	"beasy-status-busy-i"
#define BEASY_STOCK_STATUS_CHAT       	"beasy-status-chat"
#define BEASY_STOCK_STATUS_INVISIBLE  	"beasy-status-invisible"
#define BEASY_STOCK_STATUS_XA         	"beasy-status-xa"
#define BEASY_STOCK_STATUS_XA_I       	"beasy-status-xa-i"
#define BEASY_STOCK_STATUS_LOGIN      	"beasy-status-login"
#define BEASY_STOCK_STATUS_LOGOUT     	"beasy-status-logout"
#define BEASY_STOCK_STATUS_OFFLINE    	"beasy-status-offline"
#define BEASY_STOCK_STATUS_OFFLINE_I  	"beasy-status-offline"
#define BEASY_STOCK_STATUS_PERSON     	"beasy-status-person"
#define BEASY_STOCK_STATUS_MESSAGE    	"beasy-status-message"

/* Chat room emblems */
#define BEASY_STOCK_STATUS_IGNORED    "beasy-status-ignored"
#define BEASY_STOCK_STATUS_FOUNDER    "beasy-status-founder"
#define BEASY_STOCK_STATUS_OPERATOR   "beasy-status-operator"
#define BEASY_STOCK_STATUS_HALFOP     "beasy-status-halfop"
#define BEASY_STOCK_STATUS_VOICE      "beasy-status-voice"

/* Dialog icons */
#define BEASY_STOCK_DIALOG_AUTH			"beasy-dialog-auth"
#define BEASY_STOCK_DIALOG_ERROR		"beasy-dialog-error"
#define BEASY_STOCK_DIALOG_INFO			"beasy-dialog-info"
#define BEASY_STOCK_DIALOG_MAIL			"beasy-dialog-mail"
#define BEASY_STOCK_DIALOG_QUESTION		"beasy-dialog-question"
#define BEASY_STOCK_DIALOG_COOL			"beasy-dialog-cool"
#define BEASY_STOCK_DIALOG_WARNING		"beasy-dialog-warning"

/* StatusBox Animations */
#define BEASY_STOCK_ANIMATION_CONNECT0 	"beasy-anim-connect0"
#define BEASY_STOCK_ANIMATION_CONNECT1 	"beasy-anim-connect1"
#define BEASY_STOCK_ANIMATION_CONNECT2 	"beasy-anim-connect2"
#define BEASY_STOCK_ANIMATION_CONNECT3 	"beasy-anim-connect3"
#define BEASY_STOCK_ANIMATION_CONNECT4 	"beasy-anim-connect4"
#define BEASY_STOCK_ANIMATION_CONNECT5 	"beasy-anim-connect5"
#define BEASY_STOCK_ANIMATION_CONNECT6 	"beasy-anim-connect6"
#define BEASY_STOCK_ANIMATION_CONNECT7 	"beasy-anim-connect7"
#define BEASY_STOCK_ANIMATION_CONNECT8 	"beasy-anim-connect8"
#define BEASY_STOCK_ANIMATION_TYPING0	"beasy-anim-typing0"
#define BEASY_STOCK_ANIMATION_TYPING1	"beasy-anim-typing1"
#define BEASY_STOCK_ANIMATION_TYPING2	"beasy-anim-typing2"
#define BEASY_STOCK_ANIMATION_TYPING3	"beasy-anim-typing3"
#define BEASY_STOCK_ANIMATION_TYPING4	"beasy-anim-typing4"
#define BEASY_STOCK_ANIMATION_TYPING5	"beasy-anim-typing5"

/* Toolbar (and menu) icons */
#define BEASY_STOCK_TOOLBAR_ACCOUNTS     	"beasy-accounts"
#define BEASY_STOCK_TOOLBAR_BGCOLOR      	"beasy-bgcolor"
#define BEASY_STOCK_TOOLBAR_BLOCK        	"beasy-block"
#define BEASY_STOCK_TOOLBAR_FGCOLOR      	"beasy-fgcolor"
#define BEASY_STOCK_TOOLBAR_SMILEY       	"beasy-smiley"
#define BEASY_STOCK_TOOLBAR_FONT_FACE	  	"beasy-font-face"
#define BEASY_STOCK_TOOLBAR_TEXT_SMALLER 	"beasy-text-smaller"
#define BEASY_STOCK_TOOLBAR_TEXT_LARGER  	"beasy-text-larger"
#define BEASY_STOCK_TOOLBAR_INSERT       	"beasy-insert"
#define BEASY_STOCK_TOOLBAR_INSERT_IMAGE 	"beasy-insert-image"
#define BEASY_STOCK_TOOLBAR_INSERT_LINK  	"beasy-insert-link"
#define BEASY_STOCK_TOOLBAR_MESSAGE_NEW  	"beasy-message-new"
#define BEASY_STOCK_TOOLBAR_PENDING      	"beasy-pending"
#define BEASY_STOCK_TOOLBAR_PLUGINS      	"beasy-plugins"
#define BEASY_STOCK_TOOLBAR_TYPING       	"beasy-typing"
#define BEASY_STOCK_TOOLBAR_USER_INFO    	"beasy-info"
#define BEASY_STOCK_TOOLBAR_UNBLOCK      	"beasy-unblock"
#define BEASY_STOCK_TOOLBAR_SELECT_AVATAR 	"beasy-select-avatar"
#define BEASY_STOCK_TOOLBAR_SEND_FILE    	"beasy-send-file"

/* Tray icons */
#define BEASY_STOCK_TRAY_OFFLINE			"beasy-tray-offline"
#define BEASY_STOCK_TRAY_ONLINE				"beasy-tray-online"
#define BEASY_STOCK_TRAY_BUSY            	"beasy-tray-busy"
#define BEASY_STOCK_TRAY_CONNECT         	"beasy-tray-connect"
#define BEASY_STOCK_TRAY_PENDING         	"beasy-tray-pending"
#define BEASY_STOCK_TRAY_UNREAD_MSG	  		"beasy-tray-email"


/*@}*/

/**
 * For using icons that aren't one of the default GTK_ICON_SIZEs
 */
#define BEASY_ICON_SIZE_TANGO_MICROSCOPIC    "beasy-icon-size-tango-microscopic"
#define BEASY_ICON_SIZE_TANGO_EXTRA_SMALL    "beasy-icon-size-tango-extra-small"
#define BEASY_ICON_SIZE_TANGO_SMALL          "beasy-icon-size-tango-small"
#define BEASY_ICON_SIZE_TANGO_MEDIUM         "beasy-icon-size-tango-medium"
#define BEASY_ICON_SIZE_TANGO_LARGE          "beasy-icon-size-tango-large"
#define BEASY_ICON_SIZE_TANGO_HUGE           "beasy-icon-size-tango-huge"
/**
 * Sets up the beasy stock repository.
 */
void beasy_stock_init(void);

#endif /* _BEASY_STOCK_H_ */
