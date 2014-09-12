
// Author:  Greg "fugue" Santucci, 2011
// Email:   thecodewitch@gmail.com
// Web:     http://createuniverses.blogspot.com/


#include "luaInterface.h"

#ifdef __PRAXIS_WINDOWS__
#include <windows.h>
extern "C"
{
    extern HWND g_AppHWND;
    extern int g_nLastBreakTime;
}
#endif

lua_State * g_pLuaState = 0;

std::string g_sLuaOutput;
std::string g_sLuaError;

//bool g_bLuaBreak = false;
void cbLuaBreakHook(lua_State *L, lua_Debug *ar);

//int cbLuaFoo(lua_State *L);
int cbLuaPrint(lua_State *L);

// Critical sections....hmm....
// This may not be necessary at all on Linux because the midi stuff isn't being called from some super-duper accurately timed
// windows midi thread

#ifdef __PRAXIS_WINDOWS__
extern CRITICAL_SECTION g_cs;
#endif

bool g_bLuaRunning = false;

extern "C" {
int luaopen_lpeg (lua_State *L);
}

void luaInit()
{
#ifdef __PRAXIS_WINDOWS__
    EnterCriticalSection (&g_cs) ;
#endif

    g_pLuaState = luaL_newstate();

    luaL_openlibs(g_pLuaState);

    luaopen_lpeg(g_pLuaState);

//    lua_register(g_pLuaState, "foo",    cbLuaFoo);
    lua_register(g_pLuaState, "print",  cbLuaPrint);

    luaL_dostring(g_pLuaState,
                  "function onerror(s)"
                    "endGL()"
                    "return s .. \"\\n\" .. debug.traceback()"
                  "end");

    lua_sethook(g_pLuaState, cbLuaBreakHook, LUA_MASKCOUNT, 1000);

#ifdef __PRAXIS_WINDOWS__
    LeaveCriticalSection (&g_cs) ;
#endif

    g_bLuaRunning = true;
}

bool luaCall(std::string sCmd)
{
#ifdef __PRAXIS_WINDOWS__
    EnterCriticalSection (&g_cs) ;
#endif

    lua_getglobal(g_pLuaState, "onerror");
    int error = (luaL_loadstring(g_pLuaState, sCmd.c_str()) || lua_pcall(g_pLuaState, 0, LUA_MULTRET, -2));

    if (error)
    {
        g_sLuaError += lua_tostring(g_pLuaState, -1);
        g_sLuaError += "\n";

        lua_pop(g_pLuaState, 1);
    }

    // Pop the error function sitting on the stack
    lua_pop(g_pLuaState, 1);

#ifdef __PRAXIS_WINDOWS__
    LeaveCriticalSection (&g_cs) ;
#endif

    return (error == 0);
}

std::string & luaGetOutput()
{
    return g_sLuaOutput;
}

std::string & luaGetError()
{
    return g_sLuaError;
}

void luaClearError()
{
    g_sLuaError = "";
}

void luaClearOutput()
{
    g_sLuaOutput = "";
}

void luaClose()
{
    g_bLuaRunning = false;

#ifdef __PRAXIS_WINDOWS__
    EnterCriticalSection (&g_cs) ;
#endif

    lua_close(g_pLuaState);

    // Dump the error and output strings to a file?

#ifdef __PRAXIS_WINDOWS__
    LeaveCriticalSection (&g_cs) ;
#endif
}

int cbLuaPrint( lua_State *L)
{
    int n = lua_gettop(L);
    int i = 0;
    for(i = 1; i <= n; i++)
    {
        if(lua_isstring(L, i))
        {
            g_sLuaOutput += lua_tostring(L, i);
            g_sLuaOutput += "\n";
        }
        else
        {
            g_sLuaOutput += "Not a string\n";
        }
    }

    return 0;
}

bool luaIsCommandComplete(std::string sCmd)
{
    int status = luaL_loadbuffer(g_pLuaState, sCmd.c_str(), sCmd.length(), "=stdin");
    // pushes either a lua chunk or an error message

    if (status == LUA_ERRSYNTAX)
    {
        size_t lmsg;
        const char *msg = lua_tolstring(g_pLuaState, -1, &lmsg);
        const char *tp = msg + lmsg - (sizeof(LUA_QL("<eof>")) - 1);
        if (strstr(msg, LUA_QL("<eof>")) == tp)
        {
            lua_pop(g_pLuaState, 1); // Pop the error message
            return false;
        }
    }

    lua_pop(g_pLuaState, 1); // Pop the lua chunk
    return true;
}

//void luaBreak()
//{
//	g_bLuaBreak = true;
//}

void cbLuaBreakHook(lua_State *L, lua_Debug *ar)
{
    // Need some other way to do this in Linux.
#ifdef __PRAXIS_WINDOWS__
    if(::GetAsyncKeyState(VK_LCONTROL) != 0 && ::GetAsyncKeyState(0x51) != 0) // Ctrl-Q pressed
    {
        // Break detected. Check the time since last break.

        // If its too low, ignore.
        int nTime = ::timeGetTime();
        if(nTime < (g_nLastBreakTime + 200))
            return;

        // If its high enough, then break.
        g_nLastBreakTime = nTime;

        lua_pushstring(L, "User break.");
        lua_error(L);
        return;
    }
#endif

    return;

#if 0
    HGLOBAL      hGlobal ;
    PTSTR        pGlobal ;

    std::string sText;

    OpenClipboard (g_AppHWND) ;
    if (hGlobal = GetClipboardData (CF_TEXT))
    {
         pGlobal = (PTSTR)GlobalLock (hGlobal) ;
         sText = pGlobal;
         GlobalUnlock(hGlobal);
    }
    CloseClipboard () ;


    if(sText == "praxis:STOP")
    {
        OpenClipboard (g_AppHWND) ;
        EmptyClipboard();
        CloseClipboard () ;

        lua_pushstring(L, "User break.");
        lua_error(L);
    }
#endif
}

//std::string luaLogCall(std::string sCmd)
//{
//    g_luaOutputString = "";
//    lua_getglobal(g_pLuaState, "logLiveCode");
//    lua_pushstring(g_pLuaState, sCmd.c_str());
//    lua_call(g_pLuaState, 1, 0);
//    return g_luaOutputString;
//}

//int cbLuaFoo(lua_State *L)
//{
//    int n = lua_gettop(L);           /* number of arguments */
//    lua_Number sum = 0;
//    int i;
//    for (i = 1; i <= n; i++)
//    {
//        if (!lua_isnumber(L, i))
//        {
//            lua_pushstring(L, "incorrect argument");
//            lua_error(L);
//        }
//        sum += lua_tonumber(L, i);
//    }

//    lua_pushnumber(L, sum/n);        /* first result */
//    lua_pushnumber(L, sum);          /* second result */
//    return 2;                        /* number of results */
//}