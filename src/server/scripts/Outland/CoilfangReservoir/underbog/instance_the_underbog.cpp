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

/* ScriptData
SDName: Instance_The_Underbog
SD%Complete: 100
SDComment: The Underbog has boss scripts but nothing bound its map, so no
           InstanceScript existed. Map::UpdateEncounterState reads the group's
           completed-encounter mask from the instance script and writes it
           back there; without one the mask restarts at zero on every kill and
           is never saved, so the dungeon never counts as finished.
SDCategory: Underbog
EndScriptData */

#include "InstanceScript.h"
#include "ScriptMgr.h"

class instance_the_underbog : public InstanceMapScript
{
    public:
        instance_the_underbog() : InstanceMapScript("instance_the_underbog", 546) { }

        struct instance_the_underbog_InstanceMapScript : public InstanceScript
        {
            instance_the_underbog_InstanceMapScript(Map* map) : InstanceScript(map) { }
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const
        {
            return new instance_the_underbog_InstanceMapScript(map);
        }
};

void AddSC_instance_the_underbog()
{
    new instance_the_underbog();
}
