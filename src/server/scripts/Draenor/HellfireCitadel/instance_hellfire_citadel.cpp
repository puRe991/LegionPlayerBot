#include "hellfire_citadel.h"
#include "InstanceScript.h"
#include "ScriptMgr.h"

class instance_hellfire_citadel : public InstanceMapScript
{
public:
    instance_hellfire_citadel() : InstanceMapScript("instance_hellfire_citadel", 1448) {}

    struct instance_hellfire_citadel_InstanceMapScript : public InstanceScript
    {
        instance_hellfire_citadel_InstanceMapScript(Map* map) : InstanceScript(map) {}

        WorldLocation loc_res_pla;

        void Initialize() override
        {
            SetBossNumber(MAX_ENCOUNTER);
        }

        void OnCreatureCreate(Creature* creature) override
        {
            switch (creature->GetEntry())
            {
                case NPC_SIEGEMASTER_MARTAK:    _martakGuid = creature->GetGUID();      break;
                case NPC_IRON_REAVER:           _ironReaverGuid = creature->GetGUID();  break;
                case NPC_KORMROK:               _kormrokGuid = creature->GetGUID();     break;
                case NPC_KILROGG_DEADEYE:       _kilroggGuid = creature->GetGUID();     break;
                case NPC_GOREFIEND:             _gorefiendGuid = creature->GetGUID();   break;
                case NPC_SHADOW_LORD_ISKAR:     _iskarGuid = creature->GetGUID();       break;
                case NPC_SOCRETHAR_THE_ETERNAL: _socretharGuid = creature->GetGUID();   break;
                case NPC_TYRANT_VELHARI:        _velhariGuid = creature->GetGUID();     break;
                case NPC_FEL_LORD_ZAKUUN:       _zakuunGuid = creature->GetGUID();      break;
                case NPC_XHULHORAC:             _xhulhoracGuid = creature->GetGUID();   break;
                case NPC_MANNOROTH_HFC:         _mannorothGuid = creature->GetGUID();   break;
                case NPC_ARCHIMONDE:            _archimondeGuid = creature->GetGUID();  break;
                case NPC_DIA_DARKWHISPER:       _diaGuid = creature->GetGUID();         break;
                case NPC_GURTOGG_BLOODBOIL:     _gurtoggGuid = creature->GetGUID();     break;
                case NPC_BLADEMASTER_JUBEITHOS: _jubeithosGuid = creature->GetGUID();   break;
                default:
                    break;
            }
        }

        ObjectGuid GetGuidData(uint32 type) const override
        {
            switch (type)
            {
                case DATA_SIEGEMASTER_MARTAK_GUID:  return _martakGuid;
                case DATA_IRON_REAVER_GUID:         return _ironReaverGuid;
                case DATA_KORMROK_GUID:             return _kormrokGuid;
                case DATA_KILROGG_GUID:             return _kilroggGuid;
                case DATA_GOREFIEND_GUID:           return _gorefiendGuid;
                case DATA_ISKAR_GUID:               return _iskarGuid;
                case DATA_SOCRETHAR_GUID:           return _socretharGuid;
                case DATA_VELHARI_GUID:             return _velhariGuid;
                case DATA_ZAKUUN_GUID:              return _zakuunGuid;
                case DATA_XHULHORAC_GUID:           return _xhulhoracGuid;
                case DATA_MANNOROTH_GUID:           return _mannorothGuid;
                case DATA_ARCHIMONDE_GUID:          return _archimondeGuid;
                case DATA_DIA_DARKWHISPER_GUID:     return _diaGuid;
                case DATA_GURTOGG_BLOODBOIL_GUID:   return _gurtoggGuid;
                case DATA_BLADEMASTER_JUBEITHOS_GUID: return _jubeithosGuid;
                default:
                    break;
            }

            return ObjectGuid::Empty;
        }

        std::string GetSaveData() override
        {
            OUT_SAVE_INST_DATA;

            std::ostringstream saveStream;
            saveStream << "H F C " << GetBossSaveData();

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

            if (head1 == 'H' && head2 == 'F' && head3 == 'C')
            {
                for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
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

        WorldLocation* GetClosestGraveYard(float x, float y, float z) override
        {
            uint32 graveyardId = 5022;

            if (WorldSafeLocsEntry const* gy = sWorldSafeLocsStore.LookupEntry(graveyardId))
            {
                loc_res_pla.Relocate(gy->Loc.X, gy->Loc.Y, gy->Loc.Z);
                loc_res_pla.SetMapId(gy->MapID);
            }

            return &loc_res_pla;
        }

    private:
        ObjectGuid _martakGuid;
        ObjectGuid _ironReaverGuid;
        ObjectGuid _kormrokGuid;
        ObjectGuid _kilroggGuid;
        ObjectGuid _gorefiendGuid;
        ObjectGuid _iskarGuid;
        ObjectGuid _socretharGuid;
        ObjectGuid _velhariGuid;
        ObjectGuid _zakuunGuid;
        ObjectGuid _xhulhoracGuid;
        ObjectGuid _mannorothGuid;
        ObjectGuid _archimondeGuid;
        ObjectGuid _diaGuid;
        ObjectGuid _gurtoggGuid;
        ObjectGuid _jubeithosGuid;
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_hellfire_citadel_InstanceMapScript(map);
    }
};

void AddSC_instance_hellfire_citadel()
{
    new instance_hellfire_citadel();
}
