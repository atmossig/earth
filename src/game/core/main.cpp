// 
// name:		main.cpp
// description:	contains entry function
//

#if __64BIT
	// Pool heap size.
	#define POOL_HEAP_SIZE		(30*1024)
#endif // __64BIT

#if EARTH_GEN9_XBOX
// DO NOT FUCK WITH THIS UNLESS YOU KNOW WHAT YOU ARE DOING
// We have optimized the heap sizes to reduce
// TLB misses.  Change this and you could fuck up performance
// as much as (1+ ms/frame)
CompileTimeAssert(SIMPLE_HEAP_SIZE % (4 << 10) == 0);
CompileTimeAssert(DEBUG_HEAP_SIZE % (4 << 10) == 0);

// Replay system heap size, wip.
#if REPLAY
CompileTimeAssert(REPLAY_HEAP_SIZE % (4 << 20) == 0);
#endif // REPLAY

#endif

namespace earth {

PARAM(nokeyboardhook, "Don't disable windows key with low-level keyboard hook.")
PARAM(forcekeyboardhook, "Force disabling window keys with low-level keyboard hook.")

static LRESULT CALLBACK LowLevelKeyboardProc(int code, WPARAM wParam, LPARAM lParam)
{
	if (code == HC_ACTION)
	{
		// Check for pressing / releasing either Windows key, and having focus
		KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
		if ((wParam == WM_KEYUP || wParam == WM_KEYDOWN) && ((p->vkCode == VK_LWIN) || (p->vkCode == VK_RWIN)) && GRCDEVICE.GetHasFocus())
		{
			return 1; 	// Eat keystroke
		}
	}
	return CallNextHookEx(g_keyboardHook, code, wParam, lParam);
}

static void RemoveKeyboardHook()
{
	UnhookWindowsHookEx(g_keyboardHook);
}

static void AddKeyboardHook()
{
	g_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
	atexit(RemoveKeyboardHook);
}

CApp g_myApp;

// Shouldn't ever reach here.
int Main()
{
	Assertf(false, "Earth uses App style entry points.\n");
	return 0;
}

bool MainPrologue()
{
	#if !__FINAL
	if(!PARAM_nokeyboardhook.Get() && (!sysBootManager::IsDebuggerPresent() || PARAM_forcekeyboardhook.Get()))
	#else
	if (!PARAM_nokeyboardhook.Get())
	{
		AddKeyboardHook();
	}
	#endif // !__FINAL
	
	// no output
	if (!PARAM_output.Get() || PARAM_nooutput.Get())
	{
		HWND handle = GetConsoleWindow();
		if (handle)
			ShowWindow(handle, SW_HIDE);
	}
	
	// Engine turns this on by default.
	//
	// We want it off for the game though.
	sysMemAllowResourceAlloc = false;
	
	// Init diag error codes and load system info.
	diagErrorCodes::LoadLanguageFile();
	CSystemInfo::Initialize();
	
	return true;
}

bool Main_OneLoopIteration()
{
	PROFILER_FRAME("MainThread");

#if !__NO_OUTPUT
	// We want a way to crash the game even when running with no target manager
	// (eg. console set to release mode), so we can get what ever coredump the
	// system OS supports.  While this seems like an odd spot to put it, we do
	// want it only in game code (not samples), but should be all games (so not
	// part fof App::RunOneIteration()).
#if EARTH_PC
	if (ioKeyboard::KeyDown(KEY_CONTROL) && ioKeyboard::KeyDown(KEY_SHIFT) && ioKeyboard::KeyDown(KEY_DELETE))
	{
		Displayf("The player has pressed Ctrl+Shift+Delete so call __debugbreak()");
#else
	if (ioKeyboard::KeyDown(KEY_CONTROL) && ioKeyboard::KeyDown(KEY_ALT) && ioKeyboard::KeyDown(KEY_DELETE))
	{
		Displayf("The player has pressed Ctrl+Alt+Delete so call __debugbreak()");
#endif
		__debugbreak();
	}
#endif

// Backtracing to upload to server
// for bug catching and crash catching.
#if BACKTRACE_ENABLED
	BACKTRACE_TEST_CRASH;
#endif

	return g_myApp.RunOneIteration();
}

void Main_Epilogue()
{
	// nothing goes here...
}

SET_APP_ENTRY_POINTS(Main_Prologue, Main_OneLoopIteration, Main_Epilogue);

} // namespace earth