/*
    Copyright (c) 2007-2015 Contributors as noted in the AUTHORS file

    This file is part of libzmq, the ZeroMQ core engine in C++.

    libzmq is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License (LGPL) as published
    by the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    As a special exception, the Contributors give you permission to link
    this library with independent modules to produce an executable,
    regardless of the license terms of these independent modules, and to
    copy and distribute the resulting executable under terms of your choice,
    provided that you also meet, for each linked independent module, the
    terms and conditions of the license of that module. An independent
    module is a module which is not derived from or based on this library.
    If you modify this library, you must extend this exception to your
    version of the library.

    libzmq is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "mmsg.hpp"
#include "../include/zmq.h"

#include <stdlib.h>
#include <string.h>

#include "err.hpp"
#include "likely.hpp"

zmq::mmsg_t::mmsg_t () :
    head (NULL),
    final (NULL),
    size (0)
{
}

zmq::mmsg_t::~mmsg_t ()
{
    // TODO
}

zmq::msg_t *zmq::mmsg_t::get_head ()
{
    return head;
}

zmq::msg_t *zmq::mmsg_t::get_final ()
{
    return final;
}

size_t zmq::mmsg_t::get_size ()
{
    return size;
}

zmq::msg_t *zmq::mmsg_t::take_head ()
{
    msg_t *detached_head = head;
    head = final = NULL;
    size = 0;
    return detached_head;
}

void zmq::mmsg_t::append (zmq::msg_t *other)
{
    zmq_assert (other);

    if (!head) {
        zmq_assert (!final);
        zmq_assert (size == 0);
        head = other;
    }
    else {
        zmq_assert (final);
        zmq_assert (size > 0);

        //  Handle the case in which the calling code uses SNDMORE many times
        //  with the same zmq_msg_t.
        if (other == final) {
            msg_t *new_other = new msg_t;
            new_other->init ();
            new_other->copy (*other);
            other = new_other;
        }

        final->link_tail(other);
    }

    final = other;
    size = size + 1;
}

bool zmq::mmsg_t::check ()
{
    while (msg_t *cursor = head) {
        if (!cursor->check ())
            return false;
        cursor = cursor->linked_tail();
    }

    return true;
}

void zmq::mmsg_t::reset_metadata ()
{
    //  Only the final message part can hold metadata to be reset.
    final->reset_metadata ();
}

void zmq::mmsg_t::normalize_flags ()
{
    //  Unset the more flag on all linked messages.
    //  TODO: remove the need for this - make linked checks prevent more checks.
    while (msg_t *cursor = head) {
        cursor->reset_flags (msg_t::more);
        cursor = cursor->linked_tail ();
    }
}
