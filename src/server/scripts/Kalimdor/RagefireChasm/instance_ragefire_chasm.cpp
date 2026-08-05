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
SDName: Instance_Ragefire_Chasm
SD%Complete: 100
SDComment: Ragefire Chasm needs no encounter logic — its bosses run on
           SmartAI from the world database. What it does need is an
           InstanceScript, because Map::UpdateEncounterState only carries a
           group's completed-encounter mask forward when one exists. Without
           it the mask restarts at zero on every kill, the dungeon never
           counts as finished, and a random-dungeon group ends up with the
           deserter debuff instead of its reward.
SDCategory: Ragefire Chasm
EndScriptData */

#include "InstanceScript.h"
#include "ScriptMgr.h"

class instance_ragefire_chasm : public InstanceMapScript
{
    public:
        instance_ragefire_chasm() : InstanceMapScript("instance_ragefire_chasm", 389) { }

        struct instance_ragefire_chasm_InstanceMapScript : public InstanceScript
        {
            instance_ragefire_chasm_InstanceMapScript(Map* map) : InstanceScript(map) { }
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const
        {
            return new instance_ragefire_chasm_InstanceMapScript(map);
        }
};

void AddSC_instance_ragefire_chasm()
{
    new instance_ragefire_chasm();
}
