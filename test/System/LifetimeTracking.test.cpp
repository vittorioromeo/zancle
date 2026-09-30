#include "SystemUtil.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/Lifetime/LifetimeDependant.hpp"
#include "Zancle/Lifetime/LifetimeDependee.hpp"

#include "Zancle/Base/Macros.hpp"

#if defined(ZA_ENABLE_LIFETIME_TRACKING)

namespace
{
////////////////////////////////////////////////////////////
/// Dummy dependee (resource owner) and dependant (resource user)
/// for testing the lifetime tracking machinery in isolation.
////////////////////////////////////////////////////////////

struct DummyDependant;
struct DummyDefaultedDependant;

struct DummyDependee
{
    int value{};

    ZA_LIFETIME_DEPENDED_ON_BY(DummyDependee, DummyDependant); // `DummyDependee` is depended on by `DummyDependant`
};

////////////////////////////////////////////////////////////
/// Dependee of `DummyDefaultedDependant` (a class is depended on by at most one dependant type)
struct DummyDependeeB
{
    int value{};

    ZA_LIFETIME_DEPENDED_ON_BY(DummyDependeeB,
                               DummyDefaultedDependant); // `DummyDependeeB` is depended on by `DummyDefaultedDependant`
};


struct DummyDependant
{
    const DummyDependee* dependee{};

    ZA_LIFETIME_DEPENDS_ON(DummyDependee); // `DummyDependant` depends on `DummyDependee`

    explicit DummyDependant(const DummyDependee& dep) : dependee(&dep)
    {
        ZA_LIFETIME_UPDATE_DEPENDENCY(this, DummyDependee, dependee);
    }

    ~DummyDependant()
    {
        ZA_LIFETIME_RETURN_IF_TESTING_ERROR(DummyDependee);
    }

    DummyDependant(const DummyDependant& rhs) : dependee(rhs.dependee)
    {
        ZA_LIFETIME_UPDATE_DEPENDENCY(this, DummyDependee, dependee);
    }

    DummyDependant& operator=(const DummyDependant& rhs)
    {
        if (&rhs == this)
            return *this;

        dependee = rhs.dependee;
        ZA_LIFETIME_UPDATE_DEPENDENCY(this, DummyDependee, dependee);
        return *this;
    }

    DummyDependant(DummyDependant&& rhs) noexcept : dependee(rhs.dependee)
    {
        ZA_LIFETIME_UPDATE_DEPENDENCY(this, DummyDependee, dependee);
    }

    DummyDependant& operator=(DummyDependant&& rhs) noexcept
    {
        dependee = rhs.dependee;
        ZA_LIFETIME_UPDATE_DEPENDENCY(this, DummyDependee, dependee);
        return *this;
    }
};


////////////////////////////////////////////////////////////
/// Dependant whose copy and move operations are defaulted, like
/// `za::Text` and `za::GlyphMappedText`: exercises the special
/// member functions of `LifetimeDependant` itself.
struct DummyDefaultedDependant
{
    const DummyDependeeB* dependee{};

    ZA_LIFETIME_DEPENDS_ON(DummyDependeeB); // `DummyDefaultedDependant` depends on `DummyDependeeB`

    explicit DummyDefaultedDependant(const DummyDependeeB& dep) : dependee(&dep)
    {
        ZA_LIFETIME_UPDATE_DEPENDENCY(this, DummyDependeeB, dependee);
    }

    void setDependee(const DummyDependeeB* dep)
    {
        dependee = dep;
        ZA_LIFETIME_UPDATE_DEPENDENCY(this, DummyDependeeB, dependee);
    }
};

} // namespace


TEST_CASE("[System] Lifetime tracking")
{
    SECTION("Single dependant construction and destruction")
    {
        DummyDependee dep;
        {
            DummyDependant d(dep);
        }
        // dep destroyed with count == 0 -> OK
    }

    SECTION("Multiple dependants on same dependee")
    {
        DummyDependee dep;
        {
            DummyDependant a(dep);
            DummyDependant b(dep);
            DummyDependant c(dep);
        }
    }

    SECTION("Copy construction")
    {
        DummyDependee dep;
        {
            DummyDependant a(dep);
            DummyDependant b(a); // NOLINT(performance-unnecessary-copy-initialization)
            (void)b;
        }
    }

    SECTION("Copy assignment same dependee")
    {
        DummyDependee dep;
        {
            DummyDependant a(dep);
            DummyDependant b(dep);
            a = b;
        }
    }

    SECTION("Copy assignment different dependee")
    {
        DummyDependee dep1;
        DummyDependee dep2;
        {
            DummyDependant a(dep1);
            DummyDependant b(dep2);
            a = b;
            CHECK(a.dependee == &dep2);
        }
    }

    SECTION("Repeated copy assignment does not inflate count")
    {
        DummyDependee dep;
        {
            DummyDependant a(dep);
            DummyDependant b(dep);
            a = b;
            a = b;
            a = b;
            a = b;
            a = b;
        }
    }

    SECTION("Self copy assignment")
    {
        DummyDependee dep;
        {
            DummyDependant a(dep);
    #if defined(__clang__)
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wself-assign-overloaded"
    #endif
            a = a;
    #if defined(__clang__)
        #pragma clang diagnostic pop
    #endif
        }
    }

    SECTION("Move construction")
    {
        DummyDependee dep;
        {
            DummyDependant a(dep);
            DummyDependant b(ZA_MOVE(a));
            (void)b;
        }
    }

    SECTION("Move assignment same dependee")
    {
        DummyDependee dep;
        {
            DummyDependant a(dep);
            DummyDependant b(dep);
            a = ZA_MOVE(b);
        }
    }

    SECTION("Move assignment different dependee")
    {
        DummyDependee dep1;
        DummyDependee dep2;
        {
            DummyDependant a(dep1);
            DummyDependant b(dep2);
            a = ZA_MOVE(b);
            CHECK(a.dependee == &dep2);
        }
    }

    SECTION("Dependee destroyed before dependant triggers error")
    {
        const za::priv::LifetimeDependee::TestingModeGuard guard{"DummyDependee"};
        CHECK(!guard.fatalErrorTriggered("DummyDependee"));

        auto*          dep = new DummyDependee;
        DummyDependant d(*dep);

        delete dep;
        CHECK(guard.fatalErrorTriggered("DummyDependee"));
    }

    SECTION("Dependee destroyed with no dependants is fine")
    {
        const za::priv::LifetimeDependee::TestingModeGuard guard{"DummyDependee"};
        CHECK(!guard.fatalErrorTriggered("DummyDependee"));

        {
            DummyDependee dep;
            {
                DummyDependant d(dep);
            }
            // d destroyed, dep still alive -> count is 0
        }
        // dep destroyed with 0 dependants -> no error
        CHECK(!guard.fatalErrorTriggered("DummyDependee"));
    }

    SECTION("Copy of dependant keeps dependee alive requirement")
    {
        const za::priv::LifetimeDependee::TestingModeGuard guard{"DummyDependee"};
        CHECK(!guard.fatalErrorTriggered("DummyDependee"));

        auto*          dep = new DummyDependee;
        DummyDependant a(*dep);
        DummyDependant b(a); // NOLINT(performance-unnecessary-copy-initialization)

        delete dep;
        CHECK(guard.fatalErrorTriggered("DummyDependee"));
    }

    SECTION("Switching dependee via copy assignment")
    {
        const za::priv::LifetimeDependee::TestingModeGuard guard{"DummyDependee"};
        CHECK(!guard.fatalErrorTriggered("DummyDependee"));

        DummyDependee dep1;
        DummyDependee dep2;
        {
            DummyDependant a(dep1);
            DummyDependant b(dep2);

            // a switches from dep1 to dep2
            a = b;

            // dep1 should now have 0 dependants, dep2 should have 2
        }
        // All dependants destroyed, both dependees at 0
        CHECK(!guard.fatalErrorTriggered("DummyDependee"));
    }

    SECTION("Switching dependee via move assignment")
    {
        const za::priv::LifetimeDependee::TestingModeGuard guard{"DummyDependee"};
        CHECK(!guard.fatalErrorTriggered("DummyDependee"));

        DummyDependee dep1;
        DummyDependee dep2;
        {
            DummyDependant a(dep1);
            DummyDependant b(dep2);

            a = ZA_MOVE(b);
        }
        CHECK(!guard.fatalErrorTriggered("DummyDependee"));
    }
}


TEST_CASE("[System] Lifetime tracking: defaulted special members of dependants")
{
    const za::priv::LifetimeDependee::TestingModeGuard guard{"DummyDependeeB"};

    SECTION("Copy construction")
    {
        auto* dep = new DummyDependeeB;
        {
            DummyDefaultedDependant a(*dep);
            DummyDefaultedDependant b(a); // NOLINT(performance-unnecessary-copy-initialization)
        }
        delete dep;
        CHECK(!guard.fatalErrorTriggered("DummyDependeeB"));
    }

    SECTION("Move construction transfers the dependency")
    {
        auto*                   dep = new DummyDependeeB;
        DummyDefaultedDependant a(*dep);
        DummyDefaultedDependant b(ZA_MOVE(a));

        delete dep; // `b` still depends on it
        CHECK(guard.fatalErrorTriggered("DummyDependeeB"));
    }

    SECTION("Copy assignment to a different dependee")
    {
        auto* dep1 = new DummyDependeeB;
        auto* dep2 = new DummyDependeeB;
        {
            DummyDefaultedDependant a(*dep1);
            DummyDefaultedDependant b(*dep2);

            a = b;

            delete dep1; // nothing depends on it anymore
            CHECK(!guard.fatalErrorTriggered("DummyDependeeB"));
        }
        delete dep2;
        CHECK(!guard.fatalErrorTriggered("DummyDependeeB"));
    }

    SECTION("Move assignment to a different dependee releases the old one")
    {
        auto* dep1 = new DummyDependeeB;
        auto* dep2 = new DummyDependeeB;
        {
            DummyDefaultedDependant a(*dep1);
            DummyDefaultedDependant b(*dep2);

            a = ZA_MOVE(b);

            delete dep1; // nothing depends on it anymore
            CHECK(!guard.fatalErrorTriggered("DummyDependeeB"));
        }
        delete dep2; // both dependants are gone
        CHECK(!guard.fatalErrorTriggered("DummyDependeeB"));
    }

    SECTION("Move assignment keeps the new dependee tracked")
    {
        DummyDependeeB dep1;
        auto*          dep2 = new DummyDependeeB;

        DummyDefaultedDependant a(dep1);
        DummyDefaultedDependant b(*dep2);

        a = ZA_MOVE(b);

        delete dep2; // `a` now depends on it
        CHECK(guard.fatalErrorTriggered("DummyDependeeB"));
    }

    SECTION("Self move assignment")
    {
        auto* dep = new DummyDependeeB;
        {
            DummyDefaultedDependant a(*dep);
            auto&                   self = a;
            a                            = ZA_MOVE(self);
        }
        delete dep;
        CHECK(!guard.fatalErrorTriggered("DummyDependeeB"));
    }
}


TEST_CASE("[System] Lifetime tracking: switching dependees")
{
    const za::priv::LifetimeDependee::TestingModeGuard guard{"DummyDependeeB"};

    SECTION("Switching to another dependee releases the previous one")
    {
        auto* dep1 = new DummyDependeeB;
        auto* dep2 = new DummyDependeeB;

        DummyDefaultedDependant a(*dep1);
        a.setDependee(dep2);

        delete dep1;
        CHECK(!guard.fatalErrorTriggered("DummyDependeeB"));

        delete dep2; // `a` depends on it
        CHECK(guard.fatalErrorTriggered("DummyDependeeB"));
    }

    SECTION("Switching to no dependee")
    {
        auto* dep = new DummyDependeeB;

        DummyDefaultedDependant a(*dep);
        a.setDependee(nullptr);

        delete dep;
        CHECK(!guard.fatalErrorTriggered("DummyDependeeB"));
    }
}


TEST_CASE("[System] Lifetime tracking: copying and moving dependees")
{
    const za::priv::LifetimeDependee::TestingModeGuard guard{"DummyDependee"};

    SECTION("A copy starts with no dependants")
    {
        DummyDependee  dep;
        DummyDependant d(dep);

        auto* copy = new DummyDependee(dep);
        delete copy;
        CHECK(!guard.fatalErrorTriggered("DummyDependee"));
    }

    SECTION("Destroying a moved-from dependee whose dependants were not updated is an error")
    {
        auto*          dep = new DummyDependee;
        DummyDependant d(*dep);

        DummyDependee moved(ZA_MOVE(*dep));

        delete dep; // `d` still points to it
        CHECK(guard.fatalErrorTriggered("DummyDependee"));
    }

    SECTION("Destroying a moved-from dependee after updating its dependants is fine")
    {
        auto*         dep   = new DummyDependee;
        DummyDependee moved = ZA_MOVE(*dep);
        {
            DummyDependant d(*dep);
            d = DummyDependant(moved); // now depends on `moved`

            delete dep;
            CHECK(!guard.fatalErrorTriggered("DummyDependee"));
        }
    }

    SECTION("Assigning to a dependee keeps its dependants")
    {
        auto*          dep = new DummyDependee;
        DummyDependant d(*dep);

        const DummyDependee other;
        *dep = other;

        delete dep; // `d` still points to it
        CHECK(guard.fatalErrorTriggered("DummyDependee"));
    }
}

#endif // ZA_ENABLE_LIFETIME_TRACKING
