/*
 * Copyright (C) 2008-2019 TrinityCore <https://www.trinitycore.org/>
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

#include "Banner.h"
#include "GitRevision.h"
#include "StringFormat.h"

void Trinity::Banner::Show(char const* applicationName, void(*log)(char const* text), void(*logExtraInfo)())
{
    log(Trinity::StringFormat("%s (%s)", GitRevision::GetFullVersion(), applicationName).c_str());
    log("<Ctrl-C> to stop.");
    log("    ______     ________");
    log("   / ____/__  / / ____/___  _________ ____");
    log("  / /_  / _ \\/ / /_  / __ \\/ ___/ __ `/ _ \\");
    log(" / __/ /  __/ / __/ / /_/ / /  / /_/ /  __/");
    log("/_/    \\___/_/_/    \\____/_/   \\__, /\\___/");
    log("                              /____/");
    log("  World of Warcraft: Legion 7.3.5 server with built-in playerbots");
    log("  https://github.com/puRe991/LegionPlayerBot - based on TrinityCore, https://www.trinitycore.org");
    log("");

    if (logExtraInfo)
        logExtraInfo();
}
