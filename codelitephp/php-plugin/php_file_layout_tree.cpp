#include "php_file_layout_tree.h"

#include "PHP/PHPEntityFunction.h"
#include "PHP/PHPEntityVariable.h"
#include "PHP/PHPSourceFile.h"
#include "fileutils.h"
#include "globals.h"
#include "ieditor.h"
#include "imanager.h"

#include <algorithm>

#ifndef __WXMSW__
#include <wx/imaglist.h>
#endif

PHPFileLayoutTree::PHPFileLayoutTree(wxWindow* parent, IEditor* editor, IManager* manager)
    : wxTreeCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
          wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT | wxTR_NO_LINES | wxTR_SINGLE)
    , m_editor(editor)
    , m_manager(manager)
{
}

PHPFileLayoutTree::PHPFileLayoutTree(wxWindow* parent)
    : wxTreeCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
          wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT | wxTR_NO_LINES | wxTR_SINGLE)
    , m_editor(NULL)
    , m_manager(NULL)
{
    MSWSetNativeTheme(this);
}

PHPFileLayoutTree::~PHPFileLayoutTree() {}

void PHPFileLayoutTree::Construct()
{
    // Sanity
    if(!m_editor || !m_manager) return;

    wxString text = m_editor->GetTextRange(0, m_editor->GetLength());
    PHPSourceFile source(text, NULL);
    source.SetParseFunctionBody(false);
    source.SetFilename(m_editor->GetFileName());
    source.Parse();

    DeleteAllItems();
    wxTreeItemId root = AddRoot(wxT("Root"));

    wxImageList* images = new wxImageList(clGetScaledSize(16), clGetScaledSize(16), true);
    images->Add(m_manager->GetStdIcons()->LoadBitmap(wxT("cc/16/globals")));            // 0
    images->Add(m_manager->GetStdIcons()->LoadBitmap(wxT("cc/16/function_private")));   // 1
    images->Add(m_manager->GetStdIcons()->LoadBitmap(wxT("cc/16/function_protected"))); // 2
    images->Add(m_manager->GetStdIcons()->LoadBitmap(wxT("cc/16/function_public")));    // 3
    images->Add(m_manager->GetStdIcons()->LoadBitmap(wxT("cc/16/member_private")));     // 4
    images->Add(m_manager->GetStdIcons()->LoadBitmap(wxT("cc/16/member_protected")));   // 5
    images->Add(m_manager->GetStdIcons()->LoadBitmap(wxT("cc/16/member_public")));      // 6
    images->Add(m_manager->GetStdIcons()->LoadBitmap(wxT("cc/16/namespace")));          // 7
    images->Add(m_manager->GetStdIcons()->LoadBitmap(wxT("cc/16/class")));              // 8
    images->Add(m_manager->GetStdIcons()->LoadBitmap(wxT("cc/16/enumerator")));         // 9
    AssignImageList(images);

    // Build the tree view
    BuildTree(root, source.Namespace());

    if(HasChildren(GetRootItem())) {
        wxTreeItemIdValue cookie;
        wxTreeItemId child = GetFirstChild(GetRootItem(), cookie);
        if(child.IsOk()) {
            SelectItem(child);
            ScrollTo(child);
        }
        ExpandAll();
    }
}

int PHPFileLayoutTree::GetImageId(PHPEntityBase::Ptr_t entry)
{
    if(entry->Is(kEntityTypeFunction)) {
        PHPEntityFunction* func = entry->Cast<PHPEntityFunction>();

        if(func->HasFlag(kFunc_Private))
            return 1;
        else if(func->HasFlag(kFunc_Protected))
            return 2;
        else
            // public
            return 3;

    } else if(entry->Is(kEntityTypeVariable)) {
        PHPEntityVariable* var = entry->Cast<PHPEntityVariable>();
        if(!var->IsMember() && !var->IsConst()) {
            // A global variable
            return 6;

        } else if(var->IsMember()) {
            if(var->HasFlag(kVar_Const)) return 9; // constant
            // Member
            if(var->HasFlag(kVar_Private))
                return 4;
            else if(var->HasFlag(kVar_Protected))
                return 5;
            else
                return 6;

        } else if(var->IsConst()) {
            // Constant
            return 9;
        } else {
            return 6;
        }

    } else if(entry->Is(kEntityTypeNamespace)) {
        // Namespace
        return 7;
    } else if(entry->Is(kEntityTypeClass)) {
        return 8;
    }
    return -1; // Unknown
}

wxTreeItemId PHPFileLayoutTree::RecurseSearch(const wxTreeItemId& item, const wxString& word)
{
    if(!item.IsOk()) return wxTreeItemId();

    if(item != GetRootItem()) {
        if(FileUtils::FuzzyMatch(word, GetItemText(item))) {
            return item;
        }
    }

    if(ItemHasChildren(item)) {
        wxTreeItemIdValue cookie;
        wxTreeItemId child = GetFirstChild(item, cookie);
        while(child.IsOk()) {
            wxTreeItemId selection = RecurseSearch(child, word);
            if(selection.IsOk()) {
                return selection;
            }
            child = GetNextChild(item, cookie);
        }
    }
    return wxTreeItemId();
}

wxTreeItemId PHPFileLayoutTree::TryGetPrevItem(wxTreeItemId item)
{
    wxCHECK_MSG(item.IsOk(), wxTreeItemId(), wxT("invalid tree item"));

    // find out the starting point
    wxTreeItemId prevItem = GetPrevSibling(item);
    if(!prevItem.IsOk()) {
        prevItem = GetItemParent(item);
        if(prevItem == GetRootItem()) {
            return wxTreeItemId();
        }
    }

    // from there we must be able to navigate until this item
    while(prevItem.IsOk()) {

        ScrollTo(prevItem);

        if(!IsVisible(prevItem)) {
            return wxTreeItemId();
        }

        const wxTreeItemId nextItem = GetNextVisible(prevItem);
        if(!nextItem.IsOk() || nextItem == item) return prevItem;

        prevItem = nextItem;
    }

    return wxTreeItemId();
}

void PHPFileLayoutTree::BuildTree(wxTreeItemId parentTreeItem, PHPEntityBase::Ptr_t entity)
{
    int imgID = GetImageId(entity);
    wxTreeItemId parent = AppendItem(parentTreeItem, entity->GetDisplayName(), imgID, imgID, new QItemData(entity));
    // dont add the children of the function (i.e. function arguments)
    if (entity->Is(kEntityTypeFunction)) return;

    for (const auto& child : entity->GetChildren()) {
        BuildTree(parent, child);
    }
}
