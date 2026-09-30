// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md

#ifdef ZA_ENABLE_LIFETIME_TRACKING

    ////////////////////////////////////////////////////////////
    // Headers
    ////////////////////////////////////////////////////////////
    #include "Zancle/Lifetime/LifetimeDependant.hpp"

    #include "Zancle/Lifetime/LifetimeDependee.hpp"


namespace za::priv
{
////////////////////////////////////////////////////////////
LifetimeDependant::LifetimeDependant(const char* const dependeeName, LifetimeDependee* dependee) noexcept :
    m_dependeeName(dependeeName),
    m_dependee(dependee)
{
    addSelfAsDependant();
}


////////////////////////////////////////////////////////////
LifetimeDependant::~LifetimeDependant()
{
    subSelfAsDependant();
}


////////////////////////////////////////////////////////////
LifetimeDependant::LifetimeDependant(const LifetimeDependant& rhs) noexcept :
    LifetimeDependant(rhs.m_dependeeName, rhs.m_dependee)
{
}


////////////////////////////////////////////////////////////
LifetimeDependant::LifetimeDependant(LifetimeDependant&& rhs) noexcept :
    m_dependeeName(rhs.m_dependeeName),
    m_dependee(rhs.m_dependee)
{
    rhs.m_dependee = nullptr;
}


////////////////////////////////////////////////////////////
LifetimeDependant& LifetimeDependant::operator=(const LifetimeDependant& rhs) noexcept
{
    if (&rhs == this)
        return *this;

    subSelfAsDependant();

    m_dependeeName = rhs.m_dependeeName;
    m_dependee     = rhs.m_dependee;

    addSelfAsDependant();

    return *this;
}


////////////////////////////////////////////////////////////
LifetimeDependant& LifetimeDependant::operator=(LifetimeDependant&& rhs) noexcept
{
    if (&rhs == this)
        return *this;

    subSelfAsDependant();

    // Take over `rhs`'s registration, as the move constructor does
    m_dependeeName = rhs.m_dependeeName;
    m_dependee     = rhs.m_dependee;
    rhs.m_dependee = nullptr;

    return *this;
}


////////////////////////////////////////////////////////////
bool LifetimeDependant::isTestingModeErrorTriggered() const noexcept
{
    return LifetimeDependee::TestingModeGuard::fatalErrorTriggered(m_dependeeName);
}


////////////////////////////////////////////////////////////
void LifetimeDependant::update(LifetimeDependee* dependee) noexcept
{
    subSelfAsDependant();
    m_dependee = dependee;
    addSelfAsDependant();
}


////////////////////////////////////////////////////////////
void LifetimeDependant::addSelfAsDependant()
{
    if (m_dependee != nullptr && !LifetimeDependee::TestingModeGuard::fatalErrorTriggered(m_dependeeName))
        m_dependee->addDependant();
}


////////////////////////////////////////////////////////////
void LifetimeDependant::subSelfAsDependant()
{
    if (m_dependee != nullptr && !LifetimeDependee::TestingModeGuard::fatalErrorTriggered(m_dependeeName))
        m_dependee->subDependant();
}

} // namespace za::priv

#endif // ZA_ENABLE_LIFETIME_TRACKING
