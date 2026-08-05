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
SDName: Instance_Auchenai_Crypts
SD%Complete: 100
SDComment: Auchenai Crypts has boss scripts but nothing bound its map, so no
           InstanceScript existed. Map::UpdateEncounterState reads the group's
           completed-encounter mask from the instance script and writes it
           back there; without one the mask restarts at zero on every kill and
           is never saved, so the dungeon never counts as finished.
SDCategory: Auchenai Crypts
EndScriptData */

#include "InstanceScript.h"
#include "ScriptMgr.h"

class instance_auchenai_crypts : public InstanceMapScript
{
    public:
        instance_auchenai_crypts() : InstanceMapScript("instance_auchenai_crypts", 558) { }

        struct instance_auchenai_crypts_InstanceMapScript : public InstanceScript
        {
            instance_auchenai_crypts_InstanceMapScript(Map* map) : InstanceScript(map) { }
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const
        {
            return new instance_auchenai_crypts_InstanceMapScript(map);
        }
};

void AddSC_instance_auchenai_crypts()
{
    new instance_auchenai_crypts();
}
