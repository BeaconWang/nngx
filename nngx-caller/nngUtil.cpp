#include "nngx.h"
#include "nngUtil.h"

#ifdef _WIN32
#include <AccCtrl.h>
#include <AclAPI.h>
#include <Windows.h>
#include <atlbase.h>

namespace nng::util::detail {
    class CAccessSecurityDescriptor
    {
    public:
        CAccessSecurityDescriptor() {
            ATLVERIFY(AssignPermissions());
        }
        ~CAccessSecurityDescriptor()
        {
            if (m_pEveryoneSID)
            {
                FreeSid(m_pEveryoneSID);
                m_pEveryoneSID = nullptr;
            }

            if (m_pAdminSID)
            {
                FreeSid(m_pAdminSID);
                m_pAdminSID = nullptr;
            }

            if (m_pACL)
            {
                LocalFree(m_pACL);
                m_pACL = nullptr;
            }

            if (m_pSD)
            {
                LocalFree(m_pSD);
                m_pSD = nullptr;
            }
        }

        operator void* () { return m_pSD; }
    private:
        BOOL AssignPermissions()
        {
            DWORD dwRes;
            EXPLICIT_ACCESS ea[2];
            SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
            SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;

            if (!AllocateAndInitializeSid(&SIDAuthWorld, 1,
                SECURITY_WORLD_RID,
                0, 0, 0, 0, 0, 0, 0,
                &m_pEveryoneSID))
            {
                return FALSE;
            }

            ZeroMemory(&ea, 2 * sizeof(EXPLICIT_ACCESS));
            ea[0].grfAccessPermissions = (GENERIC_READ | GENERIC_WRITE);
            ea[0].grfAccessMode = SET_ACCESS;
            ea[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
            ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
            ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
            ea[0].Trustee.ptstrName = (LPTSTR)m_pEveryoneSID;

            if (!AllocateAndInitializeSid(&SIDAuthNT, 2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0,
                &m_pAdminSID))
            {
                return FALSE;
            }
            ea[1].grfAccessPermissions = GENERIC_ALL;
            ea[1].grfAccessMode = SET_ACCESS;
            ea[1].grfInheritance = NO_INHERITANCE;
            ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
            ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
            ea[1].Trustee.ptstrName = (LPTSTR)m_pAdminSID;

            // Create a new ACL that contains the new ACEs.
            dwRes = SetEntriesInAcl(2, ea, NULL, &m_pACL);
            if (ERROR_SUCCESS != dwRes)
            {
                return FALSE;
            }

            // Initialize a security descriptor.  
            m_pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
            if (NULL == m_pSD)
            {
                return FALSE;
            }

            if (!InitializeSecurityDescriptor(m_pSD, SECURITY_DESCRIPTOR_REVISION))
            {
                return FALSE;
            }

            if (!SetSecurityDescriptorDacl(m_pSD, TRUE, m_pACL, FALSE))   // not a default DACL 
            {
                return FALSE;
            }

            return TRUE;
        }

    private:
        PSID m_pEveryoneSID = nullptr;
        PSID m_pAdminSID = nullptr;
        PACL m_pACL = nullptr;
        PSECURITY_DESCRIPTOR m_pSD = nullptr;
    };

    bool IsProcessElevated() {
        bool isElevated = false;
        HANDLE hToken = NULL;

        // Open the current process token
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            TOKEN_ELEVATION elevation;
            DWORD dwSize;

            // Get token elevation information
            if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize)) {
                isElevated = elevation.TokenIsElevated != 0;
            }

            CloseHandle(hToken);
        }

        return isElevated;
    }

    bool IsSystemAuthority() {
        bool isSystem = false;
        HANDLE hToken = NULL;
        UCHAR buffer[sizeof(TOKEN_USER) + sizeof(SID)];
        PTOKEN_USER pTokenUser = (PTOKEN_USER)buffer;
        DWORD dwSize;

        // Open the current process token
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            // Get token user information
            if (GetTokenInformation(hToken, TokenUser, pTokenUser, sizeof(buffer), &dwSize)) {
                // Check if the SID is the SYSTEM account
                SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
                PSID systemSid;
                if (AllocateAndInitializeSid(&ntAuthority, 1, SECURITY_LOCAL_SYSTEM_RID,
                    0, 0, 0, 0, 0, 0, 0, &systemSid)) {
                    isSystem = EqualSid(pTokenUser->User.Sid, systemSid);
                    FreeSid(systemSid);
                }
            }

            CloseHandle(hToken);
        }

        return isSystem;
    }
}
#endif


// 初始化 NNG 库
// 返回：操作结果，0 表示成功
int nng::util::initialize() noexcept {

    nng::Dialer::set_pre_address(
        nng::util::_Pre_address
    );

    nng::Listener::set_pre_address(
        nng::util::_Pre_address
    );

    nng::Listener::set_pre_start(
        nng::util::_Pre_start_listen
    );

    return nng::init();
    }


// 释放 NNG 库资源
void nng::util::uninitialize() noexcept {
    nng::fini();
}

std::string nng::util::_Pre_address(std::string_view _Address) noexcept {
    constexpr std::string_view protocol = "ipc://";
#if defined(__OHOS__)
    /***************************************************************************************
    *1.":///data/storage/el2/base/haps/entry/files/"
    *2.":///data/app/e12/100/base/com.example.xclient/haps/entry/files/"
    *鸿蒙系统不支持绝对路径访问沙箱路径只是使用第一个路径访问沙箱
    ***************************************************************************************/
    constexpr std::string_view prefix = "/data/storage/el2/base/haps/entry/files/";
#elif defined(__linux__) || defined(__APPLE__)
    constexpr std::string_view prefix = "/tmp/";
#else
    constexpr std::string_view prefix;
    return std::string(_Address);
#endif
    // Helper function to check if a string_view starts with another string_view
    auto starts_with = [](std::string_view str, std::string_view prefix) -> bool {
        return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
        };

    // Check if the address starts with the protocol
    if (starts_with(_Address, protocol)) {
        // Extract content after the protocol
        auto content = _Address.substr(protocol.length());
        // Check if content starts with the prefix
        if (!starts_with(content, prefix)) {
            // Only modify if prefix is not present
            return protocol.data() + std::string(prefix) + std::string(content);
        }
    }
    return std::string(_Address);
}

void nng::util::_Pre_start_listen(nng::Listener& _Connector_ref) noexcept {
#ifdef _WIN32
    using namespace nng::util::detail;
    static bool fAsd = (IsProcessElevated() || IsSystemAuthority());
    if (fAsd) {
        static CAccessSecurityDescriptor asd;
        int rv = _Connector_ref.set_ptr(NNG_OPT_IPC_SECURITY_DESCRIPTOR, asd);
        assert(rv == NNG_OK);
    }
#endif
}
