#include "Configs.h"
#include "MenuWatcher.h"

MenuWatcher* MenuWatcher::instance = nullptr;
RE::BSEventNotifyControl MenuWatcher::ProcessEvent(const RE::MenuOpenCloseEvent& evn, [[maybe_unused]] RE::BSTEventSource<RE::MenuOpenCloseEvent>* src) {
	if (!evn.opening) {
		if (evn.menuName == "PauseMenu" || evn.menuName == "LoadingMenu") {
			Configs::LoadConfigs();
		}
	}
	return RE::BSEventNotifyControl::kContinue;
}
