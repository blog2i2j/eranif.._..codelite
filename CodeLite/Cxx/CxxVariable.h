#ifndef CXXVARIABLE_H
#define CXXVARIABLE_H

#include "CxxLexerAPI.h"
#include "codelite_exports.h"
#include "macros.h"
#include "wxStringHash.h"

#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>
#include <wx/string.h>

class WXDLLIMPEXP_CL CxxVariable
{
public:
    struct LexerToken {
        int type;
        int _depth;
        wxString text;
        wxString comment;

        LexerToken()
            : type(0)
            , _depth(0)
        {
        }

        LexerToken(const CxxLexerToken& token, int depth)
        {
            FromCxxLexerToken(token);
            this->_depth = depth;
        }

        int GetType() const { return type; }
        void FromCxxLexerToken(const CxxLexerToken& token)
        {
            this->type = token.GetType();
            this->comment = token.GetWXComment();
            this->text = token.GetWXString();
        }
        typedef std::vector<CxxVariable::LexerToken> Vec_t;
    };

    enum eFlags {
        kToString_None = 0,
        // Include the variable name
        kToString_Name = (1 << 0),
        // Include the default value
        kToString_DefaultValue = (1 << 1),
        kToString_Default = kToString_Name,
    };

protected:
    wxString m_name;
    CxxVariable::LexerToken::Vec_t m_type;
    wxString m_defaultValue;
    eCxxStandard m_standard;
    wxString m_pointerOrReference;
    bool m_isAuto = false;
    int m_lineNumber = wxNOT_FOUND;

public:
    using Ptr_t = std::shared_ptr<CxxVariable>;
    using Vec_t = std::vector<CxxVariable::Ptr_t>;
    using Map_t = std::unordered_map<wxString, CxxVariable::Ptr_t>;

public:
    CxxVariable(eCxxStandard standard);
    CxxVariable()
        : m_standard(eCxxStandard::kCxx11)
        , m_isAuto(false)
    {
    }

    virtual ~CxxVariable();

    void SetName(const wxString& name) { this->m_name = name; }
    void SetType(const CxxVariable::LexerToken::Vec_t& type) { this->m_type = type; }
    const wxString& GetName() const { return m_name; }
    const CxxVariable::LexerToken::Vec_t& GetType() const { return m_type; }
    // @see ToString() for description about 'table'
    wxString GetTypeAsString(const wxStringTable_t& table = wxStringTable_t()) const;
    // @see ToString() for description about 'table'
    wxString GetTypeAsCxxString(const wxStringTable_t& table = wxStringTable_t()) const;

    static wxString PackType(const CxxVariable::LexerToken::Vec_t& type, eCxxStandard standard,
                             bool omitClassKeyword = false, const wxStringTable_t& table = wxStringTable_t());

    /**
     * @brief return true if this variable was constructed from a statement like:
     * using MyInt = int;
     */
    bool IsUsing() const { return GetTypeAsString() == "using" && !m_defaultValue.IsEmpty(); }

    /**
     * @brief is this a valid variable?
     */
    bool IsOk() const { return !m_name.IsEmpty() && !m_type.empty(); }

    /**
     * @brief is this an "auto" variable?
     */
    bool IsAuto() const { return m_isAuto; }

    void SetLine(int l) { m_lineNumber = l; }
    int GetLineNumber() const { return m_lineNumber; }

    void SetIsAuto(bool b) { m_isAuto = b; }
    /**
     * @brief return a string representation for this variable
     * @param flags see values in eFlags
     */
    wxString ToString(size_t flags = CxxVariable::kToString_Default) const;

    void SetDefaultValue(const wxString& defaultValue) { this->m_defaultValue = defaultValue; }
    const wxString& GetDefaultValue() const { return m_defaultValue; }

    void SetPointerOrReference(const wxString& pointerOrReference) { this->m_pointerOrReference = pointerOrReference; }
    const wxString& GetPointerOrReference() const { return m_pointerOrReference; }
};

#endif // CXXVARIABLE_H
