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

// Grimrail Depot was the one Warlords dungeon with no scripts at all -- no
// directory, no map binding, nothing. Entries below were looked up against the
// Warlords of Draenor data on Wowhead.

#ifndef DEF_GRIMRAIL_DEPOT_H
#define DEF_GRIMRAIL_DEPOT_H

uint32 const EncounterCount = 3;

enum GDDataTypes
{
    DATA_ROCKETSPARK_AND_BORKA  = 0,
    DATA_NITROGG_THUNDERTOWER   = 1,
    DATA_SKYLORD_TOVRA          = 2,

    DATA_ROCKETSPARK_GUID       = 100,
    DATA_BORKA_GUID             = 101,
    DATA_NITROGG_GUID           = 102,
    DATA_TOVRA_GUID             = 103
};

enum GDCreatureIds
{
    NPC_RAILMASTER_ROCKETSPARK  = 77803,
    NPC_BORKA_THE_BRUTE         = 77816,
    NPC_NITROGG_THUNDERTOWER    = 79545,
    NPC_ASSAULT_CANNON          = 79548,
    NPC_SKYLORD_TOVRA           = 80005
};

enum GDActions
{
    ACTION_BORKA_DIED           = 1
};

#endif
