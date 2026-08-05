#include "Containers.h"
#include "ObjectAccessor.h"
#include "ScriptedEscortAI.h"
#include "well_of_eternity.h"

class instance_well_of_eternity : public InstanceMapScript
{
public:
    instance_well_of_eternity() : InstanceMapScript("instance_well_of_eternity", 939) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
        return new instance_well_of_eternity_InstanceMapScript(map);
    }

    struct instance_well_of_eternity_InstanceMapScript : public InstanceScript
    {
        instance_well_of_eternity_InstanceMapScript(Map* map) : InstanceScript(map), uiTeamInInstance(0)
        {
            SetBossNumber(EncounterCount);
        }

        void OnPlayerEnter(Player* pPlayer)
        {
            if (!uiTeamInInstance)
                uiTeamInInstance = pPlayer->GetTeam();
        }

        void OnCreatureCreate(Creature* pCreature)
        {
            switch (pCreature->GetEntry())
            {
                case NPC_PEROTHARN:
                    perotharnGUID = pCreature->GetGUID();
                    break;
                case NPC_QUEEN_AZSHARA:
                    queenAzsharaGUID = pCreature->GetGUID();
                    break;
                case NPC_MANNOROTH:
                    mannorothGUID = pCreature->GetGUID();
                    break;
                case NPC_CAPTAIN_VAROTHEN:
                    varothenGUID = pCreature->GetGUID();
                    break;
                default:
                    break;
            }
        }

        uint32 GetData(uint32 type) const override
        {
            if (type == DATA_TEAM_IN_INSTANCE)
                return uiTeamInInstance;

            return 0;
        }

        ObjectGuid GetGuidData(uint32 type) const
        {
            switch (type)
            {
                case DATA_PEROTHARN_GUID:
                    return perotharnGUID;
                case DATA_QUEEN_AZSHARA_GUID:
                    return queenAzsharaGUID;
                case DATA_MANNOROTH_GUID:
                    return mannorothGUID;
                case DATA_VAROTHEN_GUID:
                    return varothenGUID;
                default:
                    break;
            }

            return ObjectGuid::Empty;
        }

        bool SetBossState(uint32 type, EncounterState state)
        {
            if (!InstanceScript::SetBossState(type, state))
                return false;

            return true;
        }

        std::string GetSaveData()
        {
            OUT_SAVE_INST_DATA;

            std::string str_data;

            std::ostringstream saveStream;
            saveStream << "W o T " << GetBossSaveData();

            str_data = saveStream.str();

            OUT_SAVE_INST_DATA_COMPLETE;
            return str_data;
        }

        void Load(const char* in)
        {
            if (!in)
            {
                OUT_LOAD_INST_DATA_FAIL;
                return;
            }

            OUT_LOAD_INST_DATA(in);

            char dataHead1, dataHead2, dataHead3;

            std::istringstream loadStream(in);
            loadStream >> dataHead1 >> dataHead2 >> dataHead3;

            if (dataHead1 == 'W' && dataHead2 == 'o' && dataHead3 == 'T')
            {
                for (uint8 i = 0; i < EncounterCount; ++i)
                {
                    uint32 tmpState;
                    loadStream >> tmpState;
                    if (tmpState == IN_PROGRESS || tmpState > SPECIAL)
                        tmpState = NOT_STARTED;
                    SetBossState(i, EncounterState(tmpState));
                }} else OUT_LOAD_INST_DATA_FAIL;

            OUT_LOAD_INST_DATA_COMPLETE;
        }

        private:
            uint32 uiTeamInInstance;
            ObjectGuid perotharnGUID;
            ObjectGuid queenAzsharaGUID;
            ObjectGuid mannorothGUID;
            ObjectGuid varothenGUID;
    };
};

class spell_mannoroth_lunar_shot : public SpellScriptLoader
{
    public:
        spell_mannoroth_lunar_shot() : SpellScriptLoader("spell_mannoroth_lunar_shot") { }

        class spell_mannoroth_lunar_shot_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mannoroth_lunar_shot_SpellScript);

            void FilterTargets(std::list<WorldObject*>& unitList)
            {
                for (std::list<WorldObject*>::iterator itr = unitList.begin(); itr != unitList.end();)
                {
                    if ((*itr)->GetEntry() == 57410)
                        ++itr;
                    else
                        unitList.erase(itr++);
                }
                if (unitList.size() > 3)
                    Trinity::Containers::RandomResizeList(unitList, 3);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_mannoroth_lunar_shot_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mannoroth_lunar_shot_SpellScript();
        }
};

class spell_mannoroth_wrath_of_elune : public SpellScriptLoader
{
    public:
        spell_mannoroth_wrath_of_elune() : SpellScriptLoader("spell_mannoroth_wrath_of_elune") { }

        class spell_mannoroth_wrath_of_elune_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mannoroth_wrath_of_elune_SpellScript);

            void FilterTargets(std::list<WorldObject*>& unitList)
            {
                if (unitList.size() > 4)
                    Trinity::Containers::RandomResizeList(unitList, 4);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_mannoroth_wrath_of_elune_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mannoroth_wrath_of_elune_SpellScript();
        }
};

void AddSC_instance_well_of_eternity()
{
    new instance_well_of_eternity();
    new spell_mannoroth_lunar_shot();
    new spell_mannoroth_wrath_of_elune();
}