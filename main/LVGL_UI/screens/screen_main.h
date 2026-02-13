#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Create and show the main screen */
void screen_main_create(void);

/* Optional: periodic updates for the main screen */
void screen_main_update(void);

/* Optional: destroy screen if you add screen switching later */
void screen_main_destroy(void);

#ifdef __cplusplus
}
#endif
