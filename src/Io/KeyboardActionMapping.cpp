#include <string>
#include <tuple>
#include <vector>

#include "Io/KeyboardActionMapping.h"

#include "Engine/Engine.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Vis.h"
#include "Engine/Graphics/Weather.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Party.h"
#include "Engine/Spells/CastSpellInfo.h"
#include "Engine/Time.h"
#include "Engine/TurnEngine/TurnEngine.h"

#include "GUI/GUIWindow.h"

using Io::InputAction;
using Io::KeyToggleType;


std::shared_ptr<KeyboardActionMapping> keyboardActionMapping = nullptr;

extern std::map<InputAction, bool> key_map_conflicted;  // 506E6C

class CKeyListElement {
 public:
    std::string m_keyName;
    PlatformKey m_key;
    InputAction m_cmdId;
    KeyToggleType m_toggType;
    CKeyListElement(std::string keyName, PlatformKey key,
                    InputAction cmdId, KeyToggleType toggType)
        : m_keyName(keyName),
          m_key(key),
          m_cmdId(cmdId),
          m_toggType(toggType) {}
};

std::array<CKeyListElement, 30> keyMappingParams = {
    CKeyListElement("KEY_FORWARD", PlatformKey::Up, InputAction::MoveForward, KeyToggleType::TOGGLE_Continuously),
    CKeyListElement("KEY_BACKWARD", PlatformKey::Down, InputAction::MoveBackwards, KeyToggleType::TOGGLE_Continuously),
    CKeyListElement("KEY_LEFT", PlatformKey::Left, InputAction::TurnLeft, KeyToggleType::TOGGLE_Continuously),
    CKeyListElement("KEY_RIGHT", PlatformKey::Right, InputAction::TurnRight, KeyToggleType::TOGGLE_Continuously),
    CKeyListElement("KEY_ATTACK", PlatformKey::A, InputAction::Attack, KeyToggleType::TOGGLE_OneTimePress),
    CKeyListElement("KEY_CASTREADY", PlatformKey::S, InputAction::CastReady, KeyToggleType::TOGGLE_OneTimePress),
    CKeyListElement("KEY_YELL", PlatformKey::Y, InputAction::Yell, KeyToggleType::TOGGLE_OneTimePress),
    CKeyListElement("KEY_JUMP", PlatformKey::X, InputAction::Jump, KeyToggleType::TOGGLE_OneTimePress),
    CKeyListElement("KEY_COMBAT", PlatformKey::Return, InputAction::Combat, KeyToggleType::TOGGLE_OneTimePress),
    CKeyListElement("KEY_EVENTTRIGGER", PlatformKey::Space, InputAction::EventTrigger, KeyToggleType::TOGGLE_OneTimePress),
    CKeyListElement("KEY_CAST", PlatformKey::C, InputAction::Cast, KeyToggleType::TOGGLE_OneTimePress),
    CKeyListElement("KEY_PASS", PlatformKey::B, InputAction::Pass, KeyToggleType::TOGGLE_OneTimePress),
    CKeyListElement("KEY_CHARCYCLE", PlatformKey::Tab, InputAction::CharCycle, KeyToggleType::TOGGLE_OneTimePress),
    CKeyListElement("KEY_QUEST", PlatformKey::Q, InputAction::Quest, KeyToggleType::TOGGLE_OneTimePress),
    CKeyListElement("KEY_QUICKREF", PlatformKey::Z, InputAction::QuickRef, KeyToggleType::TOGGLE_OneTimePress),
    CKeyListElement("KEY_REST", PlatformKey::R, InputAction::Rest, KeyToggleType::TOGGLE_OneTimePress),
    CKeyListElement("KEY_TIMECAL", PlatformKey::T, InputAction::TimeCal, KeyToggleType::TOGGLE_OneTimePress),
    CKeyListElement("KEY_AUTONOTES", PlatformKey::N, InputAction::Autonotes, KeyToggleType::TOGGLE_OneTimePress),
    CKeyListElement("KEY_MAPBOOK", PlatformKey::M, InputAction::Mapbook, KeyToggleType::TOGGLE_OneTimePress),
    CKeyListElement("KEY_LOOKUP", PlatformKey::PageDown, InputAction::LookUp, KeyToggleType::TOGGLE_OneTimePress),
    CKeyListElement("KEY_LOOKDOWN", PlatformKey::Delete, InputAction::LookDown, KeyToggleType::TOGGLE_OneTimePress),
    CKeyListElement("KEY_CENTERVIEWPT", PlatformKey::End, InputAction::CenterView, KeyToggleType::TOGGLE_OneTimePress),
    CKeyListElement("KEY_ZOOMIN", PlatformKey::Add, InputAction::ZoomIn, KeyToggleType::TOGGLE_OneTimePress),
    CKeyListElement("KEY_ZOOMOUT", PlatformKey::Subtract, InputAction::ZoomOut, KeyToggleType::TOGGLE_OneTimePress),
    CKeyListElement("KEY_FLYUP", PlatformKey::PageUp, InputAction::FlyUp, KeyToggleType::TOGGLE_Continuously),
    CKeyListElement("KEY_FLYDOWN", PlatformKey::Insert, InputAction::FlyDown, KeyToggleType::TOGGLE_Continuously),
    CKeyListElement("KEY_LAND", PlatformKey::Home, InputAction::Land, KeyToggleType::TOGGLE_OneTimePress),
    CKeyListElement("KEY_ALWAYSRUN", PlatformKey::U, InputAction::AlwaysRun, KeyToggleType::TOGGLE_OneTimePress),
    CKeyListElement("KEY_STEPLEFT", PlatformKey::LeftBracket, InputAction::StrafeLeft, KeyToggleType::TOGGLE_Continuously),
    CKeyListElement("KEY_STEPRIGHT", PlatformKey::RightBracket, InputAction::StrafeRight, KeyToggleType::TOGGLE_Continuously)};


void KeyboardActionMapping::MapKey(InputAction action, PlatformKey key) {
    actionKeyMap[action] = key;
}
//----- (00459C68) --------------------------------------------------------
void KeyboardActionMapping::MapKey(InputAction action, PlatformKey key, KeyToggleType type) {
    actionKeyMap[action] = key;
    keyToggleMap[action] = type;
}

PlatformKey KeyboardActionMapping::MapDefaultKey(InputAction action) {
/*
    for (size_t i = 0; i < keyMappingParams.size(); i++) {
        if (keyMappingParams[i].m_cmdId == action) {
            return keyMappingParams[i].m_key;
        }
    }

    return GameKey::None;
*/
    return ConfigDefaultKey(action);
}

//----- (00459C82) --------------------------------------------------------
PlatformKey KeyboardActionMapping::GetKey(InputAction action) const {
    return actionKeyMap.find(action)->second;
}

KeyToggleType KeyboardActionMapping::GetToggleType(InputAction action) const {
    return keyToggleMap.find(action)->second;
}

//----- (00459C8D) --------------------------------------------------------
KeyboardActionMapping::KeyboardActionMapping(std::shared_ptr<Application::GameConfig> config) {
    this->config = config;

    SetDefaultMapping();
    ReadMappings();
}

//----- (00459CC4) --------------------------------------------------------
void KeyboardActionMapping::SetDefaultMapping() {
    for (size_t i = 0; i < keyMappingParams.size(); i++) {
        //MapKey(keyMappingParams[i].m_cmdId, keyMappingParams[i].m_key, keyMappingParams[i].m_toggType);
        MapKey(keyMappingParams[i].m_cmdId, ConfigDefaultKey(keyMappingParams[i].m_cmdId), keyMappingParams[i].m_toggType);
    }
}

//----- (00459FFC) --------------------------------------------------------
void KeyboardActionMapping::ReadMappings() {
    char str[32];

    for (size_t i = 0; i < keyMappingParams.size(); i++) {
        const char *keyName = keyMappingParams[i].m_keyName.c_str();
        //GameKey commandDefaultKeyCode = keyMappingParams[i].m_key;
        InputAction commandId = keyMappingParams[i].m_cmdId;
        KeyToggleType toggType = keyMappingParams[i].m_toggType;

        //GameKey parsedKey = GameKey::None;
        //if (strcmp(str, "DEFAULT") != 0 && TryParseDisplayName(str, &parsedKey))
        //    MapKey(commandId, parsedKey);
        //else
        //    MapKey(commandId, commandDefaultKeyCode);
        PlatformKey key = ConfigGetKey(commandId);
        if (key != PlatformKey::None)
            MapKey(commandId, key);
        else
            MapKey(commandId, ConfigDefaultKey(commandId));

        keyToggleMap[commandId] = toggType;
    }
}

//----- (0045A960) --------------------------------------------------------
void KeyboardActionMapping::StoreMappings() {
    for (size_t i = 0; i < keyMappingParams.size(); i++) {
        //std::string display_name = GetDisplayName(GetKey(keyMappingParams[i].m_cmdId));
        //OS_SetAppString(keyMappingParams[i].m_keyName.c_str(), display_name.c_str());
        ConfigSetKey(keyMappingParams[i].m_cmdId, GetKey(keyMappingParams[i].m_cmdId));
    }
}

KeyToggleType GetToggleType(InputAction action) {
    // TODO: TOGGLE_* values in the table above are not respected, instead toggle values from this function are used.

    if (action == InputAction::StrafeLeft || action == InputAction::StrafeRight
        || action == InputAction::FlyUp || action == InputAction::FlyDown) {
        return KeyToggleType::TOGGLE_Continuously;
    }

    if (action == InputAction::Attack || action == InputAction::CastReady)
        return KeyToggleType::TOGGLE_DelayContinuous;

    int i = (int)action;
    if (i > 3)
        return KeyToggleType::TOGGLE_OneTimePress;
    else
        return KeyToggleType::TOGGLE_Continuously;
}

ConfigValue<std::string> *KeyboardActionMapping::InputActionToConfigKey(InputAction action) {
    ConfigValue<std::string> *val = nullptr;

    switch (action) {
        case(InputAction::MoveForward):
            val = &config->keybindings.Forward;
            break;
        case(InputAction::MoveBackwards):
            val = &config->keybindings.Backward;
            break;
        case(InputAction::TurnLeft):
            val = &config->keybindings.Left;
            break;
        case(InputAction::TurnRight):
            val = &config->keybindings.Right;
            break;
        case(InputAction::Attack):
            val = &config->keybindings.Attack;
            break;
        case(InputAction::CastReady):
            val = &config->keybindings.CastReady;
            break;
        case(InputAction::Yell):
            val = &config->keybindings.Yell;
            break;
        case(InputAction::Jump):
            val = &config->keybindings.Jump;
            break;
        case(InputAction::Combat):
            val = &config->keybindings.Combat;
            break;
        case(InputAction::EventTrigger):
            val = &config->keybindings.EventTrigger;
            break;
        case(InputAction::Cast):
            val = &config->keybindings.Cast;
            break;
        case(InputAction::Pass):
            val = &config->keybindings.Pass;
            break;
        case(InputAction::CharCycle):
            val = &config->keybindings.CharCycle;
            break;
        case(InputAction::Quest):
            val = &config->keybindings.Quest;
            break;
        case(InputAction::QuickRef):
            val = &config->keybindings.QuickReference;
            break;
        case(InputAction::Rest):
            val = &config->keybindings.Rest;
            break;
        case(InputAction::TimeCal):
            val = &config->keybindings.TimeCalendar;
            break;
        case(InputAction::Autonotes):
            val = &config->keybindings.AutoNotes;
            break;
        case(InputAction::Mapbook):
            val = &config->keybindings.MapBook;
            break;
        case(InputAction::LookUp):
            val = &config->keybindings.LookUp;
            break;
        case(InputAction::LookDown):
            val = &config->keybindings.LookDown;
            break;
        case(InputAction::CenterView):
            val = &config->keybindings.CenterView;
            break;
        case(InputAction::ZoomIn):
            val = &config->keybindings.ZoomIn;
            break;
        case(InputAction::ZoomOut):
            val = &config->keybindings.ZoomOut;
            break;
        case(InputAction::FlyUp):
            val = &config->keybindings.FlyUp;
            break;
        case(InputAction::FlyDown):
            val = &config->keybindings.FlyDown;
            break;
        case(InputAction::Land):
            val = &config->keybindings.Land;
            break;
        case(InputAction::AlwaysRun):
            val = &config->keybindings.AlwaysRun;
            break;
        case(InputAction::StrafeLeft):
            val = &config->keybindings.StepLeft;
            break;
        case(InputAction::StrafeRight):
            val = &config->keybindings.StepRight;
            break;
        default:
            break;
    }

    return val;
}

PlatformKey KeyboardActionMapping::ConfigDefaultKey(InputAction action) {
    PlatformKey key = PlatformKey::None;
    ConfigValue<std::string> *val = InputActionToConfigKey(action);

    if (val)
        TryParseDisplayName(val->Default(), &key);

    return key;
}

PlatformKey KeyboardActionMapping::ConfigGetKey(InputAction action) {
    PlatformKey key = PlatformKey::None;
    ConfigValue<std::string> *val = InputActionToConfigKey(action);

    if (val)
        TryParseDisplayName(val->Get(), &key);

    return key;
}

void KeyboardActionMapping::ConfigSetKey(InputAction action, PlatformKey key) {
    ConfigValue<std::string> *val = InputActionToConfigKey(action);

    if (val)
        val->Set(GetDisplayName(key));
}

