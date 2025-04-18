//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// copyright            : (C) 2014 Eran Ifrah
// file name            : cl_config.h
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

#ifndef CLCONFIG_H
#define CLCONFIG_H

#include "JSON.h"
#include "codelite_exports.h"

#include <functional>
#include <map>
#include <memory>
#include <wx/filename.h>

////////////////////////////////////////////////////////

class WXDLLIMPEXP_CL clConfigItem
{
protected:
    wxString m_name;

public:
    clConfigItem(const wxString& name)
        : m_name(name)
    {
    }

    virtual ~clConfigItem()
    {
    }

    const wxString& GetName() const
    {
        return m_name;
    }

    void SetName(const wxString& name)
    {
        this->m_name = name;
    }
    virtual void FromJSON(const JSONItem& json) = 0;
    virtual JSONItem ToJSON() const = 0;
};

////////////////////////////////////////////////////////

#define kConfigLLDBTooltipW "LLDBTooltipW"
#define kConfigLLDBTooltipH "LLDBTooltipH"
#define kConfigBuildAutoScroll "build-auto-scroll"
#define kConfigCreateVirtualFoldersOnDisk "CreateVirtualFoldersOnDisk"
#define kConfigLogVerbosity "LogVerbosity"
#define kConfigRedirectLogOutput "RedirectLogOutput"
#define kConfigSingleInstance "SingleInstance"
#define kConfigCheckForNewVersion "CheckForNewVersion"
#define kConfigMaxItemsInFindReplaceDialog "MaxItemsInFindReplaceDialog"
#define kConfigMaxOpenedTabs "MaxOpenedTabs"
#define kConfigRestoreLastSession "RestoreLastSession"
#define kConfigFrameTitlePattern "FrameTitlePattern"
#define kConfigStatusbarShowLine "StatusbarShowLine"
#define kConfigStatusbarShowColumn "StatusbarShowColumn"
#define kConfigStatusbarShowLineCount "StatusbarShowLineCount"
#define kConfigStatusbarShowPosition "StatusbarShowPosition"
#define kConfigStatusbarShowLength "StatusbarShowLength"
#define kConfigStatusbarShowSelectedChars "StatusbarShowSelChars"
#define kConfigStatusbarShowSelectedLines "StatusbarShowSelLines"
#define kConfigToolbarGroupSpacing "ToolbarGroupSpacing"
#define kConfigAutoDetectCompilerOnStartup "AutoDetectCompilerOnStartup"
#define kConfigBootstrapCompleted "BootstrapCompleted"
#define kConfigUpdateParserPaths "updateParserPaths"
#define kConfigShowToolBar "ShowToolBar"
#define kConfigShowTabBar "ShowTabBar"
#define kConfigShowMenuBar "ShowMenuBar"
#define kConfigShowStatusBar "ShowStatusBar"
#define kConfigWorkspaceTabSashPosition "WorkspaceTabSashPosition"
#define kConfigTabsPaneSortAlphabetically "TabsPaneSortAlphabetically"
#define kConfigFileExplorerBookmarks "FileExplorerBookmarks"
#define kRealPathResolveSymlinks "RealPathResolveSymlinks"

class WXDLLIMPEXP_CL clConfig
{
protected:
    wxFileName m_filename;
    std::unique_ptr<JSON> m_root;
    std::map<wxString, wxArrayString> m_cacheRecentItems;

protected:
    void DoDeleteProperty(const wxString& property);
    JSONItem GetGeneralSetting();

    void DoAddRecentItem(const wxString& propName, const wxString& filename);
    wxArrayString DoGetRecentItems(const wxString& propName) const;
    void DoClearRecentItems(const wxString& propName);

public:
    // We provide a global configuration
    // and the ability to allocate a private copy with a different file
    clConfig(const wxString& filename = "codelite.conf");
    ~clConfig() = default;
    static clConfig& Get();

    // Re-read the content from the disk
    void Reload();
    // Save the content to a give file name
    void Save(const wxFileName& fn);
    // Save the content the file passed on the construction
    void Save();

    // Utility functions
    //------------------------------

    // Merge 2 arrays of strings into a single array with all duplicate entries removed
    wxArrayString MergeArrays(const wxArrayString& arr1, const wxArrayString& arr2) const;
    wxStringMap_t MergeStringMaps(const wxStringMap_t& map1, const wxStringMap_t& map2) const;
    // Workspace history
    void AddRecentWorkspace(const wxString& filename)
    {
        DoAddRecentItem("RecentWorkspaces", filename);
    }
    wxArrayString GetRecentWorkspaces() const
    {
        return DoGetRecentItems("RecentWorkspaces");
    }
    void ClearRecentWorkspaces()
    {
        DoClearRecentItems("RecentWorkspaces");
    }

    // File history
    void AddRecentFile(const wxString& filename)
    {
        DoAddRecentItem("RecentFiles", filename);
    }
    wxArrayString GetRecentFiles() const
    {
        return DoGetRecentItems("RecentFiles");
    }
    void ClearRecentFiles()
    {
        DoClearRecentItems("RecentFiles");
    }

    // Workspace tab order
    //------------------------------
    void SetWorkspaceTabOrder(const wxArrayString& tabs, int selected);
    bool GetWorkspaceTabOrder(wxArrayString& tabs, int& selected);

    // Output tab order
    //------------------------------
    void SetOutputTabOrder(const wxArrayString& tabs, int selected);
    bool GetOutputTabOrder(wxArrayString& tabs, int& selected);

    // General objects
    // -----------------------------
    bool ReadItem(clConfigItem* item, const wxString& differentName = wxEmptyString);
    void WriteItem(const clConfigItem* item, const wxString& differentName = wxEmptyString);
    // bool
    bool Read(const wxString& name, bool defaultValue);
    void Write(const wxString& name, bool value);
    // int
    int Read(const wxString& name, int defaultValue);
    void Write(const wxString& name, int value);
    // wxString
    wxString Read(const wxString& name, const wxString& defaultValue);
    void Write(const wxString& name, const wxString& value);

    // wxArrayString
    wxArrayString Read(const wxString& name, const wxArrayString& defaultValue);
    void Write(const wxString& name, const wxArrayString& value);

    // wxFont
    wxFont Read(const wxString& name, const wxFont& defaultValue);
    void Write(const wxString& name, const wxFont& value);

    // wxColour
    wxColour Read(const wxString& name, const wxColour& defaultValue);
    void Write(const wxString& name, const wxColour& value);

    // Custom items, using lambda
    // A general purpose method that writes JSONItem created by a user defined function
    //
    // Write("my_item_name", []() -> JSONItem {
    //          JSONItem item = JSON::createObject();
    //          item.addProperty("name", "eran");
    //          item.addProperty("last_name", "ifrah");
    //          return item;
    //      });
    // Note: if configFile is valid, parameter is IGNORED and the JSONItem is written ino the configFile instead
    // as an anonymous object, e.g. if configFile is set to /tmp/my_config.json, then its content will be written
    // like this:
    //  {
    //      "name" :  "eran",
    //      "last_name" : "ifrah"
    //  }
    bool Write(const wxString& name, std::function<JSONItem()> serialiser_func, const wxFileName& configFile = {});

    // read custom items, using lambda:
    // Read("my_item_name", [&my_struct](const JSONItem& item) {
    //      my_struct.name = item["name"].toString();
    //      my_struct.last_name = item["last_name"].toString();
    //  });
    // Note: if configFile is valid, `name` is ignored (see comment for Write() method above)
    void Read(const wxString& name, std::function<void(const JSONItem& item)> deserialiser_func,
              const wxFileName& configFile = {});

    // Quick Find Bar history
    wxArrayString GetQuickFindSearchItems() const;
    wxArrayString GetQuickFindReplaceItems() const;
    void SetQuickFindSearchItems(const wxArrayString& items);
    void SetQuickFindReplaceItems(const wxArrayString& items);

    // standard IDs for annoying dialogs
    int GetAnnoyingDlgAnswer(const wxString& name, int defaultValue = wxNOT_FOUND);
    void SetAnnoyingDlgAnswer(const wxString& name, int value);
    void ClearAnnoyingDlgAnswers();
};

#endif // CLCONFIG_H
