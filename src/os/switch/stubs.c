/*
 * This file is part of OpenBGI (https://openbgi.net).
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * stubs.c - MessageBox, Dialogs, Child windows, Edit controls, Debugger and PipeProc stubs for Nintendo Switch
 */
#include "switch_internal.h"

/* -------------------------------------------------------------------------
 * 1. MessageBox
 * ------------------------------------------------------------------------- */
int OS_MessageBox(const char* text, const char* caption, uint32_t flags)
{
	char uTitle[512], uMsg[1024];
	OsSwitch_SjisToUtf8(caption ? caption : "Notice", uTitle, sizeof(uTitle));
	OsSwitch_SjisToUtf8(text ? text : "", uMsg, sizeof(uMsg));

	Uint32 sdlFlags = SDL_MESSAGEBOX_INFORMATION;
	if(flags & OS_MB_ICONERROR)
		sdlFlags = SDL_MESSAGEBOX_ERROR;
	else if(flags & OS_MB_ICONWARNING)
		sdlFlags = SDL_MESSAGEBOX_WARNING;

	SDL_ShowSimpleMessageBox(sdlFlags, uTitle, uMsg, gSwitch.window);
	return OS_IDOK;
}

/* -------------------------------------------------------------------------
 * 2. Dialogs
 * ------------------------------------------------------------------------- */
int OS_FileDialog(int save, char* path, size_t n, const char* filter, const char* defExt, const char* initialDir, const char* title)
{
	BGI_UNUSED(save);
	BGI_UNUSED(path);
	BGI_UNUSED(n);
	BGI_UNUSED(filter);
	BGI_UNUSED(defExt);
	BGI_UNUSED(initialDir);
	BGI_UNUSED(title);
	return 0;
}

int OS_FileDialogN(int save, char* path, size_t n, const char* const* descs, const char* const* pats, int count,
                   int* index, const char* defExt, const char* initialDir, const char* title)
{
	BGI_UNUSED(save);
	BGI_UNUSED(path);
	BGI_UNUSED(n);
	BGI_UNUSED(descs);
	BGI_UNUSED(pats);
	BGI_UNUSED(count);
	BGI_UNUSED(index);
	BGI_UNUSED(defExt);
	BGI_UNUSED(initialDir);
	BGI_UNUSED(title);
	return 0;
}

int OS_BrowseFolder(char* path, size_t n, const char* title, const char* initial)
{
	BGI_UNUSED(path);
	BGI_UNUSED(n);
	BGI_UNUSED(title);
	BGI_UNUSED(initial);
	return 0;
}

int OS_DialogInput1(const char* title, const char* initial, int maxLen, int digits, char* out)
{
	BGI_UNUSED(title);
	BGI_UNUSED(initial);
	BGI_UNUSED(maxLen);
	BGI_UNUSED(digits);
	if(out)
		out[0] = 0;
	return 0;
}

int OS_DialogInput2(const char* title, const char* label1, const char* initial1, int maxLen1, int digits1, char* out1,
                    const char* label2, const char* initial2, int maxLen2, int digits2, char* out2)
{
	BGI_UNUSED(title);
	BGI_UNUSED(label1);
	BGI_UNUSED(initial1);
	BGI_UNUSED(maxLen1);
	BGI_UNUSED(digits1);
	BGI_UNUSED(label2);
	BGI_UNUSED(initial2);
	BGI_UNUSED(maxLen2);
	BGI_UNUSED(digits2);
	if(out1) out1[0] = 0;
	if(out2) out2[0] = 0;
	return 0;
}

int OS_DialogProfile(char* surname, char* givenName, char* nickname, char* pronoun, int32_t* month, int32_t* day)
{
	BGI_UNUSED(surname);
	BGI_UNUSED(givenName);
	BGI_UNUSED(nickname);
	BGI_UNUSED(pronoun);
	if(month) *month = 1;
	if(day) *day = 1;
	return 0;
}

int OS_LauncherDialog(OsLauncher_t* l)
{
	BGI_UNUSED(l);
	return 0;
}

int OS_InstallerDialog(int id, void* ctx)
{
	BGI_UNUSED(id);
	BGI_UNUSED(ctx);
	return 0;
}

int OS_IpcSendToRunning(const char* windowClass, const char* data)
{
	BGI_UNUSED(windowClass);
	BGI_UNUSED(data);
	return 0;
}

/* -------------------------------------------------------------------------
 * 3. Child Windows (Switch 不支持桌面级子窗口，全部安全存根)
 * ------------------------------------------------------------------------- */
struct OsChild
{
	int dummy;
};

OsChild_t* OS_ChildCreate(int index, const char* title, int x, int y, int outerW, int outerH)
{
	BGI_UNUSED(index);
	BGI_UNUSED(title);
	BGI_UNUSED(x);
	BGI_UNUSED(y);
	BGI_UNUSED(outerW);
	BGI_UNUSED(outerH);
	return NULL;
}

void OS_ChildClose(OsChild_t* c)
{
	BGI_UNUSED(c);
}

void OS_ChildShow(OsChild_t* c, int show)
{
	BGI_UNUSED(c);
	BGI_UNUSED(show);
}

void OS_ChildSetTitle(OsChild_t* c, const char* title)
{
	BGI_UNUSED(c);
	BGI_UNUSED(title);
}

void OS_ChildMove(OsChild_t* c, int x, int y, int outerW, int outerH)
{
	BGI_UNUSED(c);
	BGI_UNUSED(x);
	BGI_UNUSED(y);
	BGI_UNUSED(outerW);
	BGI_UNUSED(outerH);
}

void OS_ChildGetPos(OsChild_t* c, int* x, int* y)
{
	BGI_UNUSED(c);
	if(x) *x = 0;
	if(y) *y = 0;
}

OsDc_t* OS_ChildGetDc(OsChild_t* c)
{
	BGI_UNUSED(c);
	return NULL;
}

void OS_ChildReleaseDc(OsChild_t* c, OsDc_t* dc)
{
	BGI_UNUSED(c);
	BGI_UNUSED(dc);
}

/* -------------------------------------------------------------------------
 * 4. Edit Control (游戏内文本输入控件存根)
 * ------------------------------------------------------------------------- */
void OS_EditShow(int show)
{
	BGI_UNUSED(show);
}

void OS_EditMove(int x, int y, int w, int h)
{
	BGI_UNUSED(x);
	BGI_UNUSED(y);
	BGI_UNUSED(w);
	BGI_UNUSED(h);
}

void OS_EditSetFont(const char* face, int height, int width)
{
	BGI_UNUSED(face);
	BGI_UNUSED(height);
	BGI_UNUSED(width);
}

void OS_EditLimitText(int maxChars)
{
	BGI_UNUSED(maxChars);
}

void OS_EditSetText(const char* t)
{
	BGI_UNUSED(t);
}

void OS_EditSelectAll(void)
{
}

void OS_EditFocus(int toEdit)
{
	BGI_UNUSED(toEdit);
}

int OS_EditGetText(char* buf, size_t n)
{
	if(buf && n > 0)
		buf[0] = 0;
	return 0;
}

void OS_EditSetColour(uint32_t rgb)
{
	BGI_UNUSED(rgb);
}

void OS_EditRefresh(void)
{
}

void OS_EditDestroy(void)
{
}

void OS_EditRejectAscii(int reject)
{
	BGI_UNUSED(reject);
}

int OS_EditImeOpen(void)
{
	return 0;
}

/* -------------------------------------------------------------------------
 * 5. Debugger Window (Switch 单屏全屏运行，无附加调试器窗口)
 * ------------------------------------------------------------------------- */
int OS_DebugWindowOpen(int w, int h, const char* title)
{
	BGI_UNUSED(w);
	BGI_UNUSED(h);
	BGI_UNUSED(title);
	return 0;
}

void OS_DebugWindowClose(void)
{
}

int OS_DebugWindowAlive(void)
{
	return 0;
}

void OS_DebugWindowPresent(const OsSurface_t* s)
{
	BGI_UNUSED(s);
}

/* -------------------------------------------------------------------------
 * 6. Pipe Process (Switch 无 fork/exec 管道支持)
 * ------------------------------------------------------------------------- */
struct OsPipeProc
{
	int dummy;
};

OsPipeProc_t* OS_PipeProcStart(const char* const* argv)
{
	BGI_UNUSED(argv);
	return NULL;
}

int OS_PipeProcRead(OsPipeProc_t* p, void* buf, int n)
{
	BGI_UNUSED(p);
	BGI_UNUSED(buf);
	BGI_UNUSED(n);
	return 0;
}

void OS_PipeProcStop(OsPipeProc_t* p)
{
	BGI_UNUSED(p);
}
