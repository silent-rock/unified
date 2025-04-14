#include "Events.hpp"
#include "API/CAppManager.hpp"
#include "API/CServerExoApp.hpp"
#include "API/CNWSArea.hpp"
#include "API/CNWSObject.hpp"
#include "API/CNWSCreature.hpp"
#include "API/CNWSPlayer.hpp"
#include "API/Functions.hpp"
#include "API/Constants.hpp"

namespace Events {

using namespace NWNXLib;
using namespace NWNXLib::API;
using namespace NWNXLib::Services;

static Hooks::Hook s_SetAreaHook;

static void SetAreaHook(CNWSObject*, OBJECT_ID);

void AreaTransitionEvents() __attribute__((constructor));
void AreaTransitionEvents()
{
    InitOnFirstSubscribe("NWNX_ON_AREA_TRANSITION_.*", []() {
        s_SetAreaHook = Hooks::HookFunction(&CNWSObject::SetArea,
            &SetAreaHook, Hooks::Order::Early);
    });
}

static void SetAreaHook(CNWSObject* pObject, OBJECT_ID oidArea)
{
    // Check if the object is a creature
    if (pObject->m_nObjectType != Constants::ObjectType::Creature)
    {
        s_SetAreaHook->CallOriginal<void>(pObject, oidArea);
        return;
    }

    // Get creature pointer
    CNWSCreature* pCreature = Utils::AsNWSCreature(pObject);
    
    // Check if it's a player character
    if (pCreature && pCreature->m_bPlayerCharacter)
    {
        // Get area information
        OBJECT_ID oidPreviousArea = pCreature->m_oidArea;
        OBJECT_ID oidPlayer = pCreature->m_idSelf; // Store the player ID
        
        // Check if we're actually changing areas
        if (oidPreviousArea != oidArea)
        {
            // Prepare data for BEFORE event
            PushEventData("PREVIOUS_AREA", Utils::ObjectIDToString(oidPreviousArea));
            PushEventData("TARGET_AREA", Utils::ObjectIDToString(oidArea));
            
            // Signal BEFORE event
            std::string result;
            if (!SignalEvent("NWNX_ON_AREA_TRANSITION_BEFORE", oidPlayer, &result))
            {
                // Event was skipped, so don't perform the area change
                return;
            }
            
            // Call the original function
            s_SetAreaHook->CallOriginal<void>(pObject, oidArea);
            
            // After calling the original function, we need to get the creature pointer again
            // as it might have changed or been invalidated
            CNWSCreature* pCreatureAfter = Utils::AsNWSCreature(Utils::GetGameObject(oidPlayer));
            
            // Make sure the creature still exists and the area change happened
            if (pCreatureAfter && pCreatureAfter->m_oidArea != oidPreviousArea)
            {
                // Prepare data for AFTER event
                PushEventData("PREVIOUS_AREA", Utils::ObjectIDToString(oidPreviousArea));
                PushEventData("TARGET_AREA", Utils::ObjectIDToString(pCreatureAfter->m_oidArea));
                
                // Signal AFTER event
                SignalEvent("NWNX_ON_AREA_TRANSITION_AFTER", oidPlayer);
            }
            
            // We handled the whole flow, return now
            return;
        }
    }
    
    // If we get here, just call the original without any event
    s_SetAreaHook->CallOriginal<void>(pObject, oidArea);
}

}