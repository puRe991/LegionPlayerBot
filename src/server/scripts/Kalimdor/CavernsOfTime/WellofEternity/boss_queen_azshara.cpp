/* ScriptData
SDName: Boss_Queen_Azshara
SD%Complete: 80
SDComment: The file held one spell script and no encounter at all. Azshara does
           not fight: she sends her Enchanted Magi in pairs, never two of the
           same school at once, and charms the party with Total Obedience,
           which has to be interrupted. Servant of the Queen takes one player
           out until the hand holding them is destroyed.

           Entries 54853 and 54883, Total Obedience 103241 and Servant of the
           Queen 102334 were checked against the Cataclysm database on Wowhead.
SDCategory: Caverns of Time, Well of Eternity
EndScriptData */

#include "InstanceScript.h"
#include "well_of_eternity.h"
#include "ScriptedEscortAI.h"

enum eEnums
{
    SPELL_COLDFLAME_AURA            = 102465,
    SPELL_COLDFLAME_VISUAL_EFFECT   = 102466,
};

enum AzsharaSpells
{
    SPELL_TOTAL_OBEDIENCE           = 103241,
    SPELL_SERVANT_OF_THE_QUEEN      = 102334
};

enum AzsharaTexts
{
    SAY_AGGRO                       = 0,
    SAY_SUMMON_MAGI                 = 1,
    SAY_TOTAL_OBEDIENCE             = 2,
    SAY_DEATH                       = 3
};

enum AzsharaEvents
{
    EVENT_SUMMON_MAGI               = 1,
    EVENT_TOTAL_OBEDIENCE           = 2,
    EVENT_SERVANT_OF_THE_QUEEN      = 3
};

enum AzsharaMisc
{
    MAGI_PER_WAVE                   = 2,
    // A wave also comes in on its own if the party is too slow with the last.
    MAGI_WAVE_TIMEOUT               = 45000
};

class boss_queen_azshara : public CreatureScript
{
    public:
        boss_queen_azshara() : CreatureScript("boss_queen_azshara") { }

        struct boss_queen_azsharaAI : public ScriptedAI
        {
            boss_queen_azsharaAI(Creature* creature) : ScriptedAI(creature), _summons(creature)
            {
                _instance = creature->GetInstanceScript();
                SetCombatMovement(false);
            }

            void Reset()
            {
                _events.Reset();
                _summons.DespawnAll();
                _magiAlive = 0;

                // She never lowers herself to fight; her guard does it.
                me->SetReactState(REACT_PASSIVE);
            }

            void EnterCombat(Unit* /*who*/)
            {
                Talk(SAY_AGGRO);

                if (_instance)
                    _instance->SetBossState(DATA_QUEEN_AZSHARA, IN_PROGRESS);

                _events.ScheduleEvent(EVENT_SUMMON_MAGI, 3000);
                _events.ScheduleEvent(EVENT_TOTAL_OBEDIENCE, urand(25000, 35000));
                _events.ScheduleEvent(EVENT_SERVANT_OF_THE_QUEEN, urand(15000, 20000));
            }

            void JustDied(Unit* /*killer*/)
            {
                Talk(SAY_DEATH);
                _summons.DespawnAll();

                if (_instance)
                    _instance->SetBossState(DATA_QUEEN_AZSHARA, DONE);
            }

            void EnterEvadeMode()
            {
                _summons.DespawnAll();

                if (_instance)
                    _instance->SetBossState(DATA_QUEEN_AZSHARA, FAIL);

                ScriptedAI::EnterEvadeMode();
            }

            void JustSummoned(Creature* summon)
            {
                _summons.Summon(summon);

                if (summon->GetEntry() != NPC_ENCHANTED_MAGUS)
                    return;

                ++_magiAlive;

                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                    summon->AI()->AttackStart(target);
            }

            // The next pair walks in as soon as the last one is dealt with.
            void SummonedCreatureDies(Creature* summon, Unit* /*killer*/)
            {
                if (summon->GetEntry() != NPC_ENCHANTED_MAGUS || !_magiAlive)
                    return;

                if (!--_magiAlive)
                    _events.RescheduleEvent(EVENT_SUMMON_MAGI, 5000);
            }

            void SummonMagi()
            {
                for (uint8 i = 0; i < MAGI_PER_WAVE; ++i)
                {
                    Position pos;
                    me->GetRandomNearPosition(pos, 12.0f);
                    me->SummonCreature(NPC_ENCHANTED_MAGUS, pos, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 60000);
                }

                Talk(SAY_SUMMON_MAGI);
                _events.RescheduleEvent(EVENT_SUMMON_MAGI, MAGI_WAVE_TIMEOUT);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                _events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SUMMON_MAGI:
                            SummonMagi();
                            break;
                        case EVENT_TOTAL_OBEDIENCE:
                            // Long and interruptible on purpose -- letting it
                            // land is what wipes the group.
                            Talk(SAY_TOTAL_OBEDIENCE);
                            DoCastAOE(SPELL_TOTAL_OBEDIENCE);
                            _events.ScheduleEvent(EVENT_TOTAL_OBEDIENCE, urand(25000, 35000));
                            break;
                        case EVENT_SERVANT_OF_THE_QUEEN:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_SERVANT_OF_THE_QUEEN);
                            _events.ScheduleEvent(EVENT_SERVANT_OF_THE_QUEEN, urand(20000, 30000));
                            break;
                        default:
                            break;
                    }
                }
            }

        private:
            InstanceScript* _instance;
            EventMap _events;
            SummonList _summons;
            uint8 _magiAlive;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_queen_azsharaAI(creature);
        }
};

class spell_boss_queen_azshara_drain_sssence : public SpellScriptLoader
{
    public:
        spell_boss_queen_azshara_drain_sssence() : SpellScriptLoader("spell_boss_queen_azshara_drain_sssence") { }

        class spell_boss_queen_azshara_drain_sssence_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_boss_queen_azshara_drain_sssence_AuraScript);

            uint32 numberTick;
            float x, y, z, _angle;

            bool Load()
            {
                numberTick = 1;
                x = y = z = _angle = 0.0f;
                return true;
            }

            void PeriodicTick(AuraEffect const* /*aurEff*/)
            {
                if(Unit* target = GetTarget())
                if (Unit* victim = target->getVictim())
                {
                    float dist;
                    Position _pos;
                    if(!x)
                    {
                        victim->GetPosition(x, y, z);
                        _angle = target->GetAngle(x, y);
                    }
                    dist = (target->GetDistance(x, y, z) / 12.0f) * numberTick;
                    target->GetNearPosition(_pos, dist, _angle);
                    numberTick++;
                    target->CastSpell(_pos.GetPositionX(), _pos.GetPositionY(), _pos.GetPositionZ(), SPELL_COLDFLAME_VISUAL_EFFECT, true);
                }
            }

            // function registering
            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_boss_queen_azshara_drain_sssence_AuraScript::PeriodicTick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_boss_queen_azshara_drain_sssence_AuraScript();
        }
};

void AddSC_boss_queen_azshara()
{
    new boss_queen_azshara();
    new spell_boss_queen_azshara_drain_sssence();
}
