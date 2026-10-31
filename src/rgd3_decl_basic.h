/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_decl_basic.h - decl types owned by the framework: table, entityDef /
   mapDef, skin, pda, email, video, audio (neo/framework/DeclTable.h,
   DeclEntityDef.h, DeclSkin.h, DeclPDA.h). Material, sound, fx, particle,
   af and model decls are registered here with the default parser until
   their modules exist. */
#ifndef RGD3_DECL_BASIC_H
#define RGD3_DECL_BASIC_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

typedef struct Rgd3DeclTable
{
	Rgd3DeclBody body;
	int clamp;
	int snap;
	float* values; /* the first value is repeated at the end for lerping */
	int num_values;
} Rgd3DeclTable;

typedef struct Rgd3DeclEntityDef
{
	Rgd3DeclBody body;
	Rgd3Dict dict; /* inherited values included */
} Rgd3DeclEntityDef;

typedef struct Rgd3SkinMapping
{
	const void* from; /* material body; NULL = wildcard */
	const void* to;
} Rgd3SkinMapping;

typedef struct Rgd3DeclSkin
{
	Rgd3DeclBody body;
	Rgd3SkinMapping* mappings;
	int num_mappings;
	char** associated_models; /* editor preview only */
	int num_associated_models;
} Rgd3DeclSkin;

typedef struct Rgd3DeclEmail
{
	Rgd3DeclBody body;
	char* text;
	char* subject;
	char* date;
	char* to;
	char* from;
} Rgd3DeclEmail;

typedef struct Rgd3DeclVideo
{
	Rgd3DeclBody body;
	char* video_name;
	char* info;
	const void* preview; /* material */
	const void* video;   /* material */
	const void* audio;   /* sound shader */
} Rgd3DeclVideo;

typedef struct Rgd3DeclAudio
{
	Rgd3DeclBody body;
	char* audio_name;
	char* info;
	const void* audio; /* sound shader */
} Rgd3DeclAudio;

typedef struct Rgd3DeclPDA
{
	Rgd3DeclBody body;
	const Rgd3DeclVideo** videos;
	int num_videos;
	const Rgd3DeclAudio** audios;
	int num_audios;
	const Rgd3DeclEmail** emails;
	int num_emails;
	int original_videos;
	int original_emails;
	char* pda_name;
	char* full_name;
	char* icon;
	char* id;
	char* post;
	char* title;
	char* security;
} Rgd3DeclPDA;

void rgd3_decl_basic_register(void);     /* cvars */
void rgd3_decl_register_basic_types(void); /* idDeclManagerLocal::Init types */
void rgd3_decl_register_game_types(void);  /* idGameLocal::Init types */

float rgd3_decl_table_lookup(const Rgd3DeclTable* table, float index);
const Rgd3DeclEntityDef* rgd3_decl_find_entity_def(const char* name, int make_default);

#endif
