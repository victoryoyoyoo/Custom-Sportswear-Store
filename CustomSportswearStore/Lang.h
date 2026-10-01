#pragma once
#include <wx/string.h>

// The two languages of the interface. Every piece of text is written in both,
// next to each other, and L() hands back the one in use, so a translation can
// never drift away from the original.
//
// The first time, the language is the system's (Chinese on a Chinese Windows,
// English elsewhere); after that it is whatever was last picked on the
// welcome screen, the only place it can change.
namespace Lang {
    bool English();
    void SetEnglish(bool english);
}

inline wxString L(const wxString& chinese, const wxString& english) {
    return Lang::English() ? english : chinese;
}

// "1 item", "3 items" (English only; Chinese has no plural).
inline wxString Plural(size_t count, const wxString& one, const wxString& many) {
    return wxString::Format(wxT("%zu "), count) + (count == 1 ? one : many);
}
