//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// copyright            : (C) 2008 by Eran Ifrah
// file name            : gizmos.h
//
// -------------------------------------------------------------------------
// A
//              _____           _      _     _ _
//             /  __ \         | |    | |   (_) |
//             | /  \/ ___   __| | ___| |    _| |_ ___
//             | |    / _ \ / _  |/ _ \ |   | | __/ _ )
//             | \__/\ (_) | (_| |  __/ |___| | ||  __/
//              \____/\___/ \__,_|\___\_____/_|\__\___|
//
//                                                  F i l e
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
#ifndef GIZMOS_H
#define GIZMOS_H

#include "newclassdlg.h"
#include "newwxprojectinfo.h"
#include "plugin.h"

#include <vector>

class WizardsPlugin : public IPlugin
{
    void CreateClass(NewClassInfo& info);
    std::vector<wxMenuItem*> m_vdDynItems;
    wxString m_folderpath;

protected:
    void DoPopupButtonMenu(wxPoint pt);
    void OnFolderContentMenu(clContextMenuEvent& event);

public:
    WizardsPlugin(IManager* manager);
    ~WizardsPlugin() override;

    //--------------------------------------------
    // Abstract methods
    //--------------------------------------------
    void CreateToolBar(clToolBarGeneric* toolbar) override;
    void CreatePluginMenu(wxMenu* pluginsMenu) override;
    void HookPopupMenu(wxMenu* menu, MenuType type) override;
    void UnPlug() override;

    void DoCreateNewPlugin();
    void DoCreateNewClass();
    // event handlers
    virtual void OnNewPlugin(wxCommandEvent& e);
    virtual void OnNewClass(wxCommandEvent& e);
    virtual void OnNewClassUI(wxUpdateUIEvent& e);
    virtual void OnNewPluginUI(wxUpdateUIEvent& e);

    // event handlers
    virtual void OnGizmos(wxCommandEvent& e);
#if USE_AUI_TOOLBAR
    virtual void OnGizmosAUI(wxAuiToolBarEvent& e);
#endif
    virtual void OnGizmosUI(wxUpdateUIEvent& e);
};

#endif // GIZMOS_H
