#include "php_code_completion.h"

#include "ColoursAndFontsManager.h"
#include "LSP/LSPEvent.h"
#include "LSP/basic_types.h"
#include "PHP/PHPEntityBase.h"
#include "PHP/PHPEntityClass.h"
#include "PHP/PHPEntityFunction.h"
#include "PHP/PHPEntityFunctionAlias.h"
#include "PHP/PHPEntityKeyword.h"
#include "PHP/PHPEntityVariable.h"
#include "PHP/PHPExpression.h"
#include "clEditorBar.h"
#include "clSelectSymbolDialog.h"
#include "cl_command_event.h"
#include "cl_config.h"
#include "database/entry.h"
#include "editor_config.h"
#include "event_notifier.h"
#include "globals.h"
#include "imanager.h"
#include "macros.h"
#include "php_parser_thread.h"
#include "php_utils.h"
#include "php_workspace.h"
#include "plugin.h"
#include "tags_options_data.h"
#include "wxCodeCompletionBoxManager.h"

#include <algorithm>
#include <stack>
#include <thread>
#include <vector>
#include <wx/log.h>
#include <wx/stc/stc.h>

///////////////////////////////////////////////////////////////////

struct PHPCCUserData : public wxClientData {
    PHPEntityBase::Ptr_t entry;
    PHPCCUserData(PHPEntityBase::Ptr_t e)
        : entry(e)
    {
    }
};

/// Ascending sorting function
struct _SAscendingSort {
    bool operator()(const TagEntryPtr& rStart, const TagEntryPtr& rEnd)
    {
        return rEnd->GetName().Cmp(rStart->GetName()) > 0;
    }
};
///////////////////////////////////////////////////////////////////

PHPCodeCompletion* PHPCodeCompletion::m_instance = NULL;

PHPCodeCompletion::PHPCodeCompletion()
    : m_manager(NULL)
    , m_typeInfoTooltip(NULL)
{
    EventNotifier::Get()->Bind(wxEVT_CMD_RETAG_WORKSPACE, &PHPCodeCompletion::OnRetagWorkspace, this);
    EventNotifier::Get()->Bind(wxEVT_CMD_RETAG_WORKSPACE_FULL, &PHPCodeCompletion::OnRetagWorkspace, this);

    EventNotifier::Get()->Bind(wxEVT_FILE_SAVED, clCommandEventHandler(PHPCodeCompletion::OnFileSaved), this);
    EventNotifier::Get()->Bind(wxEVT_ACTIVE_EDITOR_CHANGED, &PHPCodeCompletion::OnActiveEditorChanged, this);
    EventNotifier::Get()->Bind(wxEVT_CC_CODE_COMPLETE_LANG_KEYWORD, &PHPCodeCompletion::OnCodeCompleteLangKeywords,
                               this);
    EventNotifier::Get()->Bind(wxEVT_CC_CODE_COMPLETE_BOX_DISMISSED, &PHPCodeCompletion::OnCodeCompletionBoxDismissed,
                               this);
    EventNotifier::Get()->Bind(wxEVT_CC_GENERATE_DOXY_BLOCK, &PHPCodeCompletion::OnInsertDoxyBlock, this);
    EventNotifier::Get()->Bind(wxPHP_PARSE_ENDED, &PHPCodeCompletion::OnParseEnded, this);

    // code completion events
    EventNotifier::Get()->Bind(wxEVT_CC_CODE_COMPLETE, &PHPCodeCompletion::OnCodeComplete, this);
    EventNotifier::Get()->Bind(wxEVT_CC_CODE_COMPLETE_FUNCTION_CALLTIP, &PHPCodeCompletion::OnFunctionCallTip, this);
    EventNotifier::Get()->Bind(wxEVT_CC_TYPEINFO_TIP, &PHPCodeCompletion::OnTypeinfoTip, this);
    EventNotifier::Get()->Bind(wxEVT_CC_FIND_SYMBOL, &PHPCodeCompletion::OnFindSymbol, this);
    EventNotifier::Get()->Bind(wxEVT_CC_JUMP_HYPER_LINK, &PHPCodeCompletion::OnQuickJump, this);
}

PHPCodeCompletion::~PHPCodeCompletion()
{
    EventNotifier::Get()->Unbind(wxEVT_ACTIVE_EDITOR_CHANGED, &PHPCodeCompletion::OnActiveEditorChanged, this);
    EventNotifier::Get()->Unbind(wxEVT_CMD_RETAG_WORKSPACE, &PHPCodeCompletion::OnRetagWorkspace, this);
    EventNotifier::Get()->Unbind(wxEVT_CMD_RETAG_WORKSPACE_FULL, &PHPCodeCompletion::OnRetagWorkspace, this);

    EventNotifier::Get()->Unbind(wxEVT_FILE_SAVED, clCommandEventHandler(PHPCodeCompletion::OnFileSaved), this);
    EventNotifier::Get()->Unbind(wxEVT_CC_CODE_COMPLETE_LANG_KEYWORD, &PHPCodeCompletion::OnCodeCompleteLangKeywords,
                                 this);
    EventNotifier::Get()->Unbind(wxEVT_CC_CODE_COMPLETE_BOX_DISMISSED, &PHPCodeCompletion::OnCodeCompletionBoxDismissed,
                                 this);
    EventNotifier::Get()->Unbind(wxEVT_CC_GENERATE_DOXY_BLOCK, &PHPCodeCompletion::OnInsertDoxyBlock, this);
    EventNotifier::Get()->Unbind(wxPHP_PARSE_ENDED, &PHPCodeCompletion::OnParseEnded, this);
    // code completion events
    EventNotifier::Get()->Unbind(wxEVT_CC_CODE_COMPLETE, &PHPCodeCompletion::OnCodeComplete, this);
    EventNotifier::Get()->Unbind(wxEVT_CC_CODE_COMPLETE_FUNCTION_CALLTIP, &PHPCodeCompletion::OnFunctionCallTip, this);
    EventNotifier::Get()->Unbind(wxEVT_CC_TYPEINFO_TIP, &PHPCodeCompletion::OnTypeinfoTip, this);
    EventNotifier::Get()->Unbind(wxEVT_CC_FIND_SYMBOL, &PHPCodeCompletion::OnFindSymbol, this);
    EventNotifier::Get()->Unbind(wxEVT_CC_JUMP_HYPER_LINK, &PHPCodeCompletion::OnQuickJump, this);
}

PHPCodeCompletion* PHPCodeCompletion::Instance()
{
    if(!m_instance) {
        m_instance = new PHPCodeCompletion();
    }
    return m_instance;
}

void PHPCodeCompletion::Release()
{
    if(m_instance) {
        delete m_instance;
    }
    m_instance = NULL;
}

void PHPCodeCompletion::DoShowCompletionBox(const PHPEntityBase::List_t& entries, PHPExpression::Ptr_t expr)
{
    wxCodeCompletionBoxEntry::Vec_t ccEntries;
    TagEntryPtrVector_t tags;
    wxStringSet_t uniqueEntries;

    // search for the old item
    for (PHPEntityBase::Ptr_t entry : entries) {
        if(uniqueEntries.count(entry->GetFullName()) == 0) {
            uniqueEntries.insert(entry->GetFullName());
        } else {
            // don't add duplicate entries
            continue;
        }

        PHPEntityBase::Ptr_t ns = expr->GetSourceFile()->Namespace(); // the namespace at the source file
        TagEntryPtr t = DoPHPEntityToTagEntry(entry);

        tags.push_back(t);
    }

    std::sort(tags.begin(), tags.end(), _SAscendingSort());
    for(size_t i = 0; i < tags.size(); ++i) {
        wxCodeCompletionBoxEntry::Ptr_t ccEntry = wxCodeCompletionBox::TagToEntry(tags.at(i));
        ccEntry->SetComment(tags.at(i)->GetComment());
        ccEntries.push_back(ccEntry);
    }
    wxCodeCompletionBoxManager::Get().ShowCompletionBox(m_manager->GetActiveEditor()->GetCtrl(), ccEntries,
                                                        wxCodeCompletionBox::kRefreshOnKeyType, wxNOT_FOUND);
}

void PHPCodeCompletion::OnCodeCompletionBoxDismissed(clCodeCompletionEvent& e) { e.Skip(); }

TagEntryPtr PHPCodeCompletion::DoPHPEntityToTagEntry(PHPEntityBase::Ptr_t entry)
{
    TagEntryPtr t(new TagEntry());
    // wxString name = entry->Is(kEntityTypeNamespace) ? entry->GetFullName() : entry->GetShortName();
    wxString name = entry->GetShortName();

    if(entry->Is(kEntityTypeVariable) && entry->Cast<PHPEntityVariable>()->IsMember() && name.StartsWith("$") &&
       !entry->Cast<PHPEntityVariable>()->IsStatic()) {
        name.Remove(0, 1);
    } else if((entry->Is(kEntityTypeClass) || entry->Is(kEntityTypeNamespace)) && name.StartsWith("\\")) {
        name.Remove(0, 1);
    }

    t->SetName(name);
    t->SetParent(entry->Parent() ? entry->Parent()->GetFullName() : "");
    t->SetPattern(t->GetName());

    // Set the document comment
    wxString comment, docComment;
    docComment = entry->GetDocComment();
    if(docComment.IsEmpty() == false) {
        docComment.Trim().Trim(false);    // The Doc comment
        comment << docComment << "\n==="; // HLine
    }

    wxFileName fn(entry->GetFilename());
    fn.MakeRelativeTo(PHPWorkspace::Get()->GetFilename().GetPath());
    comment << fn.GetFullName() << " : " << entry->GetLine();

    t->SetComment(comment);

    // Access
    if(entry->Is(kEntityTypeVariable)) {
        PHPEntityVariable* var = entry->Cast<PHPEntityVariable>();

        // visibility
        if(var->IsPrivate()) {
            t->SetAccess("private");
        } else if(var->IsProtected()) {
            t->SetAccess("protected");
        } else {
            t->SetAccess("public");
        }

        // type (affects icon)
        if(var->IsConst() || var->IsDefine()) {
            t->SetKind("macro");
        } else if(var->IsMember()) {
            t->SetKind("member");
        } else {
            t->SetKind("variable");
        }
        t->SetTypename("");

    } else if(entry->Is(kEntityTypeFunction) || entry->Is(kEntityTypeFunctionAlias)) {
        PHPEntityFunction* func = NULL;
        if(entry->Is(kEntityTypeFunctionAlias)) {
            func = entry->Cast<PHPEntityFunctionAlias>()->GetFunc()->Cast<PHPEntityFunction>();
        } else {
            func = entry->Cast<PHPEntityFunction>();
        }

        if(func->HasFlag(kFunc_Private)) {
            t->SetAccess("private");
        } else if(func->HasFlag(kFunc_Protected)) {
            t->SetAccess("protected");
        } else {
            t->SetAccess("public");
        }
        t->SetSignature(func->GetSignature());
        t->SetPattern(func->GetShortName() + func->GetSignature());
        t->SetKind("function");

    } else if(entry->Is(kEntityTypeClass)) {
        t->SetAccess("public");
        t->SetKind("class");

    } else if(entry->Is(kEntityTypeNamespace)) {
        t->SetAccess("public");
        t->SetKind("namespace");

    } else if(entry->Is(kEntityTypeKeyword)) {
        t->SetAccess("public");
        t->SetKind("cpp_keyword");
    }

    t->SetFlags(TagEntry::Tag_No_Signature_Format);
    return t;
}

void PHPCodeCompletion::OnCodeComplete(clCodeCompletionEvent& e)
{
    e.Skip(true);
    if(PHPWorkspace::Get()->IsOpen()) {
        IEditor* editor = GetEditor(e.GetFileName());
        if(editor && wxWindow::FindFocus() == editor->GetCtrl() && IsPHPFile(editor)) {
            e.Skip(false);

            // Update the settings
            TagsOptionsData d;
            clConfig ccConfig("code-completion.conf");
            ccConfig.ReadItem(&d);
            m_lookupTable.SetSizeLimit(d.GetCcNumberOfDisplayItems());

            // Check if the code completion was triggered due to user
            // typing '(', in this case, call OnFunctionCallTip()
            wxChar charAtPos = editor->GetCharAtPos(editor->GetCurrentPosition() - 1);
            if(charAtPos == '(') {
                OnFunctionCallTip(e);

            } else {
                // Perform the code completion here
                PHPExpression::Ptr_t expr(new PHPExpression(editor->GetTextRange(0, editor->GetCurrentPosition())));
                bool isExprStartsWithOpenTag = expr->IsExprStartsWithOpenTag();
                PHPEntityBase::Ptr_t entity = expr->Resolve(m_lookupTable, editor->GetFileName().GetFullPath());
                if(entity) {
                    // Suggest members for the resolved entity
                    PHPEntityBase::List_t matches;
                    expr->Suggest(entity, m_lookupTable, matches);
                    if(!expr->GetFilter().IsEmpty() && (expr->GetCount() == 0)) {
                        // Word completion
                        PHPEntityBase::List_t keywords = PhpKeywords(expr->GetFilter());

                        // Prepend the keywords
                        matches.insert(matches.end(), keywords.begin(), keywords.end());

                        // Did user typed "<?ph" or "<?php" ??
                        // If so, clear the matches
                        if(isExprStartsWithOpenTag && (expr->GetFilter() == "ph" || expr->GetFilter() == "php")) {
                            matches.clear();
                        }
                    }

                    // Remove duplicates from the list
                    if(!matches.empty()) {
                        // Show the code completion box
                        DoShowCompletionBox(matches, expr);
                    }
                }
            }
        }
    }
}

void PHPCodeCompletion::OnFunctionCallTip(clCodeCompletionEvent& e)
{
    e.Skip();
    if(PHPWorkspace::Get()->IsOpen()) {
        IEditor* editor = GetEditor(e.GetFileName());
        if(!CanCodeComplete(e, editor)) {
            return;
        }

        if(editor) {
            // we handle only .php files
            if(IsPHPFile(editor)) {
                // this is our to complete
                e.Skip(false);

                // get the position
                PHPEntityBase::Ptr_t resolved = DoGetPHPEntryUnderTheAtPos(editor, editor->GetCurrentPosition(), true);
                if(resolved) {
                    // In PHP there is no overloading, so there can be only one signature for a function
                    // so we simply place our match into TagEntryPtrVector_t structure and pass it to the editor
                    TagEntryPtrVector_t tags;
                    tags.push_back(DoPHPEntityToTagEntry(resolved));
                    clCallTipPtr callTip(new clCallTip(tags));
                    editor->ShowCalltip(callTip);
                }
            }
        }
    }
}

struct PHPFindSymbol_ClientData : public wxClientData {
    PHPEntityBase::Ptr_t m_ptr;

    PHPFindSymbol_ClientData(PHPEntityBase::Ptr_t ptr) { m_ptr = ptr; }
    virtual ~PHPFindSymbol_ClientData() {}
};

void PHPCodeCompletion::OnFindSymbol(clCodeCompletionEvent& e)
{
    e.Skip();
    if(PHPWorkspace::Get()->IsOpen()) {
        IEditor* editor = GetEditor(e.GetFileName());
        if(!CanCodeComplete(e, editor)) {
            return;
        }

        e.Skip(false);
        if(editor) {
            wxString word = editor->GetWordAtCaret();
            if(word.IsEmpty()) {
                return;
            }
            PHPEntityBase::List_t symbols = m_lookupTable.FindSymbol(word);
            if(symbols.size() == 1) {
                PHPEntityBase::Ptr_t match = *symbols.begin();
                DoOpenEditorForEntry(match);

            } else {
                // Convert the matches to clSelectSymbolDialogEntry::List_t
                clSelectSymbolDialogEntry::List_t entries;
                for (auto& entry : symbols) {
                    TagEntryPtr tag = DoPHPEntityToTagEntry(entry);
                    wxBitmap bmp = wxCodeCompletionBox::GetBitmap(tag);

                    clSelectSymbolDialogEntry m;
                    m.bmp = bmp;
                    m.name = entry->GetFullName();
                    m.clientData = new PHPFindSymbol_ClientData(entry);
                    m.help = tag->GetKind();
                    entries.push_back(m);
                }

                // Show selection dialog
                clSelectSymbolDialog dlg(EventNotifier::Get()->TopFrame(), entries);
                if(dlg.ShowModal() != wxID_OK) {
                    return;
                }
                PHPFindSymbol_ClientData* cd = dynamic_cast<PHPFindSymbol_ClientData*>(dlg.GetSelection());
                if(cd) {
                    DoOpenEditorForEntry(cd->m_ptr);
                }
            }
        }
    }
}

void PHPCodeCompletion::OnTypeinfoTip(clCodeCompletionEvent& e)
{
    if(PHPWorkspace::Get()->IsOpen()) {
        IEditor* editor = GetEditor(e.GetFileName());
        if(!CanCodeComplete(e, editor)) {
            return;
        }

        if(editor) {
            if(IsPHPFile(editor)) {
                PHPEntityBase::Ptr_t entity = GetPHPEntityAtPos(editor, e.GetPosition());
                if(entity) {
                    e.SetTooltip(entity->ToTooltip());
                }
                return;
            }
        }

    } else {
        e.Skip();
    }
}

PHPLocation::Ptr_t PHPCodeCompletion::FindDefinition(IEditor* editor)
{
    CHECK_PHP_WORKSPACE_RET_NULL();
    PHPLocation::Ptr_t loc; // Null
    if(IsPHPFile(editor)) {
        PHPEntityBase::Ptr_t resolved = GetPHPEntityAtPos(editor, editor->GetCurrentPosition());
        if(resolved) {
            if(resolved->Is(kEntityTypeFunctionAlias)) {
                // use the internal function
                resolved = resolved->Cast<PHPEntityFunctionAlias>()->GetFunc();
            }
            loc = std::make_shared<PHPLocation>();
            loc->filename = resolved->GetFilename().GetFullPath();
            loc->linenumber = resolved->GetLine();
            loc->what = resolved->GetShortName();
        }
    }
    return loc;
}

void PHPCodeCompletion::OnCodeCompleteLangKeywords(clCodeCompletionEvent& e)
{
    // disable the default behavior of the keyword CC
    // by simply hooking into this event and calling Skip(false);
    e.Skip(false);
}

PHPEntityBase::Ptr_t PHPCodeCompletion::GetPHPEntityAtPos(IEditor* editor, int pos)
{
    return DoGetPHPEntryUnderTheAtPos(editor, pos, false);
}

/* static */
bool PHPCodeCompletion::CanCodeComplete(clCodeCompletionEvent& e, IEditor* editor)
{
    int pos = e.GetPosition();
    if(pos) {
        pos -= 1;
    }

    CHECK_PTR_RET_FALSE(editor);

    // we can get style 0 if we added chars and they were not styled just yet
    // sd we use the first style near our position (backward)
    int lineNumber = editor->LineFromPos(pos);
    int lineStartPos = editor->PosFromLine(lineNumber);

    if(lineStartPos > pos) {
        return false;
    }

    int styleAt(0);
    int retryLeft(pos - lineStartPos + 2);
    while((styleAt == 0) && retryLeft && pos > 0) {
        styleAt = editor->GetStyleAtPos(pos);
        if(styleAt == 0) {
            --pos;
        }
        --retryLeft;
    }

    return (editor && !e.IsInsideCommentOrString() && IsPHPSection(styleAt) && !IsPHPCommentOrString(styleAt));
}

void PHPCodeCompletion::OnFileSaved(clCommandEvent& event)
{
    event.Skip();
    IEditor* editor = clGetManager()->GetActiveEditor();
    CHECK_PTR_RET(editor);

    // check if the saved file is a PHP file
    // In case it is, then re-parse the file and store the results
    if(::IsPHPFile(editor)) {
        PHPParserThreadRequest* req = new PHPParserThreadRequest(PHPParserThreadRequest::kParseSingleFile);
        req->file = event.GetFileName();
        req->workspaceFile = PHPWorkspace::Get()->GetFilename().GetFullPath();
        PHPParserThread::Instance()->Add(req);
    }
}

void PHPCodeCompletion::OnRetagWorkspace(wxCommandEvent& event)
{
    event.Skip();
    if(PHPWorkspace::Get()->IsOpen()) {
        event.Skip(false);
        bool isFull = (event.GetEventType() == wxEVT_CMD_RETAG_WORKSPACE_FULL);
        if(isFull) {
            // Delete the file
            m_lookupTable.ResetDatabase();
        }

        // Reparse the workspace
        PHPWorkspace::Get()->ParseWorkspace(isFull);
    }
}

PHPEntityBase::Ptr_t PHPCodeCompletion::DoGetPHPEntryUnderTheAtPos(IEditor* editor, int pos, bool forFunctionCalltip)
{
    if(!PHPWorkspace::Get()->IsOpen()) {
        return PHPEntityBase::Ptr_t(NULL);
    }
    pos = editor->GetCtrl()->WordEndPosition(pos, true);

    // Get the expression under the caret
    wxString unsavedBuffer = editor->GetTextRange(0, pos);
    wxString filter;
    PHPEntityBase::Ptr_t resolved;

    // Parse the source file
    PHPSourceFile source(unsavedBuffer, &m_lookupTable);
    source.SetFilename(editor->GetFileName());
    source.SetParseFunctionBody(false);
    source.Parse();

    PHPEntityBase::Ptr_t currentScope = source.CurrentScope();
    if(currentScope && currentScope->Is(kEntityTypeClass)) {
        // we are trying to resolve a 'word' under the caret within the class
        // body but _not_ within a function body (i.e. it can only be
        // a definition of some kind)
        // try to construct an expression that will work
        int wordStart = editor->GetCtrl()->WordStartPosition(pos, true);
        wxString theWord = editor->GetTextRange(wordStart, pos);
        wxString theWordNoDollar = theWord;
        if(theWord.StartsWith("$")) {
            theWordNoDollar = theWord.Mid(1);
        }
        PHPExpression expr2(unsavedBuffer, "<?php $this->" + theWordNoDollar, forFunctionCalltip);
        resolved = expr2.Resolve(m_lookupTable, editor->GetFileName().GetFullPath());
        filter = expr2.GetFilter();
        if(!resolved) {
            // Maybe its a static member/function/const, try using static keyword
            PHPExpression expr3(unsavedBuffer, "<?php static::" + theWord, forFunctionCalltip);
            resolved = expr2.Resolve(m_lookupTable, editor->GetFileName().GetFullPath());
            filter = expr2.GetFilter();
        }
    }

    if(!resolved) {
        PHPExpression expr(unsavedBuffer, "", forFunctionCalltip);
        resolved = expr.Resolve(m_lookupTable, editor->GetFileName().GetFullPath());
        filter = expr.GetFilter();
    }

    if(resolved && !filter.IsEmpty()) {
        resolved = m_lookupTable.FindMemberOf(resolved->GetDbId(), filter);
        if(!resolved) {
            // Fallback to functions and constants
            PHPEntityBase::List_t children =
                m_lookupTable.FindGlobalFunctionAndConsts(PHPLookupTable::kLookupFlags_ExactMatch, filter);
            if(children.size() == 1) {
                resolved = *children.begin();
            }
        }
        if(resolved && resolved->Is(kEntityTypeFunction)) {
            // for a function, we need to load its children (function arguments)
            resolved->SetChildren(m_lookupTable.LoadFunctionArguments(resolved->GetDbId()));
        } else if(resolved && resolved->Is(kEntityTypeFunctionAlias)) {
            // for a function alias, we need to load the actual functions' children (function arguments)
            PHPEntityBase::Ptr_t realFunc = resolved->Cast<PHPEntityFunctionAlias>()->GetFunc();
            realFunc->SetChildren(m_lookupTable.LoadFunctionArguments(realFunc->GetDbId()));
        }
    }
    return resolved;
}

void PHPCodeCompletion::Open(const wxFileName& workspaceFile)
{
    Close();
    m_lookupTable.Open(workspaceFile.GetPath());
    m_lookupTable.RebuildClassCache();

    // Cache the symbols into the OS caching, this is done by simply reading the entire file content and
    // then closing it
    wxFileName fnDBFile(workspaceFile.GetPath(), "phpsymbols.db");
    fnDBFile.AppendDir(".codelite");
}

void PHPCodeCompletion::Close()
{
    if(m_lookupTable.IsOpened()) {
        m_lookupTable.Close();
    }
}

void PHPCodeCompletion::OnInsertDoxyBlock(clCodeCompletionEvent& e)
{
    e.Skip();

    // Do we have a workspace open?
    CHECK_COND_RET(PHPWorkspace::Get()->IsOpen());

    // Sanity
    IEditor* editor = GetEditor(e.GetFileName());
    CHECK_PTR_RET(editor);

    // Is this a PHP editor?
    CHECK_COND_RET(IsPHPFile(editor));

    int lineNumber = editor->LineFromPos(editor->GetCurrentPosition()) + 1;

    // Get the text from the caret current position
    // until the end of file
    wxString unsavedBuffer = editor->GetTextRange(0, editor->GetLength());

    // Close comment to allow parsing
    unsavedBuffer.insert(editor->GetCurrentPosition() - 1, "/");

    PHPSourceFile source(unsavedBuffer, &m_lookupTable);
    source.SetParseFunctionBody(false);
    source.Parse();
    const auto& matches = source.GetAllMatchesInOrder();
    for(const auto& match : matches) {
        if(match->GetLine() == lineNumber && match->Is(kEntityTypeFunction)) {
            e.Skip(false); // notify codelite to stop processing this event

            CommentConfigData data;
            EditorConfigST::Get()->ReadObject("CommentConfigData", &data);

            wxString phpdoc = match->FormatPhpDoc(data);
            phpdoc.Trim();
            e.SetTooltip(phpdoc);
        }
    }
}

void PHPCodeCompletion::OnQuickJump(clCodeCompletionEvent& e)
{
    // this is identical to "Find Symbol"
    e.Skip();

    // Do we have a workspace open?
    CHECK_COND_RET(PHPWorkspace::Get()->IsOpen());

    // Sanity
    IEditor* editor = GetEditor(e.GetFileName());
    CHECK_PTR_RET(editor);
    CHECK_EXPECTED_RETURN(IsPHPFile(editor), true);

    e.Skip(false);
    GotoDefinition(editor);
}

void PHPCodeCompletion::GotoDefinition(IEditor* editor)
{
    CHECK_PTR_RET(editor);
    wxStyledTextCtrl* sci = editor->GetCtrl();
    CHECK_PTR_RET(sci);

    PHPLocation::Ptr_t definitionLocation = FindDefinition(editor);
    CHECK_PTR_RET(definitionLocation);

    // Open the file (make sure we use the 'OpenFile' so we will get a browsing record)
    IEditor* activeEditor =
        m_manager->OpenFile(definitionLocation->filename, wxEmptyString, definitionLocation->linenumber);
    if(activeEditor) {
        int selectFromPos = activeEditor->GetCtrl()->PositionFromLine(definitionLocation->linenumber);
        DoSelectInEditor(activeEditor, definitionLocation->what, selectFromPos);
    }
}

void PHPCodeCompletion::DoSelectInEditor(IEditor* editor, const wxString& what, int from)
{
    if(editor) {
        editor->GetCtrl()->ClearSelections();
        editor->FindAndSelectV(what, what, from, NULL);
    }
}

wxString PHPCodeCompletion::ExpandRequire(const wxFileName& curfile, const wxString& require)
{
    PHPScanner_t scanner = ::phpLexerNew("<?php " + require);
    if(!scanner) {
        return "";
    }

    wxString outFile;
    phpLexerToken token;
    while(::phpLexerNext(scanner, token)) {
        if(token.IsAnyComment()) {
            continue;
        }
        switch(token.type) {
        case kPHP_T_REQUIRE:
        case kPHP_T_REQUIRE_ONCE:
            break;
        case kPHP_T_CONSTANT_ENCAPSED_STRING: {
            wxString str = token.Text();
            str.Trim().Trim(false);
            // strip the " or ' from the string
            str.Remove(0, 1).RemoveLast();
            outFile << str;
            break;
        }
        case kPHP_T_FILE:
            outFile << curfile.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
            break;
        case kPHP_T_DIR:
            outFile << curfile.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
            break;
        }
    }

    wxFileName fileName(outFile);
    if(fileName.IsOk() && fileName.IsRelative()) {
        wxArrayString paths;
        paths.Add(curfile.GetPath());
        // Relative path, we try to resolve it by looking at the include path give to us
        PHPProject::Ptr_t proj = PHPWorkspace::Get()->GetActiveProject();
        if(proj) {
            wxArrayString incpaths = proj->GetSettings().GetIncludePathAsArray();
            paths.insert(paths.end(), incpaths.begin(), incpaths.end());
        }

        for(size_t i = 0; i < paths.GetCount(); ++i) {
            wxFileName tmpFile = fileName;
            if(tmpFile.MakeAbsolute(paths.Item(i))) {
                wxString fullpath = tmpFile.GetFullPath();
                if(wxFileName::Exists(fullpath)) {
                    fileName = tmpFile;
                    break;
                }
            }
        }
    }

    if(fileName.IsOk()) {
        fileName.Normalize();
        outFile = fileName.GetFullPath();
    }
    ::phpLexerDestroy(&scanner);
    return outFile;
}

int PHPCodeCompletion::GetLocationForSettersGetters(const wxString& filecontent, const wxString& classname)
{
    PHPScannerLocker locker(filecontent);
    PHPScanner_t scanner = locker.scanner;
    if(!scanner) {
        return wxNOT_FOUND;
    }

    phpLexerToken token;
    bool isOK = false;
    while(::phpLexerNext(scanner, token)) {
        if(token.type != kPHP_T_CLASS && token.type != kPHP_T_ENUM) {
            continue;
        }

        if(::phpLexerNext(scanner, token) && token.type == kPHP_T_IDENTIFIER && token.Text() == classname) {
            // we found the class definition
            isOK = true;
            break;
        }
    }

    if(!isOK) {
        return wxNOT_FOUND;
    }

    int depth = 0;
    int line = wxNOT_FOUND;

    // search for the open brace
    while(::phpLexerNext(scanner, token)) {
        if(token.type == '{') {
            ++depth;
            break;
        }
    }

    if(!depth) {
        return wxNOT_FOUND;
    }
    // Now search for the closing brace
    while(::phpLexerNext(scanner, token)) {
        if(token.type == '}') {
            line = token.lineNumber;
            --depth;
            if(depth == 0) {
                break;
            }
        } else if(token.type == '{') {
            ++depth;
        }
    }

    if(depth == 0) {
        return line;
    }
    return wxNOT_FOUND;
}

PHPEntityBase::List_t PHPCodeCompletion::PhpKeywords(const wxString& prefix) const
{
    LexerConf::Ptr_t lexer = ColoursAndFontsManager::Get().GetLexer("php");
    if(!lexer) {
        return PHPEntityBase::List_t();
    }
    wxString lcPrefix = prefix.Lower();
    PHPEntityBase::List_t lst;
    wxString phpKeywords = lexer->GetKeyWords(4);
    wxArrayString phpKeywordsArr = ::wxStringTokenize(phpKeywords, " \t", wxTOKEN_STRTOK);
    for(size_t i = 0; i < phpKeywordsArr.GetCount(); ++i) {
        wxString lcWord = phpKeywordsArr.Item(i).Lower();
        if(lcWord.StartsWith(lcPrefix)) {
            PHPEntityBase::Ptr_t keyword(new PHPEntityKeyword());
            keyword->SetFullName(phpKeywordsArr.Item(i));
            keyword->SetShortName(phpKeywordsArr.Item(i));
            lst.push_back(keyword);
        }
    }
    return lst;
}

void PHPCodeCompletion::GetMembers(IEditor* editor, PHPEntityBase::List_t& members, wxString& scope)
{
    members.clear();
    scope.clear();
    if(!editor) {
        return;
    }

    // Parse until the current position to get the current scope name
    {
        wxString text = editor->GetTextRange(0, editor->GetCurrentPosition());
        PHPSourceFile sourceFile(text, &m_lookupTable);
        sourceFile.SetParseFunctionBody(true);
        sourceFile.SetFilename(editor->GetFileName());
        sourceFile.Parse();

        const PHPEntityClass* scopeAtPoint = sourceFile.Class()->Cast<PHPEntityClass>();
        if(!scopeAtPoint) {
            return;
        }
        scope = scopeAtPoint->GetFullName();
    }

    // Second parse: parse the entire buffer so we are not limited by the caret position
    wxString text = editor->GetTextRange(0, editor->GetLength());
    PHPSourceFile sourceFile(text, &m_lookupTable);
    sourceFile.SetParseFunctionBody(true);
    sourceFile.SetFilename(editor->GetFileName());
    sourceFile.Parse();

    // Locate the scope
    PHPEntityBase::Ptr_t parentClass = sourceFile.Namespace()->FindChild(scope);
    if(!parentClass) {
        return;
    }

    // filter out
    for (const auto child : parentClass->GetChildren()) {
        if (child->Is(kEntityTypeVariable) && child->Cast<PHPEntityVariable>()->IsMember() &&
           !child->Cast<PHPEntityVariable>()->IsConst() && !child->Cast<PHPEntityVariable>()->IsStatic()) {
            // a member of a class
            members.push_back(child);
        }
    }
}

void PHPCodeCompletion::DoOpenEditorForEntry(PHPEntityBase::Ptr_t entry)
{
    // Open the file (make sure we use the 'OpenFile' so we will get a browsing record)
    IEditor* editor = m_manager->OpenFile(entry->GetFilename().GetFullPath(), wxEmptyString, entry->GetLine());
    if(editor) {
        // Select the word in the editor (its a new one)
        int selectFromPos = editor->GetCtrl()->PositionFromLine(entry->GetLine());
        DoSelectInEditor(editor, entry->GetShortName(), selectFromPos);
    }
}

void PHPCodeCompletion::OnParseEnded(clParseEvent& event)
{
    event.Skip();
    m_lookupTable.RebuildClassCache();
}

void PHPCodeCompletion::OnActiveEditorChanged(wxCommandEvent& e)
{
    e.Skip();
    IEditor* editor = clGetManager()->GetActiveEditor();
    if(!editor || FileExtManager::GetTypeFromExtension(editor->GetFileName()) != FileExtManager::TypePhp) {
        return;
    }

    PHPEntityBase::List_t functions;
    if(m_lookupTable.FindFunctionsByFile(editor->GetFileName(), functions) == 0) {
        return;
    }

    // prepare list of scopes and send them to the navigation bar
    clEditorBar::ScopeEntry::vec_t scopes;
    scopes.reserve(functions.size());

    for(PHPEntityBase::Ptr_t entity : functions) {
        if(!entity->Is(kEntityTypeFunction)) {
            continue;
        }

        // prepare a display string, exclude the function signature (it takes too much space)
        clEditorBar::ScopeEntry scope_entry;
        scope_entry.line_number = entity->GetLine();
        scope_entry.display_string = entity->GetFullName();
        scope_entry.display_string << "()";
        scopes.push_back(scope_entry);
    }

    wxString fullpath = editor->GetRemotePathOrLocal();
    clGetManager()->GetNavigationBar()->SetScopes(fullpath, scopes);

    // For the sake of outline view, fire wxEVT_LSP_DOCUMENT_SYMBOLS_QUICK_OUTLINE event

    // parse the current source file
    wxString text = editor->GetTextRange(0, editor->GetLength());

    auto parse_callback = [=](const wxString& text) {
        PHPSourceFile source(text, NULL);
        source.SetParseFunctionBody(false);
        source.SetFilename(editor->GetFileName());
        source.Parse();

        // get list of all entities
        // dfs traverse
        auto top_level = source.Namespace();
        std::stack<PHPEntityBase::Ptr_t> q;
        std::vector<LSP::SymbolInformation> symbols;
        q.push(top_level);
        while(!q.empty()) {
            PHPEntityBase::Ptr_t entity = q.top();
            q.pop();

            LSP::SymbolInformation si;
            LSP::Location location;
            LSP::Range range;
            // the location
            range.SetEnd({ entity->GetLine(), 0 }).SetStart({ entity->GetLine(), 0 });
            location.SetRange(range);
            si.SetLocation(location);

            si.SetName(entity->GetShortName());
            if(entity->Parent()) {
                si.SetContainerName(entity->Parent()->GetShortName());
            }

            if(entity->Is(kEntityTypeFunction)) {
                si.SetKind(LSP::kSK_Function);
            } else if(entity->Is(kEntityTypeClass)) {
                si.SetKind(LSP::kSK_Class);
            } else if(entity->Is(kEntityTypeNamespace)) {
                si.SetKind(LSP::kSK_Namespace);
            } else if(entity->Is(kEntityTypeVariable)) {
                si.SetKind(LSP::kSK_Variable);
            } else {
                si.SetKind(LSP::kSK_TypeParameter);
            }
            symbols.emplace_back(si);

            if(!entity->GetChildren().empty()) {
                for(auto child : entity->GetChildren()) {
                    q.push(child);
                }
            }
        }

        LSPEvent symbols_event{ wxEVT_LSP_DOCUMENT_SYMBOLS_QUICK_OUTLINE };
        symbols_event.GetSymbolsInformation().reserve(symbols.size());
        symbols_event.GetSymbolsInformation().insert(symbols_event.GetSymbolsInformation().end(), symbols.begin(),
                                                     symbols.end());

        symbols_event.SetFileName(editor->GetFileName().GetFullPath());
        EventNotifier::Get()->AddPendingEvent(symbols_event);
    };

    // perform the parsing in the background
    std::thread thr(parse_callback, text);
    thr.detach();
}

IEditor* PHPCodeCompletion::GetEditor(const wxString& filepath) const
{
    auto editor = clGetManager()->FindEditor(filepath);
    if(editor && editor == clGetManager()->GetActiveEditor()) {
        return editor;
    }
    return nullptr;
}
