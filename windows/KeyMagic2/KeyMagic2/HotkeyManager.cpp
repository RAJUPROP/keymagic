#include "stdafx.h"
#include "HotkeyManager.h"
#include "ConfigUtils.h"

HotkeyManager::HotkeyManager(std::wstring jsonPath)
	: jsonPath(jsonPath)
{
	ResetKeys();
}

std::map<WORD, bool*> HotkeyManager::GetModifierToFlagMap() {
	return {
		{VK_LSHIFT, &shift},
		{VK_RSHIFT, &shift},

		{VK_LCONTROL, &ctrl},
		{VK_RCONTROL, &ctrl},

		{VK_LMENU, &alt},
		{VK_RMENU, &alt},

		{VK_LWIN, &win},
		{VK_RWIN, &win},
	};
}

void HotkeyManager::OnKeyDown(int key)
{
	auto modifiers = this->GetModifierToFlagMap();
	auto flag = modifiers.find(key);
	
	if (flag != modifiers.end()) {
		*flag->second = true;
		ignoreMatchingOnKeyup = false;
	}
	else {
		ignoreMatchingOnKeyup = true;
		vk = key;
		// match on key down for non-modifier keys
		ValidateKeyStates();
		MatchAndCall();
	}
}

void HotkeyManager::OnKeyUp(int key)
{
	auto modifiers = this->GetModifierToFlagMap();
	auto flag = modifiers.find(key);

	if (flag != modifiers.end()) {
		if (!ignoreMatchingOnKeyup) {
			MatchAndCall();
		}
		*flag->second = false;
	}
	else if (vk == key) {
		vk = 0;
	}
}

void HotkeyManager::ValidateKeyStates()
{
	ctrl  = GetKeyState(VK_CONTROL) & 0x8000;
	alt   = GetKeyState(VK_MENU)    & 0x8000;
	shift = GetKeyState(VK_SHIFT)   & 0x8000;
	win   = GetKeyState(VK_LWIN)    & 0x8000 || GetKeyState(VK_RWIN) & 0x8000;
}

bool HotkeyManager::MatchAndCall()
{
	std::list<LPWORD> hkys = { &hky_onoff, &hky_nextkbd };
	for (auto & hky : hkys) {
		if (*hky == 0)
		{
			continue;
		}
		else if (MatchHotkey(*hky))
		{
			if (m_handlers.find(hky) != m_handlers.end()) {
				m_handlers[hky]();
			}
			return true;
		}
	}
	return false;
}

bool HotkeyManager::MatchHotkey(WORD hky)
{
	BYTE vk2 = LOBYTE(hky);
	BYTE mod = HIBYTE(hky);

	if (ctrl != ((mod & HOTKEYF_CONTROL) == HOTKEYF_CONTROL))
	{
		return false;
	}

	if (alt != ((mod & HOTKEYF_ALT) == HOTKEYF_ALT))
	{
		return false;
	}

	if (shift != ((mod & HOTKEYF_SHIFT) == HOTKEYF_SHIFT))
	{
		return false;
	}

	if (win != ((mod & HOTKEYF_EXT) == HOTKEYF_EXT))
	{
		return false;
	}

	if (vk != vk2) {
		return false;
	}

	return true;
}

void HotkeyManager::ResetKeys()
{
	ctrl = false;
	alt = false;
	shift = false;
	win = false;
	vk = 0;
}

void HotkeyManager::WriteHotkey()
{
	json config = ConfigUtils::Read();
	json hotkeys = config[ConfigKeyHotkeys];

	hotkeys[ConfigKeyHotkeysOnOff] = hky_onoff;
	hotkeys[ConfigKeyHotkeysNext] = hky_nextkbd;

	config[ConfigKeyHotkeys] = hotkeys;

	ConfigUtils::Write(config);
}

void HotkeyManager::ReloadHotkey()
{
	json config = ConfigUtils::Read();
	json hotkeys;

	hky_onoff = 0;
	hky_nextkbd = 0;


	hotkeys = config[ConfigKeyHotkeys];
	hky_onoff = hotkeys[ConfigKeyHotkeysOnOff];
	hky_nextkbd = hotkeys[ConfigKeyHotkeysNext];
}


std::string HotkeyManager::funcKeyNameForVK(BYTE vkCode)
{
	if (vkCode >= VK_F1 && vkCode <= VK_F24) {
		int number = (vkCode - VK_F1) + 1;
		auto name = std::string("F") + std::to_string(number);
		return name;
	}
	return std::string();
}

std::string HotkeyManager::wHotkeyToString(WORD hotkey)
{
	std::stringstream ss;

	BYTE vk = LOBYTE(hotkey);
	BYTE mod = HIBYTE(hotkey);

	if (mod & HOTKEYF_ALT)
	{
		ss << "Alt+";
	}
	if (mod & HOTKEYF_CONTROL)
	{
		ss << "Ctrl+";
	}
	if (mod & HOTKEYF_SHIFT)
	{
		ss << "Shift+";
	}
	if (mod & HOTKEYF_EXT)
	{
		ss << "Win+";
	}

	if (vk)
	{
		if (auto ch = (char)MapVirtualKey(vk, MAPVK_VK_TO_CHAR)) {
			ss << ch;
		}
		else {
			ss << funcKeyNameForVK(vk);
		}
	}

	std::string s = ss.str();

	if (!s.empty() && s.back() == '+') {
		s.pop_back();
	}
	else if (!s.empty() && s.back() == ' ')
	{
		s.pop_back();
		s += "Space";
	}

	return s;
}