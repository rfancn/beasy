#ifndef _OUL_SOUND_H_
#define _OUL_SOUND_H_

/**
 * A type of sound.
 */

typedef enum _OulSoundEventID
{
	OUL_SOUND_INFO_NTF,		/** Got information level notification */
	OUL_SOUND_WARN_NTF, 	/** Got warning level notification */
	OUL_SOUND_ERROR_NTF,	/** Got error level notification */
	OUL_SOUND_FATAL_NTF, 	/** Got fatal level notification */
	OUL_NUM_SOUNDS

} OulSoundEventID;

typedef struct _OulSoundUiOps
{
	void (*init)(void);
	void (*uninit)(void);
	void (*play_file)(const char *filename);
	void (*play_event)(OulSoundEventID event);

	void (*_oul_reserved1)(void);
	void (*_oul_reserved2)(void);
	void (*_oul_reserved3)(void);
	void (*_oul_reserved4)(void);
} OulSoundUiOps;

void *	oul_sound_get_handle(void);


#endif

