#pragma once
#include <wx/wx.h>
#include <functional>
#include <vector>
#include "Catalog.h"
#include "Widgets.h"

// Product list: one card per catalogue product, plus the cart shortcut.
// Product pages are opened as children of this frame and show it again when
// they close.
class LauncherFrame : public wxFrame {
public:
    LauncherFrame();

private:
    wxWindow* MakeProductCard(wxWindow* parent, int productIndex);
    void OpenProduct(int productIndex);
    void ApplyFilter(const wxString& category);  // empty = everything
    void RefreshCartButton();

    wxPanel* m_root = nullptr;
    wxGridSizer* m_grid = nullptr;
    std::vector<wxWindow*> m_cards;
    wxStaticText* m_count = nullptr;
    Widgets::FlatButton* m_cartButton = nullptr;
    bool m_opening = false;
};
