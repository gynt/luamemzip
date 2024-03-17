
#include "pch.h"
#include "luamemzip.h"

#include <string>

#include <zip.h>
#include <corecrt_malloc.h>

#include <filesystem>

#define LUAMEMZIP_ZIP_STREAM_METATABLE_NAME "luamemzipstream"
#define LUAMEMZIP_ZIP_FILE_METATABLE_NAME "luamemzip"
#define LUAMEMZIP_ERROR_CLOSED_ZIP "invalid zip handle, cannot operate on a closed zip file"

bool checkMode(const std::string &mode, char &modeResult, std::string &errorMsg) {
	if (mode.empty() || mode.length() == 0) {
		errorMsg = "invalid 'mode': NULL";
		return false; 
	}
	else if (mode != "a" && mode != "w" && mode != "r" && mode != "d") {
		errorMsg = "invalid 'mode': %s" + std::string(mode);
		return false; 
	}

	modeResult = mode[0];
	return true;
}

struct LuaZip {
	struct zip_t* handle;
	char mode;
};

struct LuaZip* lua_check_luazip(lua_State* L, int arg) {
	struct LuaZip* lz = (LuaZip*)luaL_checkudata(L, arg, LUAMEMZIP_ZIP_STREAM_METATABLE_NAME);
	luaL_argcheck(L, lz->handle != 0, arg, LUAMEMZIP_ERROR_CLOSED_ZIP);

	return lz;
}

int lua_zip_stream_open(lua_State* L) {

	size_t size = 0;
	const char* stream = NULL;
	lua_Integer level = 0;
	std::string modeString = "w";

	if (lua_gettop(L) > 0) {
		size = 0;
		if (!lua_isnil(L, 1)) {
			stream = luaL_checklstring(L, 1, &size);
			modeString = "r";
		}

		if (!lua_isnil(L, 2))
			level = luaL_checkinteger(L, 2);

		if (!lua_isnil(L, 3)) {
			modeString = luaL_checkstring(L, 3);
		}
			
	}

	char mode;
	std::string modeError;
	if(!checkMode(modeString, mode, modeError)) return luaL_error(L, modeError.c_str());

	struct LuaZip *lz = (LuaZip *) lua_newuserdata(L, sizeof(LuaZip));

	luaL_getmetatable(L, LUAMEMZIP_ZIP_STREAM_METATABLE_NAME);
	lua_setmetatable(L, -2);

	int err = 0;

	lz->handle = zip_stream_openwitherror(stream, size, level, mode, &err);
	lz->mode = mode;

	if (lz->handle == NULL) {
		return luaL_error(L, ("failed to execute lua_zip_stream_open(): " + std::string(zip_strerror(err))).c_str());
	}

	return 1;
}

int lua_zip_file_open(lua_State* L) {
	const char* zipname = luaL_checkstring(L, 1);
	lua_Integer level = luaL_checkinteger(L, 2);
	const char* modeString = luaL_checkstring(L, 3);

	char mode = 'r';
	std::string modeError;
	if (!checkMode(modeString, mode, modeError)) return luaL_error(L, modeError.c_str());

	struct LuaZip* lz = (LuaZip*)lua_newuserdata(L, sizeof(LuaZip));

	luaL_getmetatable(L, LUAMEMZIP_ZIP_FILE_METATABLE_NAME);
	lua_setmetatable(L, -2);

	int err = 0;

	lz->handle = zip_openwitherror(zipname, level, mode, &err);
	lz->mode = mode;

	if (lz->handle == NULL) {
		return luaL_error(L, ("failed to execute zip_open(): " + std::string(zip_strerror(err))).c_str());
	}

	return 1;
}

int lua_zip_entry_open(lua_State* L) {
	struct LuaZip* lz = lua_check_luazip(L, 1);
	const char* entryname = luaL_checkstring(L, 2);
	int status = zip_entry_open(lz->handle, entryname);

	if (status == 0) {
		lua_pushboolean(L, true);

		return 1;
	}

	lua_pushboolean(L, false);
	lua_pushinteger(L, status);
	lua_pushstring(L, zip_strerror(status));

	return 3;
}


int lua_zip_entry_write(lua_State* L) {
	struct LuaZip* lz = lua_check_luazip(L, 1);
	size_t bufsize = 0;
	const char* buf = luaL_checklstring(L, 2, &bufsize);
	int status = zip_entry_write(lz->handle, buf, bufsize);

	if (status == 0) {
		lua_pushboolean(L, true);

		return 1;
	}

	lua_pushboolean(L, false);
	lua_pushinteger(L, status);
	lua_pushstring(L, zip_strerror(status));

	return 3;
}

int lua_zip_entry_close(lua_State* L) {
	struct LuaZip* lz = lua_check_luazip(L, 1);
	int status = zip_entry_close(lz->handle);

	if (status == 0) {
		lua_pushboolean(L, true);

		return 1;
	}

	lua_pushboolean(L, false);
	lua_pushinteger(L, status);
	lua_pushstring(L, zip_strerror(status));

	return 3;
}

int lua_zip_entry_read(lua_State* L) {
	struct LuaZip* lz = lua_check_luazip(L, 1);

	char* buf = NULL;
	size_t bufsize = 0;

	int status = zip_entry_read(lz->handle, (void**) & buf, &bufsize);

	if (status < 0) {
		lua_pushnil(L);
		lua_pushinteger(L, status);
		lua_pushstring(L, zip_strerror(status));

		return 3;
	}

	lua_pushlstring(L, buf, bufsize);
	lua_pushinteger(L, status);

	free(buf);

	return 2;
}

int lua_zip_stream_copy(lua_State* L) {
	struct LuaZip* lz = lua_check_luazip(L, 1);

	char* buf = NULL;
	size_t bufsize = 0;

	int status = zip_stream_copy(lz->handle, (void**)&buf, &bufsize);

	if (status < 0) {
		lua_pushnil(L);
		lua_pushinteger(L, status);
		lua_pushstring(L, zip_strerror(status));

		return 3;
	}

	lua_pushlstring(L, buf, bufsize);
	lua_pushinteger(L, status);

	free(buf);

	return 2;
}

int lua_zip_file_close(lua_State* L) {
	struct LuaZip* lz = lua_check_luazip(L, 1);

	zip_close(lz->handle);

	lz->handle = 0;

	return 0;
}

int lua_zip_stream_close(lua_State* L) {
	struct LuaZip* lz = lua_check_luazip(L, 1);

	zip_stream_close(lz->handle);

	lz->handle = 0;

	return 0;
}

int lua_zip_entry_exists(lua_State* L) { 
	struct LuaZip* lz = lua_check_luazip(L, 1);
	const char* entryname = luaL_checkstring(L, 2);
	int status = zip_entry_open(lz->handle, entryname);

	if (status == 0) {
		lua_pushboolean(L, true);

		zip_entry_close(lz->handle);

		return 1;
	}
	else if (status == ZIP_ENOENT) {
		lua_pushboolean(L, false);

		return 1;
	}

	lua_pushnil(L);
	lua_pushinteger(L, status);
	lua_pushstring(L, zip_strerror(status));

	return 3;
}

int lua_zip_list_entries(lua_State* L) {
	struct LuaZip* lz = lua_check_luazip(L, 1);
	const char* entryname = ".";

	if (!lua_isnil(L, 2)) {
		entryname = luaL_checkstring(L, 2);
	}

	lua_createtable(L, 0, 0);

	std::string path = entryname;

	int count = 1;

	std::filesystem::path haystack = std::filesystem::path(path);

	int i, n = zip_entries_total(lz->handle);
	for (i = 0; i < n; ++i) {
		zip_entry_openbyindex(lz->handle, i);
		{
			const char* name = zip_entry_name(lz->handle);
			int isdir = zip_entry_isdir(lz->handle);

			std::string finalName = name;
			if (isdir) {
				if (finalName[finalName.length()] != '/') {
					finalName += '/';
				}
			}

			lua_pushstring(L, finalName.c_str());
			lua_seti(L, -2, count);

			count++;
		}
		zip_entry_close(lz->handle);
	}

	// Return the table of entries
	return 1;
}

const struct luaL_Reg lib[] = {
	{"zip_stream_open", lua_zip_stream_open},
	{"zip_entry_exists", lua_zip_entry_exists},
	{"zip_stream_close", lua_zip_stream_close},
	{"zip_stream_copy", lua_zip_stream_copy},
	{"zip_list_entries", lua_zip_list_entries},
	//{"zip_open", lua_zip_file_open},
	//{"zip_close", lua_zip_file_close},
	{"zip_entry_open", lua_zip_entry_open},
	{"zip_entry_close", lua_zip_entry_close},
	{"zip_entry_read", lua_zip_entry_read},
	{"zip_entry_write", lua_zip_entry_write},
	{NULL, NULL}
};

int lua_luazip_stream_gc(lua_State* L) {
	return lua_zip_stream_close(L);
}

int lua_luazip_stream_close(lua_State* L) {
	return lua_zip_stream_close(L);
}

int lua_luazip_stream_tostring(lua_State* L) {
	struct LuaZip* lz = lua_check_luazip(L, 1);

	int tt = luaL_getmetafield(L, 1, "__name");  /* try name */
	const char* kind = (tt == LUA_TSTRING) ? lua_tostring(L, -1) :
		luaL_typename(L, 1);
	lua_pushfstring(L, "%s: %p, mode: '%c', raw: %p", kind, lua_topointer(L, 1), lz->mode, lz->handle);

	if (tt != LUA_TNIL) lua_remove(L, -2);

	return 1;
}

const struct luaL_Reg zip_stream_metatable[] = {
	{"__gc", lua_luazip_stream_gc},
	{"__close", lua_luazip_stream_close},
	{"__tostring", lua_luazip_stream_tostring},
	{NULL, NULL}
};

int lua_luazip_file_gc(lua_State* L) {
	return lua_zip_file_close(L);
}

int lua_luazip_file_close(lua_State* L) {
	return lua_zip_file_close(L);
}

const struct luaL_Reg zip_file_metatable[] = {
	{"__gc", lua_luazip_file_gc},
	{"__close", lua_luazip_file_close},
	{NULL, NULL}
};

const char* luaInterface = R"V0G0N(

return function(lib, stream, level, mode)
	local handle = lib.zip_stream_open(stream, level, mode)

	return setmetatable({
		open_entry = function(self, entryname)
			local status, code, error = lib.zip_entry_open(handle, entryname)
			return status, code, error
		end,

		read_entry = function(self)
			local result, status, error = lib.zip_entry_read(handle)
			return result, status, error
		end,

		write_entry = function(self, contents)
			local status, code, error = lib.zip_entry_write(handle, contents)
			return status, code, error
		end,

		close_entry = function(self)
			local status, code, error = lib.zip_entry_close(handle)
			return status, code, error
		end,

		close = function(self)
			return lib.zip_stream_close(handle)
		end,

		serialize = function(self)
			return lib.zip_stream_copy(handle)
		end,

	}, {
		__close = function(self) lib.zip_stream_close(handle) end,
		__gc = function(self) lib.zip_stream_close(handle) end,
		__tostring = function(self) return tostring(handle) end,
	})
end

)V0G0N";

LUAMEMZIP int luaopen_luamemzip(lua_State * L) {
	luaL_newmetatable(L, LUAMEMZIP_ZIP_STREAM_METATABLE_NAME);
	luaL_setfuncs(L, zip_stream_metatable, 0);
	lua_pop(L, 1);

	luaL_newmetatable(L, LUAMEMZIP_ZIP_FILE_METATABLE_NAME);
	luaL_setfuncs(L, zip_file_metatable, 0);
	lua_pop(L, 1);

	luaL_newlib(L, lib);

	lua_pushinteger(L, ZIP_DEFAULT_COMPRESSION_LEVEL);
	lua_setfield(L, -2, "ZIP_DEFAULT_COMPRESSION_LEVEL");

	if (luaL_dostring(L, luaInterface) != LUA_OK) {
		return luaL_error(L, "failed to load lua interface");
	}
	lua_setfield(L, -2, "MemoryZip");

	return 1;
}