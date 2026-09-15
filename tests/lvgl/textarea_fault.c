/* Inject failures only in the real textarea module, never in production.
 * The returning assertion handler lets the test check pointer ownership.
 * The ESP firmware's handler is non-returning and invokes emergency cleanup. */
#include <stddef.h>
extern void auroraTestAssertionReached(void);
extern void *auroraTestTextAreaRealloc(void *, size_t);
#define LV_ASSERT_HANDLER auroraTestAssertionReached();
#define lv_realloc auroraTestTextAreaRealloc
#include "../../targets/waveshare_p4/managed_components/lvgl__lvgl/src/widgets/textarea/lv_textarea.c"
