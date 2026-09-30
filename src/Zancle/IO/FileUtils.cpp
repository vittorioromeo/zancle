// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/IO/FileUtils.hpp"

#include "Zancle/IO/Path.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/IntTypes.hpp"

#include <cstdio>

#ifndef ZA_SYSTEM_WINDOWS
    #include <sys/types.h> // `off_t`
#endif


namespace za
{
////////////////////////////////////////////////////////////
std::FILE* openFile(const Path& filename, const char* const mode)
{
#ifdef ZA_SYSTEM_WINDOWS
    // `fopen` modes are short ASCII strings (e.g. "rb", "w+b")
    wchar_t wideMode[8]{};

    for (int i = 0; mode[i] != '\0'; ++i)
    {
        ZA_ASSERT(i < 7 && "unexpectedly long `fopen` mode");
        wideMode[i] = static_cast<wchar_t>(mode[i]);
    }

    return ::_wfopen(filename.c_str(), wideMode);
#else
    return std::fopen(filename.c_str(), mode);
#endif
}


////////////////////////////////////////////////////////////
bool seekFile(std::FILE* const file, const za::I64 offset, const int origin)
{
#ifdef ZA_SYSTEM_WINDOWS
    return ::_fseeki64(file, offset, origin) == 0;
#else
    // `off_t` may be 32-bit (e.g. 32-bit Linux without `_FILE_OFFSET_BITS=64`)
    if (static_cast<za::I64>(static_cast<off_t>(offset)) != offset)
        return false;

    return ::fseeko(file, static_cast<off_t>(offset), origin) == 0;
#endif
}


////////////////////////////////////////////////////////////
za::I64 tellFile(std::FILE* const file)
{
#ifdef ZA_SYSTEM_WINDOWS
    return ::_ftelli64(file);
#else
    return static_cast<za::I64>(::ftello(file));
#endif
}

} // namespace za
