#include "php_project_settings_data.h"

#include "clToolBar.h"
#include "globals.h"
#include "php_configuration_data.h"
#include "php_utils.h"
#include "php_workspace.h"

#include <map>
#include <set>
#include <wx/tokenzr.h>
#include <wx/uri.h>

PHPProjectSettingsData::PHPProjectSettingsData()
    : m_runAs(0)
    , m_flags(kOpt_PauseWhenExeTermiantes | kOpt_RunCurrentEditor)
{
}

PHPProjectSettingsData::~PHPProjectSettingsData() {}

wxArrayString PHPProjectSettingsData::GetIncludePathAsArray() const
{
    PHPProjectSettingsData s = *this;
    s.MergeWithGlobalSettings();
    wxArrayString paths = wxStringTokenize(s.m_includePath, "\r\n", wxTOKEN_STRTOK);
    return paths;
}

wxArrayString PHPProjectSettingsData::GetCCIncludePathAsArray() const
{
    PHPProjectSettingsData s = *this;
    s.MergeWithGlobalSettings();
    wxArrayString paths = wxStringTokenize(s.m_ccIncludePath, "\r\n", wxTOKEN_STRTOK);
    return paths;
}

void PHPProjectSettingsData::FromJSON(const JSONItem& ele)
{
    m_runAs = ele.namedObject("m_runAs").toInt(0);
    m_phpExe = ele.namedObject("m_phpExe").toString();
    m_indexFile = ele.namedObject("m_indexFile").toString();
    m_args = ele.namedObject("m_args").toString();
    m_workingDirectory = ele.namedObject("m_workingDirectory").toString(::wxGetCwd());
    m_projectURL = ele.namedObject("m_projectURL").toString();
    m_includePath = ele.namedObject("m_includePath").toString();
    m_ccIncludePath = ele.namedObject("m_ccIncludePath").toString();
    m_flags = ele.namedObject("m_flags").toSize_t(m_flags);
    m_phpIniFile = ele.namedObject("m_phpIniFile").toString();
    m_fileMapping = ele.namedObject("m_fileMapping").toStringMap();
}

JSONItem PHPProjectSettingsData::ToJSON() const
{
    JSONItem settings = JSONItem::createObject("settings");
    settings.addProperty("m_runAs", m_runAs);
    settings.addProperty("m_phpExe", m_phpExe);
    settings.addProperty("m_indexFile", m_indexFile);
    settings.addProperty("m_args", m_args);
    settings.addProperty("m_workingDirectory", m_workingDirectory);
    settings.addProperty("m_projectURL", m_projectURL);
    settings.addProperty("m_includePath", m_includePath);
    settings.addProperty("m_ccIncludePath", m_ccIncludePath);
    settings.addProperty("m_flags", m_flags);
    settings.addProperty("m_phpIniFile", m_phpIniFile);
    settings.addProperty("m_fileMapping", m_fileMapping);
    return settings;
}

void PHPProjectSettingsData::MergeWithGlobalSettings()
{
    PHPConfigurationData globalData;
    globalData.Load();

    if(GetPhpExe().IsEmpty()) {
        SetPhpExe(globalData.GetPhpExe());
    }

    // Append the include paths (keep uniqueness)
    wxArrayString paths = ::wxStringTokenize(m_includePath, "\r\n", wxTOKEN_STRTOK);
    const wxArrayString& globalIncPaths = globalData.GetIncludePaths();
    for(size_t i = 0; i < globalIncPaths.GetCount(); ++i) {
        wxString path = wxFileName(globalIncPaths.Item(i), "").GetPath(wxPATH_UNIX | wxPATH_GET_VOLUME);
        if(paths.Index(path) == wxNOT_FOUND) {
            paths.Add(path);
        }
    }

    // Append the code completion paths (keep uniqueness)
    wxArrayString cc_paths = ::wxStringTokenize(m_ccIncludePath, "\r\n", wxTOKEN_STRTOK);
    const wxArrayString& globalCCIncPaths = globalData.GetCcIncludePath();
    for(size_t i = 0; i < globalCCIncPaths.GetCount(); ++i) {
        wxString ccpath = wxFileName(globalCCIncPaths.Item(i), "").GetPath(wxPATH_UNIX | wxPATH_GET_VOLUME);
        if(cc_paths.Index(ccpath) == wxNOT_FOUND) {
            cc_paths.Add(ccpath);
        }
    }

    m_includePath = ::wxJoin(paths, '\n');
    m_ccIncludePath = ::wxJoin(cc_paths, '\n');
}

wxString PHPProjectSettingsData::GetMappdPath(const wxString& sourcePath, bool useUrlScheme,
                                              const wxStringMap_t& additionalMapping) const
{
    wxFileName fnSource(sourcePath);
    wxStringMap_t fullMapping;
    fullMapping.insert(m_fileMapping.begin(), m_fileMapping.end());
    fullMapping.insert(additionalMapping.begin(), additionalMapping.end());

    wxString sourceFullPath = fnSource.GetFullPath();
    for (const auto& p : fullMapping) {
        if (sourceFullPath.StartsWith(p.first)) {
            sourceFullPath.Remove(0, p.first.length());
            sourceFullPath.Prepend(p.second + "/");
            sourceFullPath.Replace("\\", "/");
            while(sourceFullPath.Replace("//", "/")) {}

            if(useUrlScheme) {
                sourceFullPath = ::FileNameToURI(sourceFullPath);
            }
            return sourceFullPath;
        }
    }

    if(useUrlScheme) {

        wxString asUrlScheme = sourcePath;
        asUrlScheme.Replace("\\", "/");

        while(asUrlScheme.Replace("//", "/"))
            ;

        asUrlScheme = ::FileNameToURI(asUrlScheme);
        return asUrlScheme;

    } else {
        wxString filePath;
        if(sourcePath.Contains(" ")) {
            filePath = sourcePath;
            filePath.Prepend('"').Append('"');
        }
        // return the path without changing it
        return filePath;
    }
}
