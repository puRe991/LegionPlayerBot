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

#ifndef DEF_BLACKROCK_FOUNDRY_H
#define DEF_BLACKROCK_FOUNDRY_H

enum DataTypes
{
    DataGruul               = 0,
    DataOregorger,
    DataBlastFurnace,
    DataHansgarAndFranzok,
    DataFlamebenderKagraz,
    DataKromog,
    DataBeastlordDarmac,
    DataOperatorThogar,
    DataIronMaidens,
    DataBlackhand,

    MaxBossData,
};

// Guid lookups, offset so they cannot collide with the encounter ids above.
enum GuidDataTypes
{
    DataGruulGuid           = 100,
    DataOregorgerGuid,
    DataHeartOfTheMountainGuid,
    DataForemanFeldsparGuid,
    DataHansgarGuid,
    DataFranzokGuid,
    DataFlamebenderKagrazGuid,
    DataKromogGuid,
    DataBeastlordDarmacGuid,
    DataOperatorThogarGuid,
    DataAdmiralGaranGuid,
    DataMarakTheBloodedGuid,
    DataEnforcerSorkaGuid,
    DataBlackhandGuid
};

// Checked against the Warlords of Draenor data on Wowhead. The instance script
// had empty hooks and no entries at all before this, so nothing in the raid
// was ever recorded.
enum CreatureIds
{
    NpcGruul                = 76877,
    NpcOregorger            = 77182,
    NpcHeartOfTheMountain   = 76806,
    NpcForemanFeldspar      = 76809,
    NpcHansgar              = 76973,
    NpcFranzok              = 76974,
    NpcFlamebenderKagraz    = 76814,
    NpcKromog               = 77692,
    NpcBeastlordDarmac      = 76865,
    NpcCruelfang            = 76884,
    NpcDreadwing            = 76874,
    NpcIroncrusher          = 76945,
    NpcFaultline            = 76946,
    NpcOperatorThogar       = 76906,
    NpcAdmiralGaran         = 77557,
    NpcMarakTheBlooded      = 77477,
    NpcEnforcerSorka        = 77231,
    NpcBlackhand            = 77325
};

#endif
