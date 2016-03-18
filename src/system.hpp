#pragma once

#include "types.hpp"
#include "math.hpp"
#include "config.hpp"

#include <memory>
#include <functional>
#include <array>
#include <bitset>
#include <type_traits>
#include <vector>
#include <cstdint>

namespace boken {

//! From USB spec via SDL2
//! Physical keyboard scan codes
enum class kb_scancode : uint32_t {
    k_unknown = 0
  , k_a = 4
  , k_b = 5
  , k_c = 6
  , k_d = 7
  , k_e = 8
  , k_f = 9
  , k_g = 10
  , k_h = 11
  , k_i = 12
  , k_j = 13
  , k_k = 14
  , k_l = 15
  , k_m = 16
  , k_n = 17
  , k_o = 18
  , k_p = 19
  , k_q = 20
  , k_r = 21
  , k_s = 22
  , k_t = 23
  , k_u = 24
  , k_v = 25
  , k_w = 26
  , k_x = 27
  , k_y = 28
  , k_z = 29
  , k_1 = 30
  , k_2 = 31
  , k_3 = 32
  , k_4 = 33
  , k_5 = 34
  , k_6 = 35
  , k_7 = 36
  , k_8 = 37
  , k_9 = 38
  , k_0 = 39
  , k_return = 40
  , k_escape = 41
  , k_backspace = 42
  , k_tab = 43
  , k_space = 44
  , k_minus = 45
  , k_equals = 46
  , k_leftbracket = 47
  , k_rightbracket = 48
  , k_backslash = 49
  , k_nonushash = 50
  , k_semicolon = 51
  , k_apostrophe = 52
  , k_grave = 53
  , k_comma = 54
  , k_period = 55
  , k_slash = 56
  , k_capslock = 57
  , k_f1 = 58
  , k_f2 = 59
  , k_f3 = 60
  , k_f4 = 61
  , k_f5 = 62
  , k_f6 = 63
  , k_f7 = 64
  , k_f8 = 65
  , k_f9 = 66
  , k_f10 = 67
  , k_f11 = 68
  , k_f12 = 69
  , k_printscreen = 70
  , k_scrolllock = 71
  , k_pause = 72
  , k_insert = 73
  , k_home = 74
  , k_pageup = 75
  , k_delete = 76
  , k_end = 77
  , k_pagedown = 78
  , k_right = 79
  , k_left = 80
  , k_down = 81
  , k_up = 82
  , k_numlockclear = 83
  , k_kp_divide = 84
  , k_kp_multiply = 85
  , k_kp_minus = 86
  , k_kp_plus = 87
  , k_kp_enter = 88
  , k_kp_1 = 89
  , k_kp_2 = 90
  , k_kp_3 = 91
  , k_kp_4 = 92
  , k_kp_5 = 93
  , k_kp_6 = 94
  , k_kp_7 = 95
  , k_kp_8 = 96
  , k_kp_9 = 97
  , k_kp_0 = 98
  , k_kp_period = 99
  , k_nonusbackslash = 100
  , k_application = 101
  , k_power = 102
  , k_kp_equals = 103
  , k_f13 = 104
  , k_f14 = 105
  , k_f15 = 106
  , k_f16 = 107
  , k_f17 = 108
  , k_f18 = 109
  , k_f19 = 110
  , k_f20 = 111
  , k_f21 = 112
  , k_f22 = 113
  , k_f23 = 114
  , k_f24 = 115
  , k_execute = 116
  , k_help = 117
  , k_menu = 118
  , k_select = 119
  , k_stop = 120
  , k_again = 121
  , k_undo = 122
  , k_cut = 123
  , k_copy = 124
  , k_paste = 125
  , k_find = 126
  , k_mute = 127
  , k_volumeup = 128
  , k_volumedown = 129
  , k_lockingcapslock = 130
  , k_lockingnumlock = 131
  , k_lockingscrolllock = 132
  , k_kp_comma = 133
  , k_kp_equalsas400 = 134
  , k_international1 = 135
  , k_international2 = 136
  , k_international3 = 137
  , k_international4 = 138
  , k_international5 = 139
  , k_international6 = 140
  , k_international7 = 141
  , k_international8 = 142
  , k_international9 = 143
  , k_lang1 = 144
  , k_lang2 = 145
  , k_lang3 = 146
  , k_lang4 = 147
  , k_lang5 = 148
  , k_lang6 = 149
  , k_lang7 = 150
  , k_lang8 = 151
  , k_lang9 = 152
  , k_alterase = 153
  , k_sysreq = 154
  , k_cancel = 155
  , k_clear = 156
  , k_prior = 157
  , k_return2 = 158
  , k_separator = 159
  , k_out = 160
  , k_oper = 161
  , k_clearagain = 162
  , k_crsel = 163
  , k_exsel = 164
  , k_kp_00 = 176
  , k_kp_000 = 177
  , k_thousandsseparator = 178
  , k_decimalseparator = 179
  , k_currencyunit = 180
  , k_currencysubunit = 181
  , k_kp_leftparen = 182
  , k_kp_rightparen = 183
  , k_kp_leftbrace = 184
  , k_kp_rightbrace = 185
  , k_kp_tab = 186
  , k_kp_backspace = 187
  , k_kp_a = 188
  , k_kp_b = 189
  , k_kp_c = 190
  , k_kp_d = 191
  , k_kp_e = 192
  , k_kp_f = 193
  , k_kp_xor = 194
  , k_kp_power = 195
  , k_kp_percent = 196
  , k_kp_less = 197
  , k_kp_greater = 198
  , k_kp_ampersand = 199
  , k_kp_dblampersand = 200
  , k_kp_verticalbar = 201
  , k_kp_dblverticalbar = 202
  , k_kp_colon = 203
  , k_kp_hash = 204
  , k_kp_space = 205
  , k_kp_at = 206
  , k_kp_exclam = 207
  , k_kp_memstore = 208
  , k_kp_memrecall = 209
  , k_kp_memclear = 210
  , k_kp_memadd = 211
  , k_kp_memsubtract = 212
  , k_kp_memmultiply = 213
  , k_kp_memdivide = 214
  , k_kp_plusminus = 215
  , k_kp_clear = 216
  , k_kp_clearentry = 217
  , k_kp_binary = 218
  , k_kp_octal = 219
  , k_kp_decimal = 220
  , k_kp_hexadecimal = 221
  , k_lctrl = 224
  , k_lshift = 225
  , k_lalt = 226
  , k_lgui = 227
  , k_rctrl = 228
  , k_rshift = 229
  , k_ralt = 230
  , k_rgui = 231
  , k_mode = 257
  , k_audionext = 258
  , k_audioprev = 259
  , k_audiostop = 260
  , k_audioplay = 261
  , k_audiomute = 262
  , k_mediaselect = 263
  , k_www = 264
  , k_mail = 265
  , k_calculator = 266
  , k_computer = 267
  , k_ac_search = 268
  , k_ac_home = 269
  , k_ac_back = 270
  , k_ac_forward = 271
  , k_ac_stop = 272
  , k_ac_refresh = 273
  , k_ac_bookmarks = 274
  , k_brightnessdown = 275
  , k_brightnessup = 276
  , k_displayswitch = 277
  , k_kbdillumtoggle = 278
  , k_kbdillumdown = 279
  , k_kbdillumup = 280
  , k_eject = 281
  , k_sleep = 282
  , k_app1 = 283
  , k_app2 = 284
};

namespace detail {
inline constexpr uint32_t scancode_to_keycode(kb_scancode const k) noexcept {
    return (uint32_t {1} << 30)
         | static_cast<std::underlying_type_t<kb_scancode>>(k);
}
} //namespace detail

//! Based on the mapping that SDL2 uses for scancode <=> virtual key
//! Virtual key codes -- generally the character generated by a combination
//! of key presses.
enum class kb_keycode : uint32_t {
    k_unknown = 0
  , k_return = '\r'
  , k_escape = '\033'
  , k_backspace = '\b'
  , k_tab = '\t'
  , k_space = ' '
  , k_exclaim = '!'
  , k_quotedbl = '"'
  , k_hash = '#'
  , k_percent = '%'
  , k_dollar = '$'
  , k_ampersand = '&'
  , k_quote = '\''
  , k_leftparen = '('
  , k_rightparen = ')'
  , k_asterisk = '*'
  , k_plus = '+'
  , k_comma = ','
  , k_minus = '-'
  , k_period = '.'
  , k_slash = '/'
  , k_0 = '0'
  , k_1 = '1'
  , k_2 = '2'
  , k_3 = '3'
  , k_4 = '4'
  , k_5 = '5'
  , k_6 = '6'
  , k_7 = '7'
  , k_8 = '8'
  , k_9 = '9'
  , k_colon = ':'
  , k_semicolon = ';'
  , k_less = '<'
  , k_equals = '='
  , k_greater = '>'
  , k_question = '?'
  , k_at = '@'
  , k_leftbracket = '['
  , k_backslash = '\\'
  , k_rightbracket = ']'
  , k_caret = '^'
  , k_underscore = '_'
  , k_backquote = '`'
  , k_a = 'a'
  , k_b = 'b'
  , k_c = 'c'
  , k_d = 'd'
  , k_e = 'e'
  , k_f = 'f'
  , k_g = 'g'
  , k_h = 'h'
  , k_i = 'i'
  , k_j = 'j'
  , k_k = 'k'
  , k_l = 'l'
  , k_m = 'm'
  , k_n = 'n'
  , k_o = 'o'
  , k_p = 'p'
  , k_q = 'q'
  , k_r = 'r'
  , k_s = 's'
  , k_t = 't'
  , k_u = 'u'
  , k_v = 'v'
  , k_w = 'w'
  , k_x = 'x'
  , k_y = 'y'
  , k_z = 'z'
  , k_capslock = detail::scancode_to_keycode(kb_scancode::k_capslock)
  , k_f1 = detail::scancode_to_keycode(kb_scancode::k_f1)
  , k_f2 = detail::scancode_to_keycode(kb_scancode::k_f2)
  , k_f3 = detail::scancode_to_keycode(kb_scancode::k_f3)
  , k_f4 = detail::scancode_to_keycode(kb_scancode::k_f4)
  , k_f5 = detail::scancode_to_keycode(kb_scancode::k_f5)
  , k_f6 = detail::scancode_to_keycode(kb_scancode::k_f6)
  , k_f7 = detail::scancode_to_keycode(kb_scancode::k_f7)
  , k_f8 = detail::scancode_to_keycode(kb_scancode::k_f8)
  , k_f9 = detail::scancode_to_keycode(kb_scancode::k_f9)
  , k_f10 = detail::scancode_to_keycode(kb_scancode::k_f10)
  , k_f11 = detail::scancode_to_keycode(kb_scancode::k_f11)
  , k_f12 = detail::scancode_to_keycode(kb_scancode::k_f12)
  , k_printscreen = detail::scancode_to_keycode(kb_scancode::k_printscreen)
  , k_scrolllock = detail::scancode_to_keycode(kb_scancode::k_scrolllock)
  , k_pause = detail::scancode_to_keycode(kb_scancode::k_pause)
  , k_insert = detail::scancode_to_keycode(kb_scancode::k_insert)
  , k_home = detail::scancode_to_keycode(kb_scancode::k_home)
  , k_pageup = detail::scancode_to_keycode(kb_scancode::k_pageup)
  , k_delete = '\177'
  , k_end = detail::scancode_to_keycode(kb_scancode::k_end)
  , k_pagedown = detail::scancode_to_keycode(kb_scancode::k_pagedown)
  , k_right = detail::scancode_to_keycode(kb_scancode::k_right)
  , k_left = detail::scancode_to_keycode(kb_scancode::k_left)
  , k_down = detail::scancode_to_keycode(kb_scancode::k_down)
  , k_up = detail::scancode_to_keycode(kb_scancode::k_up)
  , k_numlockclear = detail::scancode_to_keycode(kb_scancode::k_numlockclear)
  , k_kp_divide = detail::scancode_to_keycode(kb_scancode::k_kp_divide)
  , k_kp_multiply = detail::scancode_to_keycode(kb_scancode::k_kp_multiply)
  , k_kp_minus = detail::scancode_to_keycode(kb_scancode::k_kp_minus)
  , k_kp_plus = detail::scancode_to_keycode(kb_scancode::k_kp_plus)
  , k_kp_enter = detail::scancode_to_keycode(kb_scancode::k_kp_enter)
  , k_kp_1 = detail::scancode_to_keycode(kb_scancode::k_kp_1)
  , k_kp_2 = detail::scancode_to_keycode(kb_scancode::k_kp_2)
  , k_kp_3 = detail::scancode_to_keycode(kb_scancode::k_kp_3)
  , k_kp_4 = detail::scancode_to_keycode(kb_scancode::k_kp_4)
  , k_kp_5 = detail::scancode_to_keycode(kb_scancode::k_kp_5)
  , k_kp_6 = detail::scancode_to_keycode(kb_scancode::k_kp_6)
  , k_kp_7 = detail::scancode_to_keycode(kb_scancode::k_kp_7)
  , k_kp_8 = detail::scancode_to_keycode(kb_scancode::k_kp_8)
  , k_kp_9 = detail::scancode_to_keycode(kb_scancode::k_kp_9)
  , k_kp_0 = detail::scancode_to_keycode(kb_scancode::k_kp_0)
  , k_kp_period = detail::scancode_to_keycode(kb_scancode::k_kp_period)
  , k_application = detail::scancode_to_keycode(kb_scancode::k_application)
  , k_power = detail::scancode_to_keycode(kb_scancode::k_power)
  , k_kp_equals = detail::scancode_to_keycode(kb_scancode::k_kp_equals)
  , k_f13 = detail::scancode_to_keycode(kb_scancode::k_f13)
  , k_f14 = detail::scancode_to_keycode(kb_scancode::k_f14)
  , k_f15 = detail::scancode_to_keycode(kb_scancode::k_f15)
  , k_f16 = detail::scancode_to_keycode(kb_scancode::k_f16)
  , k_f17 = detail::scancode_to_keycode(kb_scancode::k_f17)
  , k_f18 = detail::scancode_to_keycode(kb_scancode::k_f18)
  , k_f19 = detail::scancode_to_keycode(kb_scancode::k_f19)
  , k_f20 = detail::scancode_to_keycode(kb_scancode::k_f20)
  , k_f21 = detail::scancode_to_keycode(kb_scancode::k_f21)
  , k_f22 = detail::scancode_to_keycode(kb_scancode::k_f22)
  , k_f23 = detail::scancode_to_keycode(kb_scancode::k_f23)
  , k_f24 = detail::scancode_to_keycode(kb_scancode::k_f24)
  , k_execute = detail::scancode_to_keycode(kb_scancode::k_execute)
  , k_help = detail::scancode_to_keycode(kb_scancode::k_help)
  , k_menu = detail::scancode_to_keycode(kb_scancode::k_menu)
  , k_select = detail::scancode_to_keycode(kb_scancode::k_select)
  , k_stop = detail::scancode_to_keycode(kb_scancode::k_stop)
  , k_again = detail::scancode_to_keycode(kb_scancode::k_again)
  , k_undo = detail::scancode_to_keycode(kb_scancode::k_undo)
  , k_cut = detail::scancode_to_keycode(kb_scancode::k_cut)
  , k_copy = detail::scancode_to_keycode(kb_scancode::k_copy)
  , k_paste = detail::scancode_to_keycode(kb_scancode::k_paste)
  , k_find = detail::scancode_to_keycode(kb_scancode::k_find)
  , k_mute = detail::scancode_to_keycode(kb_scancode::k_mute)
  , k_volumeup = detail::scancode_to_keycode(kb_scancode::k_volumeup)
  , k_volumedown = detail::scancode_to_keycode(kb_scancode::k_volumedown)
  , k_kp_comma = detail::scancode_to_keycode(kb_scancode::k_kp_comma)
  , k_kp_equalsas400 = detail::scancode_to_keycode(kb_scancode::k_kp_equalsas400)
  , k_alterase = detail::scancode_to_keycode(kb_scancode::k_alterase)
  , k_sysreq = detail::scancode_to_keycode(kb_scancode::k_sysreq)
  , k_cancel = detail::scancode_to_keycode(kb_scancode::k_cancel)
  , k_clear = detail::scancode_to_keycode(kb_scancode::k_clear)
  , k_prior = detail::scancode_to_keycode(kb_scancode::k_prior)
  , k_return2 = detail::scancode_to_keycode(kb_scancode::k_return2)
  , k_separator = detail::scancode_to_keycode(kb_scancode::k_separator)
  , k_out = detail::scancode_to_keycode(kb_scancode::k_out)
  , k_oper = detail::scancode_to_keycode(kb_scancode::k_oper)
  , k_clearagain = detail::scancode_to_keycode(kb_scancode::k_clearagain)
  , k_crsel = detail::scancode_to_keycode(kb_scancode::k_crsel)
  , k_exsel = detail::scancode_to_keycode(kb_scancode::k_exsel)
  , k_kp_00 = detail::scancode_to_keycode(kb_scancode::k_kp_00)
  , k_kp_000 = detail::scancode_to_keycode(kb_scancode::k_kp_000)
  , k_thousandsseparator = detail::scancode_to_keycode(kb_scancode::k_thousandsseparator)
  , k_decimalseparator = detail::scancode_to_keycode(kb_scancode::k_decimalseparator)
  , k_currencyunit = detail::scancode_to_keycode(kb_scancode::k_currencyunit)
  , k_currencysubunit = detail::scancode_to_keycode(kb_scancode::k_currencysubunit)
  , k_kp_leftparen = detail::scancode_to_keycode(kb_scancode::k_kp_leftparen)
  , k_kp_rightparen = detail::scancode_to_keycode(kb_scancode::k_kp_rightparen)
  , k_kp_leftbrace = detail::scancode_to_keycode(kb_scancode::k_kp_leftbrace)
  , k_kp_rightbrace = detail::scancode_to_keycode(kb_scancode::k_kp_rightbrace)
  , k_kp_tab = detail::scancode_to_keycode(kb_scancode::k_kp_tab)
  , k_kp_backspace = detail::scancode_to_keycode(kb_scancode::k_kp_backspace)
  , k_kp_a = detail::scancode_to_keycode(kb_scancode::k_kp_a)
  , k_kp_b = detail::scancode_to_keycode(kb_scancode::k_kp_b)
  , k_kp_c = detail::scancode_to_keycode(kb_scancode::k_kp_c)
  , k_kp_d = detail::scancode_to_keycode(kb_scancode::k_kp_d)
  , k_kp_e = detail::scancode_to_keycode(kb_scancode::k_kp_e)
  , k_kp_f = detail::scancode_to_keycode(kb_scancode::k_kp_f)
  , k_kp_xor = detail::scancode_to_keycode(kb_scancode::k_kp_xor)
  , k_kp_power = detail::scancode_to_keycode(kb_scancode::k_kp_power)
  , k_kp_percent = detail::scancode_to_keycode(kb_scancode::k_kp_percent)
  , k_kp_less = detail::scancode_to_keycode(kb_scancode::k_kp_less)
  , k_kp_greater = detail::scancode_to_keycode(kb_scancode::k_kp_greater)
  , k_kp_ampersand = detail::scancode_to_keycode(kb_scancode::k_kp_ampersand)
  , k_kp_dblampersand = detail::scancode_to_keycode(kb_scancode::k_kp_dblampersand)
  , k_kp_verticalbar = detail::scancode_to_keycode(kb_scancode::k_kp_verticalbar)
  , k_kp_dblverticalbar = detail::scancode_to_keycode(kb_scancode::k_kp_dblverticalbar)
  , k_kp_colon = detail::scancode_to_keycode(kb_scancode::k_kp_colon)
  , k_kp_hash = detail::scancode_to_keycode(kb_scancode::k_kp_hash)
  , k_kp_space = detail::scancode_to_keycode(kb_scancode::k_kp_space)
  , k_kp_at = detail::scancode_to_keycode(kb_scancode::k_kp_at)
  , k_kp_exclam = detail::scancode_to_keycode(kb_scancode::k_kp_exclam)
  , k_kp_memstore = detail::scancode_to_keycode(kb_scancode::k_kp_memstore)
  , k_kp_memrecall = detail::scancode_to_keycode(kb_scancode::k_kp_memrecall)
  , k_kp_memclear = detail::scancode_to_keycode(kb_scancode::k_kp_memclear)
  , k_kp_memadd = detail::scancode_to_keycode(kb_scancode::k_kp_memadd)
  , k_kp_memsubtract = detail::scancode_to_keycode(kb_scancode::k_kp_memsubtract)
  , k_kp_memmultiply = detail::scancode_to_keycode(kb_scancode::k_kp_memmultiply)
  , k_kp_memdivide = detail::scancode_to_keycode(kb_scancode::k_kp_memdivide)
  , k_kp_plusminus = detail::scancode_to_keycode(kb_scancode::k_kp_plusminus)
  , k_kp_clear = detail::scancode_to_keycode(kb_scancode::k_kp_clear)
  , k_kp_clearentry = detail::scancode_to_keycode(kb_scancode::k_kp_clearentry)
  , k_kp_binary = detail::scancode_to_keycode(kb_scancode::k_kp_binary)
  , k_kp_octal = detail::scancode_to_keycode(kb_scancode::k_kp_octal)
  , k_kp_decimal = detail::scancode_to_keycode(kb_scancode::k_kp_decimal)
  , k_kp_hexadecimal = detail::scancode_to_keycode(kb_scancode::k_kp_hexadecimal)
  , k_lctrl = detail::scancode_to_keycode(kb_scancode::k_lctrl)
  , k_lshift = detail::scancode_to_keycode(kb_scancode::k_lshift)
  , k_lalt = detail::scancode_to_keycode(kb_scancode::k_lalt)
  , k_lgui = detail::scancode_to_keycode(kb_scancode::k_lgui)
  , k_rctrl = detail::scancode_to_keycode(kb_scancode::k_rctrl)
  , k_rshift = detail::scancode_to_keycode(kb_scancode::k_rshift)
  , k_ralt = detail::scancode_to_keycode(kb_scancode::k_ralt)
  , k_rgui = detail::scancode_to_keycode(kb_scancode::k_rgui)
  , k_mode = detail::scancode_to_keycode(kb_scancode::k_mode)
  , k_audionext = detail::scancode_to_keycode(kb_scancode::k_audionext)
  , k_audioprev = detail::scancode_to_keycode(kb_scancode::k_audioprev)
  , k_audiostop = detail::scancode_to_keycode(kb_scancode::k_audiostop)
  , k_audioplay = detail::scancode_to_keycode(kb_scancode::k_audioplay)
  , k_audiomute = detail::scancode_to_keycode(kb_scancode::k_audiomute)
  , k_mediaselect = detail::scancode_to_keycode(kb_scancode::k_mediaselect)
  , k_www = detail::scancode_to_keycode(kb_scancode::k_www)
  , k_mail = detail::scancode_to_keycode(kb_scancode::k_mail)
  , k_calculator = detail::scancode_to_keycode(kb_scancode::k_calculator)
  , k_computer = detail::scancode_to_keycode(kb_scancode::k_computer)
  , k_ac_search = detail::scancode_to_keycode(kb_scancode::k_ac_search)
  , k_ac_home = detail::scancode_to_keycode(kb_scancode::k_ac_home)
  , k_ac_back = detail::scancode_to_keycode(kb_scancode::k_ac_back)
  , k_ac_forward = detail::scancode_to_keycode(kb_scancode::k_ac_forward)
  , k_ac_stop = detail::scancode_to_keycode(kb_scancode::k_ac_stop)
  , k_ac_refresh = detail::scancode_to_keycode(kb_scancode::k_ac_refresh)
  , k_ac_bookmarks = detail::scancode_to_keycode(kb_scancode::k_ac_bookmarks)
  , k_brightnessdown = detail::scancode_to_keycode(kb_scancode::k_brightnessdown)
  , k_brightnessup = detail::scancode_to_keycode(kb_scancode::k_brightnessup)
  , k_displayswitch = detail::scancode_to_keycode(kb_scancode::k_displayswitch)
  , k_kbdillumtoggle = detail::scancode_to_keycode(kb_scancode::k_kbdillumtoggle)
  , k_kbdillumdown = detail::scancode_to_keycode(kb_scancode::k_kbdillumdown)
  , k_kbdillumup = detail::scancode_to_keycode(kb_scancode::k_kbdillumup)
  , k_eject = detail::scancode_to_keycode(kb_scancode::k_eject)
  , k_sleep = detail::scancode_to_keycode(kb_scancode::k_sleep)
};

struct kb_modifiers {
    template <size_t Bit>
    using flag_t = std::integral_constant<size_t, Bit>;

    template <size_t... Bits>
    using flags_t = std::integer_sequence<size_t, Bits...>;

    static constexpr flag_t<1>  m_left_shift  = flag_t<1> {};
    static constexpr flag_t<2>  m_right_shift  = flag_t<2> {};
    static constexpr flag_t<7>  m_left_ctrl  = flag_t<7> {};
    static constexpr flag_t<8>  m_right_ctrl  = flag_t<8> {};
    static constexpr flag_t<9>  m_left_alt  = flag_t<9> {};
    static constexpr flag_t<10> m_right_alt  = flag_t<10> {};
    static constexpr flag_t<11>  m_left_gui  = flag_t<11> {};
    static constexpr flag_t<12>  m_right_gui  = flag_t<12> {};
    static constexpr flag_t<13>  m_num  = flag_t<13> {};
    static constexpr flag_t<14> m_caps  = flag_t<14> {};
    static constexpr flag_t<15> m_mode  = flag_t<15> {};

    static constexpr flags_t<m_left_shift, m_right_shift> m_shift  = flags_t<m_left_shift, m_right_shift> {};
    static constexpr flags_t<m_left_ctrl, m_right_ctrl> m_ctrl  = flags_t<m_left_ctrl, m_right_ctrl> {};
    static constexpr flags_t<m_left_alt, m_right_alt> m_alt  = flags_t<m_left_alt, m_right_alt> {};
    static constexpr flags_t<m_left_gui, m_right_gui> m_gui  = flags_t<m_left_gui, m_right_gui> {};

    constexpr explicit kb_modifiers(uint32_t const n) noexcept
      : bits_ {n}
    {
    }

    bool none() const noexcept {
        return bits_.none();
    }

    template <size_t Bit>
    constexpr bool test(flag_t<Bit>) const noexcept {
        return bits_.test(Bit - 1);
    }

    template <size_t Bit0, size_t Bit1>
    constexpr bool test(flags_t<Bit0, Bit1>) const noexcept {
        return bits_.test(Bit0 - 1) || bits_.test(Bit1 - 1);
    }

    std::bitset<16> bits_;
};

struct kb_event {
    uint32_t    timestamp;
    kb_scancode scancode;
    kb_keycode  keycode;
    uint16_t    mods;
    bool        is_repeat;
    bool        went_down;
};

struct text_input_event {
    uint32_t timestamp;
    string_view text;
};

struct read_only_pointer_t {
    read_only_pointer_t() noexcept = default;

    template <typename T>
    read_only_pointer_t(
        T const* const beg
      , T const* const end
      , size_t   const stride = sizeof(T)
    ) noexcept
      : ptr            {static_cast<void const*>(beg)}
      , last           {static_cast<void const*>(end)}
      , element_size   {sizeof(T)}
      , element_stride {static_cast<uint16_t>(stride)}
    {
    }

    template <typename T>
    explicit read_only_pointer_t(std::vector<T> const& v, size_t const offset = 0, size_t const stride = sizeof(T)) noexcept
      : read_only_pointer_t {
            reinterpret_cast<T const*>(
                reinterpret_cast<char const*>(
                    reinterpret_cast<void const*>(v.data())
                ) + offset)
          , v.data() + v.size()
          , stride}
    {
    }

    read_only_pointer_t& operator++() noexcept {
        ptr = (ptr == last) ? ptr
          : reinterpret_cast<char const*>(ptr) + element_stride;
        return *this;
    }

    read_only_pointer_t operator++(int) noexcept {
        auto result = *this;
        ++(*this);
        return result;
    }

    template <typename T>
    T const& value() const noexcept {
        return *reinterpret_cast<T const*>(ptr);
    }

    explicit operator bool() const noexcept {
        return !!ptr;
    }

    void const* ptr            {};
    void const* last           {};
    uint16_t    element_size   {};
    uint16_t    element_stride {};
};

enum class render_data_type {
    position // x, y
  , texture  // x, y
  , color    // rgba
};

struct mouse_event {
    static constexpr size_t button_count = 4;

    enum class button_change_t : uint8_t {
        none, went_up, went_down
    };

    int x;
    int y;
    int dx;
    int dy;

    uint32_t button_state_bits() const noexcept {
        uint32_t result {};
        for (size_t i = 0; i < button_count; ++i) {
            result |= (button_state[i] ? 1u : 0u) << i;
        }
        return result;
    }

    std::array<button_change_t, button_count> button_change;
    std::array<bool,            button_count> button_state;
};

class system {
public:
    virtual ~system();

    using on_request_quit_handler = std::function<bool ()>;
    virtual void on_request_quit(on_request_quit_handler handler) = 0;

    using on_key_handler = std::function<void (kb_event, kb_modifiers)>;
    virtual void on_key(on_key_handler handler) = 0;

    using on_mouse_move_handler = std::function<void (mouse_event, kb_modifiers)>;
    virtual void on_mouse_move(on_mouse_move_handler handler) = 0;

    using on_mouse_button_handler = std::function<void (mouse_event, kb_modifiers)>;
    virtual void on_mouse_button(on_mouse_button_handler handler) = 0;

    using on_mouse_wheel_handler = std::function<void (int wy, int wx, kb_modifiers)>;
    virtual void on_mouse_wheel(on_mouse_wheel_handler handler) = 0;

    using on_text_input_handler = std::function<void (text_input_event)>;
    virtual void on_text_input(on_text_input_handler handler) = 0;

    virtual bool is_running() = 0;
    virtual int do_events() = 0;

    virtual recti render_get_client_rect() const = 0;

    virtual void render_clear()   = 0;
    virtual void render_present() = 0;

    virtual void render_fill_rect(recti r, uint32_t color) = 0;

    virtual void render_background() = 0;

    virtual void render_set_tile_size(sizeix w, sizeiy h) = 0;
    virtual void render_set_transform(float sx, float sy, float tx, float ty) = 0;

    virtual void render_set_texture(uint32_t id) = 0;
    virtual void render_set_data(render_data_type type, read_only_pointer_t data) = 0;
    virtual void render_data_n(size_t n) = 0;
};

std::unique_ptr<system> make_system();

} //namespace boken
