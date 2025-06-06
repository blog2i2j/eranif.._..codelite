//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// copyright            : (C) 2008 by Eran Ifrah
// file name            : cpptoken.h
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
#ifndef __cpptoken__
#define __cpptoken__

#include <string>
#include <list>
#include <map>
#include "codelite_exports.h"
#include <wx/string.h>
#include <wx/wxsqlite3.h>
#include <vector>
#include <wx/filename.h>
#include <set>
#include <algorithm>
#include "wxStringHash.h"

typedef std::vector<wxFileName> wxFileList_t;
class WXDLLIMPEXP_CL CppToken
{
    int m_id;
    wxString name; // the name of the token
    size_t offset; // file offset
    wxString filename;
    size_t lineNumber;

public:
    typedef std::vector<CppToken> Vec_t;

    class wxFileNameToString
    {
    public:
        wxString operator()(const wxFileName& fn) { return fn.GetFullPath(); }
    };

    // Required by later versions of clang compiler
    bool operator<(const CppToken& rhs) const;

    /**
     * @class Predicate
     * a helper class that can be used to filter tokens using std:remove_if
     * this Predicate filters files that do not exist in m_files member
     */
    class RemoveIfNotIn
    {
        std::unordered_set<wxString> m_files;

    public:
        RemoveIfNotIn(const wxFileList_t& fileList)
        {

            // first convert vector<wxFileName> to vector<wxString>
            std::vector<wxString> v;
            v.resize(fileList.size());
            std::transform(fileList.begin(), fileList.end(), v.begin(), wxFileNameToString());

            // construct a set based on the vector content
            m_files = std::unordered_set<wxString>(v.begin(), v.end());
        }

        RemoveIfNotIn(const std::unordered_set<wxString>& filesSet)
            : m_files(filesSet)
        {
        }

        /**
         * @brief return true if the token's file does _not_ exists in the
         * m_files member (which was initialized while constructing the Predicate)
         */
        bool operator()(const CppToken& token) { return m_files.count(token.getFilename()) == 0; }
    };

public:
    CppToken();
    CppToken(wxSQLite3ResultSet& res);
    ~CppToken();

    void reset();
    void append(wxChar ch);

    void setName(const wxString& name) { this->name = name; }

    void setOffset(size_t offset) { this->offset = offset; }

    const wxString& getName() const { return name; }
    size_t getOffset() const { return offset; }

    void setFilename(const wxString& filename) { this->filename = filename; }

    const wxString& getFilename() const { return filename; }

    void setId(int id) { this->m_id = id; }

    int getId() const { return m_id; }

    size_t getLineNumber() const { return lineNumber; }

    void setLineNumber(size_t lineNo) { lineNumber = lineNo; }

    // Provide lower-than operator so we can use std::vector::sort method
    bool operator<(const CppToken& rhs) { return filename < rhs.filename; }
};

class WXDLLIMPEXP_CL CppTokensMap
{
    std::unordered_map<wxString, std::vector<CppToken>*> m_tokens;

public:
    CppTokensMap();
    ~CppTokensMap();

    /**
     * @brief add token to the map. if token with same name already exists, it will be appended so multiple tokens with
     * same name is allowed
     * @param token token to add
     */
    void addToken(const CppToken& token);

    /**
     * @brief clear all token
     */
    void clear();

    /**
     * @brief return true if no tokens were found
     */
    bool is_empty();
};

#endif // __cpptoken__
