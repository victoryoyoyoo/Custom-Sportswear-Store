#include "Lang.h"
#include <wx/config.h>
#include <wx/intl.h>

namespace {
    // First run: the system's language. After that: whatever was last chosen.
    bool StartsInEnglish() {
        long saved = -1;
        if (wxConfigBase* config = wxConfigBase::Get()) config->Read(wxT("Language/English"), &saved);
        if (saved >= 0) return saved != 0;
        const wxLanguageInfo* info = wxLocale::GetLanguageInfo(wxLocale::GetSystemLanguage());
        return !(info && info->CanonicalName.StartsWith(wxT("zh")));
    }

    int g_english = -1;  // -1 until first asked (the config needs the app to exist)
}

namespace Lang {
    bool English() {
        if (g_english < 0) g_english = StartsInEnglish() ? 1 : 0;
        return g_english == 1;
    }

    void SetEnglish(bool english) {
        g_english = english ? 1 : 0;
        if (wxConfigBase* config = wxConfigBase::Get()) {
            config->Write(wxT("Language/English"), english ? 1L : 0L);
            config->Flush();
        }
    }
}
