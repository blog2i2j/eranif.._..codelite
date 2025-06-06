//////////////////////////////////////////////////////////////////////
// This file was auto-generated by codelite's wxCrafter Plugin
// wxCrafter project file: findinfiles_dlg.wxcp
// Do not modify this file by hand!
//////////////////////////////////////////////////////////////////////

#include "findinfiles_dlg.h"

// Declare the bitmap loading function
extern void wxCABC4InitBitmapResources();

namespace
{
// return the wxBORDER_SIMPLE that matches the current application theme
wxBorder get_border_simple_theme_aware_bit()
{
#if wxVERSION_NUMBER >= 3300 && defined(__WXMSW__)
    return wxSystemSettings::GetAppearance().IsDark() ? wxBORDER_SIMPLE : wxBORDER_DEFAULT;
#else
    return wxBORDER_DEFAULT;
#endif
} // DoGetBorderSimpleBit
bool bBitmapLoaded = false;
} // namespace

FindInFilesDialogBase::FindInFilesDialogBase(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos,
                                             const wxSize& size, long style)
    : wxDialog(parent, id, title, pos, size, style)
{
    if (!bBitmapLoaded) {
        // We need to initialise the default bitmap handler
        wxXmlResource::Get()->AddHandler(new wxBitmapXmlHandler);
        wxCABC4InitBitmapResources();
        bBitmapLoaded = true;
    }

    wxBoxSizer* boxSizer7 = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(boxSizer7);

    m_panelMainPanel =
        new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxTAB_TRAVERSAL);

    boxSizer7->Add(m_panelMainPanel, 1, wxALL | wxEXPAND, WXC_FROM_DIP(10));

    wxBoxSizer* boxSizer132 = new wxBoxSizer(wxHORIZONTAL);
    m_panelMainPanel->SetSizer(boxSizer132);

    wxBoxSizer* boxSizer178 = new wxBoxSizer(wxVERTICAL);

    boxSizer132->Add(boxSizer178, 1, wxALL | wxEXPAND, WXC_FROM_DIP(5));

    wxFlexGridSizer* fgSizer41 = new wxFlexGridSizer(0, 3, 0, 0);
    fgSizer41->SetFlexibleDirection(wxBOTH);
    fgSizer41->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);
    fgSizer41->AddGrowableCol(1);

    boxSizer178->Add(fgSizer41, 1, wxALL | wxEXPAND, WXC_FROM_DIP(5));

    m_staticText1 = new wxStaticText(m_panelMainPanel, wxID_ANY, _("Find :"), wxDefaultPosition,
                                     wxDLG_UNIT(m_panelMainPanel, wxSize(-1, -1)), 0);

    fgSizer41->Add(m_staticText1, 0, wxALL | wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, WXC_FROM_DIP(5));

    wxArrayString m_findStringArr;
    m_findString = new clThemedComboBox(m_panelMainPanel, wxID_ANY, wxT(""), wxDefaultPosition,
                                        wxDLG_UNIT(m_panelMainPanel, wxSize(-1, -1)), m_findStringArr, 0);
    m_findString->SetToolTip(_("Find what"));
    m_findString->SetFocus();
#if wxVERSION_NUMBER >= 3000
    m_findString->SetHint(_("Find what"));
#endif

    fgSizer41->Add(m_findString, 0, wxALL | wxEXPAND | wxALIGN_CENTER_VERTICAL, WXC_FROM_DIP(5));

    m_find = new wxButton(m_panelMainPanel, wxID_OK, _("&Find"), wxDefaultPosition,
                          wxDLG_UNIT(m_panelMainPanel, wxSize(-1, -1)), 0);
    m_find->SetDefault();
    m_find->SetToolTip(_("Begin search"));

    fgSizer41->Add(m_find, 0, wxALL | wxEXPAND, WXC_FROM_DIP(5));

    m_staticText102 = new wxStaticText(m_panelMainPanel, wxID_ANY, _("Replace:"), wxDefaultPosition,
                                       wxDLG_UNIT(m_panelMainPanel, wxSize(-1, -1)), 0);

    fgSizer41->Add(m_staticText102, 0, wxALL | wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, WXC_FROM_DIP(5));

    wxArrayString m_replaceStringArr;
    m_replaceString = new clThemedComboBox(m_panelMainPanel, wxID_ANY, wxT(""), wxDefaultPosition,
                                           wxDLG_UNIT(m_panelMainPanel, wxSize(-1, -1)), m_replaceStringArr, 0);
#if wxVERSION_NUMBER >= 3000
    m_replaceString->SetHint(_("Replace with"));
#endif

    fgSizer41->Add(m_replaceString, 0, wxALL | wxEXPAND | wxALIGN_CENTER_VERTICAL, WXC_FROM_DIP(5));

    m_replaceAll = new wxButton(m_panelMainPanel, wxID_REPLACE, _("&Replace"), wxDefaultPosition,
                                wxDLG_UNIT(m_panelMainPanel, wxSize(-1, -1)), 0);
    m_replaceAll->SetToolTip(
        _("Search for matches and place them in the 'Replace' window as candidates for possible replace operation"));

    fgSizer41->Add(m_replaceAll, 0, wxALL | wxEXPAND, WXC_FROM_DIP(5));

    m_staticText3 = new wxStaticText(m_panelMainPanel, wxID_ANY, _("Files:"), wxDefaultPosition,
                                     wxDLG_UNIT(m_panelMainPanel, wxSize(-1, -1)), 0);
    m_staticText3->SetToolTip(_("File extensions to include in the search\nWildcards are supported"));

    fgSizer41->Add(m_staticText3, 0, wxALL | wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, WXC_FROM_DIP(5));

    wxArrayString m_fileTypesArr;
    m_fileTypesArr.Add(_("*.c;*.cpp;*.cxx;*.cc;*.h;*.hpp;*.inc;*.mm;*.m;*.xrc"));
    m_fileTypes =
        new clThemedComboBox(m_panelMainPanel, wxID_ANY, wxT("*.c;*.cpp;*.cxx;*.cc;*.h;*.hpp;*.inc;*.mm;*.m;*.xrc"),
                             wxDefaultPosition, wxDLG_UNIT(m_panelMainPanel, wxSize(-1, -1)), m_fileTypesArr, 0);
    m_fileTypes->SetToolTip(_("Search these file types"));
#if wxVERSION_NUMBER >= 3000
    m_fileTypes->SetHint(wxT(""));
#endif
    m_fileTypes->SetSelection(0);

    fgSizer41->Add(m_fileTypes, 0, wxALL | wxEXPAND | wxALIGN_CENTER_VERTICAL, WXC_FROM_DIP(5));

    fgSizer41->Add(0, 0, 1, wxALL, WXC_FROM_DIP(5));

    m_staticText2 = new wxStaticText(m_panelMainPanel, wxID_ANY, _("Where:"), wxDefaultPosition,
                                     wxDLG_UNIT(m_panelMainPanel, wxSize(-1, -1)), 0);
    m_staticText2->SetToolTip(
        _("Search in these folders\nTo exclude a file from the search, use wildcard that starts with an hyphen "
          "(\"-\")\nFor example, to exclude all matches from the node_modules folder, one can use something "
          "like:\n\n/home/user/path/to/root/folder\n-*node_modules*"));

    fgSizer41->Add(m_staticText2, 0, wxALL | wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, WXC_FROM_DIP(5));

    wxArrayString m_comboBoxWhereArr;
    m_comboBoxWhere = new clThemedComboBox(m_panelMainPanel, wxID_ANY, wxT(""), wxDefaultPosition,
                                           wxDLG_UNIT(m_panelMainPanel, wxSize(-1, -1)), m_comboBoxWhereArr, 0);
#if wxVERSION_NUMBER >= 3000
    m_comboBoxWhere->SetHint(wxT(""));
#endif

    fgSizer41->Add(m_comboBoxWhere, 0, wxALL | wxEXPAND, WXC_FROM_DIP(5));

    m_btnAddPath = new wxButton(m_panelMainPanel, wxID_ANY, _("..."), wxDefaultPosition,
                                wxDLG_UNIT(m_panelMainPanel, wxSize(-1, -1)), 0);
    m_btnAddPath->SetToolTip(_("Add new search location"));

    fgSizer41->Add(m_btnAddPath, 0, wxALL | wxEXPAND, WXC_FROM_DIP(5));

    m_staticText5 = new wxStaticText(m_panelMainPanel, wxID_ANY, _("Encoding:"), wxDefaultPosition,
                                     wxDLG_UNIT(m_panelMainPanel, wxSize(-1, -1)), 0);

    fgSizer41->Add(m_staticText5, 0, wxALL | wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, WXC_FROM_DIP(5));

    wxArrayString m_comboBoxEncodingArr;
    m_comboBoxEncoding =
        new clThemedComboBox(m_panelMainPanel, wxID_ANY, wxT(""), wxDefaultPosition,
                             wxDLG_UNIT(m_panelMainPanel, wxSize(-1, -1)), m_comboBoxEncodingArr, wxCB_READONLY);

    fgSizer41->Add(m_comboBoxEncoding, 0, wxALL | wxEXPAND, WXC_FROM_DIP(5));

    m_cancel = new wxButton(m_panelMainPanel, wxID_CANCEL, _("Close"), wxDefaultPosition,
                            wxDLG_UNIT(m_panelMainPanel, wxSize(-1, -1)), 0);
    m_cancel->SetToolTip(_("Close this dialog"));

    fgSizer41->Add(m_cancel, 0, wxALL | wxEXPAND, WXC_FROM_DIP(5));

    wxGridBagSizer* gridBagSizer174 = new wxGridBagSizer(0, 0);

    boxSizer178->Add(gridBagSizer174, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, WXC_FROM_DIP(5));

    wxStaticBoxSizer* staticBoxSizer170 =
        new wxStaticBoxSizer(new wxStaticBox(m_panelMainPanel, wxID_ANY, _("Options:")), wxHORIZONTAL);

    gridBagSizer174->Add(staticBoxSizer170, wxGBPosition(0, 0), wxGBSpan(1, 2), wxALL | wxEXPAND | wxALIGN_LEFT,
                         WXC_FROM_DIP(5));

    m_matchCase = new wxCheckBox(m_panelMainPanel, wxID_ANY, _("Case"), wxDefaultPosition,
                                 wxDLG_UNIT(m_panelMainPanel, wxSize(-1, -1)), 0);
    m_matchCase->SetValue(false);
    m_matchCase->SetToolTip(_("Toggle case sensitive search"));

    staticBoxSizer170->Add(m_matchCase, 0, wxALL | wxEXPAND, WXC_FROM_DIP(5));

    m_matchWholeWord = new wxCheckBox(m_panelMainPanel, wxID_ANY, _("Word"), wxDefaultPosition,
                                      wxDLG_UNIT(m_panelMainPanel, wxSize(-1, -1)), 0);
    m_matchWholeWord->SetValue(false);
    m_matchWholeWord->SetToolTip(_("Toggle whole word search"));

    staticBoxSizer170->Add(m_matchWholeWord, 0, wxALL | wxEXPAND, WXC_FROM_DIP(5));

    m_regualrExpression = new wxCheckBox(m_panelMainPanel, wxID_ANY, _("Regex"), wxDefaultPosition,
                                         wxDLG_UNIT(m_panelMainPanel, wxSize(-1, -1)), 0);
    m_regualrExpression->SetValue(false);
    m_regualrExpression->SetToolTip(_("The 'Find What' field is a regular expression"));

    staticBoxSizer170->Add(m_regualrExpression, 0, wxALL | wxEXPAND, WXC_FROM_DIP(5));

    m_checkBoxPipeForGrep = new wxCheckBox(m_panelMainPanel, wxID_ANY, _("Pipe filter"), wxDefaultPosition,
                                           wxDLG_UNIT(m_panelMainPanel, wxSize(-1, -1)), 0);
    m_checkBoxPipeForGrep->SetValue(false);
    m_checkBoxPipeForGrep->SetToolTip(
        _("Use the pipe character (\"|\") as a special separator for applying additional filters. This has the similar "
          "effect as using the \"grep\" command line tool"));

    staticBoxSizer170->Add(m_checkBoxPipeForGrep, 0, wxALL, WXC_FROM_DIP(5));

    m_checkBoxSaveFilesBeforeSearching = new wxCheckBox(m_panelMainPanel, wxID_ANY, _("Save before"), wxDefaultPosition,
                                                        wxDLG_UNIT(m_panelMainPanel, wxSize(-1, -1)), 0);
    m_checkBoxSaveFilesBeforeSearching->SetValue(false);
    m_checkBoxSaveFilesBeforeSearching->SetToolTip(_("Save any modified files before search starts"));

    staticBoxSizer170->Add(m_checkBoxSaveFilesBeforeSearching, 0, wxALL | wxEXPAND, WXC_FROM_DIP(5));

    wxStaticBoxSizer* staticBoxSizer175 =
        new wxStaticBoxSizer(new wxStaticBox(m_panelMainPanel, wxID_ANY, _("File System:")), wxHORIZONTAL);

    gridBagSizer174->Add(staticBoxSizer175, wxGBPosition(1, 0), wxGBSpan(1, 1), wxALL | wxEXPAND, WXC_FROM_DIP(5));

    m_checkBoxFollowSymlinks = new wxCheckBox(m_panelMainPanel, wxID_ANY, _("Symlinks"), wxDefaultPosition,
                                              wxDLG_UNIT(m_panelMainPanel, wxSize(-1, -1)), 0);
    m_checkBoxFollowSymlinks->SetValue(false);
    m_checkBoxFollowSymlinks->SetToolTip(_("When enabled, follow symbolic links folders"));

    staticBoxSizer175->Add(m_checkBoxFollowSymlinks, 0, wxALL | wxEXPAND, WXC_FROM_DIP(5));

    m_checkBoxIncludeHiddenFolders = new wxCheckBox(m_panelMainPanel, wxID_ANY, _("Hidden folders"), wxDefaultPosition,
                                                    wxDLG_UNIT(m_panelMainPanel, wxSize(-1, -1)), 0);
    m_checkBoxIncludeHiddenFolders->SetValue(false);
    m_checkBoxIncludeHiddenFolders->SetToolTip(_("Search in hidden folders"));

    staticBoxSizer175->Add(m_checkBoxIncludeHiddenFolders, 0, wxALL, WXC_FROM_DIP(5));

    wxStaticBoxSizer* staticBoxSizer171 =
        new wxStaticBoxSizer(new wxStaticBox(m_panelMainPanel, wxID_ANY, _("Presets:")), wxHORIZONTAL);

    gridBagSizer174->Add(staticBoxSizer171, wxGBPosition(1, 1), wxGBSpan(1, 1), wxALL | wxEXPAND, WXC_FROM_DIP(5));

    m_checkBoxTODO = new wxCheckBox(m_panelMainPanel, wxID_ANY, _("TODO"), wxDefaultPosition,
                                    wxDLG_UNIT(m_panelMainPanel, wxSize(-1, -1)), 0);
    m_checkBoxTODO->SetValue(false);
    m_checkBoxTODO->SetToolTip(_("Search for TODO patterns in the code\nThis options enables regular expression"));

    staticBoxSizer171->Add(m_checkBoxTODO, 0, wxALL, WXC_FROM_DIP(5));

    m_checkBoxATTN = new wxCheckBox(m_panelMainPanel, wxID_ANY, _("ATTN"), wxDefaultPosition,
                                    wxDLG_UNIT(m_panelMainPanel, wxSize(-1, -1)), 0);
    m_checkBoxATTN->SetValue(false);
    m_checkBoxATTN->SetToolTip(_("Search for ATTN patterns in the code\nThis options enables regular expression"));

    staticBoxSizer171->Add(m_checkBoxATTN, 0, wxALL | wxEXPAND, WXC_FROM_DIP(5));

    m_checkBoxBUG = new wxCheckBox(m_panelMainPanel, wxID_ANY, _("BUG"), wxDefaultPosition,
                                   wxDLG_UNIT(m_panelMainPanel, wxSize(-1, -1)), 0);
    m_checkBoxBUG->SetValue(false);
    m_checkBoxBUG->SetToolTip(_("Search for BUG patterns in the code\nThis options enables regular expression"));

    staticBoxSizer171->Add(m_checkBoxBUG, 0, wxALL, WXC_FROM_DIP(5));

    m_checkBoxFIXME = new wxCheckBox(m_panelMainPanel, wxID_ANY, _("FIXME"), wxDefaultPosition,
                                     wxDLG_UNIT(m_panelMainPanel, wxSize(-1, -1)), 0);
    m_checkBoxFIXME->SetValue(false);
    m_checkBoxFIXME->SetToolTip(_("Search for FIXME patterns in the code\nThis options enables regular expression"));

    staticBoxSizer171->Add(m_checkBoxFIXME, 1, wxALL, WXC_FROM_DIP(5));
    gridBagSizer174->AddGrowableCol(0);
    gridBagSizer174->AddGrowableCol(1);

    SetName(wxT("FindInFilesDialogBase"));
    SetSize(wxDLG_UNIT(this, wxSize(-1, -1)));
    if (GetSizer()) {
        GetSizer()->Fit(this);
    }
    if (GetParent()) {
        CentreOnParent(wxBOTH);
    } else {
        CentreOnScreen(wxBOTH);
    }
    if (!wxPersistenceManager::Get().Find(this)) {
        wxPersistenceManager::Get().RegisterAndRestore(this);
    } else {
        wxPersistenceManager::Get().Restore(this);
    }
    // Connect events
    m_findString->Bind(wxEVT_COMMAND_TEXT_ENTER, &FindInFilesDialogBase::OnFindEnter, this);
    m_find->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &FindInFilesDialogBase::OnFind, this);
    m_find->Bind(wxEVT_UPDATE_UI, &FindInFilesDialogBase::OnFindWhatUI, this);
    m_replaceString->Bind(wxEVT_COMMAND_TEXT_ENTER, &FindInFilesDialogBase::OnReplaceEnter, this);
    m_replaceAll->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &FindInFilesDialogBase::OnReplace, this);
    m_replaceAll->Bind(wxEVT_UPDATE_UI, &FindInFilesDialogBase::OnReplaceUI, this);
    m_fileTypes->Bind(wxEVT_COMMAND_TEXT_ENTER, &FindInFilesDialogBase::OnFindEnter, this);
    m_comboBoxWhere->Bind(wxEVT_COMMAND_TEXT_ENTER, &FindInFilesDialogBase::OnFindEnter, this);
    m_btnAddPath->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &FindInFilesDialogBase::OnAddPath, this);
    m_comboBoxEncoding->Bind(wxEVT_COMMAND_TEXT_ENTER, &FindInFilesDialogBase::OnFindEnter, this);
    m_cancel->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &FindInFilesDialogBase::OnButtonClose, this);
    m_regualrExpression->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &FindInFilesDialogBase::OnRegex, this);
    m_checkBoxTODO->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &FindInFilesDialogBase::OnTODO, this);
    m_checkBoxATTN->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &FindInFilesDialogBase::OnATTN, this);
    m_checkBoxBUG->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &FindInFilesDialogBase::OnBUG, this);
    m_checkBoxFIXME->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &FindInFilesDialogBase::OnFIXME, this);
}

FindInFilesDialogBase::~FindInFilesDialogBase()
{
    m_findString->Unbind(wxEVT_COMMAND_TEXT_ENTER, &FindInFilesDialogBase::OnFindEnter, this);
    m_find->Unbind(wxEVT_COMMAND_BUTTON_CLICKED, &FindInFilesDialogBase::OnFind, this);
    m_find->Unbind(wxEVT_UPDATE_UI, &FindInFilesDialogBase::OnFindWhatUI, this);
    m_replaceString->Unbind(wxEVT_COMMAND_TEXT_ENTER, &FindInFilesDialogBase::OnReplaceEnter, this);
    m_replaceAll->Unbind(wxEVT_COMMAND_BUTTON_CLICKED, &FindInFilesDialogBase::OnReplace, this);
    m_replaceAll->Unbind(wxEVT_UPDATE_UI, &FindInFilesDialogBase::OnReplaceUI, this);
    m_fileTypes->Unbind(wxEVT_COMMAND_TEXT_ENTER, &FindInFilesDialogBase::OnFindEnter, this);
    m_comboBoxWhere->Unbind(wxEVT_COMMAND_TEXT_ENTER, &FindInFilesDialogBase::OnFindEnter, this);
    m_btnAddPath->Unbind(wxEVT_COMMAND_BUTTON_CLICKED, &FindInFilesDialogBase::OnAddPath, this);
    m_comboBoxEncoding->Unbind(wxEVT_COMMAND_TEXT_ENTER, &FindInFilesDialogBase::OnFindEnter, this);
    m_cancel->Unbind(wxEVT_COMMAND_BUTTON_CLICKED, &FindInFilesDialogBase::OnButtonClose, this);
    m_regualrExpression->Unbind(wxEVT_COMMAND_CHECKBOX_CLICKED, &FindInFilesDialogBase::OnRegex, this);
    m_checkBoxTODO->Unbind(wxEVT_COMMAND_CHECKBOX_CLICKED, &FindInFilesDialogBase::OnTODO, this);
    m_checkBoxATTN->Unbind(wxEVT_COMMAND_CHECKBOX_CLICKED, &FindInFilesDialogBase::OnATTN, this);
    m_checkBoxBUG->Unbind(wxEVT_COMMAND_CHECKBOX_CLICKED, &FindInFilesDialogBase::OnBUG, this);
    m_checkBoxFIXME->Unbind(wxEVT_COMMAND_CHECKBOX_CLICKED, &FindInFilesDialogBase::OnFIXME, this);
}

FindInFilesLocationsDlgBase::FindInFilesLocationsDlgBase(wxWindow* parent, wxWindowID id, const wxString& title,
                                                         const wxPoint& pos, const wxSize& size, long style)
    : wxDialog(parent, id, title, pos, size, style)
{
    if (!bBitmapLoaded) {
        // We need to initialise the default bitmap handler
        wxXmlResource::Get()->AddHandler(new wxBitmapXmlHandler);
        wxCABC4InitBitmapResources();
        bBitmapLoaded = true;
    }

    wxBoxSizer* boxSizer111 = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(boxSizer111);

    wxBoxSizer* boxSizer120 = new wxBoxSizer(wxHORIZONTAL);

    boxSizer111->Add(boxSizer120, 1, wxALL | wxEXPAND, WXC_FROM_DIP(5));

    wxArrayString m_checkListBoxLocationsArr;
    m_checkListBoxLocations = new wxCheckListBox(this, wxID_ANY, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)),
                                                 m_checkListBoxLocationsArr, wxLB_SINGLE);
    m_checkListBoxLocations->SetFocus();

    boxSizer120->Add(m_checkListBoxLocations, 1, wxALL | wxEXPAND, WXC_FROM_DIP(5));

    wxBoxSizer* boxSizer125 = new wxBoxSizer(wxVERTICAL);

    boxSizer120->Add(boxSizer125, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_TOP, WXC_FROM_DIP(5));

    m_buttonAdd = new wxButton(this, wxID_ANY, _("Add..."), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    m_buttonAdd->SetToolTip(_("Add Folder..."));

    boxSizer125->Add(m_buttonAdd, 0, wxALL | wxALIGN_CENTER_HORIZONTAL | wxALIGN_TOP, WXC_FROM_DIP(5));

    m_buttonDelete = new wxButton(this, wxID_ANY, _("Remove"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    m_buttonDelete->SetToolTip(_("Remove the selected path"));

    boxSizer125->Add(m_buttonDelete, 0, wxALL, WXC_FROM_DIP(5));

    m_stdBtnSizer113 = new wxStdDialogButtonSizer();

    boxSizer111->Add(m_stdBtnSizer113, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, WXC_FROM_DIP(10));

    m_button115 = new wxButton(this, wxID_OK, wxT(""), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    m_button115->SetDefault();
    m_stdBtnSizer113->AddButton(m_button115);

    m_button117 = new wxButton(this, wxID_CANCEL, wxT(""), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    m_stdBtnSizer113->AddButton(m_button117);
    m_stdBtnSizer113->Realize();

    SetName(wxT("FindInFilesLocationsDlgBase"));
    SetSize(wxDLG_UNIT(this, wxSize(-1, -1)));
    if (GetSizer()) {
        GetSizer()->Fit(this);
    }
    if (GetParent()) {
        CentreOnParent(wxBOTH);
    } else {
        CentreOnScreen(wxBOTH);
    }
    if (!wxPersistenceManager::Get().Find(this)) {
        wxPersistenceManager::Get().RegisterAndRestore(this);
    } else {
        wxPersistenceManager::Get().Restore(this);
    }
    // Connect events
    m_buttonAdd->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &FindInFilesLocationsDlgBase::OnAddPath, this);
    m_buttonDelete->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &FindInFilesLocationsDlgBase::OnDeletePath, this);
    m_buttonDelete->Bind(wxEVT_UPDATE_UI, &FindInFilesLocationsDlgBase::OnDeletePathUI, this);
}

FindInFilesLocationsDlgBase::~FindInFilesLocationsDlgBase()
{
    m_buttonAdd->Unbind(wxEVT_COMMAND_BUTTON_CLICKED, &FindInFilesLocationsDlgBase::OnAddPath, this);
    m_buttonDelete->Unbind(wxEVT_COMMAND_BUTTON_CLICKED, &FindInFilesLocationsDlgBase::OnDeletePath, this);
    m_buttonDelete->Unbind(wxEVT_UPDATE_UI, &FindInFilesLocationsDlgBase::OnDeletePathUI, this);
}
