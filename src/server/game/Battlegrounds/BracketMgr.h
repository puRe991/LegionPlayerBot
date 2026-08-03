#ifndef _BRACKETMGR_H
#define _BRACKETMGR_H

#include "ObjectGuid.h"

#include "Define.h"
#include <map>

#include "Bracket.h"
#include "Player.h"

class BracketMgr
{
    BracketMgr();
    ~BracketMgr();

public:
    static BracketMgr* instance();

    void LoadCharacterBrackets();

    Bracket* TryGetOrCreateBracket(ObjectGuid guid, uint8 bType);
    void DeleteBracketInfo(ObjectGuid guid);
    void ResetWeekly();

private:
    std::map<ObjectGuid, BracketList> _container;
};

#define sBracketMgr BracketMgr::instance()

#endif
