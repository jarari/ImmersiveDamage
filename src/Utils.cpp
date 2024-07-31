#include "Utils.h"

REL::Relocation<uint32_t*> Utils::ptr_invalidhandle{ REL::ID(888641) };
REL::Relocation<float*> Utils::ptr_engineTime{ REL::ID(599343) };
REL::Relocation<float*> Utils::ptr_deltaTime{ REL::ID(1013228) };

RE::TESForm* Utils::GetFormFromMod(std::string modname, uint32_t formid) {
	if (!modname.length() || !formid)
		return nullptr;
	return RE::TESDataHandler::GetSingleton()->LookupForm(formid, modname);
}

std::string Utils::SplitString(const std::string str, const std::string delimiter, std::string& remainder)
{
	std::string ret;
	size_t i = str.find(delimiter);
	if (i == std::string::npos) {
		ret = str;
		remainder = "";
		return ret;
	}

	ret = str.substr(0, i);
	remainder = str.substr(i + 1);
	return ret;
}

float Utils::BSRandomFloat(float f1, float f2)
{
	typedef float (*FnBSRandomFloat)(float, float);
	REL::Relocation<FnBSRandomFloat> func{ REL::ID(1118937) };
	return func(f1, f2);
}

WCHAR* Utils::GetClipboard()
{
	WCHAR* strData{};

	if (OpenClipboard(NULL)) {
		HANDLE hClipboardData = GetClipboardData(CF_UNICODETEXT);
		if (hClipboardData) {
			WCHAR* pchData = (WCHAR*)GlobalLock(hClipboardData);
			if (pchData) {
				strData = pchData;
				GlobalUnlock(hClipboardData);
			}
		}
		CloseClipboard();
	}
	return strData;
}

uint32_t Translation::ReadLine_w(RE::BSResourceNiBinaryStream& stream, wchar_t* dst, uint32_t dstLen, uint32_t terminator)
{
	wchar_t* iter = dst;

	if (dstLen == 0)
		return 0;

	for (uint32_t i = 0; i < dstLen - 1; i++) {
		wchar_t data;

		if (stream.binary_read(&data, sizeof(data)) != sizeof(data))
			break;

		if (data == terminator)
			break;

		*iter++ = data;
	}

	// null terminate
	*iter = 0;

	return iter - dst;
}

void Translation::ParseTranslation(RE::BSScaleformTranslator* translator, std::string name)
{
	RE::Setting* setting = RE::INISettingCollection::GetSingleton()->GetSetting("sLanguage:General");
	if (!setting)
		setting = RE::INIPrefSettingCollection::GetSingleton()->GetSetting("sLanguage:General");
	std::string path = "Interface\\Translations\\";

	// Construct translation filename
	path += name;
	path += "_";
	path += (setting && setting->GetType() == RE::Setting::SETTING_TYPE::kString) ? setting->GetString() : "en";
	path += ".txt";

	if (!std::filesystem::exists(path))
		path = "Interface\\Translations\\" + name + "_en.txt";

	RE::BSResourceNiBinaryStream fileStream(path.c_str());

	// Check if file is empty, if not check if the BOM is UTF-16
	uint16_t bom = 0;
	uint32_t ret = (uint32_t)fileStream.binary_read(&bom, sizeof(uint16_t));
	if (ret == 0) {
		logger::error("Empty translation file.");
		return;
	}
	if (bom != 0xFEFF) {
		logger::error("BOM Error, file must be encoded in UCS-2 LE.");
		return;
	}

	while (true) {
		wchar_t buf[512];
		uint32_t len = ReadLine_w(fileStream, buf, sizeof(buf) / sizeof(buf[0]), '\n');
		if (len == 0)  // End of file
			return;

		// at least $ + wchar_t + \t + wchar_t
		if (len < 4 || buf[0] != '$')
			continue;

		wchar_t last = buf[len - 1];
		if (last == '\r')
			len--;

		// null terminate
		buf[len] = 0;

		uint32_t delimIdx = 0;
		for (uint32_t i = 0; i < len; i++)
			if (buf[i] == '\t')
				delimIdx = i;

		// at least $ + wchar_t
		if (delimIdx < 2)
			continue;

		// replace \t by \0
		buf[delimIdx] = 0;

		RE::BSFixedStringWCS key(buf);
		RE::BSFixedStringWCS translation(&buf[delimIdx + 1]);

		RE::BSTTuple<RE::BSFixedStringWCS, RE::BSFixedStringWCS> item(key, translation);
		translator->translator.translationMap.insert(item);
	}

	logger::info("Translation for {} injected.", name);
}
