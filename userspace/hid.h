/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef HID_H
#define HID_H

#include "ipc.h"

#pragma pack(push, 1)

#define HID_INTERFACE_CLASS                                                             3

#define HID_SUBCLASS_NO_SUBCLASS                                                        0
#define HID_SUBCLASS_BOOT_INTERFACE                                                     1

#define HID_PROTOCOL_NONE                                                               0
#define HID_PROTOCOL_KEYBOARD                                                           1
#define HID_PROTOCOL_MOUSE                                                              2

#define HID_DESCRIPTOR_TYPE                                                             0x21
#define HID_REPORT_DESCRIPTOR_TYPE                                                      0x22

#define HID_GET_REPORT                                                                  0x01
#define HID_GET_IDLE                                                                    0x02
#define HID_GET_PROTOCOL                                                                0x03
#define HID_SET_REPORT                                                                  0x09
#define HID_SET_IDLE                                                                    0x0a
#define HID_SET_PROTOCOL                                                                0x0b

#define HID_REPORT_TYPE_INPUT                                                           0x01
#define HID_REPORT_TYPE_OUTPUT                                                          0x02
#define HID_REPORT_TYPE_FEATURE                                                         0x03

typedef struct {
    uint8_t bLength;                                                                    /* Size of this descriptor in bytes */
    uint8_t bDescriptorType;                                                            /* HID descriptor type (assigned by USB) */
    uint16_t bcdHID;                                                                    /* HID Class Specification release number in
                                                                                           binary-coded decimal—for example, 2.10 is 0x210) */
    uint8_t bCountryCode;                                                               /* Hardware target country */
    uint8_t bNumDescriptors;                                                            /* Number of HID class descriptors to follow */
    uint8_t bReportDescriptorType;                                                      /* Report descriptor type */
    uint16_t wReportDescriptorLength;                                                   /* Total length of Report descriptor */
    //optional descriptors may follow
} HID_DESCRIPTOR;

#define HID_COUNTRY_NOT_SUPPORTED                                                       0
#define HID_COUNTRY_ARABIC                                                              1
#define HID_COUNTRY_BELGIAN                                                             2
#define HID_COUNTRY_CANADIAN_BILINGUAL                                                  3
#define HID_COUNTRY_CANADIAN_FRENCH                                                     4
#define HID_COUNTRY_CZECH                                                               5
#define HID_COUNTRY_DANISH                                                              6
#define HID_COUNTRY_FINNISH                                                             7
#define HID_COUNTRY_FRENCH                                                              8
#define HID_COUNTRY_GERMAN                                                              9
#define HID_COUNTRY_GREEK                                                               10
#define HID_COUNTRY_HEBREW                                                              11
#define HID_COUNTRY_HUNGARY                                                             12
#define HID_COUNTRY_ISO                                                                 13
#define HID_COUNTRY_ITALIAN                                                             14
#define HID_COUNTRY_JAPAN                                                               15
#define HID_COUNTRY_KOREAN                                                              16
#define HID_COUNTRY_LATIN_AMERICAN                                                      17
#define HID_COUNTRY_NETHERLANDS                                                         18
#define HID_COUNTRY_NORWEGIAN                                                           19
#define HID_COUNTRY_PERSIAN                                                             20
#define HID_COUNTRY_POLAND                                                              21
#define HID_COUNTRY_PORTUGUESE                                                          22
#define HID_COUNTRY_RUSSIA                                                              23
#define HID_COUNTRY_SLOVAKIA                                                            24
#define HID_COUNTRY_SPANISH                                                             25
#define HID_COUNTRY_SWEDISH                                                             26
#define HID_COUNTRY_SWISS_F                                                             27
#define HID_COUNTRY_SWISS_G                                                             28
#define HID_COUNTRY_SWITZERLAND                                                         29
#define HID_COUNTRY_TAIWAN                                                              30
#define HID_COUNTRY_TURKISH_Q                                                           31
#define HID_COUNTRY_UK                                                                  32
#define HID_COUNTRY_US                                                                  33
#define HID_COUNTRY_YUGOSLAVIA                                                          34
#define HID_COUNTRY_TURKISH_F                                                           35

#define HID_ITEM_SIZE_0                                                                 (0 << 0)
#define HID_ITEM_SIZE_1                                                                 (1 << 0)
#define HID_ITEM_SIZE_2                                                                 (2 << 0)
#define HID_ITEM_SIZE_4                                                                 (3 << 0)

#define HID_ITEM_MAIN                                                                   (0 << 2)
#define HID_ITEM_GLOBAL                                                                 (1 << 2)
#define HID_ITEM_LOCAL                                                                  (2 << 2)

//------------------------------------------------- HID Main -----------------------------------------------------------------------

#define HID_INPUT                                                                       (0x8 << 4) | HID_ITEM_MAIN | HID_ITEM_SIZE_1
#define HID_INPUT_BUFFERED                                                              (0x8 << 4) | HID_ITEM_MAIN | HID_ITEM_SIZE_2
#define HID_OUTPUT                                                                      (0x9 << 4) | HID_ITEM_MAIN | HID_ITEM_SIZE_1
#define HID_OUTPUT_BUFFERED                                                             (0x9 << 4) | HID_ITEM_MAIN | HID_ITEM_SIZE_2
#define HID_FEATURE                                                                     (0xB << 4) | HID_ITEM_MAIN | HID_ITEM_SIZE_1
#define HID_FEATURE_BUFFERED                                                            (0xB << 4) | HID_ITEM_MAIN | HID_ITEM_SIZE_2
#define HID_COLLECTION                                                                  (0xA << 4) | HID_ITEM_MAIN | HID_ITEM_SIZE_1
#define HID_END_COLLECTION                                                              (0xC << 4) | HID_ITEM_MAIN | HID_ITEM_SIZE_0

#define HID_DATA                                                                        (0 << 0)
#define HID_CONSTANT                                                                    (1 << 0)

#define HID_ARRAY                                                                       (0 << 1)
#define HID_VARIABLE                                                                    (1 << 1)

#define HID_ABSOLUTE                                                                    (0 << 2)
#define HID_RELATIVE                                                                    (1 << 2)

#define HID_NO_WRAP                                                                     (0 << 3)
#define HID_WRAP                                                                        (1 << 3)

#define HID_LINEAR                                                                      (0 << 4)
#define HID_NONLINEAR                                                                   (1 << 4)

#define HID_PREFERRED_STATE                                                             (0 << 5)
#define HID_NO_PREFERRED_STATE                                                          (1 << 5)

#define HID_NO_NULL_STATE                                                               (0 << 6)
#define HID_NULL_STATE                                                                  (1 << 6)

#define HID_BIT_FIELDS                                                                  (0 << 0)
#define HID_BUFFERED_BYTES                                                              (1 << 0)

#define HID_COLLECTION_PHYSICAL                                                         0x00
#define HID_COLLECTION_APPLICATION                                                      0x01
#define HID_COLLECTION_LOGICAL                                                          0x02
#define HID_COLLECTION_REPORT                                                           0x03
#define HID_COLLECTION_NAMED_ARRAY                                                      0x04
#define HID_COLLECTION_USAGE_SWITCH                                                     0x05
#define HID_COLLECTION_USAGE_MODIFIER                                                   0x06

//------------------------------------------------- HID Global -----------------------------------------------------------------------

#define HID_USAGE_PAGE                                                                  (0x0 << 4) | HID_ITEM_GLOBAL | HID_ITEM_SIZE_1
#define HID_LOGICAL_MINIMUM                                                             (0x1 << 4) | HID_ITEM_GLOBAL | HID_ITEM_SIZE_1
#define HID_LOGICAL_MAXIMUM                                                             (0x2 << 4) | HID_ITEM_GLOBAL | HID_ITEM_SIZE_1
#define HID_PHYSICAL_MINIMUM                                                            (0x3 << 4) | HID_ITEM_GLOBAL | HID_ITEM_SIZE_1
#define HID_PHYSICAL_MAXIMUM                                                            (0x4 << 4) | HID_ITEM_GLOBAL | HID_ITEM_SIZE_1
#define HID_UNIT_EXPONENT                                                               (0x5 << 4) | HID_ITEM_GLOBAL | HID_ITEM_SIZE_1
#define HID_UNIT                                                                        (0x6 << 4) | HID_ITEM_GLOBAL | HID_ITEM_SIZE_1
#define HID_REPORT_SIZE                                                                 (0x7 << 4) | HID_ITEM_GLOBAL | HID_ITEM_SIZE_1
#define HID_REPORT_ID                                                                   (0x8 << 4) | HID_ITEM_GLOBAL | HID_ITEM_SIZE_1
#define HID_REPORT_COUNT                                                                (0x9 << 4) | HID_ITEM_GLOBAL | HID_ITEM_SIZE_1
#define HID_PUSH                                                                        (0xa << 4) | HID_ITEM_GLOBAL | HID_ITEM_SIZE_0
#define HID_POP                                                                         (0xb << 4) | HID_ITEM_GLOBAL | HID_ITEM_SIZE_0

#define HID_EXPO_5                                                                      0x5
#define HID_EXPO_6                                                                      0x6
#define HID_EXPO_7                                                                      0x7
#define HID_EXPO_MINUS_8                                                                0x8
#define HID_EXPO_MINUS_7                                                                0x9
#define HID_EXPO_MINUS_6                                                                0xa
#define HID_EXPO_MINUS_5                                                                0xb
#define HID_EXPO_MINUS_4                                                                0xc
#define HID_EXPO_MINUS_3                                                                0xd
#define HID_EXPO_MINUS_2                                                                0xe
#define HID_EXPO_MINUS_1                                                                0xf
//------------------------------------------------- HID Local ------------------------------------------------------------------------

#define HID_USAGE                                                                       (0x0 << 4) | HID_ITEM_LOCAL | HID_ITEM_SIZE_1
#define HID_USAGE_MINIMUM                                                               (0x1 << 4) | HID_ITEM_LOCAL | HID_ITEM_SIZE_1
#define HID_USAGE_MAXIMUM                                                               (0x2 << 4) | HID_ITEM_LOCAL | HID_ITEM_SIZE_1
#define HID_DESIGNATOR_INDEX                                                            (0x3 << 4) | HID_ITEM_LOCAL | HID_ITEM_SIZE_1
#define HID_DESIGNATOR_MINIMUM                                                          (0x4 << 4) | HID_ITEM_LOCAL | HID_ITEM_SIZE_1
#define HID_DESIGNATOR_MAXIMUM                                                          (0x5 << 4) | HID_ITEM_LOCAL | HID_ITEM_SIZE_1
#define HID_STRING_INDEX                                                                (0x7 << 4) | HID_ITEM_LOCAL | HID_ITEM_SIZE_1
#define HID_STRING_MINIMUM                                                              (0x8 << 4) | HID_ITEM_LOCAL | HID_ITEM_SIZE_1
#define HID_STRING_MAXIMUM                                                              (0x9 << 4) | HID_ITEM_LOCAL | HID_ITEM_SIZE_1
#define HID_DELIMITER                                                                   (0xA << 4) | HID_ITEM_LOCAL | HID_ITEM_SIZE_1

//------------------------------------------------- HID usage tables --------------------------------------------------------------------

#define HID_USAGE_PAGE_UNDEFINED                                                        0x00
#define HID_USAGE_PAGE_GENERIC_DESKTOP_CONTROLS                                         0x01
#define HID_USAGE_PAGE_SIMULATION_CONTROLS                                              0x02
#define HID_USAGE_PAGE_VR_CONTROLS                                                      0x03
#define HID_USAGE_PAGE_SPORT_CONTROLS                                                   0x04
#define HID_USAGE_PAGE_GAME_CONTROLS                                                    0x05
#define HID_USAGE_PAGE_GENERIC_DEVICE_CONTROLS                                          0x06

#define HID_USAGE_PAGE_KEYS                                                             0x07
#define HID_USAGE_PAGE_LEDS                                                             0x08
#define HID_USAGE_PAGE_BUTTON                                                           0x09
#define HID_USAGE_PAGE_ORDINAL                                                          0x0a
#define HID_USAGE_PAGE_TELEPHONY                                                        0x0b
#define HID_USAGE_PAGE_CONSUMER                                                         0x0c
#define HID_USAGE_PAGE_DIGITIZER                                                        0x0d
#define HID_USAGE_PAGE_PID                                                              0x0f
#define HID_USAGE_PAGE_UNICODE                                                          0x10
#define HID_USAGE_PAGE_ALPHANUMERIC_DISPLAY                                             0x14
#define HID_USAGE_PAGE_MEDICAL_INSTRUMENTS                                              0x40
#define HID_USAGE_PAGE_MONITOR0                                                         0x80
#define HID_USAGE_PAGE_MONITOR1                                                         0x81
#define HID_USAGE_PAGE_MONITOR2                                                         0x82
#define HID_USAGE_PAGE_MONITOR3                                                         0x83
#define HID_USAGE_PAGE_POWER0                                                           0x84
#define HID_USAGE_PAGE_POWER1                                                           0x85
#define HID_USAGE_PAGE_POWER2                                                           0x86
#define HID_USAGE_PAGE_POWER3                                                           0x87
#define HID_USAGE_PAGE_BARCODE_SCANNER                                                  0x8c
#define HID_USAGE_PAGE_SCALE_PAGE                                                       0x8d
#define HID_USAGE_PAGE_MAGNETIC_STRIP_READER                                            0x8e
#define HID_USAGE_CAMERA_CONTROL                                                        0x90
#define HID_USAGE_PAGE_ARCADE                                                           0x91

//------------------------------------------ HID usage generic desktop control ---------------------------------------------------------------

#define HID_USAGE_POINTER                                                               0x01
#define HID_USAGE_MOUSE                                                                 0x02
#define HID_USAGE_JOYSTICK                                                              0x04
#define HID_USAGE_GAME_PAD                                                              0x05
#define HID_USAGE_KEYBOARD                                                              0x06
#define HID_USAGE_KEYPAD                                                                0x07
#define HID_USAGE_MULTI_AXIS_CONTROLLER                                                 0x08
#define HID_USAGE_TABLET_PC_SYSTEM_CONTROL                                              0x09
#define HID_USAGE_X                                                                     0x30
#define HID_USAGE_Y                                                                     0x31
#define HID_USAGE_Z                                                                     0x32
#define HID_USAGE_RX                                                                    0x33
#define HID_USAGE_RY                                                                    0x34
#define HID_USAGE_RZ                                                                    0x35
#define HID_USAGE_SLIDER                                                                0x36
#define HID_USAGE_DIAL                                                                  0x37
#define HID_USAGE_WHEEL                                                                 0x38
#define HID_USAGE_HAT_SWITCH                                                            0x39
#define HID_USAGE_COUNTED_BUFFER                                                        0x3a
#define HID_USAGE_BYTE_COUNT                                                            0x3b
#define HID_USAGE_MOTION_WAKEUP                                                         0x3c
#define HID_USAGE_START                                                                 0x3d
#define HID_USAGE_SELECT                                                                0x3e
#define HID_USAGE_VX                                                                    0x40
#define HID_USAGE_VY                                                                    0x41
#define HID_USAGE_VZ                                                                    0x42
#define HID_USAGE_VBRX                                                                  0x43
#define HID_USAGE_VBRY                                                                  0x44
#define HID_USAGE_VBRZ                                                                  0x45
#define HID_USAGE_VNO                                                                   0x46
#define HID_USAGE_FEATURE_NOTIFICATION                                                  0x47
#define HID_USAGE_RESOLUTION_MULTIPLIER                                                 0x48
#define HID_USAGE_SYSTEM_CONTROL                                                        0x80
#define HID_USAGE_SYSTEM_POWER_DOWN                                                     0x81
#define HID_USAGE_SYSTEM_SLEEP                                                          0x82
#define HID_USAGE_SYSTEM_WAKEUP                                                         0x83
#define HID_USAGE_SYSTEM_CONTEXT_MENU                                                   0x84
#define HID_USAGE_SYSTEM_MAIN_MENU                                                      0x85
#define HID_USAGE_SYSTEM_APP_MENU                                                       0x86
#define HID_USAGE_SYSTEM_MENU_HELP                                                      0x87
#define HID_USAGE_SYSTEM_MENU_EXIT                                                      0x88
#define HID_USAGE_SYSTEM_MENU_SELECT                                                    0x89
#define HID_USAGE_SYSTEM_MENU_RIGHT                                                     0x8a
#define HID_USAGE_SYSTEM_MENU_LEFT                                                      0x8b
#define HID_USAGE_SYSTEM_MENU_UP                                                        0x8c
#define HID_USAGE_SYSTEM_MENU_DOWN                                                      0x8d
#define HID_USAGE_SYSTEM_COLD_RESTART                                                   0x8e
#define HID_USAGE_SYSTEM_WARM_RESTART                                                   0x8f
#define HID_USAGE_D_PAD_UP                                                              0x90
#define HID_USAGE_D_PAD_DOWN                                                            0x91
#define HID_USAGE_D_PAD_RIGHT                                                           0x92
#define HID_USAGE_D_PAD_LEFT                                                            0x93
#define HID_USAGE_SYSTEM_DOCK                                                           0xa0
#define HID_USAGE_SYSTEM_UNDOCK                                                         0xa1
#define HID_USAGE_SYSTEM_SETUP                                                          0xa2
#define HID_USAGE_SYSTEM_BREAK                                                          0xa3
#define HID_USAGE_SYSTEM_DEBUGGER_BREAK                                                 0xa4
#define HID_USAGE_APPLICATION_BREAK                                                     0xa5
#define HID_USAGE_APPLICATION_DEBUGGER_BREAK                                            0xa6
#define HID_USAGE_SYSTEM_SPEAKER_MUTE                                                   0xa7
#define HID_USAGE_SYSTEM_HIBERNATE                                                      0xa8
#define HID_USAGE_SYSTEM_DISPLAY_INVERT                                                 0xb0
#define HID_USAGE_SYSTEM_DISPLAY_INTERNAL                                               0xb1
#define HID_USAGE_SYSTEM_DISPLAY_EXTERNAL                                               0xb2
#define HID_USAGE_SYSTEM_DISPLAY_BOTH                                                   0xb3
#define HID_USAGE_SYSTEM_DISPLAY_DUAL                                                   0xb4
#define HID_USAGE_SYSTEM_DISPLAY_TOGGLE_EXT_INT                                         0xb5
#define HID_USAGE_SYSTEM_DISPLAY_SWAP_PRIMARY_SECONDARY                                 0xb6
#define HID_USAGE_SYSTEM_DISPLAY_LCD_AUTOSCALE                                          0xb7

//------------------------------------------ HID usage keys ---------------------------------------------------------------

#define HID_KEY_NO_PRESS                                                                0x00
#define HID_KEY_ROLLOVER                                                                0x01
#define HID_KEY_POST_FAIL                                                               0x02
#define HID_KEY_ERROR_UNDEFINED                                                         0x03
#define HID_KEY_A                                                                       0x04
#define HID_KEY_B                                                                       0x05
#define HID_KEY_C                                                                       0x06
#define HID_KEY_D                                                                       0x07
#define HID_KEY_E                                                                       0x08
#define HID_KEY_F                                                                       0x09
#define HID_KEY_G                                                                       0x0a
#define HID_KEY_H                                                                       0x0b
#define HID_KEY_I                                                                       0x0c
#define HID_KEY_J                                                                       0x0d
#define HID_KEY_K                                                                       0x0e
#define HID_KEY_L                                                                       0x0f
#define HID_KEY_M                                                                       0x10
#define HID_KEY_N                                                                       0x11
#define HID_KEY_O                                                                       0x12
#define HID_KEY_P                                                                       0x13
#define HID_KEY_Q                                                                       0x14
#define HID_KEY_R                                                                       0x15
#define HID_KEY_S                                                                       0x16
#define HID_KEY_T                                                                       0x17
#define HID_KEY_U                                                                       0x18
#define HID_KEY_V                                                                       0x19
#define HID_KEY_W                                                                       0x1a
#define HID_KEY_X                                                                       0x1b
#define HID_KEY_Y                                                                       0x1c
#define HID_KEY_Z                                                                       0x1d
#define HID_KEY_1                                                                       0x1e
#define HID_KEY_2                                                                       0x1f
#define HID_KEY_3                                                                       0x20
#define HID_KEY_4                                                                       0x21
#define HID_KEY_5                                                                       0x22
#define HID_KEY_6                                                                       0x23
#define HID_KEY_7                                                                       0x24
#define HID_KEY_8                                                                       0x25
#define HID_KEY_9                                                                       0x26
#define HID_KEY_0                                                                       0x27
#define HID_KEY_ENTER                                                                   0x28
#define HID_KEY_ESC                                                                     0x29
#define HID_KEY_DEL                                                                     0x2a
#define HID_KEY_TAB                                                                     0x2b
#define HID_KEY_SPACE                                                                   0x2c
#define HID_KEY_MINUS                                                                   0x2d
#define HID_KEY_EQUAL                                                                   0x2e
#define HID_KEY_OPEN_BRACKET                                                            0x2f
#define HID_KEY_CLOSE_BRACKET                                                           0x30
#define HID_KEY_BACK_SLASH                                                              0x31
#define HID_KEY_SHARP                                                                   0x32
#define HID_KEY_COLON                                                                   0x33
#define HID_KEY_QUOTE                                                                   0x34
#define HID_KEY_TILDE                                                                   0x35
#define HID_KEY_LESS                                                                    0x36
#define HID_KEY_GREATER                                                                 0x37
#define HID_KEY_SLASH                                                                   0x38
#define HID_KEY_CAPS_LOCK                                                               0x39
#define HID_KEY_F1                                                                      0x3a
#define HID_KEY_F2                                                                      0x3b
#define HID_KEY_F3                                                                      0x3c
#define HID_KEY_F4                                                                      0x3d
#define HID_KEY_F5                                                                      0x3e
#define HID_KEY_F6                                                                      0x3f
#define HID_KEY_F7                                                                      0x40
#define HID_KEY_F8                                                                      0x41
#define HID_KEY_F9                                                                      0x42
#define HID_KEY_F10                                                                     0x43
#define HID_KEY_F11                                                                     0x44
#define HID_KEY_F12                                                                     0x45
#define HID_KEY_PRINT_SCREEN                                                            0x46
#define HID_KEY_SCROLL_LOCK                                                             0x47
#define HID_KEY_PAUSE                                                                   0x48
#define HID_KEY_INSERT                                                                  0x49
#define HID_KEY_HOME                                                                    0x4a
#define HID_KEY_PAGEUP                                                                  0x4b
#define HID_KEY_DELETE                                                                  0x4c
#define HID_KEY_END                                                                     0x4d
#define HID_KEY_PAGEDOWN                                                                0x4e
#define HID_KEY_RIGHT                                                                   0x4f
#define HID_KEY_LEFT                                                                    0x50
#define HID_KEY_DOWN                                                                    0x51
#define HID_KEY_UP                                                                      0x52
#define HID_KEY_NUM_LOCK                                                                0x53
#define HID_KEY_KEYPAD_DIV                                                              0x54
#define HID_KEY_KEYPAD_MUL                                                              0x55
#define HID_KEY_KEYPAD_SUB                                                              0x56
#define HID_KEY_KEYPAD_ADD                                                              0x57
#define HID_KEY_KEYPAD_ENTER                                                            0x58
#define HID_KEY_KEYPAD_1                                                                0x59
#define HID_KEY_KEYPAD_2                                                                0x5a
#define HID_KEY_KEYPAD_3                                                                0x5b
#define HID_KEY_KEYPAD_4                                                                0x5c
#define HID_KEY_KEYPAD_5                                                                0x5d
#define HID_KEY_KEYPAD_6                                                                0x5e
#define HID_KEY_KEYPAD_7                                                                0x5f
#define HID_KEY_KEYPAD_8                                                                0x60
#define HID_KEY_KEYPAD_9                                                                0x61
#define HID_KEY_KEYPAD_0                                                                0x62
#define HID_KEY_KEYPAD_DELETE                                                           0x63
#define HID_KEY_KEYPAD_SLASH                                                            0x64
#define HID_KEY_APPLICATION                                                             0x65
#define HID_KEY_POWER                                                                   0x66
#define HID_KEY_KEYPAD_EQUAL                                                            0x67
#define HID_KEY_F13                                                                     0x68
#define HID_KEY_F14                                                                     0x69
#define HID_KEY_F15                                                                     0x6a
#define HID_KEY_F16                                                                     0x6b
#define HID_KEY_F17                                                                     0x6c
#define HID_KEY_F18                                                                     0x6d
#define HID_KEY_F19                                                                     0x6e
#define HID_KEY_F20                                                                     0x6f
#define HID_KEY_F21                                                                     0x70
#define HID_KEY_F22                                                                     0x71
#define HID_KEY_F23                                                                     0x72
#define HID_KEY_F24                                                                     0x73
#define HID_KEY_EXECUTE                                                                 0x74
#define HID_KEY_HELP                                                                    0x75
#define HID_KEY_MENU                                                                    0x76
#define HID_KEY_SELECT                                                                  0x77
#define HID_KEY_STOP                                                                    0x78
#define HID_KEY_AGAIN                                                                   0x79
#define HID_KEY_UNDO                                                                    0x7a
#define HID_KEY_CUT                                                                     0x7b
#define HID_KEY_COPY                                                                    0x7c
#define HID_KEY_PASTE                                                                   0x7d
#define HID_KEY_FIND                                                                    0x7e
#define HID_KEY_MUTE                                                                    0x7f
#define HID_KEY_VOLUME_UP                                                               0x80
#define HID_KEY_VOLUME_DOWN                                                             0x81
#define HID_KEY_LOCKING_CAPS_LOCK                                                       0x82
#define HID_KEY_LOCKING_NUM_LOCK                                                        0x83
#define HID_KEY_LOCKING_SCROLL_LOCK                                                     0x84
#define HID_KEY_KEYPAD_COMMA                                                            0x85
#define HID_KEY_KEYPAD_EQUAL_SIGN                                                       0x86
#define HID_KEY_INTERNATIONAL_1                                                         0x87
#define HID_KEY_INTERNATIONAL_2                                                         0x88
#define HID_KEY_INTERNATIONAL_3                                                         0x89
#define HID_KEY_INTERNATIONAL_4                                                         0x8a
#define HID_KEY_INTERNATIONAL_5                                                         0x8b
#define HID_KEY_INTERNATIONAL_6                                                         0x8c
#define HID_KEY_INTERNATIONAL_7                                                         0x8d
#define HID_KEY_INTERNATIONAL_8                                                         0x8e
#define HID_KEY_INTERNATIONAL_9                                                         0x8f
#define HID_KEY_LANG_1                                                                  0x90
#define HID_KEY_LANG_2                                                                  0x91
#define HID_KEY_LANG_3                                                                  0x92
#define HID_KEY_LANG_4                                                                  0x93
#define HID_KEY_LANG_5                                                                  0x94
#define HID_KEY_LANG_6                                                                  0x95
#define HID_KEY_LANG_7                                                                  0x96
#define HID_KEY_LANG_8                                                                  0x97
#define HID_KEY_LANG_9                                                                  0x98
#define HID_KEY_ALTERNATE_ERASE                                                         0x99
#define HID_KEY_SYSREQ                                                                  0x9a
#define HID_KEY_CANCEL                                                                  0x9b
#define HID_KEY_CLEAR                                                                   0x9c
#define HID_KEY_PRIOR                                                                   0x9d
#define HID_KEY_RETURN                                                                  0x9e
#define HID_KEY_SEPARATOR                                                               0x9f
#define HID_KEY_OUT                                                                     0xa0
#define HID_KEY_OPER                                                                    0xa1
#define HID_KEY_CLEAR_AGAIN                                                             0xa2
#define HID_KEY_CRSEL                                                                   0xa3
#define HID_KEY_EXSEL                                                                   0xa4
#define HID_KEY_KEYPAD_00                                                               0xb0
#define HID_KEY_KEYPAD_000                                                              0xb1
#define HID_KEY_THOUSANDS_SEPARATOR                                                     0xb2
#define HID_KEY_DECIMAL_SEPARATOR                                                       0xb3
#define HID_KEY_CURRENCY_UNIT                                                           0xb4
#define HID_KEY_CURRENCY_SUB_UNIT                                                       0xb5
#define HID_KEY_KEYPAD_OPEN_PARENTHESIS                                                 0xb6
#define HID_KEY_KEYPAD_CLOSE_PARENTHESIS                                                0xb7
#define HID_KEY_KEYPAD_OPEN_BRACE                                                       0xb8
#define HID_KEY_KEYPAD_CLOSE_BRACE                                                      0xb9
#define HID_KEY_KEYPAD_TAB                                                              0xba
#define HID_KEY_KEYPAD_BACKSPACE                                                        0xbb
#define HID_KEY_KEYPAD_A                                                                0xbc
#define HID_KEY_KEYPAD_B                                                                0xbd
#define HID_KEY_KEYPAD_C                                                                0xbe
#define HID_KEY_KEYPAD_D                                                                0xbf
#define HID_KEY_KEYPAD_E                                                                0xco
#define HID_KEY_KEYPAD_F                                                                0xc1
#define HID_KEY_KEYPAD_XOR                                                              0xc2
#define HID_KEY_KEYPAD_CARET                                                            0xc3
#define HID_KEY_KEYPAD_PERCENT                                                          0xc4
#define HID_KEY_KEYPAD_LESSER                                                           0xc5
#define HID_KEY_KEYPAD_GREATER                                                          0xc6
#define HID_KEY_KEYPAD_AND                                                              0xc7
#define HID_KEY_KEYPAD_LOGICAL_AND                                                      0xc8
#define HID_KEY_KEYPAD_OR                                                               0xc9
#define HID_KEY_KEYPAD_LOGICAL_OR                                                       0xca
#define HID_KEY_KEYPAD_COLON                                                            0xcb
#define HID_KEY_KEYPAD_SHARP                                                            0xcc
#define HID_KEY_KEYPAD_SPACE                                                            0xcd
#define HID_KEY_KEYPAD_AT                                                               0xce
#define HID_KEY_KEYPAD_BANG                                                             0xcf
#define HID_KEY_KEYPAD_MEMORY_STORE                                                     0xd0
#define HID_KEY_KEYPAD_MEMORY_RECALL                                                    0xd1
#define HID_KEY_KEYPAD_MEMORY_CLEAD                                                     0xd2
#define HID_KEY_KEYPAD_MEMORY_ADD                                                       0xd3
#define HID_KEY_KEYPAD_MEMORY_SUBSTRACT                                                 0xd4
#define HID_KEY_KEYPAD_MEMORY_MULTIPLY                                                  0xd5
#define HID_KEY_KEYPAD_MEMORY_DIVIDE                                                    0xd6
#define HID_KEY_KEYPAD_SIGN                                                             0xd7
#define HID_KEY_KEYPAD_CLEAR                                                            0xd8
#define HID_KEY_KEYPAD_CLEAR_ENTRY                                                      0xd9
#define HID_KEY_KEYPAD_BINARY                                                           0xda
#define HID_KEY_KEYPAD_OCTAL                                                            0xdb
#define HID_KEY_KEYPAD_DECIMAL                                                          0xdc
#define HID_KEY_KEYPAD_HEXADECIMAL                                                      0xdd

#define HID_KEY_LEFT_CONTROL                                                            0xe0
#define HID_KEY_LEFT_SHIFT                                                              0xe1
#define HID_KEY_LEFT_ALT                                                                0xe2
#define HID_KEY_LEFT_GUI                                                                0xe3
#define HID_KEY_RIGHT_CONTROL                                                           0xe0
#define HID_KEY_RIGHT_SHIFT                                                             0xe1
#define HID_KEY_RIGHT_ALT                                                               0xe2
#define HID_KEY_RIGHT_GUI                                                               0xe3

//------------------------------------------ HID usage LEDs ---------------------------------------------------------------

#define HID_LED_UNDEFINED                                                               0x00
#define HID_LED_NUM_LOCK                                                                0x01
#define HID_LED_CAPS_LOCK                                                               0x02
#define HID_LED_SCROLL_LOCK                                                             0x03
#define HID_LED_COMPOSE                                                                 0x04
#define HID_LED_CANA                                                                    0x05
#define HID_LED_POWER                                                                   0x06
#define HID_LED_SHIFT                                                                   0x07
#define HID_LED_DO_NOT_DISTURB                                                          0x08
#define HID_LED_MUTE                                                                    0x09
#define HID_LED_TONE_ENABLE                                                             0x0a
#define HID_LED_HIGH_CUT_FILTER                                                         0x0b
#define HID_LED_LOW_CUT_FILTER                                                          0x0c
#define HID_LED_EQUALIZER_ENABLE                                                        0x0d
#define HID_LED_SOUND_FIELD_ON                                                          0x0e
#define HID_LED_SURROUND_ON                                                             0x0f
#define HID_LED_REPEAT                                                                  0x10
#define HID_LED_STEREO                                                                  0x11
#define HID_LED_SAMPLING_RATE_DETECT                                                    0x12
#define HID_LED_SPINNING                                                                0x13
#define HID_LED_CAV                                                                     0x14
#define HID_LED_CLV                                                                     0x15
#define HID_LED_RECORDING_FORMAT_DETECT                                                 0x16
#define HID_LED_OFF_HOOK                                                                0x17
#define HID_LED_RING                                                                    0x18
#define HID_LED_MESSAGE_WAITING                                                         0x19
#define HID_LED_DATA_MODE                                                               0x1a
#define HID_LED_BATTERY_OPERATION                                                       0x1b
#define HID_LED_BATTERY_OK                                                              0x1c
#define HID_LED_BATTERY_LOW                                                             0x1d
#define HID_LED_SPEAKER                                                                 0x1e
#define HID_LED_HEAD_SET                                                                0x1f
#define HID_LED_HOLD                                                                    0x20
#define HID_LED_MICROPHONE                                                              0x21
#define HID_LED_COVERAGE                                                                0x22
#define HID_LED_NIGHT_MODE                                                              0x23
#define HID_LED_SEND_CALLS                                                              0x24
#define HID_LED_CALL_PICKUP                                                             0x25
#define HID_LED_CONFERENCE                                                              0x26
#define HID_LED_STAND_BY                                                                0x27
#define HID_LED_CAMERA_ON                                                               0x28
#define HID_LED_CAMERA_OFF                                                              0x29
#define HID_LED_ON_LINE                                                                 0x2a
#define HID_LED_OFF_LINE                                                                0x2b
#define HID_LED_BUSY                                                                    0x2c
#define HID_LED_READY                                                                   0x2d
#define HID_LED_PAPER_OUT                                                               0x2e
#define HID_LED_PAPER_JAM                                                               0x2f
#define HID_LED_REMOTE                                                                  0x30
#define HID_LED_FORMAT                                                                  0x31
#define HID_LED_REVERSE                                                                 0x32
#define HID_LED_STOP                                                                    0x33
#define HID_LED_REWIND                                                                  0x34
#define HID_LED_FAST_FORWARD                                                            0x35
#define HID_LED_PLAY                                                                    0x36
#define HID_LED_PAUSE                                                                   0x37
#define HID_LED_RECORD                                                                  0x38
#define HID_LED_ERROR                                                                   0x39
#define HID_LED_USAGE_SELECTED_INDICATOR                                                0x3a
#define HID_LED_USAGE_IN_MODE_INDICATOR                                                 0x3b
#define HID_LED_USAGE_MODE_INDICATOR                                                    0x3c
#define HID_LED_INDICATOR_ON                                                            0x3d
#define HID_LED_INDICATOR_FLASH                                                         0x3e
#define HID_LED_INDICATOR_SLOW_BLINK                                                    0x3f
#define HID_LED_INDICATOR_FAST_BLINK                                                    0x40
#define HID_LED_INDICATOR_OFF                                                           0x41
#define HID_LED_FLASH_ON_TIME                                                           0x42
#define HID_LED_SLOW_BLINK_ON_TIME                                                      0x43
#define HID_LED_SLOW_BLINK_OFF_TIME                                                     0x44
#define HID_LED_FAST_BLINK_ON_TIME                                                      0x45
#define HID_LED_FAST_BLINK_OFF_TIME                                                     0x46
#define HID_LED_USAGE_INDICATOR_COLOR                                                   0x47
#define HID_LED_INDICATOR_RED                                                           0x48
#define HID_LED_INDICATOR_GREEN                                                         0x49
#define HID_LED_INDICATOR_AMBER                                                         0x4a
#define HID_LED_GENERIC_INDICATOR                                                       0x4b
#define HID_LED_SYSTEM_SUSPEND                                                          0x4c
#define HID_LED_EXTERNAL_POWER_CONNECTED                                                0x4d

#define HID_LEFT_CONTROL                                                                (1 << 0)
#define HID_LEFT_SHIFT                                                                  (1 << 1)
#define HID_LEFT_ALT                                                                    (1 << 2)
#define HID_LEFT_GUI                                                                    (1 << 3)
#define HID_RIGHT_CONTROL                                                               (1 << 0)
#define HID_RIGHT_SHIFT                                                                 (1 << 1)
#define HID_RIGHT_ALT                                                                   (1 << 2)
#define HID_RIGHT_GUI                                                                   (1 << 3)

typedef struct {
    uint8_t modifier;
    uint8_t reserved;
    uint8_t leds;
    uint8_t keys[6];
} BOOT_KEYBOARD;

#define HID_BOOT_KEYBOARD_REPORT_SIZE                                                   0x3f

typedef enum {
    //reserved for system use
    USB_HID_KBD_IDLE = IPC_USER,
    USB_HID_KBD_MODIFIER_CHANGE,
    USB_HID_KBD_KEY_PRESS,
    USB_HID_KBD_KEY_RELEASE,
    USB_HID_KBD_GET_LEDS_STATE,
    USB_HID_KBD_LEDS_STATE_CHANGED,
    USB_HID_KBD_MAX
} USB_HID_KBD_REQUESTS;


#pragma pack(pop)

#endif // HID_H
