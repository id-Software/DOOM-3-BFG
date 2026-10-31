/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_lang.h - the localized string table (idLangDict / idLocalization,
   neo/idlib/LangDict.cpp; loading as idCommonLocal::InitLanguageDict,
   neo/framework/Common.cpp). Keys look like "#str_02990"; a value that is
   itself a key redirects, at most two levels deep. */
#ifndef RGD3_LANG_H
#define RGD3_LANG_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

#define RGD3_LANG_KEY_PREFIX "#str_"
#define RGD3_LANG_MAX_REDIRECTION_DEPTH 2

void rgd3_lang_register(void);
void rgd3_lang_init(void); /* loads strings/<sys_lang>*.lang */
void rgd3_lang_shutdown(void);
void rgd3_lang_clear(void);
int rgd3_lang_load(const u8* buffer, int length, const char* name);

const char* rgd3_lang_get_string(const char* str);  /* the string itself when not found */
const char* rgd3_lang_find_string(const char* str); /* NULL when not found */
int rgd3_lang_find_string_index(const char* str);
int rgd3_lang_is_string_id(const char* str);
int rgd3_lang_num_strings(void);

#endif
