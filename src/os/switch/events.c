/*
 * This file is part of OpenBGI (https://openbgi.net).
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * events.c - SDL2 Event loop, Joy-Con / Gamepad, Touchscreen and Input mapping for Nintendo Switch
 */
#include "switch_internal.h"

extern void OsSwitch_ScreenToClient(int* x, int* y);
extern void OsSwitch_ClientToScreen(int* x, int* y);

void OS_SetEventHandlers(const OsEventHandlers_t* h)
{
	if(h)
		gSwitch.handlers = *h;
	else
		memset(&gSwitch.handlers, 0, sizeof(gSwitch.handlers));
}

void OS_PostQuit(void)
{
	gSwitch.quitPosted = 1;
}

void OS_DragAccept(int accept)
{
	BGI_UNUSED(accept);
}

void OS_CursorSetShape(int shape)
{
	BGI_UNUSED(shape);
}

void OS_CursorShow(int show)
{
	BGI_UNUSED(show);
}

void OS_CursorGetPos(int* x, int* y)
{
	int cx = gSwitch.mouseX;
	int cy = gSwitch.mouseY;
	OsSwitch_ClientToScreen(&cx, &cy);
	if(x) *x = cx;
	if(y) *y = cy;
}

void OS_CursorSetPos(int x, int y)
{
	OsSwitch_ScreenToClient(&x, &y);
	gSwitch.mouseX = x;
	gSwitch.mouseY = y;
	if(gSwitch.handlers.mouse_move)
		gSwitch.handlers.mouse_move(x, y);
}

void OS_ScreenToClient(int* x, int* y)
{
	OsSwitch_ScreenToClient(x, y);
}

void OS_ClientToScreen(int* x, int* y)
{
	OsSwitch_ClientToScreen(x, y);
}

int OS_MouseButtonsSwapped(void)
{
	return 0;
}

int OS_KeyState(int vk)
{
	if(vk >= 0 && vk < 256)
		return gSwitch.keys[vk] ? 0x8000 : 0;
	return 0;
}

void OS_KeyboardState(uint8_t out[256])
{
	if(!out)
		return;
	for(int i = 0; i < 256; i++)
	{
		out[i] = gSwitch.keys[i] ? 0x80 : 0x00;
	}
}

/* 记录按键状态 */
static void SetKey(int vk, int down)
{
	if(vk > 0 && vk < 256)
		gSwitch.keys[vk] = down ? 1 : 0;
}

/* SDL 键码转 Win32 Virtual Key (VK_*) */
static int SdlKeyToVk(SDL_Keycode k)
{
	if(k >= SDLK_a && k <= SDLK_z)
		return 0x41 + (k - SDLK_a);
	if(k >= SDLK_0 && k <= SDLK_9)
		return 0x30 + (k - SDLK_0);

	switch(k)
	{
		case SDLK_RETURN:    return 0x0D; // VK_RETURN
		case SDLK_ESCAPE:    return 0x1B; // VK_ESCAPE
		case SDLK_BACKSPACE: return 0x08; // VK_BACK
		case SDLK_TAB:       return 0x09; // VK_TAB
		case SDLK_SPACE:     return 0x20; // VK_SPACE
		case SDLK_LCTRL:
		case SDLK_RCTRL:     return 0x11; // VK_CONTROL
		case SDLK_LSHIFT:
		case SDLK_RSHIFT:    return 0x10; // VK_SHIFT
		case SDLK_LALT:
		case SDLK_RALT:      return 0x12; // VK_MENU
		case SDLK_UP:        return 0x26; // VK_UP
		case SDLK_DOWN:      return 0x28; // VK_DOWN
		case SDLK_LEFT:      return 0x25; // VK_LEFT
		case SDLK_RIGHT:     return 0x27; // VK_RIGHT
		case SDLK_PAGEUP:    return 0x21; // VK_PRIOR
		case SDLK_PAGEDOWN:  return 0x22; // VK_NEXT
		case SDLK_HOME:      return 0x24; // VK_HOME
		case SDLK_END:       return 0x23; // VK_END
		case SDLK_INSERT:    return 0x2D; // VK_INSERT
		case SDLK_DELETE:    return 0x2E; // VK_DELETE
		case SDLK_F1:        return 0x70;
		case SDLK_F2:        return 0x71;
		case SDLK_F3:        return 0x72;
		case SDLK_F4:        return 0x73;
		case SDLK_F5:        return 0x74;
		case SDLK_F6:        return 0x75;
		case SDLK_F7:        return 0x76;
		case SDLK_F8:        return 0x77;
		case SDLK_F9:        return 0x78;
		case SDLK_F10:       return 0x79;
		case SDLK_F11:       return 0x7A;
		case SDLK_F12:       return 0x7B;
		default:             return 0;
	}
}

/* 处理鼠标/触控点击与双击判断 */
static void OnMouseDown(int btn, int cx, int cy)
{
	gSwitch.mouseX = cx;
	gSwitch.mouseY = cy;

	int vk = (btn == 0) ? 0x01 : (btn == 1 ? 0x02 : 0x04);
	SetKey(vk, 1);

	uint32_t now = OS_TicksMs();
	int isDbl = (btn == gSwitch.lastClickBtn &&
	             (now - gSwitch.lastClickTime) <= 400 &&
	             abs(cx - gSwitch.lastClickX) <= 15 &&
	             abs(cy - gSwitch.lastClickY) <= 15);

	gSwitch.lastClickBtn = btn;
	gSwitch.lastClickTime = now;
	gSwitch.lastClickX = cx;
	gSwitch.lastClickY = cy;

	if(isDbl && gSwitch.handlers.mouse_dblclick)
	{
		gSwitch.handlers.mouse_dblclick(btn, cx, cy);
	}
	else if(gSwitch.handlers.mouse_down)
	{
		gSwitch.handlers.mouse_down(btn, cx, cy);
	}
}

static void OnMouseUp(int btn, int cx, int cy)
{
	gSwitch.mouseX = cx;
	gSwitch.mouseY = cy;

	int vk = (btn == 0) ? 0x01 : (btn == 1 ? 0x02 : 0x04);
	SetKey(vk, 0);

	if(gSwitch.handlers.mouse_up)
		gSwitch.handlers.mouse_up(btn, cx, cy);
}

/* Switch 手柄按键映射为 Galgame 核心操作 */
static void HandleControllerButton(Uint8 btn, bool down)
{
	switch(btn)
	{
		/* A 键: 确认 / 前进 / 鼠标左键 */
		case SDL_CONTROLLER_BUTTON_A:
			SetKey(0x0D, down); // VK_RETURN
			if(down)
			{
				if(gSwitch.handlers.key_down) gSwitch.handlers.key_down(0x0D, 0);
				OnMouseDown(0, gSwitch.mouseX, gSwitch.mouseY);
			}
			else
			{
				if(gSwitch.handlers.key_up) gSwitch.handlers.key_up(0x0D);
				OnMouseUp(0, gSwitch.mouseX, gSwitch.mouseY);
			}
			break;

		/* B 键: 取消 / 隐藏文字框 / 鼠标右键 */
		case SDL_CONTROLLER_BUTTON_B:
			SetKey(0x1B, down); // VK_ESCAPE
			if(down)
			{
				OnMouseDown(1, gSwitch.mouseX, gSwitch.mouseY);
			}
			else
			{
				OnMouseUp(1, gSwitch.mouseX, gSwitch.mouseY);
			}
			break;

		/* X 键: 自动推进 (Auto Mode, 'A' 或 Space) */
		case SDL_CONTROLLER_BUTTON_X:
			SetKey(0x41, down); // 'A'
			if(down && gSwitch.handlers.key_down) gSwitch.handlers.key_down(0x41, 0);
			if(!down && gSwitch.handlers.key_up) gSwitch.handlers.key_up(0x41);
			break;

		/* Y 键: 历史回溯 (Log / PageUp) */
		case SDL_CONTROLLER_BUTTON_Y:
			SetKey(0x21, down); // VK_PRIOR (PageUp)
			if(down && gSwitch.handlers.key_down) gSwitch.handlers.key_down(0x21, 0);
			if(!down && gSwitch.handlers.key_up) gSwitch.handlers.key_up(0x21);
			break;

		/* R / ZR 键: 快进 (Skip, 按住 Ctrl) */
		case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
			SetKey(0x11, down); // VK_CONTROL
			if(down && gSwitch.handlers.key_down) gSwitch.handlers.key_down(0x11, 0);
			if(!down && gSwitch.handlers.key_up) gSwitch.handlers.key_up(0x11);
			break;

		/* Plus (+) 键: 呼出主菜单 (ESC) */
		case SDL_CONTROLLER_BUTTON_START:
			SetKey(0x1B, down); // VK_ESCAPE
			if(down && gSwitch.handlers.key_down) gSwitch.handlers.key_down(0x1B, 0);
			if(!down && gSwitch.handlers.key_up) gSwitch.handlers.key_up(0x1B);
			break;

		/* 十字键方向 */
		case SDL_CONTROLLER_BUTTON_DPAD_UP:
			SetKey(0x26, down);
			if(down && gSwitch.handlers.key_down) gSwitch.handlers.key_down(0x26, 0);
			if(!down && gSwitch.handlers.key_up) gSwitch.handlers.key_up(0x26);
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
			SetKey(0x28, down);
			if(down && gSwitch.handlers.key_down) gSwitch.handlers.key_down(0x28, 0);
			if(!down && gSwitch.handlers.key_up) gSwitch.handlers.key_up(0x28);
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
			SetKey(0x25, down);
			if(down && gSwitch.handlers.key_down) gSwitch.handlers.key_down(0x25, 0);
			if(!down && gSwitch.handlers.key_up) gSwitch.handlers.key_up(0x25);
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
			SetKey(0x27, down);
			if(down && gSwitch.handlers.key_down) gSwitch.handlers.key_down(0x27, 0);
			if(!down && gSwitch.handlers.key_up) gSwitch.handlers.key_up(0x27);
			break;

		default:
			break;
	}
}

/* 核心事件泵：每帧被上层循环驱动 */
int OS_PumpMessages(void)
{
	if(gSwitch.quitPosted)
	{
		gSwitch.quitPosted = 0;
		if(gSwitch.handlers.quit)
			gSwitch.handlers.quit();
		return 1;
	}

	if(gSwitch.closeRequested)
	{
		gSwitch.closeRequested = 0;
		if(gSwitch.handlers.close_request)
			gSwitch.handlers.close_request();
		return 1;
	}

	SDL_Event ev;
	int gotEvent = 0;

	while(SDL_PollEvent(&ev))
	{
		gotEvent = 1;
		switch(ev.type)
		{
			case SDL_QUIT:
				if(gSwitch.handlers.close_request)
					gSwitch.handlers.close_request();
				break;

			/* 触控输入 */
			case SDL_FINGERDOWN:
			{
				int sx = (int)(ev.tfinger.x * SWITCH_SCREEN_WIDTH);
				int sy = (int)(ev.tfinger.y * SWITCH_SCREEN_HEIGHT);
				int cx = sx, cy = sy;
				OsSwitch_ScreenToClient(&cx, &cy);
				if(gSwitch.handlers.mouse_move)
					gSwitch.handlers.mouse_move(cx, cy);
				OnMouseDown(0, cx, cy);
				break;
			}
			case SDL_FINGERMOTION:
			{
				int sx = (int)(ev.tfinger.x * SWITCH_SCREEN_WIDTH);
				int sy = (int)(ev.tfinger.y * SWITCH_SCREEN_HEIGHT);
				int cx = sx, cy = sy;
				OsSwitch_ScreenToClient(&cx, &cy);
				gSwitch.mouseX = cx;
				gSwitch.mouseY = cy;
				if(gSwitch.handlers.mouse_move)
					gSwitch.handlers.mouse_move(cx, cy);
				break;
			}
			case SDL_FINGERUP:
			{
				int sx = (int)(ev.tfinger.x * SWITCH_SCREEN_WIDTH);
				int sy = (int)(ev.tfinger.y * SWITCH_SCREEN_HEIGHT);
				int cx = sx, cy = sy;
				OsSwitch_ScreenToClient(&cx, &cy);
				OnMouseUp(0, cx, cy);
				break;
			}

			/* 物理鼠标输入 */
			case SDL_MOUSEBUTTONDOWN:
			{
				int cx = ev.button.x, cy = ev.button.y;
				OsSwitch_ScreenToClient(&cx, &cy);
				int btn = (ev.button.button == SDL_BUTTON_RIGHT) ? 1 :
				          (ev.button.button == SDL_BUTTON_MIDDLE ? 2 : 0);
				OnMouseDown(btn, cx, cy);
				break;
			}
			case SDL_MOUSEBUTTONUP:
			{
				int cx = ev.button.x, cy = ev.button.y;
				OsSwitch_ScreenToClient(&cx, &cy);
				int btn = (ev.button.button == SDL_BUTTON_RIGHT) ? 1 :
				          (ev.button.button == SDL_BUTTON_MIDDLE ? 2 : 0);
				OnMouseUp(btn, cx, cy);
				break;
			}
			case SDL_MOUSEMOTION:
			{
				int cx = ev.motion.x, cy = ev.motion.y;
				OsSwitch_ScreenToClient(&cx, &cy);
				gSwitch.mouseX = cx;
				gSwitch.mouseY = cy;
				if(gSwitch.handlers.mouse_move)
					gSwitch.handlers.mouse_move(cx, cy);
				break;
			}
			case SDL_MOUSEWHEEL:
			{
				if(gSwitch.handlers.mouse_wheel)
					gSwitch.handlers.mouse_wheel(ev.wheel.y * 120, gSwitch.mouseX, gSwitch.mouseY);
				break;
			}

			/* 手柄按键 */
			case SDL_CONTROLLERBUTTONDOWN:
				HandleControllerButton(ev.cbutton.button, true);
				break;
			case SDL_CONTROLLERBUTTONUP:
				HandleControllerButton(ev.cbutton.button, false);
				break;

			/* 手柄左摇杆模拟鼠标平滑指针移动 */
			case SDL_CONTROLLERAXISMOTION:
			{
				if(ev.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX || ev.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY)
				{
					static Sint16 axisX = 0, axisY = 0;
					if(ev.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) axisX = ev.caxis.value;
					if(ev.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) axisY = ev.caxis.value;

					if(abs(axisX) > 8000 || abs(axisY) > 8000)
					{
						int stepX = (axisX / 4000);
						int stepY = (axisY / 4000);
						gSwitch.mouseX += stepX;
						gSwitch.mouseY += stepY;
						if(gSwitch.mouseX < 0) gSwitch.mouseX = 0;
						if(gSwitch.mouseY < 0) gSwitch.mouseY = 0;
						if(gSwitch.mouseX >= gSwitch.clientW) gSwitch.mouseX = gSwitch.clientW - 1;
						if(gSwitch.mouseY >= gSwitch.clientH) gSwitch.mouseY = gSwitch.clientH - 1;

						if(gSwitch.handlers.mouse_move)
							gSwitch.handlers.mouse_move(gSwitch.mouseX, gSwitch.mouseY);
					}
				}
				break;
			}

			/* 物理键盘输入 */
			case SDL_KEYDOWN:
			{
				int vk = SdlKeyToVk(ev.key.keysym.sym);
				if(vk)
				{
					SetKey(vk, 1);
					if(gSwitch.handlers.key_down)
						gSwitch.handlers.key_down(vk, ev.key.repeat ? 1 : 0);
				}
				break;
			}
			case SDL_KEYUP:
			{
				int vk = SdlKeyToVk(ev.key.keysym.sym);
				if(vk)
				{
					SetKey(vk, 0);
					if(gSwitch.handlers.key_up)
						gSwitch.handlers.key_up(vk);
				}
				break;
			}

			default:
				break;
		}
	}

	return gotEvent;
}
