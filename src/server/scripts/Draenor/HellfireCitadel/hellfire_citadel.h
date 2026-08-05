#ifndef DEF_HELLFIRE_CITADEL_H
#define DEF_HELLFIRE_CITADEL_H

enum DataTypes
{
    DATA_HELLFIRE_ASSAULT     = 0,
    DATA_IRON_REAVER          = 1,
    DATA_KORMROK              = 2,
    DATA_KILROGG              = 3,
    DATA_COUNCIL              = 4,
    DATA_GOREFIEND            = 5,
    DATA_ISKAR                = 6,
    DATA_SOCRETHAR_ETERNAL    = 7,
    DATA_VELHARI              = 8,
    DATA_ZAKUUN               = 9,
    DATA_XHULHORAC            = 10,
    DATA_MANNOROTH            = 11,
    DATA_ARCHIMONDE           = 12,

    MAX_ENCOUNTER,
};

// Guid lookups, offset so they cannot collide with the encounter ids above.
enum HFCGuidDataTypes
{
    DATA_SIEGEMASTER_MARTAK_GUID    = 100,
    DATA_IRON_REAVER_GUID,
    DATA_KORMROK_GUID,
    DATA_KILROGG_GUID,
    DATA_GOREFIEND_GUID,
    DATA_ISKAR_GUID,
    DATA_SOCRETHAR_GUID,
    DATA_VELHARI_GUID,
    DATA_ZAKUUN_GUID,
    DATA_XHULHORAC_GUID,
    DATA_MANNOROTH_GUID,
    DATA_ARCHIMONDE_GUID,
    DATA_DIA_DARKWHISPER_GUID,
    DATA_GURTOGG_BLOODBOIL_GUID,
    DATA_BLADEMASTER_JUBEITHOS_GUID
};

// Checked against the Warlords of Draenor data on Wowhead. The instance script
// had empty hooks and no entries at all before this.
enum HFCCreatureIds
{
    NPC_SIEGEMASTER_MARTAK          = 95068,
    NPC_IRON_REAVER                 = 90284,
    NPC_KORMROK                     = 90435,
    NPC_KILROGG_DEADEYE             = 90378,
    NPC_GOREFIEND                   = 91809,
    NPC_SHADOW_LORD_ISKAR           = 90316,   // 95067 ist der gleichnamige NPC im Tanaandschungel, nicht der Raidboss
    NPC_SOCRETHAR_THE_ETERNAL       = 90296,   // Seelengebundener Konstrukt -- traegt die Begegnung
    NPC_TYRANT_VELHARI              = 93439,
    NPC_FEL_LORD_ZAKUUN             = 89890,
    NPC_XHULHORAC                   = 93068,
    NPC_MANNOROTH_HFC               = 91349,
    NPC_ARCHIMONDE                  = 91331,

    // Hellfire High Council -- DATA_COUNCIL covers all three together.
    NPC_DIA_DARKWHISPER             = 92144,
    NPC_GURTOGG_BLOODBOIL           = 92146,
    NPC_BLADEMASTER_JUBEITHOS       = 92142
};

#endif