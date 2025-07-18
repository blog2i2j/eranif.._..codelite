//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// copyright            : (C) 2008 by Eran Ifrah
// file name            : manager.cpp
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
#include "manager.h"

#include "AsyncProcess/asyncprocess.h"
#include "AsyncProcess/processreaderthread.h"
#include "BreakpointsView.hpp"
#include "BuildTab.hpp"
#include "CompilersModifiedDlg.h"
#include "Console/clConsoleBase.h"
#include "Debugger/debuggersettings.h"
#include "Debugger/memoryview.h"
#include "DebuggerCallstackView.h"
#include "FileSystemWorkspace/clFileSystemWorkspace.hpp"
#include "Keyboard/clKeyboardManager.h"
#include "NewProjectDialog.h"
#include "SideBar.hpp"
#include "StdToWX.h"
#include "WorkspaceImporter/WSImporter.h"
#include "app.h"
#include "attachdbgprocdlg.h"
#include "build_settings_config.h"
#include "buildmanager.h"
#include "clFileSystemEvent.h"
#include "clProfileHandler.h"
#include "clRemoteHost.hpp"
#include "clSFTPManager.hpp"
#include "clStrings.h"
#include "clWorkspaceManager.h"
#include "clWorkspaceView.h"
#include "cl_command_event.h"
#include "cl_editor.h"
#include "clean_request.h"
#include "compile_request.h"
#include "context_manager.h"
#include "ctags_manager.h"
#include "custombuildrequest.h"
#include "debuggerasciiviewer.h"
#include "debuggerconfigtool.h"
#include "dirsaver.h"
#include "dockablepanemenumanager.h"
#include "editor_config.h"
#include "environmentconfig.h"
#include "envvarlist.h"
#include "event_notifier.h"
#include "exelocator.h"
#include "file_logger.h"
#include "fileextmanager.h"
#include "fileutils.h"
#include "frame.h"
#include "globals.h"
#include "language.h"
#include "localstable.h"
#include "macromanager.h"
#include "macros.h"
#include "menumanager.h"
#include "new_quick_watch_dlg.h"
#include "pluginmanager.h"
#include "reconcileproject.h"
#include "renamefiledlg.h"
#include "search_thread.h"
#include "sessionmanager.h"
#include "simpletable.h"
#include "tabgroupmanager.h"
#include "threadlistpanel.h"
#include "workspacetab.h"
#include "wxCodeCompletionBoxManager.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <wx/arrstr.h>
#include <wx/aui/auibook.h>
#include <wx/busyinfo.h>
#include <wx/dir.h>
#include <wx/file.h>
#include <wx/msgdlg.h>
#include <wx/progdlg.h>
#include <wx/regex.h>
#include <wx/stdpaths.h>
#include <wx/tokenzr.h>

#ifndef __WXMSW__
#include <sys/wait.h>
#endif

//---------------------------------------------------------------
// Debugger helper method
//---------------------------------------------------------------
namespace
{
wxArrayString DoGetTemplateTypes(const wxString& tmplDecl)
{
    wxArrayString types;
    int depth(0);
    wxString type;

    wxString tmpstr(tmplDecl);
    tmpstr.Trim().Trim(false);

    if (tmpstr.StartsWith(wxT("<"))) {
        tmpstr.Remove(0, 1);
    }

    if (tmpstr.EndsWith(wxT(">"))) {
        tmpstr.RemoveLast();
    }
    tmpstr.Trim().Trim(false);

    for (size_t i = 0; i < tmpstr.Length(); i++) {
        wxChar ch = tmpstr.GetChar(i);
        switch (ch) {
        case wxT(','):
            if (depth > 0) {
                type << wxT(",");
            } else {
                type.Trim().Trim(false);
                if (type.Contains(wxT("std::basic_string<char"))) {
                    type = wxT("string");
                } else if (type.Contains(wxT("std::basic_string<wchar_t"))) {
                    type = wxT("wstring");
                }
                types.Add(type);
                type.Empty();
            }
            break;
        case wxT('<'):
            depth++;
            type << wxT("<");
            break;
        case wxT('>'):
            depth--;
            type << wxT(">");
            break;
        default:
            type << tmpstr.GetChar(i);
            break;
        }
    }

    if (depth == 0 && type.IsEmpty() == false) {
        type.Trim().Trim(false);
        if (type.Contains(wxT("std::basic_string<char"))) {
            type = wxT("string");
        } else if (type.Contains(wxT("std::basic_string<wchar_t"))) {
            type = wxT("wstring");
        }
        types.Add(type);
    }

    return types;
}

const wxString CLANGD_FILE_CONTENT = R"EOF(
CompileFlags:
    Add:
        - "-ferror-limit=0"
---
If:
    PathMatch: .*\.h
CompileFlags:
    Add:
        - "-xc++"
)EOF";

const wxString CLANG_FORMAT_FILE_CONTENT = R"EOF(
---
Language:        Cpp
AccessModifierOffset: -4
AlignEscapedNewlinesLeft: true
AlignTrailingComments: true
AllowAllParametersOfDeclarationOnNextLine: true
AllowShortBlocksOnASingleLine: true
AllowShortFunctionsOnASingleLine: true
#AllowShortIfStatementsOnASingleLine: true
AllowShortLoopsOnASingleLine: false
AlwaysBreakBeforeMultilineStrings: false
AlwaysBreakTemplateDeclarations: false
BinPackParameters: true
BreakBeforeBraces: Linux
BreakBeforeTernaryOperators: true
BreakConstructorInitializersBeforeComma: true
ColumnLimit: 120
ConstructorInitializerAllOnOneLineOrOnePerLine: false
ConstructorInitializerIndentWidth: 4
ContinuationIndentWidth: 4
Cpp11BracedListStyle: false
DerivePointerAlignment: false
DisableFormat:   false
ExperimentalAutoDetectBinPacking: false
ForEachMacros:   [ foreach, Q_FOREACH, BOOST_FOREACH ]
IndentCaseLabels: false
IndentWidth:     4
IndentWrappedFunctionNames: false
KeepEmptyLinesAtTheStartOfBlocks: true
MaxEmptyLinesToKeep: 1
NamespaceIndentation: Inner
ObjCSpaceAfterProperty: true
ObjCSpaceBeforeProtocolList: true
PenaltyBreakBeforeFirstCallParameter: 19
PenaltyBreakComment: 300
PenaltyBreakFirstLessLess: 120
PenaltyBreakString: 1000
PenaltyExcessCharacter: 1000000
PenaltyReturnTypeOnItsOwnLine: 60
PointerAlignment: Left
SpaceBeforeAssignmentOperators: true
SpaceBeforeParens: false
SpaceInEmptyParentheses: false
SpacesBeforeTrailingComments: 1
SpacesInAngles:  false
SpacesInContainerLiterals: true
SpacesInCStyleCastParentheses: false
SpacesInParentheses: false
Standard: C++11
TabWidth: 4
UseTab: Never
SortIncludes: true
IncludeBlocks: Regroup
)EOF";
} // namespace

//---------------------------------------------------------------
//
// The CodeLite manager class
//
//---------------------------------------------------------------

Manager::Manager()
    : m_shellProcess(NULL)
    , m_programProcess(NULL)
    , m_breakptsmgr(new BreakptMgr)
    , m_isShutdown(false)
    , m_dbgCanInteract(false)
    , m_useTipWin(false)
    , m_tipWinPos(wxNOT_FOUND)
    , m_frameLineno(wxNOT_FOUND)
    , m_watchDlg(NULL)
    , m_retagInProgress(false)
{
    Bind(wxEVT_RESTART_CODELITE, &Manager::OnRestart, this);
    Bind(wxEVT_FORCE_RESTART_CODELITE, &Manager::OnForcedRestart, this);
    Bind(wxEVT_ASYNC_PROCESS_OUTPUT, &Manager::OnProcessOutput, this);
    Bind(wxEVT_ASYNC_PROCESS_TERMINATED, &Manager::OnProcessEnd, this);

    EventNotifier::Get()->Connect(wxEVT_CMD_PROJ_SETTINGS_SAVED,
                                  clProjectSettingsEventHandler(Manager::OnProjectSettingsModified), NULL, this);
    EventNotifier::Get()->Connect(wxEVT_BUILD_ENDED, clBuildEventHandler(Manager::OnBuildEnded), NULL, this);
    EventNotifier::Get()->Connect(wxEVT_BUILD_STARTING, clBuildEventHandler(Manager::OnBuildStarting), NULL, this);
    EventNotifier::Get()->Connect(wxEVT_PROJ_RENAMED, clCommandEventHandler(Manager::OnProjectRenamed), NULL, this);
    EventNotifier::Get()->Bind(wxEVT_FINDINFILES_DLG_DISMISSED, &Manager::OnFindInFilesDismissed, this);
    EventNotifier::Get()->Bind(wxEVT_FINDINFILES_DLG_SHOWING, &Manager::OnFindInFilesShowing, this);

    EventNotifier::Get()->Bind(wxEVT_DEBUGGER_REFRESH_PANE, &Manager::OnUpdateDebuggerActiveView, this);
    EventNotifier::Get()->Bind(wxEVT_DEBUGGER_SET_MEMORY, &Manager::OnDebuggerSetMemory, this);
    EventNotifier::Get()->Bind(wxEVT_TOOLTIP_DESTROY, &Manager::OnHideGdbTooltip, this);
    EventNotifier::Get()->Bind(wxEVT_DEBUG_ENDING, &Manager::OnDebuggerStopping, this);
    EventNotifier::Get()->Bind(wxEVT_DEBUG_ENDED, &Manager::OnDebuggerStopped, this);
    EventNotifier::Get()->Bind(wxEVT_DEBUG_SET_FILELINE, &Manager::OnDebuggerAtFileLine, this);
    // Add new workspace type
    clWorkspaceManager::Get().RegisterWorkspace(new clCxxWorkspace());

    // Instantiate the profile manager
    clProfileHandler::Get();

#if USE_SFTP
    // Instantiate the remote host manager executor
    clRemoteHost::Instance();
#endif
}

Manager::~Manager()
{
    EventNotifier::Get()->Unbind(wxEVT_TOOLTIP_DESTROY, &Manager::OnHideGdbTooltip, this);
    Unbind(wxEVT_RESTART_CODELITE, &Manager::OnRestart, this);
    Unbind(wxEVT_FORCE_RESTART_CODELITE, &Manager::OnForcedRestart, this);
    Unbind(wxEVT_ASYNC_PROCESS_OUTPUT, &Manager::OnProcessOutput, this);
    Unbind(wxEVT_ASYNC_PROCESS_TERMINATED, &Manager::OnProcessEnd, this);

    EventNotifier::Get()->Disconnect(wxEVT_CMD_PROJ_SETTINGS_SAVED,
                                     clProjectSettingsEventHandler(Manager::OnProjectSettingsModified), NULL, this);
    EventNotifier::Get()->Disconnect(wxEVT_BUILD_ENDED, clBuildEventHandler(Manager::OnBuildEnded), NULL, this);
    EventNotifier::Get()->Disconnect(wxEVT_BUILD_STARTING, clBuildEventHandler(Manager::OnBuildStarting), NULL, this);
    EventNotifier::Get()->Disconnect(wxEVT_PROJ_RENAMED, clCommandEventHandler(Manager::OnProjectRenamed), NULL, this);
    EventNotifier::Get()->Unbind(wxEVT_FINDINFILES_DLG_DISMISSED, &Manager::OnFindInFilesDismissed, this);
    EventNotifier::Get()->Unbind(wxEVT_FINDINFILES_DLG_SHOWING, &Manager::OnFindInFilesShowing, this);
    EventNotifier::Get()->Unbind(wxEVT_DEBUGGER_REFRESH_PANE, &Manager::OnUpdateDebuggerActiveView, this);
    EventNotifier::Get()->Unbind(wxEVT_DEBUGGER_SET_MEMORY, &Manager::OnDebuggerSetMemory, this);
    EventNotifier::Get()->Unbind(wxEVT_DEBUG_ENDING, &Manager::OnDebuggerStopping, this);
    EventNotifier::Get()->Unbind(wxEVT_DEBUG_ENDED, &Manager::OnDebuggerStopped, this);
    EventNotifier::Get()->Unbind(wxEVT_DEBUG_SET_FILELINE, &Manager::OnDebuggerAtFileLine, this);

    // stop background processes
    IDebugger* debugger = DebuggerMgr::Get().GetActiveDebugger();

    if (debugger && debugger->IsRunning())
        DbgStop();
    {
        // wxLogNull noLog;
        SearchThreadST::Get()->Stop();
    }

    // free all plugins
    PluginManager::Get()->UnLoad();
    DebuggerMgr::Free();
    TagsManagerST::Free(); // it is important to release it *before* the TagsManager
    LanguageST::Free();
    clCxxWorkspaceST::Free();
    ContextManager::Free();
    BuildManagerST::Free();
    BuildSettingsConfigST::Free();
    SearchThreadST::Free();
    MenuManager::Free();
    EnvironmentConfig::Release();

#if USE_SFTP
    clRemoteHost::Release();
#endif

    wxDELETE(m_shellProcess);
    wxDELETE(m_breakptsmgr);
    TabGroupsManager::Free();
    clKeyboardManager::Release();
    wxCodeCompletionBoxManager::Free();
}

//--------------------------- Workspace Loading -----------------------------

bool Manager::IsWorkspaceOpen() const { return clCxxWorkspaceST::Get()->GetName().IsEmpty() == false; }

void Manager::CreateWorkspace(const wxString& name, const wxString& path)
{

    // make sure that the workspace pane is visible
    // ShowWorkspacePane(clMainFrame::Get()->GetWorkspaceTab()->GetCaption());

    wxString errMsg;
    bool res = clCxxWorkspaceST::Get()->CreateWorkspace(name, path, errMsg);
    if (!res) {
        wxMessageBox(errMsg, _("Error"), wxOK | wxICON_HAND);
        return;
    }

    OpenWorkspace(path + PATH_SEP + name + wxT(".workspace"));
}

void Manager::OpenWorkspace(const wxString& path)
{
    wxLogNull noLog;
    CloseWorkspace();

    wxString errMsg;
    bool res = clCxxWorkspaceST::Get()->OpenWorkspace(path, errMsg);
    if (!res) {
        // in case part of the workspace was opened, close the workspace
        CloseWorkspace();
        wxMessageBox(errMsg, _("Error"), wxOK | wxICON_HAND);
        return;
    }

    // OpenWorkspace returned true, but errMsg is not empty
    // this could only mean that we removed a fauly project
    if (errMsg.IsEmpty() == false) {
        clMainFrame::Get()->GetMessageBar()->DisplayMessage(errMsg, wxICON_ERROR);
    }

    if (GetActiveProjectName().IsEmpty()) {
        // This might happen if a removed faulty project was active
        wxArrayString list;
        clCxxWorkspaceST::Get()->GetProjectList(list);
        if (!list.IsEmpty()) {
            clCxxWorkspaceST::Get()->SetActiveProject(list.Item(0));
        }
    }

    DoSetupWorkspace(path);
    clGetManager()->ClosePage(_("Welcome!"));
}

void Manager::ReloadWorkspace()
{
    if (!IsWorkspaceOpen())
        return;

    // Save the current session before re-loading
    EventNotifier::Get()->NotifyWorkspaceReloadStartEvet(clCxxWorkspaceST::Get()->GetWorkspaceFileName().GetFullPath());

    DbgStop();
    clCxxWorkspaceST::Get()->ReloadWorkspace();
    DoSetupWorkspace(clCxxWorkspaceST::Get()->GetWorkspaceFileName().GetFullPath());
    EventNotifier::Get()->NotifyWorkspaceReloadEndEvent(clCxxWorkspaceST::Get()->GetWorkspaceFileName().GetFullPath());
}

void Manager::DoSetupWorkspace(const wxString& path)
{
    wxString errMsg;
    wxBusyCursor cursor;
    AddToRecentlyOpenedWorkspaces(path);

    // set the C++ workspace as the active one
    clWorkspaceManager::Get().SetWorkspace(clCxxWorkspaceST::Get());

    clWorkspaceEvent evtWorkspaceLoaded(wxEVT_WORKSPACE_LOADED);
    evtWorkspaceLoaded.SetFileName(path);
    evtWorkspaceLoaded.SetString(path);
    evtWorkspaceLoaded.SetWorkspaceType(clCxxWorkspaceST::Get()->GetWorkspaceType());
    EventNotifier::Get()->ProcessEvent(evtWorkspaceLoaded);

    // Update the refactoring cache
    std::vector<wxFileName> allfiles;
    GetWorkspaceFiles(allfiles, true);
    clGetManager()->LoadWorkspaceSession(path);

    // Update the parser search paths
    UpdateParserPaths(false);
    clMainFrame::Get()->SelectBestEnvSet();

    // send an event to the main frame indicating that a re-tag is required
    // we do this only if the "smart retagging" is on
    TagsOptionsData tagsopt = TagsManagerST::Get()->GetCtagsOptions();
    if (tagsopt.GetFlags() & CC_RETAG_WORKSPACE_ON_STARTUP) {
        wxCommandEvent e(wxEVT_COMMAND_MENU_SELECTED, XRCID("retag_workspace"));
        clMainFrame::Get()->GetEventHandler()->AddPendingEvent(e);
    }

    // Ensure that the "C++" view is selected
    clGetManager()->GetWorkspaceView()->SelectPage(clCxxWorkspaceST::Get()->GetWorkspaceType());
}

void Manager::CloseWorkspace()
{
    if (!IsShutdownInProgress()) {
        clWorkspaceEvent closing_event(wxEVT_WORKSPACE_CLOSING);
        EventNotifier::Get()->ProcessEvent(closing_event);
    }

    DbgClearWatches();

    // If we got a running debugging session - terminate it
    IDebugger* debugger = DebuggerMgr::Get().GetActiveDebugger();
    if (debugger && debugger->IsRunning())
        DbgStop();

    // save the current session before closing
    SessionEntry session;
    session.SetWorkspaceName(clCxxWorkspaceST::Get()->GetWorkspaceFileName().GetFullPath());
    clMainFrame::Get()->GetMainBook()->SaveSession(session);

    // Store the session
    GetBreakpointsMgr()->SaveSession(session);
    SessionManager::Get().Save(clCxxWorkspaceST::Get()->GetWorkspaceFileName().GetFullPath(), session);

    // Delete any breakpoints belong to the current workspace
    GetBreakpointsMgr()->DelAllBreakpoints();

    // since we closed the workspace, we also need to set the 'LastActiveWorkspaceName' to be
    // default
    SessionManager::Get().SetLastSession(wxT("Default"));
    clCxxWorkspaceST::Get()->CloseWorkspace();

#ifdef __WXMSW__
    // Under Windows, and in order to avoid locking the directory set the working directory back to the start up
    // directory
    wxSetWorkingDirectory(GetStartupDirectory());
#endif

    /////////////////////////////////////////////////////////////////
    // set back the "Default" environment variable as the active set
    /////////////////////////////////////////////////////////////////

    EnvVarList vars = EnvironmentConfig::Instance()->GetSettings();
    if (vars.IsSetExist(wxT("Default"))) {
        vars.SetActiveSet(wxT("Default"));
    }
    EnvironmentConfig::Instance()->SetSettings(vars);
    clMainFrame::Get()->SelectBestEnvSet();

    if (!IsShutdownInProgress()) {
        clWorkspaceEvent closed_event(wxEVT_WORKSPACE_CLOSED);
        EventNotifier::Get()->ProcessEvent(closed_event);
    }
}

void Manager::AddToRecentlyOpenedWorkspaces(const wxString& fileName)
{
    // Add this workspace to the history. Don't check for uniqueness:
    // if it's already on the list, wxFileHistory will move it to the top
    m_recentWorkspaces.AddFileToHistory(fileName);

    // sync between the history object and the configuration file
    clConfig::Get().AddRecentWorkspace(fileName);

    // The above call to AddFileToHistory() rewrote the Recent Workspaces menu
    // Unfortunately it rewrote it with path/to/foo.workspace, and we'd prefer
    // not to display the extension. So reload it again the way we like it :)
    clMainFrame::Get()->CreateRecentlyOpenedWorkspacesMenu();
}

void Manager::ClearWorkspaceHistory()
{
    size_t count = m_recentWorkspaces.GetCount();
    for (size_t i = 0; i < count; i++) {
        m_recentWorkspaces.RemoveFileFromHistory(0);
    }
    clConfig::Get().ClearRecentWorkspaces();
}

void Manager::GetRecentlyOpenedWorkspaces(wxArrayString& files) { files = clConfig::Get().GetRecentWorkspaces(); }

//--------------------------- Workspace Projects Mgmt -----------------------------

void Manager::CreateProject(ProjectData& data, const wxString& workspaceFolder)
{
    if (IsWorkspaceOpen() == false) {
        // create a workspace before creating a project
        CreateWorkspace(data.m_name, data.m_path);
    }

    wxString errMsg;
    bool res = clCxxWorkspaceST::Get()->CreateProject(data.m_name, data.m_path,
                                                      data.m_srcProject->GetSettings()->GetProjectType(wxEmptyString),
                                                      workspaceFolder, false, errMsg);
    if (!res) {
        wxMessageBox(errMsg, _("Error"), wxOK | wxICON_HAND);
        return;
    }
    ProjectPtr proj = clCxxWorkspaceST::Get()->FindProjectByName(data.m_name, errMsg);

    // copy the project settings to the new one
    proj->SetSettings(data.m_srcProject->GetSettings());

    proj->SetProjectInternalType(data.m_srcProject->GetProjectInternalType());

    // now add the new project to the build matrix
    clCxxWorkspaceST::Get()->AddProjectToBuildMatrix(proj);
    ProjectSettingsPtr settings = proj->GetSettings();

    // set the compiler type
    ProjectSettingsCookie cookie;
    BuildConfigPtr bldConf = settings->GetFirstBuildConfiguration(cookie);
    wxString projectType = settings->GetProjectType(wxEmptyString);
    BuilderPtr builder = BuildManagerST::Get()->GetBuilder(data.m_builderName);
    Builder::OptimalBuildConfig optimalConf;
    if (builder) {
        optimalConf = builder->GetOptimalBuildConfig(projectType);
    }
    while (bldConf) {
#ifndef __WXMSW__
        // The -mwindows linker flag is at best useless in !MSW, and breaks linking in the latest g++ (fedora17)
        wxString linkoptions = bldConf->GetLinkOptions();
        if (linkoptions.Contains(wxT("-mwindows;"))) {
            linkoptions.Replace(wxT("-mwindows;"), wxT(""));
        }
        // And again, without the ';' to catch an end-of-line one
        else if (linkoptions.Contains(wxT(";-mwindows"))) {
            linkoptions.Replace(wxT(";-mwindows"), wxT(""));
        }

        bldConf->SetLinkOptions(linkoptions);
#endif
        // Update the compiler according to the user selection
        bldConf->SetCompilerType(data.m_cmpType);

        // Update the debugger according to the user selection
        bldConf->SetDebuggerType(data.m_debuggerType);

        // Update the build system
        bldConf->SetBuildSystem(data.m_builderName);
        bldConf->SetIntermediateDirectory(optimalConf.intermediateDirectory);
        bldConf->SetOutputFileName(optimalConf.outputFile);
        bldConf->SetCommand(optimalConf.command);
        bldConf->SetWorkingDirectory(optimalConf.workingDirectory);

        // Make sure that the build configuration has a project type associated with it
        if (bldConf->GetProjectType().IsEmpty()) {
            bldConf->SetProjectType(projectType);
        }
        bldConf = settings->GetNextBuildConfiguration(cookie);
    }
    proj->SetSettings(settings);

    // copy the files as they appear in the source project
    proj->SetFiles(data.m_srcProject);

    // copy plugins data
    std::map<wxString, wxString> pluginsData;
    data.m_srcProject->GetAllPluginsData(pluginsData);
    proj->SetAllPluginsData(pluginsData);

    {
        // copy the actual files from the template directory to the new project path
        DirSaver ds;
        wxSetWorkingDirectory(proj->GetFileName().GetPath());

        wxString workingDirectory = wxGetCwd();
        // get list of files
        std::vector<wxFileName> files;
        data.m_srcProject->GetFilesAsVectorOfFileName(files);
        for (size_t i = 0; i < files.size(); i++) {
            wxFileName targetFile(workingDirectory, files[i].GetFullName());
            wxFileName sourceFile(files[i].GetFullPath());
            if (targetFile.FileExists()) {
                // Prompt the user
                wxString message;
                message << _("A file with similar name '") << targetFile.GetFullName() << _("' already exists") << "\n";
                message << _("Overwrite it?");
                if (wxYES != ::wxMessageBox(message, _("Warning"), wxYES_NO | wxCANCEL_DEFAULT | wxCANCEL)) {
                    continue;
                }
            }
            wxCopyFile(sourceFile.GetFullPath(), targetFile.GetFullPath());

            // Create .clangd file
            wxFileName clangd_file(proj->GetFileName());
            clangd_file.SetFullName(".clangd");
            FileUtils::WriteFileContent(clangd_file, CLANGD_FILE_CONTENT);

            // Create .clang-format file
            wxFileName clang_format_file(proj->GetFileName());
            clang_format_file.SetFullName(".clang-format");
            FileUtils::WriteFileContent(clang_format_file, CLANG_FORMAT_FILE_CONTENT);
        }
    }

    wxString projectName = proj->GetName();
    RetagProject(projectName, true);

    // Update the parser search paths
    CallAfter(&Manager::UpdateParserPaths, false);

    // Notify that a project was added
    clCommandEvent evtProjectAdded(wxEVT_PROJ_ADDED);
    evtProjectAdded.SetString(projectName);
    evtProjectAdded.SetEventObject(this);
    EventNotifier::Get()->AddPendingEvent(evtProjectAdded);
}

void Manager::AddProject(const wxString& path)
{
    // create a workspace if there is non
    if (IsWorkspaceOpen() == false) {

        wxFileName fn(path);

        // create a workspace before creating a project
        CreateWorkspace(fn.GetName(), fn.GetPath());
    }

    wxString errMsg;
    bool res = clCxxWorkspaceST::Get()->AddProject(path, errMsg);
    if (!res) {
        wxMessageBox(errMsg, _("Error"), wxOK | wxICON_HAND);
        return;
    }

    wxFileName fn(path);
    wxString projectName(fn.GetName());
    RetagProject(projectName, true);

    clCommandEvent evtProjectAdded(wxEVT_PROJ_ADDED);
    evtProjectAdded.SetString(projectName);
    evtProjectAdded.SetEventObject(this);
    EventNotifier::Get()->AddPendingEvent(evtProjectAdded);
}

void Manager::ReconcileProject(const wxString& projectName)
{
    wxCHECK_RET(IsWorkspaceOpen(), wxT("Trying to reconcile a project with no open workspace"));

    wxString projname = projectName;
    if (projname.empty()) {
        projname = GetActiveProjectName();
        wxCHECK_RET(!projname.empty(), wxT("Failed to find the active project"));
    }

    ReconcileProjectDlg dlg(clMainFrame::Get(),
                            projname.c_str()); // In theory the deep copy is unnecessary, but I got segs in the dtor...
    if (dlg.LoadData()) {
        dlg.ShowModal();
    }
}

void Manager::ImportMSVSSolution(const wxString& path, const wxString& defaultCompiler)
{
    wxFileName fn(path);
    if (fn.FileExists() == false) {
        return;
    }

    // Show some messages to the user
    wxBusyCursor busyCursor;
    wxBusyInfo info(_("Importing IDE solution/workspace..."), clMainFrame::Get());

    wxString errMsg;
    WSImporter importer;
    importer.Load(path, defaultCompiler);
    if (importer.Import(errMsg)) {
        wxString wspfile;
        wspfile << fn.GetPath() << wxT("/") << fn.GetName() << wxT(".workspace");
        OpenWorkspace(wspfile);

        // Retag workspace
        wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, XRCID("retag_workspace"));
        clMainFrame::Get()->GetEventHandler()->AddPendingEvent(event);
    } else {
        wxMessageBox(_("Solution/workspace unsupported"), wxMessageBoxCaptionStr, wxOK | wxCENTRE | wxSTAY_ON_TOP);
    }
}

bool Manager::RemoveProject(const wxString& name, bool notify)
{
    if (name.IsEmpty()) {
        return false;
    }

    ProjectPtr proj = GetProject(name);

    wxString errMsg;
    bool res = clCxxWorkspaceST::Get()->RemoveProject(name, errMsg, proj->GetWorkspaceFolder());
    if (!res) {
        wxMessageBox(errMsg, _("Error"), wxOK | wxICON_HAND);
        return false;
    }

    if (proj) {
        // remove symbols from the database
        std::vector<wxFileName> projectFiles;
        proj->GetFilesAsVectorOfFileName(projectFiles);
        wxArrayString prjfls;
        for (size_t i = 0; i < projectFiles.size(); i++) {
            prjfls.Add(projectFiles[i].GetFullPath());
        }

        if (notify) {
            clCommandEvent evtFileRemoved(wxEVT_PROJ_FILE_REMOVED);
            evtFileRemoved.SetStrings(prjfls);
            evtFileRemoved.SetString(proj->GetName());
            evtFileRemoved.SetEventObject(this);
            EventNotifier::Get()->ProcessEvent(evtFileRemoved);
        }
    }

    if (notify) {
        clCommandEvent evnt(wxEVT_PROJ_REMOVED);
        evnt.SetString(name);
        evnt.SetEventObject(this);
        EventNotifier::Get()->AddPendingEvent(evnt);
    }
    return true;
}

void Manager::GetProjectList(wxArrayString& list) { clCxxWorkspaceST::Get()->GetProjectList(list); }

ProjectPtr Manager::GetProject(const wxString& name) const
{
    wxString projectName(name);
    projectName.Trim().Trim(false);

    if (projectName.IsEmpty())
        return NULL;

    wxString errMsg;
    ProjectPtr proj = clCxxWorkspaceST::Get()->FindProjectByName(name, errMsg);
    if (!proj) {
        clLogMessage(errMsg);
        return NULL;
    }
    return proj;
}

wxString Manager::GetActiveProjectName() { return clCxxWorkspaceST::Get()->GetActiveProjectName(); }

void Manager::SetActiveProject(const wxString& name)
{
    clCxxWorkspaceST::Get()->SetActiveProject(name);
    clMainFrame::Get()->SelectBestEnvSet();
}

BuildMatrixPtr Manager::GetWorkspaceBuildMatrix() const { return clCxxWorkspaceST::Get()->GetBuildMatrix(); }

void Manager::SetWorkspaceBuildMatrix(BuildMatrixPtr matrix)
{
    clCxxWorkspaceST::Get()->SetBuildMatrix(matrix);

    // Notify about the configuration change to the plugins
    wxCommandEvent e(wxEVT_WORKSPACE_CONFIG_CHANGED);
    e.SetString(matrix->GetSelectedConfigurationName());
    EventNotifier::Get()->ProcessEvent(e);
}

//--------------------------- Workspace Files Mgmt -----------------------------
void Manager::GetWorkspaceFiles(wxArrayString& files)
{
    // Send an event to the plugins to get a list of the workspace files.
    // If no plugin has replied, use the default GetWorkspaceFiles method
    wxCommandEvent getFilesEevet(wxEVT_CMD_GET_WORKSPACE_FILES);
    getFilesEevet.SetEventObject(this);
    getFilesEevet.SetClientData(&files);
    if (EventNotifier::Get()->ProcessEvent(getFilesEevet)) {
        return;
    }

    if (clFileSystemWorkspace::Get().IsOpen()) {
        const auto& V = clFileSystemWorkspace::Get().GetFiles();
        if (V.empty()) {
            return;
        }
        files.Alloc(V.size());
        for (const auto& f : V) {
            files.Add(f.GetFullPath());
        }
        return;
    } else {
        if (!IsWorkspaceOpen()) {
            return;
        }

        wxArrayString projects;
        GetProjectList(projects);

        for (size_t i = 0; i < projects.GetCount(); i++) {
            GetProjectFiles(projects.Item(i), files);
        }
    }
}

void Manager::GetWorkspaceFiles(std::vector<wxFileName>& files, bool absPath)
{
    if (clFileSystemWorkspace::Get().IsOpen()) {
        const auto& V = clFileSystemWorkspace::Get().GetFiles();
        if (V.empty()) {
            return;
        }
        files.reserve(V.size());
        if (absPath) {
            files.insert(files.end(), V.begin(), V.end());
        } else {
            const wxFileName& fnWorkspace = clFileSystemWorkspace::Get().GetFileName();
            wxString path = fnWorkspace.GetPath();
            for (const auto& f : V) {
                wxFileName fn(f);
                fn.MakeRelativeTo(path);
                files.push_back(fn);
            }
        }
    } else {
        wxArrayString projects;
        GetProjectList(projects);
        for (size_t i = 0; i < projects.GetCount(); i++) {
            ProjectPtr p = GetProject(projects.Item(i));
            p->GetFilesAsVectorOfFileName(files, absPath);
        }
    }
}

void Manager::RetagWorkspace(TagsManager::RetagType type)
{
    if (!clWorkspaceManager::Get().IsWorkspaceOpened()) {
        return;
    }

    if (type == TagsManager::Retag_Quick) {
        TagsManagerST::Get()->ParseWorkspaceIncremental();
    } else {
        TagsManagerST::Get()->ParseWorkspaceFull(clWorkspaceManager::Get().GetWorkspace()->GetDir());
    }
}

void Manager::RetagFile(const wxString& filename) { wxUnusedVar(filename); }

//--------------------------- Project Files Mgmt -----------------------------

int Manager::AddVirtualDirectory(const wxString& virtualDirFullPath, bool createIt)
{
    if (clCxxWorkspaceST::Get()->IsVirtualDirectoryExists(virtualDirFullPath)) {
        return VD_EXISTS;
    }

    wxString errMsg;
    bool res = clCxxWorkspaceST::Get()->CreateVirtualDirectory(virtualDirFullPath, errMsg, createIt);
    if (!res) {
        wxMessageBox(errMsg, _("Error"), wxOK | wxICON_HAND);
        return VD_ERROR;
    }
    return VD_OK;
}

void Manager::RemoveVirtualDirectory(const wxString& virtualDirFullPath)
{
    wxString errMsg;
    wxString project = virtualDirFullPath.BeforeFirst(wxT(':'));
    ProjectPtr p = clCxxWorkspaceST::Get()->FindProjectByName(project, errMsg);
    if (!p) {
        return;
    }

    // Update symbol tree and database
    wxString vdPath = virtualDirFullPath.AfterFirst(wxT(':'));
    wxArrayString files;
    p->GetFilesByVirtualDir(vdPath, files);
    wxFileName tagsDb = TagsManagerST::Get()->GetDatabase()->GetDatabaseFileName();
    for (size_t i = 0; i < files.Count(); i++) {
        TagsManagerST::Get()->Delete(tagsDb, files.Item(i));
    }

    // and finally, remove the virtual dir from the workspace
    bool res = clCxxWorkspaceST::Get()->RemoveVirtualDirectory(virtualDirFullPath, errMsg);
    if (!res) {
        wxMessageBox(errMsg, _("Error"), wxOK | wxICON_HAND);
        return;
    }

    clCommandEvent evtFileRemoved(wxEVT_PROJ_FILE_REMOVED);
    evtFileRemoved.SetStrings(files);
    evtFileRemoved.SetString(project);
    evtFileRemoved.SetEventObject(this);
    EventNotifier::Get()->ProcessEvent(evtFileRemoved);
}

bool Manager::AddNewFileToProject(const wxString& fileName, const wxString& vdFullPath, bool openIt)
{
    wxFile file;
    wxFileName fn(fileName);
    if (!fn.DirExists()) {
        // ensure that the path to the file exists
        fn.Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    }

    if (!file.Create(fileName.GetData(), true))
        return false;

    if (file.IsOpened()) {
        file.Close();
    }

    return AddFileToProject(fileName, vdFullPath, openIt);
}

bool Manager::AddFileToProject(const wxString& fileName, const wxString& vdFullPath, bool openIt)
{
    wxString project;
    project = vdFullPath.BeforeFirst(wxT(':'));

    // Add the file to the project
    wxString errMsg;
    bool res = clCxxWorkspaceST::Get()->AddNewFile(vdFullPath, fileName, errMsg);
    if (!res) {
        // file or virtual dir does not exist
        return false;
    }

    if (openIt) {
        clMainFrame::Get()->GetMainBook()->OpenFile(fileName, project);
    }

    // send notification command event that files was added to
    // project
    wxArrayString files;
    files.Add(fileName);

    clCommandEvent evtAddFiles(wxEVT_PROJ_FILE_ADDED);
    evtAddFiles.SetStrings(files);
    evtAddFiles.SetString(project);
    EventNotifier::Get()->AddPendingEvent(evtAddFiles);
    return true;
}

void Manager::AddFilesToProject(const wxArrayString& files, const wxString& vdFullPath, wxArrayString& actualAdded)
{
    wxString project;
    project = vdFullPath.BeforeFirst(wxT(':'));

    // Add the file to the project
    wxString errMsg;
    // bool res = true;
    size_t i = 0;

    // try to find this file in the workspace
    for (i = 0; i < files.GetCount(); i++) {
        wxString file = files.Item(i);
        wxString projName = this->GetProjectNameByFile(file);
        // allow adding the file, only if it does not already exist under the current project
        //(it can be already exist under the different project)
        if (projName.IsEmpty() || projName != project) {
            actualAdded.Add(file);
        }
#if defined(__WXGTK__)
        else {
            // In Linux, files 'abc' and 'Abc' can happily co-exist, so see if that's what's happening
            wxString projName = this->GetProjectNameByFile(file, true); // 'true' is case-sensitive comparison
            if (projName.IsEmpty() || projName != project) {
                wxString msg1(wxString::Format(_("There is already a file in this folder with a name:\n%s\nthat "
                                                 "matches using case-insensitive comparison"),
                                               file));
                wxString msg2(
                    _("\nThis won't be a problem on Linux, but it may be on other, case-insensitive platforms"));
                wxString msg3(_("\n\nAdd the file anyway?"));
                int ans = wxMessageBox(msg1 + msg2 + msg3, _("Possible name-clash"),
                                       wxICON_WARNING | wxYES_NO | wxCANCEL, clMainFrame::Get());
                if (ans == wxYES) {
                    actualAdded.Add(file);
                } else if (ans == wxCANCEL) {
                    return;
                }
            }
        }
#endif
    }

    for (i = 0; i < actualAdded.GetCount(); i++) {
        clCxxWorkspace* wsp = clCxxWorkspaceST::Get();
        wsp->AddNewFile(vdFullPath, actualAdded.Item(i), errMsg);
    }

    // convert wxArrayString to vector for the ctags api
    std::vector<wxFileName> vFiles;
    for (size_t i = 0; i < actualAdded.GetCount(); i++) {
        vFiles.push_back(actualAdded.Item(i));
    }

    // re-tag the added files
    if (vFiles.empty() == false) {
        TagsManagerST::Get()->ParseWorkspaceIncremental();
    }

    if (!actualAdded.IsEmpty()) {
        clCommandEvent evtAddFiles(wxEVT_PROJ_FILE_ADDED);
        evtAddFiles.SetStrings(actualAdded);
        evtAddFiles.SetString(project);
        EventNotifier::Get()->AddPendingEvent(evtAddFiles);
    }

    if (actualAdded.GetCount() < files.GetCount()) {
        wxString msg = wxString::Format(_("%u file(s) not added, probably due to a name-clash"),
                                        (unsigned int)(files.GetCount() - actualAdded.GetCount()));
        wxMessageBox(msg, _("CodeLite"), wxOK, clMainFrame::Get());
    }
}

bool Manager::RemoveFile(const wxString& fileName, const wxString& vdFullPath, wxString& fullpathRemoved, bool notify)
{
    fullpathRemoved.Clear();
    wxString project = vdFullPath.BeforeFirst(wxT(':'));
    wxFileName absPath(fileName);
    absPath.MakeAbsolute(GetProjectCwd(project));

    clMainFrame::Get()->GetMainBook()->ClosePage(absPath.GetFullPath());

    wxString errMsg;
    bool res = clCxxWorkspaceST::Get()->RemoveFile(vdFullPath, fileName, errMsg);
    if (!res) {
        wxMessageBox(errMsg, _("Error"), wxOK | wxICON_HAND, clMainFrame::Get());
        return false;
    }

    TagsManagerST::Get()->Delete(TagsManagerST::Get()->GetDatabase()->GetDatabaseFileName(), absPath.GetFullPath());

    // Set the fullpath of the removed file
    fullpathRemoved = absPath.GetFullPath();

    if (notify) {
        wxArrayString files(1, &fileName);
        clCommandEvent evtFileRemoved(wxEVT_PROJ_FILE_REMOVED);
        evtFileRemoved.SetStrings(files);
        evtFileRemoved.SetString(project);
        evtFileRemoved.SetEventObject(this);
        EventNotifier::Get()->ProcessEvent(evtFileRemoved);
    }
    return true;
}

bool Manager::RenameFile(const wxString& origName, const wxString& newName, const wxString& vdFullPath)
{
    // Step: 1
    // remove the file from the workspace (this will erase it from the symbol database and will
    // also close the editor that it is currently opened in (if any)
    wxString pathRemoved;
    if (!RemoveFile(origName, vdFullPath, pathRemoved))
        return false;

    // Step: 2
    // Notify the plugins, maybe they want to override the
    // default behavior (e.g. Subversion plugin)
    clFileSystemEvent renameEvent(wxEVT_FILE_RENAMED);
    renameEvent.SetPath(origName);
    renameEvent.SetNewpath(newName);
    if (!EventNotifier::Get()->ProcessEvent(renameEvent)) {
        // rename the file on filesystem
        wxLogNull noLog;
        wxRenameFile(origName, newName);
    }

    // read file to project with the new name
    wxString projName = vdFullPath.BeforeFirst(wxT(':'));
    ProjectPtr proj = GetProject(projName);
    proj->FastAddFile(newName, vdFullPath.AfterFirst(wxT(':')));

    // Step 3: retag the new file
    RetagFile(newName);

    // Step 4: send an event about new file was added
    // to the workspace
    wxArrayString files;
    files.Add(newName);

    clCommandEvent evtAddFiles(wxEVT_PROJ_FILE_ADDED);
    evtAddFiles.SetStrings(files);
    evtAddFiles.SetString(projName);
    EventNotifier::Get()->AddPendingEvent(evtAddFiles);

    // Open the newly created file
    clMainFrame::Get()->GetMainBook()->OpenFile(newName, projName);

    // Step 5: Change all include files refering to the old
    // file
    if (!IsWorkspaceOpen()) {
        // if there is no workspace opened, we are done
        return true;
    }

    wxArrayString workspaceFiles;
    GetWorkspaceFiles(workspaceFiles);
    std::vector<IncludeStatement> includes, matches;

    for (size_t i = 0; i < workspaceFiles.GetCount(); i++) {

        // Dont attempt to scan binary files
        // Skip binary files
        if (TagsManagerST::Get()->IsBinaryFile(workspaceFiles.Item(i), TagsManagerST::Get()->GetCtagsOptions())) {
            continue;
        }

        // Scan only C/C++/h files
        switch (FileExtManager::GetType(workspaceFiles.Item(i))) {
        case FileExtManager::TypeHeader:
        case FileExtManager::TypeSourceC:
        case FileExtManager::TypeSourceCpp:
            IncludeFinder(workspaceFiles.Item(i).mb_str(wxConvUTF8).data(), includes);
            break;
        default:
            // ignore any other type
            break;
        }
    }

    // Filter non-relevant matches
    wxFileName oldName(origName);
    for (size_t i = 0; i < includes.size(); i++) {

        wxString inclName(includes.at(i).file.c_str(), wxConvUTF8);
        wxFileName inclFn(inclName);

        if (oldName.GetFullName() == inclFn.GetFullName()) {
            matches.push_back(includes.at(i));
        }
    }

    // Prompt the user with the list of files which are about to be modified
    wxFileName newFile(newName);
    if (matches.empty() == false) {
        RenameFileDlg dlg(clMainFrame::Get(), newFile.GetFullName(), matches);
        if (dlg.ShowModal() == wxID_OK) {
            matches.clear();
            matches = dlg.GetMatches();

            for (size_t i = 0; i < matches.size(); i++) {
                IncludeStatement includeStatement = matches.at(i);
                wxString editorFileName(includeStatement.includedFrom.c_str(), wxConvUTF8);
                wxString findWhat(includeStatement.pattern.c_str(), wxConvUTF8);
                wxString oldIncl(includeStatement.file.c_str(), wxConvUTF8);

                // We want to keep the original open/close braces
                // "" or <>
                wxFileName strippedOldInc(oldIncl);
                wxString replaceWith(findWhat);

                replaceWith.Replace(strippedOldInc.GetFullName(), newFile.GetFullName());

                clEditor* editor = clMainFrame::Get()->GetMainBook()->OpenFile(editorFileName, wxEmptyString, 0);
                if (editor && (editor->GetFileName().GetFullPath().CmpNoCase(editorFileName) == 0)) {
                    editor->ReplaceAllExactMatch(findWhat, replaceWith);
                }
            }
        }
    }
    return true;
}

bool Manager::MoveFileToVD(const wxString& fileName, const wxString& srcVD, const wxString& targetVD)
{
    // to move the file between targets, we need to change the file path, we do this
    // by changing the file to be in absolute path related to the src's project
    // and then making it relative to the target's project
    wxString srcProject, targetProject;
    srcProject = srcVD.BeforeFirst(wxT(':'));
    targetProject = targetVD.BeforeFirst(wxT(':'));
    wxFileName srcProjWd(GetProjectCwd(srcProject), wxEmptyString);

    // set a dir saver point
    wxFileName fn(fileName);
    wxArrayString files;
    files.Add(fn.GetFullPath());

    // remove the file from the source project
    wxString errMsg;
    bool res = clCxxWorkspaceST::Get()->RemoveFile(srcVD, fileName, errMsg);
    if (!res) {
        wxMessageBox(errMsg, _("Error"), wxOK | wxICON_HAND);
        return false;
    }

    // Add the file to the project
    res = clCxxWorkspaceST::Get()->AddNewFile(targetVD, fn.GetFullPath(), errMsg);
    if (!res) {
        // file or virtual dir does not exist
        return false;
    }
    return true;
}

void Manager::RetagProject(const wxString& projectName, bool quickRetag)
{
    wxUnusedVar(projectName);
    wxUnusedVar(quickRetag);
    TagsManagerST::Get()->ParseWorkspaceIncremental();
}

void Manager::GetProjectFiles(const wxString& project, wxArrayString& files)
{
    ProjectPtr p = GetProject(project);
    if (p) {
        p->GetFilesAsStringArray(files);
    }
}

wxString Manager::GetProjectNameByFile(const wxString& fullPathFileName, bool caseSensitive /*= false*/)
{
    wxString fPFN(fullPathFileName);
    return GetProjectNameByFile(fPFN, caseSensitive);
}
wxString Manager::GetProjectNameByFile(wxString& fullPathFileName, bool caseSensitive /*= false*/)
{
    wxArrayString projects;
    GetProjectList(projects);

    std::vector<ProjectPtr> vProjects;
    vProjects.reserve(projects.size());

    // Attempt 1:
    // Assume that the file is a real file
    wxString linkDestination = fullPathFileName;
    for (size_t i = 0; i < projects.GetCount(); i++) {
        ProjectPtr proj = GetProject(projects.Item(i));
        vProjects.push_back(proj); // keep the project, incase we will need it in the second iteration
        // The second call copes with the searched-for file being a symlink
        if (proj->IsFileExist(fullPathFileName) || proj->IsFileExist(linkDestination)) {
            return proj->GetName();
        }
    }

#if defined(__WXGTK__) || defined(__WXOSX__)
    // Attempt 2:
    // assume symlinks are involved

    // On gtk/macOS either fullPathFileName or the 'matching' project filename (or both) may be (or their paths contain)
    // symlinks
    linkDestination = FileUtils::RealPath(fullPathFileName);
    if (linkDestination != fullPathFileName) {
        for (auto& proj : vProjects) {
            // The second call copes with the searched-for file being a symlink
            if (proj->IsFileExist(fullPathFileName) || proj->IsFileExist(linkDestination)) {
                return proj->GetName();
            }
            wxString fileNameInProject; // Try again, checking if the _project_ filePath is a symlink
            if (proj->IsFileExist(fullPathFileName, fileNameInProject)) {
                fullPathFileName = fileNameInProject; // Hopefully the calling function will now use this
                return proj->GetName();
            }
        }
    }
#endif
    return wxEmptyString;
}

//--------------------------- Project Settings Mgmt -----------------------------

wxString Manager::GetProjectCwd(const wxString& project) const
{
    wxString errMsg;
    ProjectPtr p = clCxxWorkspaceST::Get()->FindProjectByName(project, errMsg);
    if (!p) {
        return wxGetCwd();
    }

    wxFileName projectFileName(p->GetFileName());
    projectFileName.MakeAbsolute();
    return projectFileName.GetPath();
}

ProjectSettingsPtr Manager::GetProjectSettings(const wxString& projectName) const
{
    wxString errMsg;
    ProjectPtr proj = clCxxWorkspaceST::Get()->FindProjectByName(projectName, errMsg);
    if (!proj) {
        clLogMessage(errMsg);
        return NULL;
    }

    return proj->GetSettings();
}

void Manager::SetProjectSettings(const wxString& projectName, ProjectSettingsPtr settings)
{
    wxString errMsg;
    ProjectPtr proj = clCxxWorkspaceST::Get()->FindProjectByName(projectName, errMsg);
    if (!proj) {
        clLogMessage(errMsg);
        return;
    }

    proj->SetSettings(settings);
}

void Manager::SetProjectGlobalSettings(const wxString& projectName, BuildConfigCommonPtr settings)
{
    wxString errMsg;
    ProjectPtr proj = clCxxWorkspaceST::Get()->FindProjectByName(projectName, errMsg);
    if (!proj) {
        clLogMessage(errMsg);
        return;
    }

    proj->SetGlobalSettings(settings);
}

wxString Manager::GetProjectExecutionCommand(const wxString& projectName, wxString& wd, bool considerPauseWhenExecuting)
{
    ProjectPtr proj = GetProject(projectName);
    if (!proj) {
        clWARNING() << "Manager::GetProjectExecutionCommand(): could not find project:" << projectName;
        return wxEmptyString;
    }

    BuildConfigPtr bldConf = clCxxWorkspaceST::Get()->GetProjBuildConf(projectName, wxEmptyString);
    if (!bldConf) {
        clWARNING() << "Manager::GetProjectExecutionCommand(): failed to find project configuration for project:"
                    << projectName;
        return wxEmptyString;
    }

    wxString projectPath = proj->GetFileName().GetPath();

    // expand variables
    wxString cmd = bldConf->GetCommand();
    cmd = MacroManager::Instance()->Expand(cmd, NULL, projectName);

    wxString cmdArgs = bldConf->GetCommandArguments();
    cmdArgs = MacroManager::Instance()->Expand(cmdArgs, NULL, projectName);

    // Execute command & cmdArgs
    wd = bldConf->GetWorkingDirectory();
    wd = MacroManager::Instance()->Expand(wd, NULL, projectName);

    wxFileName workingDir(wd, "");
    if (workingDir.IsRelative()) {
        workingDir.MakeAbsolute(projectPath);
    }

    wxFileName fileExe(cmd);
    if (fileExe.IsRelative()) {
        fileExe.MakeAbsolute(workingDir.GetPath());
    }
    fileExe.Normalize();

#ifdef __WXMSW__
    if (!fileExe.Exists() && fileExe.GetExt().IsEmpty()) {
        // Try with .exe
        wxFileName withExe(fileExe);
        withExe.SetExt("exe");
        if (withExe.Exists()) {
            fileExe = withExe;
        }
    }
#endif
    wd = workingDir.GetPath();
    cmd = fileExe.GetFullPath();

    clDEBUG() << "Command to execute:" << cmd;
    clDEBUG() << "Working directory:" << wd;

    clConsoleBase::Ptr_t console = clConsoleBase::GetTerminal();
    console->SetWorkingDirectory(wd);
    console->SetCommand(cmd, cmdArgs);
    console->SetWaitWhenDone(considerPauseWhenExecuting);
    console->SetTerminalNeeded(!bldConf->IsGUIProgram());
    return console->PrepareCommand();
}

//--------------------------- Top Level Pane Management -----------------------------

bool Manager::IsPaneVisible(const wxString& pane_name)
{
    wxAuiPaneInfo& info = clMainFrame::Get()->GetDockingManager().GetPane(pane_name);
    if (info.IsOk() && info.IsShown()) {
        return true;
    }
    return false;
}

bool Manager::ShowOutputPane(const wxString& focusWin, bool show, bool take_focus)
{
    // make the output pane visible
    GetPerspectiveManager().ShowOutputPane(focusWin, show, take_focus);
    return true;
}

void Manager::ShowDebuggerPane(bool show)
{
    // make the output pane visible
    const wxArrayString dbgPanes = StdToWX::ToArrayString(
        { wxT("Debugger"), wxGetTranslation(DebuggerPane::LOCALS), wxGetTranslation(DebuggerPane::FRAMES),
          wxGetTranslation(DebuggerPane::WATCHES), wxGetTranslation(DebuggerPane::BREAKPOINTS),
          wxGetTranslation(DebuggerPane::THREADS), wxGetTranslation(DebuggerPane::MEMORY),
          wxGetTranslation(DebuggerPane::ASCII_VIEWER) });

    wxAuiManager* aui = &clMainFrame::Get()->GetDockingManager();
    if (show) {

        for (size_t i = 0; i < dbgPanes.GetCount(); i++) {
            wxAuiPaneInfo& info = clMainFrame::Get()->GetDockingManager().GetPane(dbgPanes.Item(i));
            // show all debugger related panes
            if (info.IsOk() && !info.IsShown()) {
                DockablePaneMenuManager::DockablePaneMenuManager::HackShowPane(info, aui);
            }
        }

    } else {

        // hide all debugger related panes
        for (size_t i = 0; i < dbgPanes.GetCount(); i++) {
            wxAuiPaneInfo& info = clMainFrame::Get()->GetDockingManager().GetPane(dbgPanes.Item(i));
            // show all debugger related panes
            if (info.IsOk() && info.IsShown()) {
                DockablePaneMenuManager::HackHidePane(true, info, aui);
            }
        }
    }
}

void Manager::ShowWorkspacePane(wxString focusWin, bool commit)
{
    // make the output pane visible
    auto& aui = clMainFrame::Get()->GetDockingManager();
    wxAuiPaneInfo& info = aui.GetPane(PANE_LEFT_SIDEBAR);
    if (info.IsOk() && !info.IsShown()) {
        DockablePaneMenuManager::HackShowPane(info, &aui);
    }

    // set the selection to focus win
    auto book = clMainFrame::Get()->GetWorkspacePane()->GetNotebook();
    int index = book->GetPageIndex(focusWin);
    if (index != wxNOT_FOUND && index != book->GetSelection()) {
        book->SetSelection((size_t)index);
    } else if (index == wxNOT_FOUND) {
        clMainFrame::Get()->GetWorkspacePane()->ShowTab(focusWin, true);
    }
}

void Manager::ShowSecondarySideBarPane(wxString focusWin, bool commit)
{
    // make the output pane visible
    auto& aui = clMainFrame::Get()->GetDockingManager();
    wxAuiPaneInfo& info = aui.GetPane(PANE_RIGHT_SIDEBAR);
    if (info.IsOk() && !info.IsShown()) {
        DockablePaneMenuManager::HackShowPane(info, &aui);
    }

    // set the selection to focus win
    auto book = clMainFrame::Get()->GetSecondarySideBar()->GetNotebook();
    int index = book->GetPageIndex(focusWin);
    if (index != wxNOT_FOUND && index != book->GetSelection()) {
        book->SetSelection((size_t)index);
    }
}

void Manager::HidePane(const wxString& paneName, bool commit)
{
    wxAuiPaneInfo& info = clMainFrame::Get()->GetDockingManager().GetPane(paneName);
    if (info.IsOk() && info.IsShown()) {
        wxAuiManager* aui = &clMainFrame::Get()->GetDockingManager();
        DockablePaneMenuManager::HackHidePane(commit, info, aui);
    }
}

void Manager::ShowPane(const wxString& paneName, bool commit)
{
    wxAuiPaneInfo& info = clMainFrame::Get()->GetDockingManager().GetPane(paneName);
    if (info.IsOk() && !info.IsShown()) {
        wxAuiManager* aui = &clMainFrame::Get()->GetDockingManager();
        DockablePaneMenuManager::HackShowPane(info, aui);
    }
}

void Manager::TogglePanes()
{
    static wxString savedLayout;
    if (savedLayout.IsEmpty()) {

        savedLayout = clMainFrame::Get()->GetDockingManager().SavePerspective();
        wxAuiManager& aui = clMainFrame::Get()->GetDockingManager();
        wxAuiPaneInfoArray& all_panes = aui.GetAllPanes();
        for (size_t i = 0; i < all_panes.GetCount(); ++i) {
            if (all_panes.Item(i).IsOk() && all_panes.Item(i).IsShown() &&
                all_panes.Item(i).dock_direction != wxAUI_DOCK_CENTER) {
                all_panes.Item(i).Hide();
            }
        }

        // update changes
        aui.Update();

    } else {
        clMainFrame::Get()->GetDockingManager().LoadPerspective(savedLayout);
        savedLayout.Clear();
    }
}

//--------------------------- Menu and Accelerator Mmgt -----------------------------

void Manager::UpdateMenuAccelerators()
{
    // Load the accelerators (this function merges the old settings with the new settings)
    clKeyboardManager::Get()->Update();
}

//--------------------------- Run Program (No Debug) -----------------------------

bool Manager::IsProgramRunning() const { return m_programProcess; }

void Manager::ExecuteNoDebug(const wxString& projectName)
{
    // an instance is already running
    if (IsProgramRunning()) {
        return;
    }
    wxString wd;

    // we call it here once for the 'wd'
    wxString execLine;
    ProjectPtr proj;

    {
        EnvSetter env1(NULL, NULL, projectName, wxEmptyString);
        execLine = GetProjectExecutionCommand(projectName, wd, true);
        proj = GetProject(projectName);
    }

    DirSaver ds;

    // Build the working directory
    // Expand macros
    wd = MacroManager::Instance()->Expand(wd, NULL, projectName);
    wd = wd.Trim().Trim(false);
    if (!wd.IsEmpty()) {
        wxFileName projectWd(wd, "");
        if (!projectWd.IsAbsolute()) {
            projectWd.MakeAbsolute(proj->GetFileName().GetPath());
        }
        wd = projectWd.GetPath();
    } else {
        // Set the working directory to the project path
        wd = proj->GetFileName().GetPath();
    }

    // execute the program:
    //- no hiding the console
    //- no redirection of the stdin/out
    wxString configName;
    BuildConfigPtr bldConf = clCxxWorkspaceST::Get()->GetProjBuildConf(projectName, wxEmptyString);
    if (bldConf) {
        configName = bldConf->GetName();
    }
    EnvSetter env(NULL, NULL, projectName, configName);

    // call it again here to get the actual exection line - we do it here since
    // the environment has been applied
    size_t createProcessFlags = IProcessCreateDefault;
    if (!bldConf->IsGUIProgram()) {
        createProcessFlags = IProcessNoRedirect | IProcessCreateConsole;
    }

    wxString dummy;
    execLine = GetProjectExecutionCommand(projectName, dummy, bldConf->GetPauseWhenExecEnds());
    wxUnusedVar(dummy);
    clEnvList_t env_list;
    bldConf->GetCompiler()->CreatePathEnv(&env_list);

    m_programProcess = ::CreateAsyncProcess(this, execLine, createProcessFlags, wd, &env_list);
    if (m_programProcess) {
        clGetManager()->AppendOutputTabText(kOutputTab_Output, wxString()
                                                                   << _("Working directory is set to: ") << wd << "\n");
        clGetManager()->AppendOutputTabText(kOutputTab_Output, wxString() << _("Executing: ") << execLine << "\n");

        // Notify about program execution
        clExecuteEvent startEvent(wxEVT_PROGRAM_STARTED);
        EventNotifier::Get()->AddPendingEvent(startEvent);
    }
}

void Manager::KillProgram()
{
    if (!IsProgramRunning())
        return;
    m_programProcess->Terminate();
}

void Manager::OnProcessOutput(clProcessEvent& event)
{
    clMainFrame::Get()->GetOutputPane()->GetOutputWindow()->AppendText(event.GetOutput());
}

void Manager::OnProcessEnd(clProcessEvent& event)
{
    wxDELETE(m_programProcess);
    clGetManager()->AppendOutputTabText(kOutputTab_Output, _("Program exited\n"));

    // Notify about program termination
    clExecuteEvent stopEvent(wxEVT_PROGRAM_TERMINATED);
    EventNotifier::Get()->AddPendingEvent(stopEvent);

    // Raise CodeLite
    clMainFrame::Get()->CallAfter(&wxFrame::Raise);

    // return the focus back to the editor
    if (clMainFrame::Get()->GetMainBook()->GetActiveEditor()) {
        clMainFrame::Get()->GetMainBook()->GetActiveEditor()->SetActive();
    }
}

//--------------------------- Debugger Support -----------------------------

static void DebugMessage(const wxString& msg)
{
    clMainFrame::Get()->GetDebuggerPane()->GetDebugWindow()->AppendLine(msg);
}

void Manager::UpdateDebuggerPane()
{
    clCommandEvent evtDbgRefreshViews(wxEVT_DEBUGGER_UPDATE_VIEWS);
    EventNotifier::Get()->AddPendingEvent(evtDbgRefreshViews);

    DebuggerPane* pane = clMainFrame::Get()->GetDebuggerPane();
    DoUpdateDebuggerTabControl(pane->GetNotebook()->GetCurrentPage());
}

void Manager::DoUpdateDebuggerTabControl(wxWindow* curpage)
{
    // Update the debugger pane
    DebuggerPane* pane = clMainFrame::Get()->GetDebuggerPane();

    // make sure that the debugger pane is visible
    if (!IsPaneVisible(PANE_DEBUGGER))
        return;

    if (curpage == (wxWindow*)pane->GetBreakpointView() || IsPaneVisible(wxGetTranslation(DebuggerPane::BREAKPOINTS))) {
        pane->GetBreakpointView()->Initialize();
    }

    IDebugger* dbgr = DebuggerMgr::Get().GetActiveDebugger();
    if (dbgr && dbgr->IsRunning() && DbgCanInteract()) {

        //--------------------------------------------------------------------
        // Lookup the selected tab in the debugger notebook and update it
        // once this is done, we need to go through the list of detached panes
        // and scan for another debugger tabs which are visible and need to be
        // updated
        //--------------------------------------------------------------------

        if (curpage == (wxWindow*)pane->GetLocalsTable() || IsPaneVisible(wxGetTranslation(DebuggerPane::LOCALS))) {
            // update the locals tree
            dbgr->QueryLocals();
        }

        if (curpage == (wxWindow*)pane->GetDisassemblyTab() ||
            IsPaneVisible(wxGetTranslation(DebuggerPane::DISASSEMBLY))) {
            dbgr->ListRegisters();
        }

        if (curpage == pane->GetWatchesTable() || IsPaneVisible(wxGetTranslation(DebuggerPane::WATCHES))) {
            pane->GetWatchesTable()->RefreshValues();
        }
        if (curpage == (wxWindow*)pane->GetFrameListView() || IsPaneVisible(wxGetTranslation(DebuggerPane::FRAMES))) {
            // update the stack call
            dbgr->ListFrames();
        }

        if (curpage == (wxWindow*)pane->GetBreakpointView() ||
            IsPaneVisible(wxGetTranslation(DebuggerPane::BREAKPOINTS))) {

            // update the breakpoint view
            pane->GetBreakpointView()->Initialize();
        }
        if (curpage == (wxWindow*)pane->GetThreadsView() || IsPaneVisible(wxGetTranslation(DebuggerPane::THREADS))) {

            // update the thread list
            dbgr->ListThreads();
        }

        if (curpage == (wxWindow*)pane->GetMemoryView() || IsPaneVisible(wxGetTranslation(DebuggerPane::MEMORY))) {

            // Update the memory view tab
            MemoryView* memView = pane->GetMemoryView();
            if (memView->GetExpression().IsEmpty() == false) {

                wxString output;
                dbgr->WatchMemory(memView->GetExpression(), memView->GetSize(), memView->GetColumns());
            }
        }
    }
}

void Manager::SetMemory(const wxString& address, size_t count, const wxString& hex_value)
{
    IDebugger* dbgr = DebuggerMgr::Get().GetActiveDebugger();
    if (dbgr && dbgr->IsRunning() && DbgCanInteract()) {
        dbgr->SetMemory(address, count, hex_value);
    }
}

// Debugger API
void Manager::DbgStart(long attachPid)
{
    // set the working directory to the project directory
    DirSaver ds;
    wxString errMsg;
    wxString output;
    wxString debuggerName;
    wxString exepath;
    wxString wd;
    wxString args;
    BuildConfigPtr bldConf;
    ProjectPtr proj;
    DebuggerStartupInfo startup_info;
    long PID(-1);

    if (attachPid == -1) {
        // Start debugger ( when attachPid != -1 it means we are attaching to process )
        // Let the plugin know that we are about to start debugging
        clDebugEvent dbgEvent(wxEVT_DBG_UI_START);
        if (clWorkspaceManager::Get().IsWorkspaceOpened()) {
            dbgEvent.SetDebuggerName(clWorkspaceManager::Get().GetWorkspace()->GetDebuggerName());
        }
        if (EventNotifier::Get()->ProcessEvent(dbgEvent)) {
            return;
        }
    }

#if defined(__WXGTK__)
    wxString where;
    wxString terminal;
    wxArrayString tokens;
    wxArrayString configuredTerminal;
    terminal = wxT("xterm");
    if (!EditorConfigST::Get()->GetOptions()->GetProgramConsoleCommand().IsEmpty()) {
        tokens =
            wxStringTokenize(EditorConfigST::Get()->GetOptions()->GetProgramConsoleCommand(), wxT(" "), wxTOKEN_STRTOK);
        if (!tokens.IsEmpty()) {
            configuredTerminal = wxStringTokenize(tokens.Item(0), wxT("/"), wxTOKEN_STRTOK);
            if (!configuredTerminal.IsEmpty()) {
                terminal = configuredTerminal.Last();
                tokens.Clear();
                configuredTerminal.Clear();
            }
        }
    }
    if (!ExeLocator::Locate(terminal, where)) {
        wxMessageBox(_("Failed to locate the configured default terminal application required by CodeLite, please "
                       "install it or check your configuration!"),
                     _("CodeLite"), wxOK | wxCENTER | wxICON_WARNING, clMainFrame::Get());
        return;
    }
    terminal.Clear();
#endif

    if (attachPid == 1) { // attach to process
        AttachDbgProcDlg dlg(EventNotifier::Get()->TopFrame());
        if (dlg.ShowModal() != wxID_OK) {
            return;
        }

        wxString processId = dlg.GetProcessId();
        exepath = dlg.GetExeName();
        debuggerName = dlg.GetDebugger();
        DebuggerMgr::Get().SetActiveDebugger(debuggerName);

        long processID(wxNOT_FOUND);
        if (!processId.ToCLong(&processID)) {
            processID = wxNOT_FOUND;
        }

        // Let the plugins process this first
        clDebugEvent event(wxEVT_DBG_UI_ATTACH_TO_PROCESS);
        event.SetInt(processID);
        event.SetDebuggerName(debuggerName);
        if (EventNotifier::Get()->ProcessEvent(event)) {
            return;
        }

        if (exepath.IsEmpty() == false) {
            wxFileName fn(exepath);
            wxSetWorkingDirectory(fn.GetPath());
            exepath = fn.GetFullName();
        }
        PID = processID;
        startup_info.pid = processID;
    }

    if (attachPid == wxNOT_FOUND) {
        // need to debug the current project
        proj = clCxxWorkspaceST::Get()->FindProjectByName(GetActiveProjectName(), errMsg);
        if (proj) {
            wxSetWorkingDirectory(proj->GetFileName().GetPath());
            bldConf = clCxxWorkspaceST::Get()->GetProjBuildConf(proj->GetName(), wxEmptyString);
            if (bldConf) {
                debuggerName = bldConf->GetDebuggerType();
                DebuggerMgr::Get().SetActiveDebugger(debuggerName);
            }
        }

        startup_info.project = GetActiveProjectName();
    }

    // make sure we have an active debugger
    IDebugger* dbgr = DebuggerMgr::Get().GetActiveDebugger();
    if (!dbgr) {
        // No debugger available,
        wxString message;
        message << _("Failed to launch debugger '") << debuggerName << _("': debugger not loaded\n");
        message << _("Make sure that you have an open workspace and that the active project is of type 'Executable'");
        wxMessageBox(message, _("CodeLite"), wxOK | wxICON_WARNING);
        return;
    }
    startup_info.debugger = dbgr;

    // Save the current layout
    GetPerspectiveManager().SavePerspective(NORMAL_LAYOUT);

    // set the debugger information
    DebuggerInformation dinfo;
    DebuggerMgr::Get().GetDebuggerInformation(debuggerName, dinfo);

    // if user override the debugger path, apply it
    wxString userDebuggr;
    if (bldConf) {
        userDebuggr = bldConf->GetDebuggerPath();
        userDebuggr.Trim().Trim(false);
        if (userDebuggr.IsEmpty() == false) {
            // expand project macros
            userDebuggr = MacroManager::Instance()->Expand(userDebuggr, PluginManager::Get(), proj->GetName(),
                                                           bldConf->GetName());

            // Convert any relative path to absolute path
            // see bug# https://sourceforge.net/p/codelite/bugs/871/
            wxFileName fnDebuggerPath(userDebuggr);
            if (fnDebuggerPath.IsRelative()) {
                if (fnDebuggerPath.MakeAbsolute(proj->GetFileName().GetPath())) {
                    userDebuggr = fnDebuggerPath.GetFullPath();
                }
            }
            dinfo.path = userDebuggr;
        } else if (bldConf->GetCompiler() && !bldConf->GetCompiler()->GetTool("Debugger").IsEmpty()) {
            // User specified a different compiler for this compiler - use it
            userDebuggr = bldConf->GetCompiler()->GetTool("Debugger");
            dinfo.path = userDebuggr;
        }
    }

    // read the console command
    dinfo.consoleCommand = EditorConfigST::Get()->GetOptions()->GetProgramConsoleCommand();
    dbgr->SetDebuggerInformation(dinfo);

    // Apply the environment variables before starting
    EnvSetter env(NULL, NULL, proj ? proj->GetName() : wxString(), bldConf ? bldConf->GetName() : wxString());

    if (!bldConf && attachPid == wxNOT_FOUND) {
        wxString errmsg;
        errmsg << _("Could not find project configuration!\n")
               << _("Make sure that everything is set properly in your project settings");
        ::wxMessageBox(errmsg, wxT("CodeLite"), wxOK | wxICON_ERROR);
        return;

    } else if (attachPid == wxNOT_FOUND) {
        exepath = bldConf->GetCommand();

        // Get the debugging arguments.
        if (bldConf->GetUseSeparateDebugArgs()) {
            args = bldConf->GetDebugArgs();
        } else {
            args = bldConf->GetCommandArguments();
        }

        exepath.Prepend(wxT("\""));
        exepath.Append(wxT("\""));

        // Apply environment variables
        wd = bldConf->GetWorkingDirectory();

        // Expand variables before passing them to the debugger
        wd = MacroManager::Instance()->Expand(wd, NULL, proj->GetName());
        exepath = MacroManager::Instance()->Expand(exepath, NULL, proj->GetName());
    }

    wxString dbgname = dinfo.path;
    dbgname = EnvironmentConfig::Instance()->ExpandVariables(dbgname, true);

    // set ourselves as the observer for the debugger class
    dbgr->SetObserver(this);

    // Set the 'Is remote debugging' flag'
    dbgr->SetIsRemoteDebugging(bldConf && bldConf->GetIsDbgRemoteTarget() && PID == wxNOT_FOUND);
    dbgr->SetIsRemoteExtended(bldConf && bldConf->GetIsDbgRemoteExtended() && PID == wxNOT_FOUND);

    if (proj) {
        dbgr->SetProjectName(proj->GetName());
    } else {
        dbgr->SetProjectName(wxT(""));
    }

    // Loop through the open editors and let each editor
    // a chance to update the debugger manager with any line
    // changes (i.e. file was edited and breakpoints were moved)
    // this loop must take place before the debugger start up
    // or else call to UpdateBreakpoint() will yield an attempt to
    // actually add the breakpoint before Run() is called - this can
    // be a problem when adding breakpoint to dll files.
    if (wxNOT_FOUND == attachPid) {
        clMainFrame::Get()->GetMainBook()->UpdateBreakpoints();
    }

    // We can now get all the gathered breakpoints from the manager

    // since files may have been updated and the breakpoints may have been moved,
    // delete all the information
    std::vector<clDebuggerBreakpoint> bps = GetBreakpointsMgr()->GetBreakpoints();

    // notify plugins that we're about to start debugging
    clDebugEvent eventStarting(wxEVT_DEBUG_STARTING);
    eventStarting.SetClientData(&startup_info);
    if (EventNotifier::Get()->ProcessEvent(eventStarting)) {
        return;
    }

    if (!eventStarting.GetBreakpoints().empty()) {
        bps.swap(eventStarting.GetBreakpoints());
    }

    // Take the opportunity to store them in the pending array too
    GetBreakpointsMgr()->SetPendingBreakpoints(bps);

    // Launch the terminal
    if (bldConf && !bldConf->IsGUIProgram()) { // debugging a project and the project is not considered a "GUI" program
        m_debuggerTerminal.Clear();
#ifndef __WXMSW__
        m_debuggerTerminal.Launch(clDebuggerTerminalPOSIX::MakeExeTitle(exepath, args));
        if (!m_debuggerTerminal.IsValid()) {
            ::wxMessageBox(_("Could not launch terminal for debugger"), "CodeLite", wxOK | wxCENTER | wxICON_ERROR,
                           clMainFrame::Get());
            return;
        }
#endif
    }

    // read
    wxArrayString dbg_cmds;
    DebugSessionInfo si;

    si.debuggerPath = dbgname;
    si.exeName = exepath;
    si.cwd = wd;
    si.bpList = bps;
    si.ttyName = m_debuggerTerminal.GetTty();
    si.PID = PID;
    si.enablePrettyPrinting = dinfo.enableGDBPrettyPrinting;
    si.cmds = ::wxStringTokenize(dinfo.initFileCommands, "\r\n", wxTOKEN_STRTOK);

    if (bldConf) {
        si.searchPaths = bldConf->GetDebuggerSearchPaths();
    }

    clEnvList_t env_list;
    if (bldConf) {
        bldConf->GetCompiler()->CreatePathEnv(&env_list);
    }

    if (attachPid == wxNOT_FOUND) {
        // it is now OK to start the debugger...
        dbg_cmds = wxStringTokenize(bldConf->GetDebuggerStartupCmds(), "\r\n", wxTOKEN_STRTOK);

        // append project level commands to the global commands
        si.cmds.insert(si.cmds.end(), dbg_cmds.begin(), dbg_cmds.end());

        if (!dbgr->Start(si, &env_list)) {
            wxString errMsg;
            errMsg << _("Failed to initialize debugger: ") << dbgname << wxT("\n");
            DebugMessage(errMsg);
            return;
        }
    } else {
        // append project level commands to the global commands
        si.cmds.insert(si.cmds.end(), dbg_cmds.begin(), dbg_cmds.end());

        // Attach to process...
        if (!dbgr->Attach(si, &env_list)) {
            wxString errMsg;
            errMsg << _("Failed to initialize debugger: ") << dbgname << wxT("\n");
            DebugMessage(errMsg);
            return;
        }
    }

    // notify plugins that the debugger just started
    {
        clDebugEvent eventStarted(wxEVT_DEBUG_STARTED);
        eventStarted.SetClientData(&startup_info);
        EventNotifier::Get()->ProcessEvent(eventStarted);
    }

    // Clear the debugger output window
    clMainFrame::Get()->GetDebuggerPane()->Clear();

    // Now the debugger has been fed the breakpoints, re-Initialise the breakpt view,
    // so that it uses debugger_ids instead of internal_ids
    // Hmm. The above comment is probably no longer true; but it'll do no harm to Initialise() anyway
    clMainFrame::Get()->GetDebuggerPane()->GetBreakpointView()->Initialize();

    // Initialize the 'Locals' table. We do this for performane reason so we
    // wont need to read from the XML each time perform 'next' step
    clMainFrame::Get()->GetDebuggerPane()->GetLocalsTable()->Initialize();

    // let the active editor get the focus
    clEditor* editor = clMainFrame::Get()->GetMainBook()->GetActiveEditor();
    if (editor) {
        editor->SetActive();
    }

    // mark that we are waiting for the first GotControl()
    DebugMessage(output);
    DebugMessage(_("Debug session started successfully!\n"));

    if (dbgr->GetIsRemoteDebugging()) {
        // debugging remote target
        wxString host = bldConf->GetDbgHostName();
        wxString port = bldConf->GetDbgHostPort();

        // Trim whitespaces
        host = host.Trim(false).Trim();
        port = port.Trim(false).Trim();

        if (!port.IsEmpty()) {
            host << wxT(":") << port;
        }

        dbgr->Run(args, host);

    } else if (attachPid == wxNOT_FOUND) {

        // debugging local target
        dbgr->Run(args, wxEmptyString);
    }
    GetPerspectiveManager().LoadPerspective(DEBUG_LAYOUT);
}

void Manager::OnDebuggerStopping(clDebugEvent& event) { event.Skip(); }

void Manager::OnDebuggerStopped(clDebugEvent& event)
{
    // Debugger stopped. Cleanup UI layout & store session values
    event.Skip();
    clDEBUG() << "Debugger stopped!" << clEndl;
    // Restore the Normal layout
    GetPerspectiveManager().LoadPerspective(NORMAL_LAYOUT);
    if (m_watchDlg) {
        m_watchDlg->Destroy();
        m_watchDlg = NULL;
    }

    m_debuggerTerminal.Clear();

    // remove all debugger markers
    DbgUnMarkDebuggerLine();

    // Mark the debugger as non interactive
    m_dbgCanInteract = false;

    // Keep the current watches for the next debug session
    m_dbgWatchExpressions = clMainFrame::Get()->GetDebuggerPane()->GetWatchesTable()->GetExpressions();

    // clear the debugger pane
    clMainFrame::Get()->GetDebuggerPane()->Clear();

    // Notify the breakpoint manager that the debugger has stopped
    GetBreakpointsMgr()->DebuggerStopped();

    clMainFrame::Get()->GetDebuggerPane()->GetBreakpointView()->Initialize();

    // Clear the ascii viewer
    clMainFrame::Get()->GetDebuggerPane()->GetAsciiViewer()->UpdateView(wxT(""), wxT(""));

    // update toolbar state
    UpdateStopped();
    m_dbgCurrentFrameInfo.Clear();
}

void Manager::DbgStop()
{
    IDebugger* dbgr = DebuggerMgr::Get().GetActiveDebugger();
    CHECK_PTR_RET(dbgr);

    if (dbgr->IsRunning()) {
        clDEBUG() << "Stopping the debugger... (user request)" << clEndl;
        dbgr->Stop();
    }
    DebuggerMgr::Get().SetActiveDebugger(wxEmptyString);
}

void Manager::DbgMarkDebuggerLine(const wxString& fileName, int lineno)
{
    DbgUnMarkDebuggerLine();
    if (lineno < 0) {
        return;
    }

    // try to open the file
    wxFileName fn(fileName);
    auto callback = [=](IEditor* editor) {
        editor->GetCtrl()->ClearSelections();
        editor->HighlightLine(lineno - 1);
        editor->CenterLine(lineno - 1);
    };
    clGetManager()->OpenFileAndAsyncExecute(fn.GetFullPath(), callback);
}

void Manager::DbgUnMarkDebuggerLine()
{
    // remove all debugger markers from all editors
    clMainFrame::Get()->GetMainBook()->UnHighlightAll();
}

void Manager::DbgDoSimpleCommand(int cmd)
{
    // Hide tooltip dialog if its ON
    if (GetDebuggerTip() && GetDebuggerTip()->IsShown()) {
        GetDebuggerTip()->HideDialog();
    }

    IDebugger* dbgr = DebuggerMgr::Get().GetActiveDebugger();
    if (dbgr && dbgr->IsRunning()) {
        switch (cmd) {
        case DBG_PAUSE:
            GetBreakpointsMgr()->SetExpectingControl(true);
            dbgr->Interrupt();
            dbgr->ListFrames();
            break;
        case DBG_NEXT:
            clMainFrame::Get()->GetDebuggerPane()->GetLocalsTable()->ResetTableColors();
            clMainFrame::Get()->GetDebuggerPane()->GetWatchesTable()->ResetTableColors();
            dbgr->Next();
            break;
        case DBG_STEPIN:
            clMainFrame::Get()->GetDebuggerPane()->GetLocalsTable()->ResetTableColors();
            clMainFrame::Get()->GetDebuggerPane()->GetWatchesTable()->ResetTableColors();
            dbgr->StepIn();
            break;
        case DBG_STEPI:
            clMainFrame::Get()->GetDebuggerPane()->GetLocalsTable()->ResetTableColors();
            clMainFrame::Get()->GetDebuggerPane()->GetWatchesTable()->ResetTableColors();
            dbgr->StepInInstruction();
            break;
        case DBG_STEPOUT:
            clMainFrame::Get()->GetDebuggerPane()->GetLocalsTable()->ResetTableColors();
            clMainFrame::Get()->GetDebuggerPane()->GetWatchesTable()->ResetTableColors();
            dbgr->StepOut();
            break;
        case DBG_SHOW_CURSOR:
            dbgr->QueryFileLine();
            break;
        case DBG_NEXTI:
            clMainFrame::Get()->GetDebuggerPane()->GetLocalsTable()->ResetTableColors();
            clMainFrame::Get()->GetDebuggerPane()->GetWatchesTable()->ResetTableColors();
            dbgr->NextInstruction();
            break;
        default:
            break;
        }
    }
}

void Manager::DbgSetFrame(int frame, int lineno)
{
    wxAuiPaneInfo& info = clMainFrame::Get()->GetDockingManager().GetPane(PANE_DEBUGGER);
    if (info.IsShown()) {
        IDebugger* dbgr = DebuggerMgr::Get().GetActiveDebugger();
        if (dbgr && dbgr->IsRunning() && DbgCanInteract()) {
            // set the frame
            dbgr->SetFrame(frame);
            m_frameLineno = lineno;
        }
    }
}

void Manager::DbgSetThread(long threadId)
{
    IDebugger* dbgr = DebuggerMgr::Get().GetActiveDebugger();
    if (dbgr && dbgr->IsRunning() && DbgCanInteract()) {
        // set the frame
        dbgr->SelectThread(threadId);
        dbgr->QueryFileLine();
    }
}

// Event handlers from the debugger

void Manager::UpdateAddLine(const wxString& line, const bool OnlyIfLoggingOn /*=false*/)
{
    // There are a few messages that are only worth displaying if full logging is enabled
    if (OnlyIfLoggingOn) {
        IDebugger* dbgr = DebuggerMgr::Get().GetActiveDebugger();
        if (dbgr && (dbgr->IsLoggingEnabled() == false)) {
            return;
        }
    }

    DebugMessage(line + wxT("\n"));
}

void Manager::UpdateGotControl(const DebuggerEventData& e)
{
    int reason = e.m_controlReason;
    bool userTriggered = GetBreakpointsMgr()->GetExpectingControl();
    GetBreakpointsMgr()->SetExpectingControl(false);

    IDebugger* dbgr = DebuggerMgr::Get().GetActiveDebugger();
    if (dbgr == NULL) {
        wxLogDebug(wxT("Active debugger not found :/"));
        return;
    }

    DebuggerInformation dinfo;
    DebuggerMgr::Get().GetDebuggerInformation(dbgr->GetName(), dinfo);
    // Raise CodeLite (unless the cause is a hit bp, and we're configured not to
    //   e.g. because there's a command-list breakpoint ending in 'continue')
    if ((reason != DBG_BP_HIT) || dinfo.whenBreakpointHitRaiseCodelite) {
        if (clMainFrame::Get()->IsIconized() || !clMainFrame::Get()->IsShown()) {
            clMainFrame::Get()->Restore();
        }
        long curFlags = clMainFrame::Get()->GetWindowStyleFlag();
        clMainFrame::Get()->SetWindowStyleFlag(curFlags | wxSTAY_ON_TOP);
        clMainFrame::Get()->Raise();
        clMainFrame::Get()->SetWindowStyleFlag(curFlags);
        m_dbgCanInteract = true;
    }

    SendCmdEvent(wxEVT_DEBUG_EDITOR_GOT_CONTROL);

    if (dbgr && dbgr->IsRunning()) {
        // in case we got an error, try and clear the Locals view
        if (reason == DBG_CMD_ERROR) {
            clMainFrame::Get()->GetDebuggerPane()->GetLocalsTable()->Clear();
        }
    }

    switch (reason) {
    case DBG_RECV_SIGNAL_SIGTRAP:        // DebugBreak()
    case DBG_RECV_SIGNAL_EXC_BAD_ACCESS: // SIGSEGV on Mac
    case DBG_RECV_SIGNAL_SIGABRT:        // assert() ?
    case DBG_RECV_SIGNAL_SIGPIPE:
    case DBG_RECV_SIGNAL_SIGSEGV: { // program received signal sigsegv

        // Clear the 'Locals' view
        clMainFrame::Get()->GetDebuggerPane()->GetLocalsTable()->Clear();

        wxString signame = wxT("SIGSEGV");

        // show the dialog only if the signal is not sigtrap
        // since sigtap might be triggered by user inserting a breakpoint
        // into an already running debug session
        bool showDialog(true);
        if (reason == DBG_RECV_SIGNAL_EXC_BAD_ACCESS) {
            signame = wxT("EXC_BAD_ACCESS");

        } else if (reason == DBG_RECV_SIGNAL_SIGABRT) {
            signame = wxT("SIGABRT");

        } else if (reason == DBG_RECV_SIGNAL_SIGTRAP) {
            signame = wxT("SIGTRAP");
            showDialog = !userTriggered;
        } else if (reason == DBG_RECV_SIGNAL_SIGPIPE) {
            signame = "SIGPIPE";
        }

        DebugMessage(_("Program Received signal ") + signame + wxT("\n"));
        if (showDialog) {
            wxMessageDialog dlg(clMainFrame::Get(),
                                _("Program Received signal ") + signame + wxT("\n") +
                                    _("Stack trace is available in the 'Call Stack' tab\n"),
                                _("CodeLite"), wxICON_ERROR | wxOK);
            dlg.ShowModal();
        }

        // Print the stack trace
        if (showDialog) {
            // select the "Call Stack" tab
            clMainFrame::Get()->GetDebuggerPane()->SelectTab(wxGetTranslation(DebuggerPane::FRAMES));
        }

        if (!userTriggered) {
            if (dbgr && dbgr->IsRunning()) {
                dbgr->QueryFileLine();
                dbgr->BreakList();
                // Apply all previous watches
                DbgRestoreWatches();
            }
        }
    } break;
    case DBG_BP_ASSERTION_HIT: {
        // Clear the 'Locals' view
        clMainFrame::Get()->GetDebuggerPane()->GetLocalsTable()->Clear();

        wxMessageDialog dlg(clMainFrame::Get(),
                            _("Assertion failed!\nStack trace is available in the 'Call Stack' tab\n"), _("CodeLite"),
                            wxICON_ERROR | wxOK);
        dlg.ShowModal();

        // Print the stack trace
        wxAuiPaneInfo& info = clMainFrame::Get()->GetDockingManager().GetPane(PANE_DEBUGGER);
        if (info.IsShown()) {
            clMainFrame::Get()->GetDebuggerPane()->SelectTab(wxGetTranslation(DebuggerPane::FRAMES));
            CallAfter(&Manager::UpdateDebuggerPane);
        }
    } break;
    case DBG_BP_HIT: // breakpoint reached
        // Clear the 'Locals' view
        clMainFrame::Get()->GetDebuggerPane()->GetLocalsTable()->Clear();
    // fall through...
    case DBG_END_STEPPING: // finished one of the following: next/step/nexti/stepi
    case DBG_FUNC_FINISHED:
    case DBG_UNKNOWN: // the most common reason: temporary breakpoint
        if (dbgr && dbgr->IsRunning()) {
            dbgr->QueryFileLine();
            dbgr->BreakList();
            // Apply all previous watches
            DbgRestoreWatches();
        }
        break;

    case DBG_DBGR_KILLED:
        m_dbgCanInteract = false;
        break;

    case DBG_EXIT_WITH_ERROR: {
        wxMessageBox(wxString::Format(_("Debugger exited with the following error string:\n%s"), e.m_text.c_str()),
                     _("CodeLite"), wxOK | wxICON_ERROR);
        // fall through
    }

    case DBG_EXITED_NORMALLY:
        DbgStop();
        break;
    default:
        break;
    }
}

void Manager::UpdateLostControl()
{
    // debugger lost control
    // hide the marker
    DbgUnMarkDebuggerLine();
    m_dbgCanInteract = false;
    DebugMessage(_("Continuing...\n"));

    // Reset the debugger call-stack pane
    clMainFrame::Get()->GetDebuggerPane()->GetFrameListView()->Clear();
    clMainFrame::Get()->GetDebuggerPane()->GetFrameListView()->SetCurrentLevel(0);

    SendCmdEvent(wxEVT_DEBUG_EDITOR_LOST_CONTROL);
}

void Manager::UpdateTypeResolved(const wxString& expr, const wxString& type_name)
{
    IDebugger* dbgr = DebuggerMgr::Get().GetActiveDebugger();
    // Sanity
    if (dbgr == NULL) {
        return;
    }
    if (dbgr->IsRunning() == false) {
        return;
    }
    if (DbgCanInteract() == false) {
        return;
    }

    // gdb returns usually expression like:
    // const string &, so in order to get the actual type
    // we construct a valid expression by appending a valid identifier followed by a semi colon.
    wxString expression;
    wxString command(expr);
    wxString dbg_command(wxT("print"));
    wxString expression_type;

    DebuggerSettingsPreDefMap data;
    DebuggerConfigTool::Get()->ReadObject(wxT("DebuggerCommands"), &data);
    DebuggerPreDefinedTypes preDefTypes = data.GetActiveSet();
    DebuggerCmdDataVec cmds = preDefTypes.GetCmds();

    expression << wxT("/^");
    expression << type_name;
    expression << wxT(" someValidName;");
    expression << wxT("$/");

    Variable variable;
    if (LanguageST::Get()->VariableFromPattern(expression, wxT("someValidName"), variable)) {
        expression_type = variable.m_type.c_str();
        for (size_t i = 0; i < cmds.size(); i++) {
            DebuggerCmdData cmd = cmds.at(i);
            if (cmd.GetName() == expression_type) {
                // prepare the string to be evaluated
                command = cmd.GetCommand();
                command = MacroManager::Instance()->Replace(command, wxT("variable"), expr, true);

                dbg_command = cmd.GetDbgCommand();

                //---------------------------------------------------
                // Special handling for the templates
                //---------------------------------------------------

                wxArrayString types = DoGetTemplateTypes(variable.m_templateDecl);

                // Case 1: list
                // The user defined scripts requires that we pass info like this:
                // plist <list name> <T>
                if (expression_type == wxT("list") && types.GetCount() > 0) {
                    command << wxT(" ") << types.Item(0);
                }
                // Case 2: map & multimap
                // The user defined script requires that we pass the TLeft & TRight
                // pmap <list name> TLeft TRight
                if ((expression_type == wxT("map") || expression_type == wxT("multimap")) && types.GetCount() > 1) {
                    command << wxT(" ") << types.Item(0) << wxT(" ") << types.Item(1);
                }

                break;
            }
        }
    }

    wxString output;
    bool get_tip(false);

    Notebook* book = clMainFrame::Get()->GetDebuggerPane()->GetNotebook();
    if (book->GetPageText(book->GetSelection()) == wxGetTranslation(DebuggerPane::ASCII_VIEWER) ||
        IsPaneVisible(wxGetTranslation(DebuggerPane::ASCII_VIEWER))) {
        get_tip = true;
    }

    if (get_tip) {
        dbgr->GetAsciiViewerContent(dbg_command, command); // Will trigger a call to UpdateTip()
    }
}

void Manager::UpdateAsciiViewer(const wxString& expression, const wxString& tip)
{
    clMainFrame::Get()->GetDebuggerPane()->GetAsciiViewer()->UpdateView(expression, tip);
}

void Manager::UpdateRemoteTargetConnected(const wxString& line)
{
    IDebugger* dbgr = DebuggerMgr::Get().GetActiveDebugger();

    if (dbgr && dbgr->IsRunning()) {
        wxString commands;
        // An old behavior for legacy workspace
        if (IsWorkspaceOpen()) {
            // we currently do not support this feature when debugging using 'Quick debug'
            wxString errMsg;
            ProjectPtr proj = clCxxWorkspaceST::Get()->FindProjectByName(GetActiveProjectName(), errMsg);
            BuildConfigPtr bldConf = clCxxWorkspaceST::Get()->GetProjBuildConf(proj->GetName(), wxEmptyString);
            if (bldConf) {
                commands = bldConf->GetDebuggerPostRemoteConnectCmds();
            }

        // Filesystem workspace and so on
        } else if (!dbgr->GetPostRemoteConnectCommands().empty()) {
            commands = dbgr->GetPostRemoteConnectCommands();
        }

        // - Execute commands
        wxArrayString dbg_cmds = wxStringTokenize(commands, wxT("\n"), wxTOKEN_STRTOK);
        for (size_t i = 0; i < dbg_cmds.GetCount(); i++) {
            dbgr->ExecuteCmd(dbg_cmds.Item(i));
        }
    }

    // log the line
    UpdateAddLine(line);
}

//--------------------------- Build Management -----------------------------

bool Manager::IsBuildInProgress() const
{
    clBuildEvent buildEvent(wxEVT_GET_IS_BUILD_IN_PROGRESS);
    EventNotifier::Get()->ProcessEvent(buildEvent);
    return buildEvent.IsRunning() || (m_shellProcess && m_shellProcess->IsBusy());
}

void Manager::StopBuild()
{
    clBuildEvent stopEvent(wxEVT_STOP_BUILD);
    if (EventNotifier::Get()->ProcessEvent(stopEvent)) {
        return;
    }

    // Mark this build as 'interrupted'
    clMainFrame::Get()->GetOutputPane()->GetBuildTab()->SetBuildInterrupted(true);

    if (m_shellProcess && m_shellProcess->IsBusy()) {
        m_shellProcess->Stop();
    }
    m_buildQueue.clear();
}

void Manager::PushQueueCommand(const QueueCommand& buildInfo) { m_buildQueue.push_back(buildInfo); }

void Manager::ProcessCommandQueue()
{
    if (m_buildQueue.empty()) {
        return;
    }

    // pop the next build build and process it
    QueueCommand qcmd = m_buildQueue.front();
    m_buildQueue.pop_front();

    if (qcmd.GetCheckBuildSuccess() && !IsBuildEndedSuccessfully()) {
        // build failed, remove command from the queue
        return;
    }

    switch (qcmd.GetKind()) {
    case QueueCommand::kExecuteNoDebug:
        ExecuteNoDebug(qcmd.GetProject());
        break;

    case QueueCommand::kCustomBuild:
        DoCustomBuild(qcmd);
        break;

    case QueueCommand::kClean:
        DoCleanProject(qcmd);
        break;

    case QueueCommand::kRebuild:
    case QueueCommand::kBuild:
        DoBuildProject(qcmd);
        break;

    case QueueCommand::kDebug:
        DbgStart(wxNOT_FOUND);
        break;
    }
}

void Manager::BuildWorkspace()
{
    DoCmdWorkspace(QueueCommand::kBuild);
    ProcessCommandQueue();
}

void Manager::CleanWorkspace()
{
    DoCmdWorkspace(QueueCommand::kClean);
    ProcessCommandQueue();
}

void Manager::RebuildWorkspace()
{
    DoCmdWorkspace(QueueCommand::kClean);
    DoCmdWorkspace(QueueCommand::kBuild);
    ProcessCommandQueue();
}

void Manager::RunCustomPreMakeCommand(const wxString& project)
{
    if (m_shellProcess && m_shellProcess->IsBusy()) {
        return;
    }

    wxString conf;
    // get the selected configuration to be built
    BuildConfigPtr bldConf = clCxxWorkspaceST::Get()->GetProjBuildConf(project, wxEmptyString);
    if (bldConf) {
        conf = bldConf->GetName();
    }
    QueueCommand info(project, conf, false, QueueCommand::kBuild);

    if (m_shellProcess) {
        delete m_shellProcess;
    }
    m_shellProcess = new CompileRequest(info,
                                        wxEmptyString, // no file name (valid only for build file only)
                                        true);         // run premake step only
    m_shellProcess->Process(PluginManager::Get());
}

void Manager::CompileFile(const wxString& projectName, const wxString& fileName, bool preprocessOnly)
{
    if (m_shellProcess && m_shellProcess->IsBusy()) {
        return;
    }

    DoSaveAllFilesBeforeBuild();

    // If a debug session is running, stop it.
    IDebugger* dbgr = DebuggerMgr::Get().GetActiveDebugger();
    if (dbgr && dbgr->IsRunning()) {
        if (wxMessageBox(_("This would terminate the current debug session, continue?"), _("Confirm"),
                         wxICON_WARNING | wxYES_NO | wxCANCEL) != wxYES)
            return;
        DbgStop();
    }

    wxString conf;
    BuildConfigPtr bldConf = clCxxWorkspaceST::Get()->GetProjBuildConf(projectName, wxEmptyString);
    if (bldConf) {
        conf = bldConf->GetName();
    }

    QueueCommand info(projectName, conf, false, QueueCommand::kBuild);
    if (bldConf && bldConf->IsCustomBuild()) {
        info.SetCustomBuildTarget(preprocessOnly ? _("Preprocess File") : _("Compile Single File"));
        info.SetKind(QueueCommand::kCustomBuild);
    }

    if (m_shellProcess) {
        delete m_shellProcess;
    }
    switch (info.GetKind()) {
    case QueueCommand::kBuild:
        m_shellProcess = new CompileRequest(info, fileName, false, preprocessOnly);
        break;
    case QueueCommand::kCustomBuild:
        m_shellProcess = new CustomBuildRequest(info, fileName);
        break;
    default:
        m_shellProcess = NULL;
        break;
    }
    m_shellProcess->Process(PluginManager::Get());
}

bool Manager::IsBuildEndedSuccessfully() const
{
    // return the result of the last build
    return clMainFrame::Get()->GetOutputPane()->GetBuildTab()->GetBuildEndedSuccessfully();
}

void Manager::DoBuildProject(const QueueCommand& buildInfo)
{
    if (m_shellProcess && m_shellProcess->IsBusy())
        return;

    DoSaveAllFilesBeforeBuild();

    // If a debug session is running, stop it.
    IDebugger* dbgr = DebuggerMgr::Get().GetActiveDebugger();
    if (dbgr && dbgr->IsRunning()) {
        if (wxMessageBox(_("This would terminate the current debug session, continue?"), _("Confirm"),
                         wxICON_WARNING | wxYES_NO | wxCANCEL) != wxYES)
            return;
        DbgStop();
    }

    if (m_shellProcess) {
        delete m_shellProcess;
    }

    m_shellProcess = new CompileRequest(buildInfo);
    m_shellProcess->Process(PluginManager::Get());
}

void Manager::DoCleanProject(const QueueCommand& buildInfo)
{
    if (m_shellProcess && m_shellProcess->IsBusy()) {
        return;
    }

    if (m_shellProcess) {
        delete m_shellProcess;
    }
    m_shellProcess = new CleanRequest(buildInfo);
    m_shellProcess->Process(PluginManager::Get());
}

void Manager::DoCustomBuild(const QueueCommand& buildInfo)
{
    if (m_shellProcess && m_shellProcess->IsBusy()) {
        return;
    }

    DoSaveAllFilesBeforeBuild();

    // If a debug session is running, stop it.
    IDebugger* dbgr = DebuggerMgr::Get().GetActiveDebugger();
    if (dbgr && dbgr->IsRunning()) {
        if (wxMessageBox(_("This would terminate the current debug session, continue?"), _("Confirm"),
                         wxICON_WARNING | wxYES_NO | wxCANCEL) != wxYES)
            return;
        DbgStop();
    }

    wxDELETE(m_shellProcess);
    m_shellProcess = new CustomBuildRequest(buildInfo, wxEmptyString);
    m_shellProcess->Process(PluginManager::Get());
}

void Manager::DoCmdWorkspace(int cmd)
{
    // get list of projects
    wxArrayString projects;
    wxArrayString optimizedList;

    ManagerST::Get()->GetProjectList(projects);

    for (size_t i = 0; i < projects.GetCount(); i++) {
        ProjectPtr p = GetProject(projects.Item(i));
        BuildConfigPtr buildConf = clCxxWorkspaceST::Get()->GetProjBuildConf(projects.Item(i), wxEmptyString);
        if (p && buildConf && buildConf->IsProjectEnabled()) {
            wxArrayString deps = p->GetDependencies(buildConf->GetName());
            for (size_t j = 0; j < deps.GetCount(); j++) {
                // add a project only if it does not exist yet
                if (optimizedList.Index(deps.Item(j)) == wxNOT_FOUND) {
                    optimizedList.Add(deps.Item(j));
                }
            }
            // add the project itself now, again only if it is not included yet
            if (optimizedList.Index(projects.Item(i)) == wxNOT_FOUND) {
                optimizedList.Add(projects.Item(i));
            }
        }
    }

    // add a build/clean project only command for every project in the optimized list
    for (size_t i = 0; i < optimizedList.GetCount(); i++) {
        BuildConfigPtr buildConf = clCxxWorkspaceST::Get()->GetProjBuildConf(optimizedList.Item(i), wxEmptyString);
        if (buildConf && buildConf->IsProjectEnabled()) {
            QueueCommand bi(optimizedList.Item(i), buildConf->GetName(), true, cmd);
            if (buildConf->IsCustomBuild()) {
                bi.SetKind(QueueCommand::kCustomBuild);
                switch (cmd) {
                case QueueCommand::kBuild:
                    bi.SetCustomBuildTarget(wxT("Build"));
                    break;
                case QueueCommand::kClean:
                    bi.SetCustomBuildTarget(wxT("Clean"));
                    break;
                }
            }
            bi.SetCleanLog(i == 0);
            PushQueueCommand(bi);
        }
    }
}

void Manager::DbgClearWatches() { m_dbgWatchExpressions.Clear(); }

void Manager::DebuggerUpdate(const DebuggerEventData& event)
{
    IDebugger* dbgr = DebuggerMgr::Get().GetActiveDebugger();
    DebuggerInformation dbgInfo = dbgr ? dbgr->GetDebuggerInformation() : DebuggerInformation();
    switch (event.m_updateReason) {

    case DBG_UR_GOT_CONTROL:
        // keep the current function name
        m_dbgCurrentFrameInfo.func = event.m_frameInfo.function;
        UpdateGotControl(event);
        break;

    case DBG_UR_LOST_CONTROL:
        UpdateLostControl();
        break;

    case DBG_UR_FRAMEDEPTH: {
        long frameDepth(0);
        event.m_frameInfo.level.ToLong(&frameDepth);
        m_dbgCurrentFrameInfo.depth = frameDepth;
        // clMainFrame::Get()->GetDebuggerPane()->GetLocalsTable()->UpdateFrameInfo();
        break;
    }

    case DBG_UR_VAROBJUPDATE:
        // notify the 'Locals' view to remove all
        // out-of-scope variable objects
        clMainFrame::Get()->GetDebuggerPane()->GetLocalsTable()->OnVariableObjUpdate(event);
        clMainFrame::Get()->GetDebuggerPane()->GetWatchesTable()->OnUpdateVariableObject(event);
        break;

    case DBG_UR_ADD_LINE:
        UpdateAddLine(event.m_text, event.m_onlyIfLogging);
        break;

    case DBG_UR_BP_ADDED:
        GetBreakpointsMgr()->SetBreakpointDebuggerID(event.m_bpInternalId, event.m_bpDebuggerId);
        break;

    case DBG_UR_STOPPED:
        // Nothing to do here
        break;

    case DBG_UR_LOCALS:
        clMainFrame::Get()->GetDebuggerPane()->GetLocalsTable()->UpdateLocals(event.m_locals);
#ifdef __WXMAC__
        {
            for (size_t i = 0; i < event.m_locals.size(); i++) {
                LocalVariable v = event.m_locals.at(i);
                if (v.gdbId.IsEmpty() == false) {
                    dbgr->DeleteVariableObject(v.gdbId);
                }
            }
        }
#endif
        break;

    case DBG_UR_FUNC_ARGS:
        clMainFrame::Get()->GetDebuggerPane()->GetLocalsTable()->UpdateFuncArgs(event.m_locals);
#ifdef __WXMAC__
        {
            for (size_t i = 0; i < event.m_locals.size(); i++) {
                LocalVariable v = event.m_locals.at(i);
                if (v.gdbId.IsEmpty() == false) {
                    dbgr->DeleteVariableObject(v.gdbId);
                }
            }
        }
#endif
        break;

    case DBG_UR_EXPRESSION:
        // clMainFrame::Get()->GetDebuggerPane()->GetWatchesTable()->UpdateExpression ( event.m_expression,
        // event.m_evaluated );
        break;

    case DBG_UR_FUNCTIONFINISHED:
        clMainFrame::Get()->GetDebuggerPane()->GetLocalsTable()->UpdateFuncReturnValue(event.m_expression);
        break;

    case DBG_UR_REMOTE_TARGET_CONNECTED:
        UpdateRemoteTargetConnected(event.m_text);
        break;

    case DBG_UR_BP_HIT:
        GetBreakpointsMgr()->BreakpointHit(event.m_bpDebuggerId);
        break;

    case DBG_UR_TYPE_RESOLVED:
        if (event.m_userReason == DBG_USERR_WATCHTABLE) {
            clMainFrame::Get()->GetDebuggerPane()->GetWatchesTable()->OnTypeResolved(event);

        } else if (event.m_userReason == DBG_USERR_QUICKWACTH) {
            wxString expr = ::DbgPrependCharPtrCastIfNeeded(event.m_expression, event.m_evaluated);
            dbgr->CreateVariableObject(expr, false, DBG_USERR_QUICKWACTH);

        } else {
            // Default
            UpdateTypeResolved(event.m_expression, event.m_evaluated);
        }
        break;

    case DBG_UR_ASCII_VIEWER:
        UpdateAsciiViewer(event.m_expression, event.m_text);
        break;

    case DBG_UR_LISTTHRAEDS:
        clMainFrame::Get()->GetDebuggerPane()->GetThreadsView()->PopulateList(event.m_threads);
        break;

    case DBG_UR_WATCHMEMORY:
        clMainFrame::Get()->GetDebuggerPane()->GetMemoryView()->SetViewString(event.m_evaluated);
        break;

    case DBG_UR_VARIABLEOBJUPDATEERR:
        // Variable object update fail!
        break;

    case DBG_UR_VARIABLEOBJCREATEERR:
        // Variable creation error, remove it from the relevant table
        if (event.m_userReason == DBG_USERR_WATCHTABLE) {
            clMainFrame::Get()->GetDebuggerPane()->GetWatchesTable()->OnCreateVariableObjError(event);

        } else if (event.m_userReason == DBG_USERR_QUICKWACTH) {
            GetDebuggerTip()->OnCreateVariableObjError(event);
        }
        break;

    case DBG_UR_VARIABLEOBJ: {
        // CreateVariableObject callback
        if (event.m_userReason == DBG_USERR_QUICKWACTH) {

            /////////////////////////////////////////////
            // Handle Tooltips
            /////////////////////////////////////////////
            DoShowQuickWatchDialog(event);

            // Handle ASCII Viewer
            wxString expression(event.m_expression);
            if (event.m_variableObject.isPtr && !event.m_expression.StartsWith(wxT("*"))) {
                if (event.m_variableObject.typeName.Contains(wxT("char *")) ||
                    event.m_variableObject.typeName.Contains(wxT("wchar_t *")) ||
                    event.m_variableObject.typeName.Contains(wxT("QChar *")) ||
                    event.m_variableObject.typeName.Contains(wxT("wxChar *"))) {
                    // dont de-reference
                } else {
                    expression.Prepend(wxT("(*"));
                    expression.Append(wxT(")"));
                }
            }
            UpdateTypeResolved(expression, event.m_variableObject.typeName);
        } else if (event.m_userReason == DBG_USERR_WATCHTABLE) {
            // Double clicked on the 'Watches' table
            clMainFrame::Get()->GetDebuggerPane()->GetWatchesTable()->OnCreateVariableObject(event);

        } else if (event.m_userReason == DBG_USERR_LOCALS) {
            clMainFrame::Get()->GetDebuggerPane()->GetLocalsTable()->OnCreateVariableObj(event);
        }
    } break;
    case DBG_UR_LISTCHILDREN: {

        if (event.m_userReason == QUERY_NUM_CHILDS || event.m_userReason == LIST_WATCH_CHILDS) {
            // Watch table
            clMainFrame::Get()->GetDebuggerPane()->GetWatchesTable()->OnListChildren(event);

        } else if (event.m_userReason == QUERY_LOCALS_CHILDS || event.m_userReason == LIST_LOCALS_CHILDS ||
                   event.m_userReason == QUERY_LOCALS_CHILDS_FAKE_NODE) {
            // Locals table
            clMainFrame::Get()->GetDebuggerPane()->GetLocalsTable()->OnListChildren(event);

        } else {
            // Tooltip
            IDebugger* dbgr = DebuggerMgr::Get().GetActiveDebugger();
            if (dbgr && dbgr->IsRunning() && DbgCanInteract()) {
                if (GetDebuggerTip() && !GetDebuggerTip()->IsShown()) {
                    GetDebuggerTip()->BuildTree(event.m_varObjChildren, dbgr);
                    GetDebuggerTip()->m_mainVariableObject = event.m_expression;
                    GetDebuggerTip()->ShowDialog(
                        (event.m_userReason == DBG_USERR_WATCHTABLE || event.m_userReason == DBG_USERR_LOCALS));

                } else if (GetDebuggerTip()) {
                    // The dialog is shown
                    GetDebuggerTip()->AddItems(event.m_expression, event.m_varObjChildren);
                }
            }
        }
    } break;
    case DBG_UR_EVALVARIABLEOBJ:

        // EvaluateVariableObject callback
        if (event.m_userReason == DBG_USERR_WATCHTABLE) {
            clMainFrame::Get()->GetDebuggerPane()->GetWatchesTable()->OnEvaluateVariableObj(event);

        } else if (event.m_userReason == DBG_USERR_LOCALS) {
            clMainFrame::Get()->GetDebuggerPane()->GetLocalsTable()->OnEvaluateVariableObj(event);

        } else if (GetDebuggerTip()) {
            GetDebuggerTip()->UpdateValue(event.m_expression, event.m_evaluated);
        }
        break;
    case DBG_UR_INVALID:
    default:
        break;
    }
}

void Manager::DbgRestoreWatches()
{
    // restore any saved watch expressions from previous debug sessions
    if (m_dbgWatchExpressions.empty() == false) {
        for (size_t i = 0; i < m_dbgWatchExpressions.GetCount(); i++) {
            DebugMessage(wxT("Restoring watch: ") + m_dbgWatchExpressions.Item(i) + wxT("\n"));
            wxCommandEvent e(wxEVT_COMMAND_MENU_SELECTED, XRCID("add_watch"));
            e.SetString(m_dbgWatchExpressions.Item(i));
            clMainFrame::Get()->GetDebuggerPane()->GetWatchesTable()->GetEventHandler()->ProcessEvent(e);
        }
        m_dbgWatchExpressions.Clear();
    }
}

void Manager::DoRestartCodeLite([[maybe_unused]] bool force)
{
    wxString restartCodeLiteCommand;
    wxString workingDirectory;
    CodeLiteApp* app = dynamic_cast<CodeLiteApp*>(wxTheApp);

#if defined(__WXMSW__) || defined(__WXGTK__)
#ifdef __WXMSW__
    // We only support force restarts on Windows
    if (!force) {
        CodeLiteApp::SetRestartCodeLite(false);
        return;
    }
#endif

    restartCodeLiteCommand << clStandardPaths::Get().GetExecutablePath();
    // Restore the original working dir and any parameters
    for (int i = 1; i < wxTheApp->argc; ++i) {
        wxString cmdArg = wxTheApp->argv[i];
        ::WrapWithQuotes(cmdArg);
        restartCodeLiteCommand << wxT(" ") << cmdArg;
    }
    workingDirectory = GetOriginalCwd();

#else // OSX

    // on OSX, we use the open command
    wxFileName bundlePath(clStandardPaths::Get().GetBinFolder(), "");
    bundlePath.RemoveLastDir(); // MacOS
    bundlePath.RemoveLastDir(); // Contents
    wxString bundlePathStr = bundlePath.GetPath();
    ::WrapWithQuotes(bundlePathStr);
    restartCodeLiteCommand << "sleep 2 && open " << bundlePathStr;
    ::WrapInShell(restartCodeLiteCommand);
#endif

    // Fire an exit event (the restart takes place just before CodeLite exits)
    wxCommandEvent event(wxEVT_MENU, wxID_EXIT);
    clMainFrame::Get()->GetEventHandler()->AddPendingEvent(event);
    CodeLiteApp::SetRestartCodeLite(true);
    CodeLiteApp::SetRestartCommand(restartCodeLiteCommand, workingDirectory);
}

void Manager::OnRestart(clCommandEvent& event) { DoRestartCodeLite(false); }
void Manager::OnForcedRestart(clCommandEvent& event) { DoRestartCodeLite(true); }

void Manager::DoShowQuickWatchDialog(const DebuggerEventData& event)
{
    /////////////////////////////////////////////
    // Handle Tooltips
    /////////////////////////////////////////////

    bool useDialog = (event.m_userReason == DBG_USERR_WATCHTABLE || event.m_userReason == DBG_USERR_LOCALS);
    IDebugger* dbgr = DebuggerMgr::Get().GetActiveDebugger();
    bool canInteract = (dbgr && dbgr->IsRunning() && DbgCanInteract());
    DisplayVariableDlg* view = NULL;

    if (canInteract) {
        // First see if this type has a user-defined alternative
        // If so, don't do anything here, just create a new  for the u-d type
        // That'll bring us back here, but with the correct data
        DebuggerSettingsPreDefMap data;
        DebuggerConfigTool::Get()->ReadObject(wxT("DebuggerCommands"), &data);
        DebuggerPreDefinedTypes preDefTypes = data.GetActiveSet();

        wxString preDefinedType =
            preDefTypes.GetPreDefinedTypeForTypename(event.m_variableObject.typeName, event.m_expression);
        wxString exp, pdt;
        pdt = preDefinedType;
        exp = event.m_expression;

        pdt.Trim().Trim(false);
        exp.Trim().Trim(false);

        if (!pdt.IsEmpty() && pdt != exp) {
            dbgr->CreateVariableObject(pdt, false, DBG_USERR_QUICKWACTH);
#ifdef __WXMAC__
            if (!event.m_variableObject.gdbId.IsEmpty()) {
                dbgr->DeleteVariableObject(event.m_variableObject.gdbId);
            }
#endif
            return;
        }

        // Editor Tooltip
        view = GetDebuggerTip();
        view->m_mainVariableObject = event.m_variableObject.gdbId;
        view->m_variableName = event.m_expression;
        view->m_expression = event.m_expression;

        if (event.m_evaluated.IsEmpty() == false) {
            view->m_variableName << wxT(" = ") << event.m_evaluated;
        }

        if (event.m_variableObject.typeName.IsEmpty() == false) {
            view->m_variableName << wxT(" [") << event.m_variableObject.typeName << wxT("] ");
        }

        if (event.m_variableObject.numChilds > 0 || event.m_variableObject.has_more) {
            // Complex type
            dbgr->ListChildren(event.m_variableObject.gdbId, event.m_userReason);

        } else {
            // Simple type, no need for further calls, show the dialog
            if (!view->IsShown()) {
                view->BuildTree(event.m_varObjChildren, dbgr);
                // If the reason for showing the dialog was the 'Watches' table being d-clicked,
                // center the dialog
                view->ShowDialog(useDialog);
            }
        }
    }
}

void Manager::UpdateParserPaths(bool notify) { wxUnusedVar(notify); }

void Manager::DoSaveAllFilesBeforeBuild()
{
    // Save all files before compiling, but dont saved new documents
    SendCmdEvent(wxEVT_FILE_SAVE_BY_BUILD_START);
    if (!clMainFrame::Get()->GetMainBook()->SaveAll(false, false)) {
        SendCmdEvent(wxEVT_FILE_SAVE_BY_BUILD_END);
        return;
    }
    SendCmdEvent(wxEVT_FILE_SAVE_BY_BUILD_END);
}

DisplayVariableDlg* Manager::GetDebuggerTip()
{
    if (!m_watchDlg) {
        m_watchDlg = new DisplayVariableDlg(clMainFrame::Get()->GetMainBook());
    }
    return m_watchDlg;
}

void Manager::OnProjectSettingsModified(clProjectSettingsEvent& event)
{
    event.Skip();
    // Get the project settings
    clMainFrame::Get()->SelectBestEnvSet();
}

void Manager::GetActiveProjectAndConf(wxString& project, wxString& conf)
{
    if (!IsWorkspaceOpen()) {
        project.Clear();
        conf.Clear();
        return;
    }

    project = GetActiveProjectName();
    BuildMatrixPtr matrix = clCxxWorkspaceST::Get()->GetBuildMatrix();
    if (!matrix) {
        return;
    }

    wxString workspaceConf = matrix->GetSelectedConfigurationName();
    matrix->GetProjectSelectedConf(workspaceConf, project);
}

void Manager::UpdatePreprocessorFile(clEditor* editor) { wxUnusedVar(editor); }

bool Manager::DbgCanInteract()
{
    /// First, we also propogate this question to the plugins
    clDebugEvent de(wxEVT_DBG_CAN_INTERACT);
    if (EventNotifier::Get()->ProcessEvent(de)) {
        // a plugin answered this question, we assume that a debug session is running by
        // another plugin so avoid using our built-in debugger
        return de.IsAnswer();
    }
    return m_dbgCanInteract;
}

void Manager::OnBuildEnded(clBuildEvent& event) { event.Skip(); }

void Manager::DbgContinue()
{
    IDebugger* debugger = DebuggerMgr::Get().GetActiveDebugger();
    if (debugger && debugger->IsRunning()) {
        // debugger is already running, so issue a 'cont' command
        clMainFrame::Get()->GetDebuggerPane()->GetLocalsTable()->ResetTableColors();
        clMainFrame::Get()->GetDebuggerPane()->GetWatchesTable()->ResetTableColors();
        debugger->Continue();
    }
}

void Manager::OnBuildStarting(clBuildEvent& event)
{
    // Always Skip it
    event.Skip();

    if (!clCxxWorkspaceST::Get()->IsOpen())
        return;

    wxStringSet_t usedCompilers, deletedCompilers;
    clCxxWorkspaceST::Get()->GetCompilers(usedCompilers);

    // Check to see if any of the compilers were deleted
    wxString strDeletedCompilers;
    for (const auto& compilerName : usedCompilers) {
        if (!BuildSettingsConfigST::Get()->IsCompilerExist(compilerName)) {
            deletedCompilers.insert(compilerName);
            strDeletedCompilers << "'" << compilerName << "'\n";
        }
    }

    if (deletedCompilers.empty())
        // nothing more to be done here
        return;

    strDeletedCompilers.RemoveLast(); // remove last \n

    // Prompt the user and suggest an alternative
    CompilersModifiedDlg dlg(NULL, deletedCompilers);
    if (dlg.ShowModal() != wxID_OK) {
        event.Skip(false);
        wxString message;
        message << _("Build cancelled. The following compilers referred by the workspace could not be found:\n")
                << strDeletedCompilers << "\n"
                << _("Please fix your project settings by selecting a valid compiler");
        ::wxMessageBox(message, _("Build Aborted"), wxOK | wxCENTER | wxICON_ERROR, EventNotifier::Get()->TopFrame());
        return;

    } else {

        // User mapped the old compilers with new ones -> create an alias between the actual compiler and the
        wxStringMap_t table = dlg.GetReplacementTable();
        wxString defaultCompiler;

        // Clone each compiler
        for (const auto& p : table) {
            // We can't create a compiler without name, so we'll try to adjust the project setting instead
            if (p.first.IsEmpty()) {
                defaultCompiler = p.second;
                continue;
            }
            CompilerPtr pCompiler = BuildSettingsConfigST::Get()->GetCompiler(p.second);
            pCompiler->SetName(p.first);
            BuildSettingsConfigST::Get()->SetCompiler(pCompiler);
        }

        // For projects with no valid compiler set, replace them with the one specified by user
        if (!defaultCompiler.IsEmpty()) {
            clCxxWorkspaceST::Get()->ReplaceCompilers({ { "", defaultCompiler } });
        }

        // Prompt the user and cancel the build
        ::wxMessageBox(_("Compilers updated successfully!\nYou can now build your workspace"), "CodeLite",
                       wxOK | wxCENTER | wxICON_INFORMATION);
        event.Skip(false);
    }
}

bool Manager::StartTTY(const wxString& title, wxString& tty)
{
#ifndef __WXMSW__
    m_debuggerTerminal.Clear();
    m_debuggerTerminal.Launch(title);
    if (m_debuggerTerminal.IsValid()) {
        tty = m_debuggerTerminal.GetTty();
    }
    return m_debuggerTerminal.IsValid();
#else
    wxUnusedVar(title);
    wxUnusedVar(tty);
    return false;
#endif
}

void Manager::OnProjectRenamed(clCommandEvent& event)
{
    event.Skip();
    if (clCxxWorkspaceST::Get()->IsOpen()) {
        ReloadWorkspace();
    }
}

void Manager::OnFindInFilesDismissed(clFindInFilesEvent& event)
{
    event.Skip();
    if (clCxxWorkspaceST::Get()->IsOpen()) {
        clConfig::Get().Write("FindInFiles/CXX/Mask", event.GetFileMask());
        clConfig::Get().Write("FindInFiles/CXX/LookIn", event.GetPaths());
    }
}

bool Manager::IsDebuggerViewVisible(const wxString& name)
{
    DebuggerPane* debuggerPane = clMainFrame::Get()->GetDebuggerPane();
    if (debuggerPane) {
        int sel = debuggerPane->GetNotebook()->GetSelection();
        if (sel != wxNOT_FOUND) {
            if (debuggerPane->GetNotebook()->GetPageText(sel) == name) {
                return true;
            }
        }
    }

    // Also test if the pane is detached
    return IsPaneVisible(name);
}

void Manager::ShowNewProjectWizard(const wxString& workspaceFolder)
{
    wxFrame* mainFrame = EventNotifier::Get()->TopFrame();
    clNewProjectEvent newProjectEvent(wxEVT_NEW_PROJECT_WIZARD_SHOWING);
    newProjectEvent.SetEventObject(mainFrame);
    if (EventNotifier::Get()->ProcessEvent(newProjectEvent)) {
        return;
    }

    NewProjectDialog dlg(mainFrame);
    if (dlg.ShowModal() == wxID_OK) {

        ProjectData data = dlg.GetProjectData();
        // Ensure that the project directory exists
        if (!wxFileName::DirExists(data.m_path)) {
            wxFileName::Mkdir(data.m_path, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
        }

        // Try the plugins first
        clNewProjectEvent finishEvent(wxEVT_NEW_PROJECT_WIZARD_FINISHED);
        finishEvent.SetEventObject(mainFrame);
        finishEvent.SetToolchain(data.m_cmpType);
        finishEvent.SetDebugger(data.m_debuggerType);
        finishEvent.SetProjectName(data.m_name);
        finishEvent.SetProjectFolder(data.m_path);
        finishEvent.SetTemplateName(data.m_sourceTemplate);
        if (EventNotifier::Get()->ProcessEvent(finishEvent)) {
            return;
        }

        // Carry on with the default behaviour
        CreateProject(data, workspaceFolder);
    }
}

void Manager::OnUpdateDebuggerActiveView(clDebugEvent& event)
{
    if (DebuggerMgr::Get().IsNativeDebuggerRunning()) {
        UpdateDebuggerPane();

    } else {
        event.Skip();
    }
}

void Manager::OnDebuggerSetMemory(clDebugEvent& event)
{
    if (DebuggerMgr::Get().IsNativeDebuggerRunning()) {
        SetMemory(event.GetMemoryAddress(), event.GetMemoryBlockSize(), event.GetMemoryBlockValue());
    } else {
        event.Skip();
    }
}

void Manager::OnHideGdbTooltip(clCommandEvent& event)
{
    event.Skip();
    if (GetDebuggerTip()) {
        GetDebuggerTip()->HideDialog();
    }
}

void Manager::OnFindInFilesShowing(clFindInFilesEvent& event)
{
    event.Skip();
    if (clCxxWorkspaceST::Get()->IsOpen()) {
        // Load the C++ workspace values from the configuration
        event.SetFileMask(clConfig::Get().Read("FindInFiles/CXX/Mask",
                                               wxString("*.c;*.cpp;*.cxx;*.cc;*.h;*.hpp;*.inc;*.mm;*.m;*.xrc;"
                                                        "*.xml;*.json;*.sql;*.txt;*.plist;CMakeLists.txt;*.rc;*.iss")));
        event.SetPaths(clConfig::Get().Read("FindInFiles/CXX/LookIn", wxString("<Entire Workspace>")));
    }
}

void Manager::OnDebuggerAtFileLine(clDebugEvent& event)
{
    event.Skip();
    if (event.IsSSHDebugging()) {
#if USE_SFTP
        DbgUnMarkDebuggerLine();
        if (event.GetLineNumber() < 0) {
            return;
        }
        // Open the file using ssh
        auto editor = clSFTPManager::Get().OpenFile(event.GetFileName(), event.GetSshAccount());
        if (editor) {
            clEditor* cl_editor = dynamic_cast<clEditor*>(editor);
            if (cl_editor) {
                cl_editor->HighlightLine(event.GetLineNumber() - 1);
                cl_editor->SetEnsureCaretIsVisible(cl_editor->PositionFromLine(event.GetLineNumber() - 1), false);
            }
        }
#endif
    } else {
        const wxString& fileName = event.GetFileName();
        long lineNumber = event.GetLineNumber();
        if (m_frameLineno != wxNOT_FOUND) {
            lineNumber = m_frameLineno;
            m_frameLineno = wxNOT_FOUND;
        }

        DbgMarkDebuggerLine(fileName, lineNumber);
        UpdateDebuggerPane();
    }
}
