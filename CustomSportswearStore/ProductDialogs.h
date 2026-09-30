#pragma once
#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <vector>
#include "Catalog.h"
#include "Widgets.h"

// Suggests a size from height/weight (apparel) or foot length (shoes).
class SizeAdvisorDialog : public wxDialog {
public:
    SizeAdvisorDialog(wxWindow* parent, const Product& product);
    int Recommended() const { return m_recommended; }  // index into product.sizes

private:
    void Recalculate();

    const Product& m_product;
    int m_recommended = 0;
    wxSpinCtrl* m_height = nullptr;
    wxSpinCtrl* m_weight = nullptr;
    wxSpinCtrlDouble* m_footLength = nullptr;
    wxStaticText* m_result = nullptr;
    wxStaticText* m_detail = nullptr;
};

// A whole team's jerseys in one go: one row per player (name, number, size).
// Rows can be added, removed, or pasted from a spreadsheet.
class TeamOrderDialog : public wxDialog {
public:
    struct Player {
        wxString name;
        int number;
        int sizeIndex;
    };

    TeamOrderDialog(wxWindow* parent, const Product& product, const Colorway& colorway, const wxString& team);

    std::vector<Player> Players() const;
    wxString TeamName() const;

private:
    struct Row {
        wxPanel* panel;
        wxStaticText* index;
        wxTextCtrl* name;
        wxSpinCtrl* number;
        wxChoice* size;
    };

    void AddRow(const wxString& name, int number, int sizeIndex);
    void RemoveRow(wxPanel* panel);
    void PasteRoster();
    void RefreshSummary();

    const Product& m_product;
    std::vector<Row> m_rows;
    wxScrolledWindow* m_list = nullptr;
    wxBoxSizer* m_listSizer = nullptr;
    wxTextCtrl* m_team = nullptr;
    wxStaticText* m_summary = nullptr;
    wxStaticText* m_warning = nullptr;
    Widgets::FlatButton* m_confirm = nullptr;
};
