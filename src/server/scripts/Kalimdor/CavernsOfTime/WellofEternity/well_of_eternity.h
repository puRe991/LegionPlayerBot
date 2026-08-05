/*
 * Copyright (C) 2008-2017 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// This header shipped as a zero byte file, so every script including it had to
// redeclare its own constants and nothing could be shared. The entries below
// were checked against the Cataclysm database on Wowhead. Peroth'arn's five
// spell ids were already in boss_perotharn.cpp and match that source exactly,
// which is what makes the rest of it trustworthy.

#ifndef DEF_WELL_OF_ETERNITY_H
#define DEF_WELL_OF_ETERNITY_H

uint32 const EncounterCount = 3;

enum WoEDataTypes
{
    DATA_PEROTHARN              = 0,
    DATA_QUEEN_AZSHARA          = 1,
    DATA_MANNOROTH              = 2,

    DATA_PEROTHARN_GUID         = 10,
    DATA_QUEEN_AZSHARA_GUID     = 11,
    DATA_MANNOROTH_GUID         = 12,
    DATA_VAROTHEN_GUID          = 13,

    DATA_TEAM_IN_INSTANCE       = 20
};

enum WoECreatureIds
{
    NPC_PEROTHARN               = 55085,
    NPC_QUEEN_AZSHARA           = 54853,
    NPC_ENCHANTED_MAGUS         = 54883,
    NPC_MANNOROTH               = 54969,
    NPC_CAPTAIN_VAROTHEN        = 55419,

    // Referenced by spell_mannoroth_lunar_shot, which filters Tyrande's shot
    // down to this entry.
    NPC_DEMONIC_INVADER         = 57410
};

enum WoEActions
{
    ACTION_VAROTHEN_DIED        = 1,
    ACTION_BLADE_THROWN         = 2
};

#endif
