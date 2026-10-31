/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_registry.c - the one list of module registration functions. It
   replaces the original static-constructor registration of idCVar,
   idCommandLink, idEventDef and idTypeInfo. Order matters only where a
   module's commands read another module's cvars at registration time. */

void rgd3_registry_register_all(void)
{
	rgd3_cvar_register_commands();
	rgd3_cmd_register_builtins();
	rgd3_log_register();
	rgd3_error_register();
	rgd3_mem_register();
	rgd3_fs_register();
	rgd3_keys_register();
	rgd3_usercmd_register();
	rgd3_dict_register();
	rgd3_lang_register();
	rgd3_decl_register();
	rgd3_decl_basic_register();
	rgd3_manifest_register();
	rgd3_mapfile_register();
	rgd3_savegame_register();
	rgd3_jobs_register();
	rgd3_hooks_register();
	rgd3_session_register();
	rgd3_common_register();
	rgd3_console_register();
	rgd3_screenshot_register();
	rgd3_app_register();
}
