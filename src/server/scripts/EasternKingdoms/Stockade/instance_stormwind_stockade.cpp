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
SDName: Instance_Stormwind_Stockade
SD%Complete: 100
SDComment: The Stockade has no scripted encounters — its bosses run on SmartAI
           from the world database. The InstanceScript exists so the group's
           completed-encounter mask survives, which is what Map::
           UpdateEncounterState needs to hand out the random-dungeon reward.
SDCategory: The Stockade
EndScriptData */

#include "InstanceScript.h"
#include "ScriptMgr.h"

class instance_stormwind_stockade : public InstanceMapScript
{
    public:
        instance_stormwind_stockade() : InstanceMapScript("instance_stormwind_stockade", 34) { }

        struct instance_stormwind_stockade_InstanceMapScript : public InstanceScript
        {
            instance_stormwind_stockade_InstanceMapScript(Map* map) : InstanceScript(map) { }
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const
        {
            return new instance_stormwind_stockade_InstanceMapScript(map);
        }
};

void AddSC_instance_stormwind_stockade()
{
    new instance_stormwind_stockade();
}
