//! \defgroup Input
//! Gamepad, mouse, keyboard
//! \{
#ifndef CETECH_GAMEPAD_H
#define CETECH_GAMEPAD_H

//==============================================================================
// Includes
//==============================================================================

#include "cetech/types.h"
#include "cetech/math/math_types.h"

enum {
    KEYBOARD_API_ID = 3,
    MOUSE_API_ID = 4,
    GAMEPAD_API_ID = 5,
};

//==============================================================================
// Api
//==============================================================================

//! Gamepad API V0
struct gamepad_api_v0 {
    //! Is gamepad active?
    //! \param gamepad Gamepad
    //! \return 1 if active else 0
    int (*is_active)(uint32_t idx);

    //! Return button index
    //! \param button_name Button name
    //! \return Button index
    uint32_t (*button_index)(const char *button_name);

    //! Return button name
    //! \param button_index Button index
    //! \return Button name
    const char *(*button_name)(const uint32_t button_index);

    //! Return button state
    //! \param button_index Button index
    //! \return 1 if button is in current frame down else 0
    int (*button_state)(uint32_t idx,
                        const uint32_t button_index);

    //! Is button pressed?
    //! \param button_index Button index
    //! \return 1 if button is in current frame pressed else 0
    int (*button_pressed)(uint32_t idx,
                          const uint32_t button_index);

    //! Is button released?
    //! \param button_index Button index
    //! \return 1 if button is in current frame released else 0
    int (*button_released)(uint32_t idx,
                           const uint32_t button_index);

    //! Return axis index
    //! \param axis_name Axis name
    //! \return Axis index
    uint32_t (*axis_index)(const char *axis_name);

    //! Return axis name
    //! \param axis_index Axis index
    //! \return Axis name
    const char *(*axis_name)(const uint32_t axis_index);

    //! Return axis value
    //! \param axis_index Axis index
    //! \return Axis value
    cel_vec2f_t (*axis)(uint32_t idx,
                        const uint32_t axis_index);

    //! Play rumble
    //! \param gamepad Gamepad
    //! \param strength Rumble strength
    //! \param length Rumble length
    void (*play_rumble)(uint32_t idx,
                        float strength,
                        uint32_t length);
};

//==============================================================================
// Api
//==============================================================================

//! Keyboard API V0
struct keyboard_api_v0 {
    //! Return button index
    //! \param button_name Button name
    //! \return Button index
    uint32_t (*button_index)(const char *button_name);

    //! Return button name
    //! \param button_index Button index
    //! \return Button name
    const char *(*button_name)(const uint32_t button_index);

    //! Return button state
    //! \param button_index Button index
    //! \return 1 if button is in current frame down else 0
    int (*button_state)(uint32_t idx,
                        const uint32_t button_index);

    //! Is button pressed?
    //! \param button_index Button index
    //! \return 1 if button is in current frame pressed else 0
    int (*button_pressed)(uint32_t idx,
                          const uint32_t button_index);

    //! Is button released?
    //! \param button_index Button index
    //! \return 1 if button is in current frame released else 0
    int (*button_released)(uint32_t idx,
                           const uint32_t button_index);
};

//==============================================================================
// Api
//==============================================================================

//! Mouse API V0
struct mouse_api_v0 {
    //! Return button index
    //! \param button_name Button name
    //! \return Button index
    uint32_t (*button_index)(const char *button_name);

    //! Return button name
    //! \param button_index Button index
    //! \return Button name
    const char *(*button_name)(const uint32_t button_index);

    //! Return button state
    //! \param button_index Button index
    //! \return 1 if button is in current frame down else 0
    int (*button_state)(uint32_t idx,
                        const uint32_t button_index);

    //! Is button pressed?
    //! \param button_index Button index
    //! \return 1 if button is in current frame pressed else 0
    int (*button_pressed)(uint32_t idx,
                          const uint32_t button_index);

    //! Is button released?
    //! \param button_index Button index
    //! \return 1 if button is in current frame released else 0
    int (*button_released)(uint32_t idx,
                           const uint32_t button_index);

    //! Return axis index
    //! \param axis_name Axis name
    //! \return Axis index
    uint32_t (*axis_index)(const char *axis_name);

    //! Return axis name
    //! \param axis_index Axis index
    //! \return Axis name
    const char *(*axis_name)(const uint32_t axis_index);

    //! Return axis value
    //! \param axis_index Axis index
    //! \return Axis value
    cel_vec2f_t (*axis)(uint32_t idx,
                        const uint32_t axis_index);
};


#endif //CETECH_GAMEPAD_H
//! \}