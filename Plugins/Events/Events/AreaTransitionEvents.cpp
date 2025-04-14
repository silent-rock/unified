#include "Events.hpp"
#include "API/CAppManager.hpp"
#include "API/CServerExoApp.hpp"
#include "API/CNWSArea.hpp"
#include "API/CNWSCreature.hpp"
#include "API/CNWSPlayer.hpp"
#include "API/Functions.hpp"
#include "API/Constants.hpp"

namespace Events {
using namespace NWNXLib;
using namespace NWNXLib::API;
using namespace NWNXLib::Services;

static Hooks::Hook s_AddToAreaHook;
static std::unordered_map<OBJECT_ID, OBJECT_ID> s_PreviousAreaMap;

static void AddToAreaHook(CNWSCreature*, CNWSArea*, float, float, float, BOOL);

void AreaTransitionEvents() __attribute__((constructor));
void AreaTransitionEvents()
{
    InitOnFirstSubscribe("NWNX_ON_AREA_TRANSITION_.*", []() {
        s_AddToAreaHook = Hooks::HookFunction(&CNWSCreature::AddToArea,
            &AddToAreaHook, Hooks::Order::Early);
    });
}

static void AddToAreaHook(CNWSCreature* pCreature, CNWSArea* pArea, float x, float y, float z, BOOL bRunScripts)
{
    if (!pCreature || !pArea)
    {
        s_AddToAreaHook->CallOriginal<void>(pCreature, pArea, x, y, z, bRunScripts);
        return;
    }

    // Only process player characters
    if (pCreature->m_bPlayerCharacter)
    {
        OBJECT_ID oidPlayer = pCreature->m_idSelf;
        OBJECT_ID oidNewArea = pArea->m_idSelf;
        OBJECT_ID oidPreviousArea = pCreature->m_oidArea;
        
        // Store the previous area for after event
        s_PreviousAreaMap[oidPlayer] = oidPreviousArea;
        
        // Only fire event if actually changing areas
        if (oidPreviousArea != oidNewArea)
        {
            // Push event data for BEFORE event
            PushEventData("PREVIOUS_AREA", Utils::ObjectIDToString(oidPreviousArea));
            PushEventData("TARGET_AREA", Utils::ObjectIDToString(oidNewArea));
            
            // Signal BEFORE event
            std::string result;
            if (!SignalEvent("NWNX_ON_AREA_TRANSITION_BEFORE", oidPlayer, &result))
            {
                // Event was skipped - don't change area
                return;
            }
        }
    }
    
    // Call original function
    s_AddToAreaHook->CallOriginal<void>(pCreature, pArea, x, y, z, bRunScripts);
    
    // Process AFTER event
    if (pCreature->m_bPlayerCharacter)
    {
        OBJECT_ID oidPlayer = pCreature->m_idSelf;
        
        // Get previous area from map
        auto it = s_PreviousAreaMap.find(oidPlayer);
        if (it != s_PreviousAreaMap.end())
        {
            OBJECT_ID oidPreviousArea = it->second;
            OBJECT_ID oidCurrentArea = pCreature->m_oidArea;
            
            // Only fire event if actually changed areas
            if (oidPreviousArea != oidCurrentArea)
            {
                // Push event data for AFTER event
                PushEventData("PREVIOUS_AREA", Utils::ObjectIDToString(oidPreviousArea));
                PushEventData("TARGET_AREA", Utils::ObjectIDToString(oidCurrentArea));
                
                // Signal AFTER event
                SignalEvent("NWNX_ON_AREA_TRANSITION_AFTER", oidPlayer);
            }
            
            // Remove from map to avoid memory leaks
            s_PreviousAreaMap.erase(it);
        }
    }
}

}