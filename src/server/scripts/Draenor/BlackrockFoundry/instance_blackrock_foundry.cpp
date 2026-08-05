#include "blackrock_foundry.h"
#include "InstanceScript.h"
#include "ScriptMgr.h"

class instance_blackrock_foundry : public InstanceMapScript
{
public:
    instance_blackrock_foundry() : InstanceMapScript("instance_blackrock_foundry", 1205) {}

    struct instance_blackrock_foundry_InstanceMapScript : public InstanceScript
    {
        instance_blackrock_foundry_InstanceMapScript(Map* map) : InstanceScript(map) {}

        WorldLocation loc_res_pla;

        void Initialize() override
        {
            SetBossNumber(MaxBossData);
        }

        void OnCreatureCreate(Creature* creature) override
        {
            switch (creature->GetEntry())
            {
                case NpcGruul:              _gruulGuid = creature->GetGUID();               break;
                case NpcOregorger:          _oregorgerGuid = creature->GetGUID();           break;
                case NpcHeartOfTheMountain: _heartOfTheMountainGuid = creature->GetGUID();  break;
                case NpcForemanFeldspar:    _foremanFeldsparGuid = creature->GetGUID();     break;
                case NpcHansgar:            _hansgarGuid = creature->GetGUID();             break;
                case NpcFranzok:            _franzokGuid = creature->GetGUID();             break;
                case NpcFlamebenderKagraz:  _flamebenderKagrazGuid = creature->GetGUID();   break;
                case NpcKromog:             _kromogGuid = creature->GetGUID();              break;
                case NpcBeastlordDarmac:    _beastlordDarmacGuid = creature->GetGUID();     break;
                case NpcOperatorThogar:     _operatorThogarGuid = creature->GetGUID();      break;
                case NpcAdmiralGaran:       _admiralGaranGuid = creature->GetGUID();        break;
                case NpcMarakTheBlooded:    _marakTheBloodedGuid = creature->GetGUID();     break;
                case NpcEnforcerSorka:      _enforcerSorkaGuid = creature->GetGUID();       break;
                case NpcBlackhand:          _blackhandGuid = creature->GetGUID();           break;
                default:
                    break;
            }
        }

        ObjectGuid GetGuidData(uint32 type) const override
        {
            switch (type)
            {
                case DataGruulGuid:                 return _gruulGuid;
                case DataOregorgerGuid:             return _oregorgerGuid;
                case DataHeartOfTheMountainGuid:    return _heartOfTheMountainGuid;
                case DataForemanFeldsparGuid:       return _foremanFeldsparGuid;
                case DataHansgarGuid:               return _hansgarGuid;
                case DataFranzokGuid:               return _franzokGuid;
                case DataFlamebenderKagrazGuid:     return _flamebenderKagrazGuid;
                case DataKromogGuid:                return _kromogGuid;
                case DataBeastlordDarmacGuid:       return _beastlordDarmacGuid;
                case DataOperatorThogarGuid:        return _operatorThogarGuid;
                case DataAdmiralGaranGuid:          return _admiralGaranGuid;
                case DataMarakTheBloodedGuid:       return _marakTheBloodedGuid;
                case DataEnforcerSorkaGuid:         return _enforcerSorkaGuid;
                case DataBlackhandGuid:             return _blackhandGuid;
                default:
                    break;
            }

            return ObjectGuid::Empty;
        }

        std::string GetSaveData() override
        {
            OUT_SAVE_INST_DATA;

            std::ostringstream saveStream;
            saveStream << "B R F " << GetBossSaveData();

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

            if (head1 == 'B' && head2 == 'R' && head3 == 'F')
            {
                for (uint8 i = 0; i < MaxBossData; ++i)
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
            uint32 graveyardId = 4778;

            if (WorldSafeLocsEntry const* gy = sWorldSafeLocsStore.LookupEntry(graveyardId))
            {
                loc_res_pla.Relocate(gy->Loc.X, gy->Loc.Y, gy->Loc.Z);
                loc_res_pla.SetMapId(gy->MapID);
            }

            return &loc_res_pla;
        }

    private:
        ObjectGuid _gruulGuid;
        ObjectGuid _oregorgerGuid;
        ObjectGuid _heartOfTheMountainGuid;
        ObjectGuid _foremanFeldsparGuid;
        ObjectGuid _hansgarGuid;
        ObjectGuid _franzokGuid;
        ObjectGuid _flamebenderKagrazGuid;
        ObjectGuid _kromogGuid;
        ObjectGuid _beastlordDarmacGuid;
        ObjectGuid _operatorThogarGuid;
        ObjectGuid _admiralGaranGuid;
        ObjectGuid _marakTheBloodedGuid;
        ObjectGuid _enforcerSorkaGuid;
        ObjectGuid _blackhandGuid;
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_blackrock_foundry_InstanceMapScript(map);
    }
};

void AddSC_instance_blackrock_foundry()
{
    new instance_blackrock_foundry();
}
