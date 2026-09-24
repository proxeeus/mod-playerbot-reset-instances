#include "Chat.h"
#include "Config.h"
#include "Creature.h"
#include "Group.h"
#include "InstanceSaveMgr.h"
#include "Log.h"
#include "Player.h"
#include "PlayerbotAI.h"
#include "PlayerbotMgr.h"
#include "Playerbots.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "World.h"

namespace
{
    enum NoriolActions : uint32
    {
        ACTION_RESET_INSTANCES = 1,
        ACTION_BOOST_BOTS      = 2,
        ACTION_SKIP_OUTLAND    = 3,
        ACTION_DESCRIPTION     = 4,
        ACTION_BACK            = 5
    };

    // npc_text rows shipped in data/sql/db-world/
    enum NoriolTexts : uint32
    {
        NPC_TEXT_INTRO       = 190012,
        NPC_TEXT_DESCRIPTION = 190013
    };

    enum NoriolSpells : uint32
    {
        SPELL_TIME_BENDING_FX = 52759
    };

    constexpr uint32 COPPER_PER_GOLD = 10000;
    // Highest gold cost whose copper value still fits in ModifyMoney's int32 argument.
    constexpr uint32 MAX_GOLD_COST = MAX_MONEY_AMOUNT / COPPER_PER_GOLD;

    struct ChronomancerConfig
    {
        bool Enabled = true;
        bool GoldCostEnabled = true;
        uint32 ResetCost = 10;
        uint32 BotBoostCost = 1000;
        uint32 SkipOutlandCost = 5000;
        uint8 SkipOutlandFromLevel = 58;
        uint8 SkipOutlandToLevel = 68;
    };

    ChronomancerConfig sChronomancerConfig;

    uint32 LoadGoldCost(char const* name, uint32 defaultValue)
    {
        uint32 cost = sConfigMgr->GetOption<uint32>(name, defaultValue);
        if (cost > MAX_GOLD_COST)
        {
            LOG_ERROR("module", "Chronomancer: {} = {} exceeds the maximum of {} gold, clamping.",
                name, cost, MAX_GOLD_COST);
            cost = MAX_GOLD_COST;
        }
        return cost;
    }

    uint8 MaxPlayerLevel()
    {
        return static_cast<uint8>(sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL));
    }

    // Only bots controlled by this player: never real players, nor bots belonging to someone else in the group.
    bool IsOwnedPlayerBot(Player* owner, Player* member)
    {
        if (!member || member == owner)
            return false;

        PlayerbotAI* botAI = sPlayerbotsMgr.GetPlayerbotAI(member);
        return botAI && botAI->IsBotAI() && botAI->GetMaster() == owner;
    }

    // Same logic as `.instance unbind all`: the map the target currently stands in is kept.
    void UnbindAllInstances(Player* target)
    {
        for (uint8 i = 0; i < MAX_DIFFICULTY; ++i)
        {
            BoundInstancesMap const& boundInstances =
                sInstanceSaveMgr->PlayerGetBoundInstances(target->GetGUID(), Difficulty(i));
            for (BoundInstancesMap::const_iterator itr = boundInstances.begin(); itr != boundInstances.end();)
            {
                if (itr->first != target->GetMapId())
                {
                    sInstanceSaveMgr->PlayerUnbindInstance(target->GetGUID(), itr->first, Difficulty(i), true, target);
                    itr = boundInstances.begin();
                }
                else
                    ++itr;
            }
        }
    }

    // Returns false (and tells the player) when they cannot afford the service.
    bool TryChargeGold(Player* player, Creature* creature, uint32 goldCost)
    {
        if (!sChronomancerConfig.GoldCostEnabled || !goldCost)
            return true;

        uint32 costInCopper = goldCost * COPPER_PER_GOLD;
        if (!player->HasEnoughMoney(costInCopper))
        {
            creature->Say("Time magic isn't free. Come back with more gold, friend.", LANG_UNIVERSAL);
            return false;
        }

        player->ModifyMoney(-static_cast<int32>(costInCopper));
        ChatHandler(player->GetSession()).PSendSysMessage("You pay {} gold to Chronomancer Noriol.", goldCost);
        return true;
    }
}

class chronomancer_noriol : public CreatureScript
{
public:
    chronomancer_noriol() : CreatureScript("chronomancer_noriol") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (!sChronomancerConfig.Enabled)
        {
            ChatHandler(player->GetSession()).SendSysMessage("Chronomancer Noriol's powers are currently dormant.");
            CloseGossipMenuFor(player);
            return true;
        }

        ClearGossipMenuFor(player);

        // Safeguard Instance-reset and Playerbot-boosts at max level to prevent early exploits.
        if (player->GetLevel() >= MaxPlayerLevel())
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Please unravel the threads of fate.",
                GOSSIP_SENDER_MAIN, ACTION_RESET_INSTANCES);

            if (HasBoostableBot(player))
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Please elevate the fate of my companions.",
                    GOSSIP_SENDER_MAIN, ACTION_BOOST_BOTS);
        }

        if (CanSkipOutland(player))
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I want to assist in the Northrend campaign immediately.",
                GOSSIP_SENDER_MAIN, ACTION_SKIP_OUTLAND);

        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Could you explain to me your purpose?",
            GOSSIP_SENDER_MAIN, ACTION_DESCRIPTION);
        SendGossipMenuFor(player, NPC_TEXT_INTRO, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        ClearGossipMenuFor(player);

        if (!sChronomancerConfig.Enabled)
        {
            CloseGossipMenuFor(player);
            return true;
        }

        switch (action)
        {
            case ACTION_RESET_INSTANCES:
                ResetInstances(player, creature);
                break;
            case ACTION_BOOST_BOTS:
                BoostBots(player, creature);
                break;
            case ACTION_SKIP_OUTLAND:
                SkipOutland(player, creature);
                break;
            case ACTION_DESCRIPTION:
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I have more questions.", GOSSIP_SENDER_MAIN, ACTION_BACK);
                SendGossipMenuFor(player, NPC_TEXT_DESCRIPTION, creature->GetGUID());
                return true;
            case ACTION_BACK:
                return OnGossipHello(player, creature);
            default:
                break;
        }

        CloseGossipMenuFor(player);
        return true;
    }

private:
    static bool HasBoostableBot(Player* player)
    {
        Group* group = player->GetGroup();
        if (!group)
            return false;

        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->GetSource();
            if (IsOwnedPlayerBot(player, member) && member->GetLevel() < MaxPlayerLevel())
                return true;
        }

        return false;
    }

    static bool CanSkipOutland(Player* player)
    {
        uint8 fromLevel = sChronomancerConfig.SkipOutlandFromLevel;
        return fromLevel && player->GetLevel() == fromLevel
            && sChronomancerConfig.SkipOutlandToLevel <= MaxPlayerLevel();
    }

    static void ResetInstances(Player* player, Creature* creature)
    {
        if (player->GetLevel() < MaxPlayerLevel())
        {
            creature->Say("Your timeline is not yet ready for this.", LANG_UNIVERSAL);
            return;
        }

        if (!TryChargeGold(player, creature, sChronomancerConfig.ResetCost))
            return;

        creature->Say("Time bends to my will. Be still... and begin anew.", LANG_UNIVERSAL);
        creature->CastSpell(player, SPELL_TIME_BENDING_FX, true);
        creature->HandleEmoteCommand(EMOTE_ONESHOT_SPELL_CAST_OMNI);

        ChatHandler handler(player->GetSession());
        UnbindAllInstances(player);

        if (Group* group = player->GetGroup())
        {
            for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
            {
                Player* member = itr->GetSource();
                if (!IsOwnedPlayerBot(player, member))
                    continue;

                UnbindAllInstances(member);
                handler.PSendSysMessage("{}'s timeline has also been reset.", member->GetName());
            }
        }

        handler.SendSysMessage("Your timeline has been reset.");
    }

    static void BoostBots(Player* player, Creature* creature)
    {
        ChatHandler handler(player->GetSession());
        uint8 maxLevel = MaxPlayerLevel();
        bool boosted = false;

        if (player->GetLevel() >= maxLevel)
        {
            if (Group* group = player->GetGroup())
            {
                for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
                {
                    Player* member = itr->GetSource();
                    if (!IsOwnedPlayerBot(player, member) || member->GetLevel() >= maxLevel)
                        continue;

                    if (!TryChargeGold(player, creature, sChronomancerConfig.BotBoostCost))
                        break;

                    member->GiveLevel(maxLevel);
                    boosted = true;
                    handler.PSendSysMessage("{}'s fate has been augmented.", member->GetName());
                }
            }
        }

        if (boosted)
            handler.SendSysMessage("The timelines have been altered.");
        else
            handler.SendSysMessage("No suitable companions found to augment.");
    }

    static void SkipOutland(Player* player, Creature* creature)
    {
        if (!CanSkipOutland(player))
        {
            creature->Say("Your timeline no longer allows this jump.", LANG_UNIVERSAL);
            return;
        }

        if (!TryChargeGold(player, creature, sChronomancerConfig.SkipOutlandCost))
            return;

        player->GiveLevel(sChronomancerConfig.SkipOutlandToLevel);
        player->UpdateSkillsToMaxSkillsForLevel();
        player->SetFullHealth();
        if (player->getPowerType() == POWER_MANA)
            player->SetPower(POWER_MANA, player->GetMaxPower(POWER_MANA));

        creature->Say("Your fate has been fast-forwarded.", LANG_UNIVERSAL);
    }
};

class chronomancer_noriol_world : public WorldScript
{
public:
    chronomancer_noriol_world() : WorldScript("chronomancer_noriol_world") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        ChronomancerConfig config;
        config.Enabled = sConfigMgr->GetOption<bool>("Chronomancer.EnableModule", true);
        config.GoldCostEnabled = sConfigMgr->GetOption<bool>("Chronomancer.EnableGoldCost", true);
        config.ResetCost = LoadGoldCost("Chronomancer.GoldCostAmount", 10);
        config.BotBoostCost = LoadGoldCost("Chronomancer.BotBoostCostAmount", 1000);
        config.SkipOutlandCost = LoadGoldCost("Chronomancer.SkipOutlandCostAmount", 5000);

        uint32 fromLevel = sConfigMgr->GetOption<uint32>("Chronomancer.SkipOutlandFromLevel", 58);
        uint32 toLevel = sConfigMgr->GetOption<uint32>("Chronomancer.SkipOutlandToLevel", 68);
        if (fromLevel && (toLevel <= fromLevel || toLevel > STRONG_MAX_LEVEL))
        {
            LOG_ERROR("module", "Chronomancer: invalid Outland skip levels ({} -> {}), disabling the Outland skip.",
                fromLevel, toLevel);
            fromLevel = 0;
        }
        config.SkipOutlandFromLevel = static_cast<uint8>(fromLevel);
        config.SkipOutlandToLevel = static_cast<uint8>(toLevel);

        sChronomancerConfig = config;
    }
};

void Addmod_playerbot_reset_instancesScripts()
{
    new chronomancer_noriol();
    new chronomancer_noriol_world();
}
