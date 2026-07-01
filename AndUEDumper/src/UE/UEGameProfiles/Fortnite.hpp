#pragma once

#include "../UEGameProfile.hpp"
using namespace UEMemory;

// Fortnite Mobile (UE5.x)
// - Library: libUnreal.so (UE5 mobile renamed from libUE4.so)
// - Package: com.epicgames.fortnite2240023070899 (and similar)
// - Uses FNamePool (UE 4.23+ style)
// - UE5.2+ uses UFunction::Func offset = 0x278 (verified via static analysis)
class FortniteProfile : public IGameProfile
{
public:
    FortniteProfile() = default;

    bool ArchSupprted() const override
    {
        // arm64-v8a only
        auto e_machine = GetUnrealELF().header().e_machine;
        return e_machine == EM_AARCH64;
    }

    std::string GetAppName() const override
    {
        return "Fortnite";
    }

    std::vector<std::string> GetAppIDs() const override
    {
        return {
            "com.epicgames.fortnite",
            "com.epicgames.fortnite2240023070899",
            // add more Fortnite build-specific package IDs here as needed
        };
    }

    // Override to provide UE5 library name (libUnreal.so instead of libUE4.so)
    std::vector<std::string> GetUESoNames() const override
    {
        return {"libUnreal.so", "libUE4.so"};
    }

    bool isUsingCasePreservingName() const override
    {
        return false;
    }

    bool IsUsingFNamePool() const override
    {
        return true;  // UE5 uses FNamePool
    }

    bool isUsingOutlineNumberName() const override
    {
        return false;
    }

    // Use UE5.0-5.2 offsets (verified to work for Fortnite Mobile 22.40)
    UE_Offsets *GetOffsets() const override
    {
        // UE5_03 uses UE5_00_02 as base + adjusts FField layout
        // For Fortnite Mobile 22.40 (UE5.2+), UE5_03 is the best match
        static UE_Offsets offsets = UE_DefaultOffsets::UE5_03(
            isUsingCasePreservingName(),
            isUsingOutlineNumberName()
        );
        return &offsets;
    }

    // GUObjectArray: try symbol first, then patterns
    uintptr_t GetGUObjectArrayPtr() const override
    {
        // Try symbol lookup first
        uintptr_t guobjectarray = GetUnrealELF().findSymbol("GUObjectArray");
        if (guobjectarray != 0)
            return guobjectarray;

        // Try multiple patterns (UE5 mobile GUObjectArray access patterns)
        std::vector<std::pair<std::string, int>> idaPatterns = {
            {"B4 21 0C 40 B9 ? ? ? ? ? ? ? 91", 5},
            {"69 3e 40 b9 1f 01 09 6b ? ? ? 54 e1 03 13 aa ? ? ? ? f4 4f ? a9 ? ? ? ? ? ? ? 91", 0x18},
            {"91 E1 03 ? AA E0 03 08 AA E2 03 1F 2A", -7},
        };

        PATTERN_MAP_TYPE map_type = isEmulator() ? PATTERN_MAP_TYPE::ANY_R : PATTERN_MAP_TYPE::ANY_X;

        for (const auto &it : idaPatterns)
        {
            uintptr_t found = findIdaPattern(map_type, it.first, it.second);
            if (found != 0)
                return found;
        }

        return 0;
    }

    // GNames (FNamePool): try symbol, then patterns
    uintptr_t GetNamesPtr() const override
    {
        // Try symbol lookup first
        uintptr_t gnames = GetUnrealELF().findSymbol("FNamePool");
        if (gnames != 0)
            return gnames;

        gnames = GetUnrealELF().findSymbol("GNAMES");
        if (gnames != 0)
            return gnames;

        // Fallback patterns
        std::vector<std::pair<std::string, int>> idaPatterns = {
            {"81 80 52 ? ? ? ? ? 81 80 52 ? 03 1F 2A", 0x1f},
        };

        PATTERN_MAP_TYPE map_type = isEmulator() ? PATTERN_MAP_TYPE::ANY_R : PATTERN_MAP_TYPE::ANY_X;

        for (const auto &it : idaPatterns)
        {
            uintptr_t found = findIdaPattern(map_type, it.first, it.second);
            if (found != 0)
                return found;
        }

        return 0;
    }
};
