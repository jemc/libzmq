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

#include "likely.hpp"

zmq::mmsg_t::mmsg_t () :
    capacity (0),
    size (0),
    msgs (NULL)
{
}

zmq::mmsg_t::~mmsg_t ()
{
    // TODO
}

size_t zmq::mmsg_t::get_size ()
{
    return size;
}

zmq::msg_t *zmq::mmsg_t::get (size_t index_)
{
    if (unlikely (index_ >= size))
        return NULL;
    return msgs[index_];
}

int zmq::mmsg_t::push (zmq::msg_t *msg_)
{
    size_t index = size;

    int rc = set_size (size + 1);
    if (rc != 0)
        return -1;

    msgs[index] = msg_;
    return 0;
}

bool zmq::mmsg_t::check ()
{
    for (size_t i = 0; i < size; i++) {
        if (!msgs[i]->check ())
            return false;
    }
    return true;
}

void zmq::mmsg_t::reset_metadata ()
{
    for (size_t i = 0; i < size; i++)
        msgs[i]->reset_metadata ();
}

void zmq::mmsg_t::normalize_flags ()
{
    size_t last = (size - 1);
    for (size_t i = 0; i < last; i++)
        msgs[i]->set_flags (msg_t::more);

    msgs[last]->reset_flags (msg_t::more);
}


int zmq::mmsg_t::set_size(size_t new_size_)
{
    // If the target size is greater than current capacity, we must expand.
    if (capacity < new_size_) {
        // TODO: use larger capacity to reduce reallocations (next power of 2?).
        size_t new_capacity = new_size_;

        // Allocate the new array of pointers.
        zmq::msg_t **new_msgs = \
            (zmq::msg_t **) malloc (new_capacity * sizeof (zmq::msg_t *));
        if (unlikely (!new_msgs)) {
            errno = ENOMEM;
            return -1;
        }

        // If there is an existing array, it must be copied then freed.
        if (msgs) {
            memcpy (new_msgs, msgs, size * sizeof (zmq::msg_t *));
            free (msgs);
        }

        capacity = new_capacity;
        msgs = new_msgs;
    }

    size = new_size_;
    return 0;
}
