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

#include "rep.hpp"

#include "macros.hpp"
#include "err.hpp"
#include "msg.hpp"

zmq::rep_t::rep_t (class ctx_t *parent_, uint32_t tid_, int sid_) :
    socket_base_t (parent_, tid_, sid_),
    sending_reply (false),
    request_begins (true),
    reply_pipe (NULL)
{
    options.type = ZMQ_REP;
}

zmq::rep_t::~rep_t ()
{
}

int zmq::rep_t::xsend (msg_t *msg_)
{
    //  If we are in the middle of receiving a request, we cannot send reply.
    if (!sending_reply) {
        errno = EFSM;
        return -1;
    }

    zmq_assert (reply_pipe);
    if (!reply_pipe->write (msg_)) {
        errno = EAGAIN;
        return -1;
    }

    reply_pipe->flush ();

    //  Detach the original message from the data buffer.
    //  TODO: what to do with the tail here?
    int rc = msg_->init ();
    errno_assert (rc == 0);

    sending_reply = false;

    return 0;
}

int zmq::rep_t::xrecv (msg_t *msg_)
{
    // //  If we are in middle of sending a reply, we cannot receive next request.
    // if (sending_reply) {
    //     errno = EFSM;
    //     return -1;
    // }

    // //  First thing to do when receiving a request is to copy all the labels
    // //  to the reply pipe.
    // if (request_begins) {
    //     while (true) {
    //         int rc = router_t::xrecv (msg_);
    //         if (rc != 0)
    //             return rc;

    //         if ((msg_->flags () & msg_t::more)) {
    //             //  Empty message part delimits the traceback stack.
    //             bool bottom = (msg_->size () == 0);

    //             //  Push it to the reply pipe.
    //             rc = router_t::xsend (msg_);
    //             errno_assert (rc == 0);

    //             if (bottom)
    //                 break;
    //         }
    //         else {
    //             //  If the traceback stack is malformed, discard anything
    //             //  already sent to pipe (we're at end of invalid message).
    //             rc = router_t::rollback ();
    //             errno_assert (rc == 0);
    //         }
    //     }
    //     request_begins = false;
    // }

    // //  Get next message part to return to the user.
    // int rc = router_t::xrecv (msg_);
    // if (rc != 0)
    //    return rc;

    // //  If whole request is read, flip the FSM to reply-sending state.
    // if (!(msg_->flags () & msg_t::more)) {
    //     sending_reply = true;
    //     request_begins = true;
    // }

    return 0;
}

bool zmq::rep_t::xhas_in ()
{
    if (sending_reply)
        return false;

    return fq.has_in ();
}

bool zmq::rep_t::xhas_out ()
{
    if (!sending_reply)
        return false;

    return reply_pipe->check_write ();
}


void zmq::rep_t::xattach_pipe (pipe_t *pipe_, bool subscribe_to_all_)
{
    LIBZMQ_UNUSED (subscribe_to_all_);

    zmq_assert (pipe_);
    fq.attach (pipe_);
}

void zmq::rep_t::xread_activated (pipe_t *pipe_)
{
    fq.activated (pipe_);
}

void zmq::rep_t::xpipe_terminated (pipe_t *pipe_)
{
    fq.pipe_terminated (pipe_);

    if (pipe_ == reply_pipe) {
        reply_pipe = NULL;
        sending_reply = false; // There is no hope of ever replying now.
    }
}
