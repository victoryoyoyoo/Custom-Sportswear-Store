#pragma once
#include <wx/string.h>

namespace Lang {
    bool English();
    void SetEnglish(bool english);
}

inline wxString L(const wxString& chinese, const wxString& english) {
    return Lang::English() ? english : chinese;
}

inline wxString Plural(size_t count, const wxString& one, const wxString& many) {
    return wxString::Format(wxT("%zu "), count) + (count == 1 ? one : many);
}
