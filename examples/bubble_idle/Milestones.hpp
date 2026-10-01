#pragma once

#include "ExactArray.hpp"
#include "ShrineType.hpp"

#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/Limits.hpp"


////////////////////////////////////////////////////////////
using MilestoneTimestamp                  = za::U64;
static inline constexpr auto maxMilestone = ZA_UNSIGNED_LONG_LONG_MAX;

////////////////////////////////////////////////////////////
struct [[nodiscard]] Milestones
{
    MilestoneTimestamp firstCat      = maxMilestone;
    MilestoneTimestamp firstUnicat   = maxMilestone;
    MilestoneTimestamp firstDevilcat = maxMilestone;
    MilestoneTimestamp firstAstrocat = maxMilestone;

    MilestoneTimestamp fiveCats      = maxMilestone;
    MilestoneTimestamp fiveUnicats   = maxMilestone;
    MilestoneTimestamp fiveDevilcats = maxMilestone;
    MilestoneTimestamp fiveAstrocats = maxMilestone;

    MilestoneTimestamp tenCats      = maxMilestone;
    MilestoneTimestamp tenUnicats   = maxMilestone;
    MilestoneTimestamp tenDevilcats = maxMilestone;
    MilestoneTimestamp tenAstrocats = maxMilestone;

    MilestoneTimestamp prestigeLevel2  = maxMilestone;
    MilestoneTimestamp prestigeLevel3  = maxMilestone;
    MilestoneTimestamp prestigeLevel4  = maxMilestone;
    MilestoneTimestamp prestigeLevel5  = maxMilestone;
    MilestoneTimestamp prestigeLevel6  = maxMilestone;
    MilestoneTimestamp prestigeLevel10 = maxMilestone;
    MilestoneTimestamp prestigeLevel15 = maxMilestone;
    MilestoneTimestamp prestigeLevel20 = maxMilestone;

    MilestoneTimestamp revenue10000      = maxMilestone;
    MilestoneTimestamp revenue100000     = maxMilestone;
    MilestoneTimestamp revenue1000000    = maxMilestone;
    MilestoneTimestamp revenue10000000   = maxMilestone;
    MilestoneTimestamp revenue100000000  = maxMilestone;
    MilestoneTimestamp revenue1000000000 = maxMilestone;

    EXACT_ARRAY(MilestoneTimestamp,
                shrineCompletions,
                nShrineTypes,
                {
                    maxMilestone, // Voodoo
                    maxMilestone, // Magic
                    maxMilestone, // Clicking
                    maxMilestone, // Automation
                    maxMilestone, // Repulsion
                    maxMilestone, // Attraction
                    maxMilestone, // Camouflage
                    maxMilestone, // Victory
                });

    // TODO P1: other milestones, achievements for fast milestones
};

////////////////////////////////////////////////////////////
struct [[nodiscard]] SpeedrunningSplits
{
    MilestoneTimestamp prestigeLevel2 = maxMilestone;
    MilestoneTimestamp prestigeLevel3 = maxMilestone;
    MilestoneTimestamp prestigeLevel4 = maxMilestone;
    MilestoneTimestamp prestigeLevel5 = maxMilestone;
};
