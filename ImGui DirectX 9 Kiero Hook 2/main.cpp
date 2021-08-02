#include "includes.h"

#include "csgo.hpp"

using namespace hazedumper::netvars;
using namespace hazedumper::signatures;

#ifdef _WIN64
#define GWL_WNDPROC GWLP_WNDPROC
#endif

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

EndScene oEndScene = NULL;
WNDPROC oWndProc;
static HWND window = NULL;

void InitImGui(LPDIRECT3DDEVICE9 pDevice)
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX9_Init(pDevice);
}

bool init = false;

bool show_menu = true;

bool esp = false;

bool bhop = false;

bool recoil = false;

bool triggerbot = false;

bool triggerRandomess = false;

bool triggerDelay = false;

int triggerCustomDelay = 0;

struct Vec3 {
	float x, y, z;
	Vec3 operator+ (Vec3 temp) {
		return { x + temp.x, y + temp.y, z + temp.z };
	}
	Vec3 operator- (Vec3 temp) {
		return { x - temp.x, y - temp.y, z - temp.z };
	}
	Vec3 operator* (float temp) {
		return { x * temp, y * temp, z * temp };
	}

	void normalize() {
		while (y < -180)
		{
			y = 360;
		}
		while (y > 180)
		{
			y = -360;
		}
		while (x > 89)
		{
			x = 89;
		}
		while (x < -89)
		{
			x = -89;
		}
	}
};

DWORD clientModule;
DWORD engineModule;
DWORD localPlayer;

int* iShotsFired;

float recoil_amount = 0;

Vec3* viewAngles;

Vec3* aimRecoilPunch;

Vec3 oPunch{ 0, 0, 0 };

void HackInit()
{
	clientModule = (uintptr_t)GetModuleHandle("client.dll");
	engineModule = (uintptr_t)GetModuleHandle("engine.dll");

	localPlayer = *(uintptr_t*)(clientModule + dwLocalPlayer);

	iShotsFired = (int*)(localPlayer + m_iShotsFired);

	viewAngles = (Vec3*)(*(uintptr_t*)(engineModule + dwClientState) + dwClientState_ViewAngles);

	aimRecoilPunch = (Vec3*)(localPlayer + m_aimPunchAngle);
}

long __stdcall hkEndScene(LPDIRECT3DDEVICE9 pDevice)
{
	if (!init)
	{
		InitImGui(pDevice);
		init = true;
		HackInit();
	}

	if (GetAsyncKeyState(VK_INSERT) & 1)
	{
		show_menu = !show_menu;
	}

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (show_menu)
	{
		ImGui::Begin("Le Dung Hacking");
		ImGui::Spacing();
		ImGui::Text("ESP Settings");
		ImGui::Checkbox("ESP", &esp);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Checkbox("BHOP", &bhop);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Checkbox("No recoil", &recoil);
		if (recoil)
		{
			ImGui::SliderFloat("Amount", &recoil_amount, 0, 1);
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Text("TriggerBot Settings");
		ImGui::Checkbox("Enabled", &triggerbot);
		if (true)
		{
			ImGui::Spacing();
			ImGui::Checkbox("Enable Custom Delay", &triggerDelay);
			if (triggerDelay)
			{
				ImGui::SliderInt("Custom Delay (ms)", &triggerCustomDelay, 0, 250);
			}
			else {
				ImGui::Checkbox("Random Delay", &triggerRandomess);
			}
		}
		ImGui::End();
	}	

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	return oEndScene(pDevice);
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (true && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam)
{
	DWORD wndProcId;
	GetWindowThreadProcessId(handle, &wndProcId);

	if (GetCurrentProcessId() != wndProcId)
		return TRUE; // skip to next window

	window = handle;
	return FALSE; // window found abort search
}

HWND GetProcessWindow()
{
	window = NULL;
	EnumWindows(EnumWindowsCallback, NULL);
	return window;
}

DWORD WINAPI MainThread(LPVOID lpReserved)
{
	bool attached = false;
	do
	{
		if (kiero::init(kiero::RenderType::D3D9) == kiero::Status::Success)
		{
			kiero::bind(42, (void**)& oEndScene, hkEndScene);
			do
				window = GetProcessWindow();
			while (window == NULL);
			oWndProc = (WNDPROC)SetWindowLongPtr(window, GWL_WNDPROC, (LONG_PTR)WndProc);
			attached = true;
		}
	} while (!attached);
	return TRUE;
}

DWORD WINAPI TriggerThread(LPVOID lp)
{
	DWORD gameModule = (DWORD)GetModuleHandle("client.dll");
	while (true)
	{
		if (triggerbot)
		{
			DWORD localPlayer = *(DWORD*)(gameModule + dwLocalPlayer);
			int crosshair = *(int*)(localPlayer + m_iCrosshairId);
			int localTeam = *(int*)(localPlayer + m_iTeamNum);

			if (crosshair != 0 && crosshair < 64)
			{
				uintptr_t entity = *(uintptr_t*)(gameModule + dwEntityList + (crosshair - 1) * 0x10);
				int team = *(int*)(entity + m_iTeamNum);
				int health = *(int*)(entity + m_iHealth);

				if (team != localTeam && health > 0 && health < 101)
				{
					if (triggerRandomess)
					{
						Sleep((rand() * 250) + 50); 
						*(int*)(gameModule + dwForceAttack) = 5;
						Sleep(20);
						*(int*)(gameModule + dwForceAttack) = 4;
					}
					else {
						if (triggerDelay)
						{
							Sleep(triggerCustomDelay);
							*(int*)(gameModule + dwForceAttack) = 5;
							Sleep(20);
							*(int*)(gameModule + dwForceAttack) = 4;
						}
						else {
							*(int*)(gameModule + dwForceAttack) = 5;
							*(int*)(gameModule + dwForceAttack) = 4;
						}
					}
				}
			}

		}
	}
}

DWORD WINAPI BhopThread(LPVOID lp)
{
	DWORD gameModule = (DWORD)GetModuleHandle("client.dll");
	DWORD localPlayer = *(DWORD*)(gameModule + dwLocalPlayer);

	while (localPlayer == NULL)
	{
		localPlayer = *(DWORD*)(gameModule + dwLocalPlayer);
	}

	while (true)
	{
		if (bhop)
		{
			DWORD flag = *(BYTE*)(localPlayer + m_fFlags);
			if (GetAsyncKeyState(VK_SPACE) && flag && (1 << 0))
			{
				*(DWORD*)(gameModule + dwForceJump) = 6;
			}
		}
	}
}

DWORD WINAPI RecoilThread(LPVOID lp) 
{
	while (true)
	{
		if (recoil && !GetAsyncKeyState(VK_RBUTTON))
		{
			Vec3 punchAngle = *aimRecoilPunch * (recoil_amount * 2);
			if (*iShotsFired > 1 && GetAsyncKeyState(VK_LBUTTON))
			{
				Vec3 newAngle = *viewAngles + oPunch - punchAngle;
				newAngle.normalize();
				*viewAngles = newAngle;
			}
			oPunch = punchAngle;
		}
	}
}



BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hMod);
		CreateThread(nullptr, 0, MainThread, hMod, 0, nullptr);
		CreateThread(nullptr, 0, BhopThread, hMod, 0, nullptr);
		CreateThread(nullptr, 0, RecoilThread , hMod, 0, nullptr);
		CreateThread(nullptr, 0, TriggerThread , hMod, 0, nullptr);
		break;
	case DLL_PROCESS_DETACH:
		kiero::shutdown();
		break;
	}
	return TRUE;
}
