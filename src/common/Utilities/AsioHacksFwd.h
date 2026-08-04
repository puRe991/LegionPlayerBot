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

#ifndef AsioHacksFwd_h__
#define AsioHacksFwd_h__

#include <boost/version.hpp>

#if BOOST_VERSION >= 106600

// Boost 1.66 dropped the trailing service template parameters from
// basic_resolver and basic_deadline_timer and gave the remaining ones default
// arguments. Those defaults cannot be repeated in a forward declaration, so
// pull in the real headers instead of hand-rolling the declarations.
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/date_time/posix_time/ptime.hpp>

namespace boost
{
    namespace asio
    {
        namespace ip
        {
            typedef basic_endpoint<tcp> tcp_endpoint;
            typedef tcp::resolver tcp_resolver;
        }
    }
}

#else

namespace boost
{
    namespace posix_time
    {
        class ptime;
    }

    namespace asio
    {
        namespace ip
        {
            class address;

            class tcp;

            template <typename InternetProtocol>
            class basic_endpoint;

            typedef basic_endpoint<tcp> tcp_endpoint;

            template <typename InternetProtocol>
            class resolver_service;

            template <typename InternetProtocol, typename ResolverService>
            class basic_resolver;

            typedef basic_resolver<tcp, resolver_service<tcp>> tcp_resolver;
        }

        template <typename Time>
        struct time_traits;

        template <typename TimeType, typename TimeTraits>
        class deadline_timer_service;

        template <typename Time, typename TimeTraits, typename TimerService>
        class basic_deadline_timer;

        typedef basic_deadline_timer<posix_time::ptime, time_traits<posix_time::ptime>, deadline_timer_service<posix_time::ptime, time_traits<posix_time::ptime>>> deadline_timer;
    }
}

#endif

namespace Trinity
{
    class AsioStrand;
}

#endif // AsioHacksFwd_h__
