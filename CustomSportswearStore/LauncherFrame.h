#pragma once
#include <wx/wx.h>
#include <functional>
#include <vector>
#include "Catalog.h"
#include "Theme.h"
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
    void ApplyFilter();  // category chip + favourites + search box
    void RefreshCartButton();

    wxPanel* m_root = nullptr;
    wxScrolledWindow* m_scroll = nullptr;
    wxGridSizer* m_grid = nullptr;
    std::vector<wxWindow*> m_cards;
    std::vector<Widgets::HeartToggle*> m_hearts;
    std::vector<Theme::ImagePanel*> m_pictures;
    std::vector<bool> m_lastMatches;  // which cards were shown last time
    wxString m_category;
    bool m_favoritesOnly = false;
    class wxSearchCtrl* m_search = nullptr;
    wxStaticText* m_empty = nullptr;
    wxStaticText* m_count = nullptr;
    Widgets::FlatButton* m_cartButton = nullptr;
    wxLongLong m_lastOpenMs = 0;  // last time a product page was opened
};
