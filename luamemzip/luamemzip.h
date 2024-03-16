#pragma once

#ifdef LUAMEMZIP_EXPORTS
#define LUAMEMZIP extern "C" __declspec(dllexport) 
#else
#define LUAMEMZIP extern "C" __declspec(dllimport) 
#endif // LUAMEMZIP_EXPORTS


#include <lua.hpp>
LUAMEMZIP int luaopen_luamemzip(lua_State * L);

