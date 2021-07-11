
#ifndef __LUA_EXTRA_H_
#define __LUA_EXTRA_H_

#ifdef LUACJSON_DLL
#ifdef LUACJSON_EXPORT	
#define LUACJSON_API __declspec(dllexport)
#else						
#define LUACJSON_API __declspec(dllimport)
#endif	
#else
#ifdef STATIC_LIB
#define LUACJSON_API extern
#else
#define LUACJSON_API
#endif
#endif
#if __cplusplus
extern "C" {
#endif

#include "lauxlib.h"

	void LUACJSON_API luaopen_lua_cjson(lua_State *L);
    
#if __cplusplus
}
#endif

#endif /* __LUA_EXTRA_H_ */
