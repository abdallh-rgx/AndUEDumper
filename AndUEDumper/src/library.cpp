#include <cerrno>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <set>
#include <string>
#include <unordered_map>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <jni.h>

#include "Utils/Logger.hpp"
#include "Utils/ProgressUtils.hpp"

#include "Dumper.hpp"

#include "UE/UEMemory.hpp"
#include "UE/UEGameProfile.hpp"

#include "UE/UEGameProfiles/ArenaBreakout.hpp"
#include "UE/UEGameProfiles/BlackClover.hpp"
#include "UE/UEGameProfiles/Dislyte.hpp"
#include "UE/UEGameProfiles/Farlight.hpp"
#include "UE/UEGameProfiles/Fortnite.hpp"
#include "UE/UEGameProfiles/MortalKombat.hpp"
#include "UE/UEGameProfiles/PES.hpp"
#include "UE/UEGameProfiles/Torchlight.hpp"
#include "UE/UEGameProfiles/WutheringWaves.hpp"
#include "UE/UEGameProfiles/RealBoxing2.hpp"
#include "UE/UEGameProfiles/OdinValhalla.hpp"
#include "UE/UEGameProfiles/Injustice2.hpp"
#include "UE/UEGameProfiles/DeltaForce.hpp"
#include "UE/UEGameProfiles/RooftopsParkour.hpp"
#include "UE/UEGameProfiles/BabyYellow.hpp"
#include "UE/UEGameProfiles/TowerFantasy.hpp"
#include "UE/UEGameProfiles/BladeSoul.hpp"
#include "UE/UEGameProfiles/Lineage2.hpp"
#include "UE/UEGameProfiles/NightCrows.hpp"
#include "UE/UEGameProfiles/Case2.hpp"
#include "UE/UEGameProfiles/KingArthur.hpp"
#include "UE/UEGameProfiles/Century.hpp"
#include "UE/UEGameProfiles/HelloNeighbor.hpp"
#include "UE/UEGameProfiles/HelloNeighborND.hpp"
#include "UE/UEGameProfiles/SFG2.hpp"
#include "UE/UEGameProfiles/ArkUltimate.hpp"
#include "UE/UEGameProfiles/Auroria.hpp"
#include "UE/UEGameProfiles/LineageW.hpp"
#include "UE/UEGameProfiles/RLSideswipe.hpp"
#include "UE/UEGameProfiles/PUBG.hpp"

std::vector<IGameProfile *> UE_Games = {
    new PESProfile(),
    new DislyteProfile(),
    new MortalKombatProfile(),
    new FarlightProfile(),
    new TorchlightProfile(),
    new ArenaBreakoutProfile(),
    new BlackCloverProfile(),
    new WutheringWavesProfile(),
    new RealBoxing2Profile(),
    new OdinValhallaProfile(),
    new Injustice2Profile(),
    new DeltaForceProfile(),
    new RooftopParkourProfile(),
    new BabyYellowProfile(),
    new TowerFantasyProfile(),
    new BladeSoulProfile(),
    new Lineage2Profile(),
    new Case2Profile(),
    new CenturyProfile(),
    new KingArthurProfile(),
    new NightCrowsProfile(),
    new HelloNeighborProfile(),
    new HelloNeighborNDProfile(),
    new SFG2Profile(),
    new ArkUltimateProfile(),
    new AuroriaProfile(),
    new LineageWProfile(),
    new RLSideswipeProfile(),
    new PUBGProfile(),
    new FortniteProfile(),
};

#define kPROGRAM_VER "4.3.2"

// increase if needed
#define WAIT_TIME_SEC 20

void dump_thread(bool bDumpLib);

extern "C" void callMe(bool bDumpLib)
{
    LOGI("Starting UEDump3r thread...");
    std::thread(dump_thread, bDumpLib).detach();
}

extern "C" jint JNIEXPORT JNI_OnLoad(JavaVM*, void *)
{

    LOGI("Using UE Dumper %s", kPROGRAM_VER);
    // callMe(true); // dump lib from memory
    callMe(false);
    
    return JNI_VERSION_1_6;
}

void dump_thread(bool bDumpLib)
{
    LOGI("Dump will start after %d seconds.", WAIT_TIME_SEC);
    sleep(WAIT_TIME_SEC);
    LOGI("==========================");

    std::string sGamePackage = getprogname();
    pid_t gamePID = getpid();

    // dumping at external app data folder to avoid external storage permission
    std::string sOutDirectory = KittyUtils::Android::getAppExternalFilesDir(sGamePackage);

    LOGI("Game: %s", sGamePackage.c_str());
    LOGI("Process ID: %d", gamePID);
    LOGI("Output directory: %s", sOutDirectory.c_str());
    LOGI("Dump Library: %s", bDumpLib ? "true" : "false");
    LOGI("==========================");

    std::string sDumpDir = sOutDirectory + "/UEDump3r";
    std::string sDumpGameDir = sDumpDir + "/" + sGamePackage;
    // Don't delete directory - keep previous logs for debugging
    // IOUtils::delete_directory(sDumpGameDir);

    if (IOUtils::mkdir_recursive(sDumpGameDir, 0777) == -1)
    {
        int err = errno;
        LOGE("Couldn't create Output Directory [\"%s\"] error=%d | %s.", sDumpDir.c_str(), err, strerror(err));
        return;
    }

    // ============ FILE LOGGING (no logcat needed) ============
    // Write all logs to debug.log in the dump directory
    std::string sDebugLogPath = sDumpGameDir + "/debug.log";
    FILE *debugLog = fopen(sDebugLogPath.c_str(), "w");
    if (debugLog)
    {
        fprintf(debugLog, "=== UEDump3r %s Debug Log ===\n", kPROGRAM_VER);
        fprintf(debugLog, "Timestamp: %ld\n", time(nullptr));
        fprintf(debugLog, "Game Package: %s\n", sGamePackage.c_str());
        fprintf(debugLog, "Process ID: %d\n", gamePID);
        fprintf(debugLog, "Output Directory: %s\n", sOutDirectory.c_str());
        fprintf(debugLog, "Dump Library: %s\n", bDumpLib ? "true" : "false");
        fprintf(debugLog, "==========================\n");
        fflush(debugLog);
    }

    auto fileLog = [&](const char *fmt, ...)
    {
        if (!debugLog) return;
        va_list args;
        va_start(args, fmt);
        vfprintf(debugLog, fmt, args);
        va_end(args);
        fprintf(debugLog, "\n");
        fflush(debugLog);
    };

    fileLog("Starting memory initialization...");

    LOGE("Initializing memory...");
    if (!kMgr.initialize(gamePID, EK_MEM_OP_SYSCALL, false) && !kMgr.initialize(gamePID, EK_MEM_OP_IO, false))
    {
        LOGE("Failed to initialize KittyMemoryMgr.");
        fileLog("ERROR: Failed to initialize KittyMemoryMgr (SYSCALL and IO both failed)");
        if (debugLog) fclose(debugLog);
        return;
    }
    fileLog("Memory initialized successfully");

    // ============ FIND GAME PROFILE ============
    // Use flexible matching: package name may have suffix (e.g. process name vs package name)
    IGameProfile *matchedProfile = nullptr;
    fileLog("Looking for game profile matching '%s' ...", sGamePackage.c_str());

    for (auto &it : UE_Games)
    {
        for (auto &pkg : it->GetAppIDs())
        {
            // Exact match
            if (sGamePackage == pkg)
            {
                matchedProfile = it;
                fileLog("  Exact match found: %s -> %s", pkg.c_str(), it->GetAppName().c_str());
                break;
            }
            // Prefix match (package name starts with profile's package)
            if (sGamePackage.find(pkg) == 0 || pkg.find(sGamePackage) == 0)
            {
                matchedProfile = it;
                fileLog("  Prefix match found: %s ~ %s -> %s", pkg.c_str(), sGamePackage.c_str(), it->GetAppName().c_str());
                break;
            }
        }
        if (matchedProfile) break;
    }

    // Fallback: if no profile matched, use FortniteProfile (since we're injecting into Fortnite)
    if (!matchedProfile)
    {
        fileLog("WARNING: No profile matched. Looking for FortniteProfile as fallback...");
        for (auto &it : UE_Games)
        {
            if (it->GetAppName() == "Fortnite")
            {
                matchedProfile = it;
                fileLog("  Using FortniteProfile as default fallback");
                break;
            }
        }
    }

    if (!matchedProfile)
    {
        fileLog("ERROR: No game profile found and no fallback available");
        if (debugLog) fclose(debugLog);
        return;
    }

    fileLog("Using profile: %s", matchedProfile->GetAppName().c_str());

    // ============ DUMP LIBRARY (optional) ============
    if (bDumpLib)
    {
        auto ue_elf = matchedProfile->GetUnrealELF();
        if (!ue_elf.isValid())
        {
            fileLog("WARNING: Couldn't find a valid UE ELF in target process maps.");
        }
        else
        {
            fileLog("Dumping unreal lib from memory (base=0x%lx, end=0x%lx)...",
                    (unsigned long)ue_elf.base(), (unsigned long)ue_elf.end());
            std::string libDumpPath = KittyUtils::String::fmt("%s/libUE_%p-%p.so", sDumpGameDir.c_str(),
                                                               (void *)ue_elf.base(), (void *)ue_elf.end());
            bool res = kMgr.dumpMemELF(ue_elf, libDumpPath);
            fileLog("  Lib dump: %s", res ? "success" : "failed");
            if (res)
                fileLog("  Path: %s", libDumpPath.c_str());
        }
    }

    // ============ FIND UE ELF ============
    fileLog("Locating Unreal Engine library...");
    auto ue_elf = matchedProfile->GetUnrealELF();
    if (!ue_elf.isValid())
    {
        fileLog("ERROR: Couldn't find UE ELF. Tried names:");
        for (auto &name : matchedProfile->GetUESoNames())
            fileLog("  - %s", name.c_str());
        // List all loaded libraries for debugging
        fileLog("All loaded libraries in /proc/self/maps:");
        FILE *maps = fopen("/proc/self/maps", "r");
        if (maps)
        {
            char line[1024];
            std::set<std::string> seen_libs;
            while (fgets(line, sizeof(line), maps))
            {
                if (strstr(line, ".so"))
                {
                    // Extract library name
                    char *p = strrchr(line, '/');
                    if (p)
                    {
                        std::string libname(p + 1);
                        libname = libname.substr(0, libname.find_first_of(" \n"));
                        if (seen_libs.find(libname) == seen_libs.end())
                        {
                            seen_libs.insert(libname);
                            fileLog("  %s", libname.c_str());
                        }
                    }
                }
            }
            fclose(maps);
        }
        if (debugLog) fclose(debugLog);
        return;
    }
    fileLog("UE ELF found: base=0x%lx, end=0x%lx, size=%lu KB",
            (unsigned long)ue_elf.base(), (unsigned long)ue_elf.end(),
            (unsigned long)(ue_elf.end() - ue_elf.base()) / 1024);

    // ============ INITIALIZE DUMPER ============
    // Note: GetNamesPtr() and GetGUObjectArrayPtr() are called internally
    // by UEDumper.Init() - we can't call them directly (they're protected)
    UEDumper uEDumper{};

    uEDumper.setDumpExeInfoNotify([&fileLog](bool bFinished)
    {
        if (!bFinished) fileLog("Dumping Executable Info...");
        else fileLog("Executable Info done");
    });

    uEDumper.setDumpNamesInfoNotify([&fileLog](bool bFinished)
    {
        if (!bFinished) fileLog("Dumping Names Info...");
        else fileLog("Names Info done");
    });

    uEDumper.setDumpObjectsInfoNotify([&fileLog](bool bFinished)
    {
        if (!bFinished) fileLog("Dumping Objects Info...");
        else fileLog("Objects Info done");
    });

    uEDumper.setDumpOffsetsInfoNotify([&fileLog](bool bFinished)
    {
        if (!bFinished) fileLog("Dumping Offsets Info...");
        else fileLog("Offsets Info done");
    });

    uEDumper.setObjectsProgressCallback([&fileLog](const SimpleProgressBar &)
    {
        static bool once = false;
        if (!once)
        {
            once = true;
            fileLog("Gathering UObjects....");
        };
    });

    uEDumper.setDumpProgressCallback([&fileLog](const SimpleProgressBar &)
    {
        static bool once = false;
        if (!once)
        {
            once = true;
            fileLog("Dumping....");
        };
    });

    fileLog("Initializing UEDumper...");
    bool dumpSuccess = false;
    std::unordered_map<std::string, BufferFmt> dumpbuffersMap;

    if (uEDumper.Init(matchedProfile))
    {
        fileLog("UEDumper initialized. Starting dump...");
        dumpSuccess = uEDumper.Dump(&dumpbuffersMap);
        fileLog("Dump result: %s", dumpSuccess ? "success" : "failed");
    }
    else
    {
        fileLog("ERROR: UEDumper.Init() failed: %s", uEDumper.GetLastError().c_str());
    }

    if (!dumpSuccess)
    {
        fileLog("ERROR: Dump failed. Last error: %s", uEDumper.GetLastError().c_str());
        if (dumpbuffersMap.empty())
        {
            fileLog("ERROR: No buffers generated. Dump cannot continue.");
            if (debugLog) fclose(debugLog);
            return;
        }
        fileLog("WARNING: Dump reported failure but buffers exist - will save them");
    }

    fileLog("Saving %d files...", (int)dumpbuffersMap.size());
    for (const auto &it : dumpbuffersMap)
    {
        if (!it.first.empty())
        {
            std::string path = KittyUtils::String::fmt("%s/%s", sDumpGameDir.c_str(), it.first.c_str());
            bool written = it.second.writeBufferToFile(path);
            fileLog("  %s: %s (%d bytes)", it.first.c_str(), written ? "OK" : "FAILED", (int)it.second.size());
        }
    }

    fileLog("==========================");
    fileLog("Dump complete!");
    fileLog("Output: %s", sDumpGameDir.c_str());
    if (!uEDumper.GetLastError().empty())
        fileLog("Last error: %s", uEDumper.GetLastError().c_str());
    fileLog("==========================");

    if (debugLog) fclose(debugLog);
}
