//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// copyright            : (C) 2009 by Eran Ifrah
// file name            : istorage.h
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

#ifndef ISTORAGE_H
#define ISTORAGE_H

#include "comment.h"
#include "entry.h"
#include "fileentry.h"
#include "pptable.h"
#include "tag_tree.h"

#include <memory>
#include <wx/filename.h>

#define MAX_SEARCH_LIMIT 250

/**
 * @class ITagsStorage defined the tags storage API used by codelite
 * @author eran
 * @date 10/06/09
 * @file istorage.h
 * @brief
 */
class ITagsStorage
{
protected:
    wxFileName m_fileName;
    int m_singleSearchLimit;
    bool m_useCache;
    bool m_enableCaseInsensitive;

public:
    enum { OrderNone, OrderAsc, OrderDesc };

public:
    ITagsStorage()
        : m_singleSearchLimit(MAX_SEARCH_LIMIT)
        , m_useCache(false)
        , m_enableCaseInsensitive(true)
    {
    }

    virtual ~ITagsStorage() = default;
    virtual void SetEnableCaseInsensitive(bool b) { m_enableCaseInsensitive = b; }
    virtual void SetUseCache(bool useCache) { this->m_useCache = useCache; }

    virtual bool GetUseCache() const { return m_useCache; }

    /**
     * @brief clear the storage cache
     */
    virtual void ClearCache() = 0;

    /**
     * Return the currently opened database.
     * @return Currently open database
     */
    const wxFileName& GetDatabaseFileName() const { return m_fileName; }

    void SetSingleSearchLimit(int singleSearchLimit)
    {
        if(singleSearchLimit < 0) {
            singleSearchLimit = MAX_SEARCH_LIMIT;
        }
        this->m_singleSearchLimit = singleSearchLimit;
    }

    int GetSingleSearchLimit() const { return m_singleSearchLimit; }

    // -----------------------------------------------------------------
    // -------------------- API pure virtual ---------------------------
    // -----------------------------------------------------------------

    /**
     * @brief return list of tags based on scope and name
     * @param scope the scope to search. If 'scope' is empty the scope is omitted from the search
     * @param name
     * @param partialNameAllowed
     */
    virtual void GetTagsByScopeAndName(const wxString& scope, const wxString& name, bool partialNameAllowed,
                                       std::vector<TagEntryPtr>& tags) = 0;
    virtual void GetTagsByScopeAndName(const wxArrayString& scope, const wxString& name, bool partialNameAllowed,
                                       std::vector<TagEntryPtr>& tags) = 0;

    /**
     * @brief return list of tags by scope. If the cache is enabled, tags will be fetched from the
     * cache instead of accessing the disk
     * @param scope
     * @param limit maximum items to fetch
     * @param limitExceeded set to true if the
     * @param tags
     */
    virtual void GetTagsByScope(const wxString& scope, std::vector<TagEntryPtr>& tags) = 0;

    /**
     * @brief return array of tags by kind.
     * @param kinds array of kinds
     * @param orderingColumn the column that the output should be ordered by (leave empty for no sorting)
     * @param order OrderAsc, OrderDesc or use OrderNone for no ordering the results
     * @param tags
     */
    virtual void GetTagsByKind(const wxArrayString& kinds, const wxString& orderingColumn, int order,
                               std::vector<TagEntryPtr>& tags) = 0;

    /**
     * @brief return array of items by path
     * @param path
     * @param tags
     */
    virtual void GetTagsByPath(const wxArrayString& path, std::vector<TagEntryPtr>& tags) = 0;
    virtual void GetTagsByPath(const wxString& path, std::vector<TagEntryPtr>& tags, int limit = 1) = 0;
    virtual void GetTagsByPathAndKind(const wxString& path, std::vector<TagEntryPtr>& tags,
                                      const std::vector<wxString>& kinds, int limit = 1) = 0;

    /**
     * @brief return array of items by name and parent
     * @param path
     * @param tags
     */
    virtual void GetTagsByNameAndParent(const wxString& name, const wxString& parent,
                                        std::vector<TagEntryPtr>& tags) = 0;

    /**
     * @brief return array of tags by kind and path
     * @param kinds array of kind
     * @param path
     * @param tags  [output]
     */
    virtual void GetTagsByKindAndPath(const wxArrayString& kinds, const wxString& path,
                                      std::vector<TagEntryPtr>& tags) = 0;

    /**
     * @brief return tags by file and line number
     * @param file
     * @param line
     * @param tags [output]
     */
    virtual void GetTagsByFileAndLine(const wxString& file, int line, std::vector<TagEntryPtr>& tags) = 0;

    /**
     * @brief return list by kind and scope while using a filter
     * @param scope
     * @param kinds
     * @param tags [output]
     */
    virtual void GetTagsByScopeAndKind(const wxString& scope, const wxArrayString& kinds, const wxString& filter,
                                       std::vector<TagEntryPtr>& tags) = 0;

    /**
     * @brief get list of tags by kind and file
     * @param kind
     * @param orderingColumn the column that the output should be ordered by (leave empty for no sorting)
     * @param order OrderAsc, OrderDesc or use OrderNone for no ordering the results
     * @param tags
     */
    virtual void GetTagsByKindAndFile(const wxArrayString& kind, const wxString& fileName,
                                      const wxString& orderingColumn, int order, std::vector<TagEntryPtr>& tags) = 0;

    /**
     * @brief
     * @param scope
     * @param tags
     */
    virtual void GetDereferenceOperator(const wxString& scope, std::vector<TagEntryPtr>& tags) = 0;

    /**
     * @brief
     * @param scope
     * @param tags
     */
    virtual void GetSubscriptOperator(const wxString& scope, std::vector<TagEntryPtr>& tags) = 0;

    // -------------------------- Files Table -------------------------------------------

    /**
     * @brief delete an entry by file name
     * @param filename
     * @return
     */
    virtual int DeleteFileEntry(const wxString& filename) = 0;

    /**
     * @brief insert entry by file name
     * @param filename
     * @return
     */
    virtual int InsertFileEntry(const wxString& filename, int timestamp) = 0;

    /**
     * @brief update file entry using file name as key
     * @param filename
     * @param timestamp new timestamp
     * @return
     */
    virtual int UpdateFileEntry(const wxString& filename, int timestamp) = 0;

    // -------------------------- TagEntry -------------------------------------------
    /**
     * Return a result set of tags according to file name.
     * @param file Source file name
     * @param path Database file name
     * @return result set
     */
    virtual void SelectTagsByFile(const wxString& file, std::vector<TagEntryPtr>& tags,
                                  const wxFileName& path = wxFileName()) = 0;

    /**
     * @brief return true if type exist under a given scope.
     * Incase it exist but under the <global> scope, 'scope' will be modified
     * @param typeName type to search
     * @param scope [intput/output]
     * @return true on success
     */
    virtual bool IsTypeAndScopeExist(wxString& typeName, wxString& scope) = 0;

    /**
     * @brief return true if typeName & scope exists in the storage (it uses LIMIT 1, hence LimitOne)
     * @param typeName
     * @param scope
     * @return
     */
    virtual bool IsTypeAndScopeExistLimitOne(const wxString& typeName, const wxString& scope) = 0;

    /**
     * Return the current version of the database library .
     * @return current version of the database library
     */
    virtual const wxString& GetVersion() const = 0;

    /**
     * Schema version as appears in TAGS_VERSION table
     * @return schema's version
     */
    virtual wxString GetSchemaVersion() const = 0;

    /**
     * @brief determine the current scope based on file name and line number
     */
    virtual TagEntryPtr GetScope(const wxString& filename, int line_number) = 0;

    /**
     * Store tree of tags into db.
     * @param tree Tags tree to store
     * @param path Database file name
     * @param autoCommit handle the Store operation inside a transaction or let the user handle it
     */
    virtual void Store(const std::vector<TagEntryPtr>& tags, bool auto_commit = true) = 0;

    /**
     * return list of files from the database. The returned list is ordered
     * by name (ascending)
     * @param partialName part of the file name to act as a filter
     * @param files [output] array of files
     */
    virtual void GetFiles(const wxString& partialName, std::vector<FileEntryPtr>& files) = 0;

    /**
     * @brief return list of files from the database
     * @param files vector of database record
     */
    virtual void GetFiles(std::vector<FileEntryPtr>& files) = 0;

    /**
     * @brief this function is for supporting CC inside an include statement
     * line
     */
    virtual void GetFilesForCC(const wxString& userTyped, wxArrayString& matches) = 0;

    /**
     * @brief for transactional storage, provide begin/commit/rollback methods
     */
    virtual void Begin() = 0;
    virtual void Commit() = 0;
    virtual void Rollback() = 0;

    /**
     * Delete all entries from database that are related to filename.
     * @param path Database name
     * @param fileName File name
     * @param autoCommit handle the Delete operation inside a transaction or let the user handle it
     */
    virtual void DeleteByFileName(const wxFileName& path, const wxString& fileName, bool autoCommit = true) = 0;

    /**
     * Open sqlite database.
     * @param fileName Database file name
     */
    virtual void OpenDatabase(const wxFileName& fileName) = 0;

    /**
     * Test whether the database is opened
     * @return true if database is attached to a file
     */
    virtual const bool IsOpen() const = 0;

    /**
     * @brief return list of tags by scopes and kinds
     * @param scopes array of possible scopes
     * @param kinds array of possible kinds
     * @param tags [output]
     */
    virtual void GetTagsByScopesAndKind(const wxArrayString& scopes, const wxArrayString& kinds,
                                        std::vector<TagEntryPtr>& tags) = 0;

    /**
     * @brief return macro evaluation
     * @param name macro to search
     */
    virtual PPToken GetMacro(const wxString& name) = 0;

    /**
     * @brief return list of tags for a given prefix
     */
    virtual void GetTagsByName(const wxString& prefix, std::vector<TagEntryPtr>& tags, bool exactMatch = false) = 0;

    /**
     * @brief return list of tags for a given partial name
     */
    virtual void GetTagsByPartName(const wxString& partname, std::vector<TagEntryPtr>& tags) = 0;

    /**
     * @brief same as above, but allow multiple name parts
     */
    virtual void GetTagsByPartName(const wxArrayString& parts, std::vector<TagEntryPtr>& tags) = 0;

    /**
     * @brief search for a single match in the database for an entry with a given name
     */
    virtual TagEntryPtr GetTagsByNameLimitOne(const wxString& name) = 0;

    /**
     * @brief return list of tags only visible to `filepath`
     * this usually includes static members, or any entity
     * in an anonymous namespace
     */
    virtual size_t GetFileScopedTags(const wxString& filepath, const wxString& name, const wxArrayString& kinds,
                                     std::vector<TagEntryPtr>& tags) = 0;

    /**
     * @brief load function parameters
     */
    virtual size_t GetParameters(const wxString& function_path, std::vector<TagEntryPtr>& tags) = 0;

    /**
     * @brief load all lambda functions for a given function
     */
    virtual size_t GetLambdas(const wxString& parent_function, std::vector<TagEntryPtr>& tags) = 0;
};

enum { TagOk = 0, TagExist, TagError };

using ITagsStoragePtr = std::shared_ptr<ITagsStorage>;

#endif // ISTORAGE_H
