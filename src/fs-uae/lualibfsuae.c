#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "fs-uae.h"
#include <fs/emu.h>
#include <fs/log.h>
#include <uae/uae.h>

#ifdef WITH_LUA

static int l_fs_uae_get_save_state_number(lua_State *L) {
    lua_pushinteger(L, g_fs_uae_state_number);
    return 1;
}

static int l_fs_uae_get_input_event(lua_State *L) {
    lua_pushinteger(L, g_fs_uae_last_input_event);
    lua_pushinteger(L, g_fs_uae_last_input_event_state);
    return 2;
}

static int l_fs_uae_set_input_event(lua_State *L) {
    int input_event = luaL_checkint(L, -2);
    int state = luaL_checkint(L, -1);
    g_fs_uae_last_input_event = input_event;
    g_fs_uae_last_input_event_state = state;
    return 0;
}

static int l_fs_uae_send_input_event(lua_State *L) {
    int input_event = luaL_checkint(L, -2);
    int state = luaL_checkint(L, -1);
/*
    fs_uae_process_input_event(input_event, state);
*/
    return 0;
}

static int l_fs_uae_get_state_checksum(lua_State *L) {
    lua_pushinteger(L, amiga_get_state_checksum());
    return 1;
}

static int l_fs_uae_get_rand_checksum(lua_State *L) {
    lua_pushinteger(L, amiga_get_state_checksum());
    return 1;
}

static int l_floppy_set_file(lua_State *L)
{
    int index = luaL_checkint(L, -2);
    const char *name = luaL_checkstring(L, -1);
    amiga_floppy_set_file(index, name);
    return 0;
}

static int l_cdrom_set_file(lua_State *L)
{
    int index = luaL_checkint(L, -2);
    const char *name = luaL_checkstring(L, -1);
    amiga_cdrom_set_file(index, name);
    return 0;
}

static const struct luaL_Reg fsuaelib[] = {
    {"get_input_event", l_fs_uae_get_input_event},
    {"set_input_event", l_fs_uae_set_input_event},
    {"send_input_event", l_fs_uae_send_input_event},
    {"get_save_state_number", l_fs_uae_get_save_state_number},
    {"get_state_checksum", l_fs_uae_get_state_checksum},
    {"get_rand_checksum", l_fs_uae_get_rand_checksum},
    {"floppy_set_file", l_floppy_set_file},
    {"cdrom_set_file", l_cdrom_set_file},
    {NULL, NULL}
};

int luaopen_fsuaelib(lua_State *L)
{
    fs_log("lua: open fsuaelib for state %p\n", L);
    lua_newtable(L);
    luaL_setfuncs(L, fsuaelib, 0);
    // top is now uaelib table
    return 1;
}

#endif
