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
SDName: Instance_Grimrail_Depot
SD%Complete: 100
SDComment: Map 1208 was bound to nothing at all -- the only Warlords dungeon
           with no scripts in the tree.
SDCategory: Grimrail Depot
EndScriptData */

#include "InstanceScript.h"
#include "ScriptMgr.h"
#include "grimrail_depot.h"

class instance_grimrail_depot : public InstanceMapScript
{
    public:
        instance_grimrail_depot() : InstanceMapScript("instance_grimrail_depot", 1208) { }

        struct instance_grimrail_depot_InstanceMapScript : public InstanceScript
        {
            instance_grimrail_depot_InstanceMapScript(Map* map) : InstanceScript(map) { }

            void Initialize() override
            {
                SetBossNumber(EncounterCount);
            }

            void OnCreatureCreate(Creature* creature) override
            {
                switch (creature->GetEntry())
                {
                    case NPC_RAILMASTER_ROCKETSPARK: _rocketsparkGuid = creature->GetGUID(); break;
                    case NPC_BORKA_THE_BRUTE:        _borkaGuid = creature->GetGUID();       break;
                    case NPC_NITROGG_THUNDERTOWER:   _nitroggGuid = creature->GetGUID();     break;
                    case NPC_SKYLORD_TOVRA:          _tovraGuid = creature->GetGUID();       break;
                    default:
                        break;
                }
            }

            ObjectGuid GetGuidData(uint32 type) const override
            {
                switch (type)
                {
                    case DATA_ROCKETSPARK_GUID: return _rocketsparkGuid;
                    case DATA_BORKA_GUID:       return _borkaGuid;
                    case DATA_NITROGG_GUID:     return _nitroggGuid;
                    case DATA_TOVRA_GUID:       return _tovraGuid;
                    default:
                        break;
                }

                return ObjectGuid::Empty;
            }

            std::string GetSaveData() override
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << "G R D " << GetBossSaveData();

                OUT_SAVE_INST_DATA_COMPLETE;
                return saveStream.str();
            }

            void Load(char const* data) override
            {
                if (!data)
                {
                    OUT_LOAD_INST_DATA_FAIL;
                    return;
                }

                OUT_LOAD_INST_DATA(data);

                char head1, head2, head3;
                std::istringstream loadStream(data);
                loadStream >> head1 >> head2 >> head3;

                if (head1 == 'G' && head2 == 'R' && head3 == 'D')
                {
                    for (uint8 i = 0; i < EncounterCount; ++i)
                    {
                        uint32 state;
                        loadStream >> state;
                        if (state == IN_PROGRESS || state > SPECIAL)
                            state = NOT_STARTED;
                        SetBossState(i, EncounterState(state));
                    }
                }
                else
                    OUT_LOAD_INST_DATA_FAIL;

                OUT_LOAD_INST_DATA_COMPLETE;
            }

        private:
            ObjectGuid _rocketsparkGuid;
            ObjectGuid _borkaGuid;
            ObjectGuid _nitroggGuid;
            ObjectGuid _tovraGuid;
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const override
        {
            return new instance_grimrail_depot_InstanceMapScript(map);
        }
};

void AddSC_instance_grimrail_depot()
{
    new instance_grimrail_depot();
}
