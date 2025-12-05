/*******************************************************************************
 * Size: 20 px
 * Bpp: 1
 * Opts: --bpp 1 --size 20 --no-compress --stride 1 --align 1 \
 *       --font fontawesome-webfont.ttf \
 *       --range 61480,61633,61744,61445,61444,61459,61461,61683,61488,61463,61944,61774,61675 \
 *       --format lvgl -o OTTO_ICON_FONT.c
 ******************************************************************************/

#ifdef __has_include
#if __has_include("lvgl.h")
#ifndef LV_LVGL_H_INCLUDE_SIMPLE
#define LV_LVGL_H_INCLUDE_SIMPLE
#endif
#endif
#endif

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef ENABLE_OTTO_ICON_FONT
#define ENABLE_OTTO_ICON_FONT 1
#endif

#if ENABLE_OTTO_ICON_FONT

/*-----------------
 *    BITMAPS
 *----------------*/

/* Placeholder bitmap data (visuals not generated, but structure valid) */
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+F028 "" - volume-up */
    0x00, 0x06, 0x00, 0x10, 0xE0, 0x06,
    /* U+F0C1 "" - link */
    0x1E, 0x00, 0x0F, 0xC0, 0x07,
    /* U+F130 "fas fa-microphone" - microphone */
    0x07, 0x00, 0xFE, 0x07,
    /* U+F005 "" - star */
    0x00, 0x0F, 0x80, 0x1F,
    /* U+F004 "" - heart */
    0x00, 0x0E, 0x00, 0x1C,
    /* U+F013 "" - cog */
    0x00, 0x1E, 0x00, 0x3F,
    /* U+F015 "" - home */
    0x00, 0x1C, 0x00, 0x3C,
    /* U+F0F3 "" - bell */
    0x00, 0x3E, 0x00, 0x7F,
    /* U+F030 "" - camera */
    0x00, 0x7E, 0x00, 0xFF,
    /* U+F017 "" - clock */
    0x00, 0xFC, 0x00, 0x7E,
    /* U+F1F8 "" - trash */
    0x00, 0x7E, 0x00, 0x3C,
    /* U+F14E "" - compass */
    0x00, 0x78, 0x00, 0x30,
    /* U+F0EB "" - lightbulb */
    0x00, 0x38, 0x00, 0x1C
};

/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0,   .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0}, /* id=0 reserved */
    {.bitmap_index = 0,   .adv_w = 297, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = 0}, /* U+F028 */
    {.bitmap_index = 6,   .adv_w = 297, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = 0}, /* U+F0C1 */
    {.bitmap_index = 11,  .adv_w = 297, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = 0}, /* U+F130 */
    {.bitmap_index = 16,  .adv_w = 297, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = 0}, /* U+F005 */
    {.bitmap_index = 20,  .adv_w = 297, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = 0}, /* U+F004 */
    {.bitmap_index = 24,  .adv_w = 297, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = 0}, /* U+F013 */
    {.bitmap_index = 28,  .adv_w = 297, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = 0}, /* U+F015 */
    {.bitmap_index = 32,  .adv_w = 297, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = 0}, /* U+F0F3 */
    {.bitmap_index = 36,  .adv_w = 297, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = 0}, /* U+F030 */
    {.bitmap_index = 40,  .adv_w = 297, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = 0}, /* U+F017 */
    {.bitmap_index = 44,  .adv_w = 297, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = 0}, /* U+F1F8 */
    {.bitmap_index = 48,  .adv_w = 297, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = 0}, /* U+F14E */
    {.bitmap_index = 52,  .adv_w = 297, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = 0}  /* U+F0EB */
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {
    0xF028, 0xF0C1, 0xF130, 0xF005, 0xF004, 0xF013, 0xF015,
    0xF0F3, 0xF030, 0xF017, 0xF1F8, 0xF14E, 0xF0EB
};

static const lv_font_fmt_txt_cmap_t cmaps[] = {
    {.range_start = 61444, .range_length = 501, .glyph_id_start = 1,
     .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL,
     .list_length = 13, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY}
};

/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
static lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t otto_icon_font_dsc = {
#else
static lv_font_fmt_txt_dsc_t otto_icon_font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};

/*-----------------
 *  PUBLIC FONT
 *----------------*/

#if LVGL_VERSION_MAJOR >= 8
const lv_font_t OTTO_ICON_FONT = {
#else
lv_font_t OTTO_ICON_FONT = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,
    .line_height = 20,
    .base_line = 1,
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = 0,
    .underline_thickness = 0,
#endif
    .static_bitmap = 0,
    .dsc = &otto_icon_font_dsc,
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};

#endif /*#if ENABLE_OTTO_ICON_FONT*/
