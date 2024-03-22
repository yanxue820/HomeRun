#pragma once

#include "macoro/task.h"
#include "macoro/sync_wait.h"
#include "macoro/when_all.h"
#include <volePSI/RsOprf.h>
#include <coproto/Socket/AsioSocket.h>
#include <coproto/Socket/Socket.h>
#include <coproto/coproto.h>

namespace
{
    // inline auto eval(macoro::task<>& t0, macoro::task<>& t1)
    // {
    //     auto r = macoro::sync_wait(macoro::when_all_ready(std::move(t0), std::move(t1)));
    //     std::get<0>(r).result();
    //     std::get<1>(r).result();
    // }
    inline auto eval(macoro::task<>& t0)
    {
        macoro::sync_wait(t0);
    }

    coproto::task<> echoClient(std::vector<volePSI::block> message, coproto::Socket& socket)
    {
        // first we have to start with the MC_BEGIN(task<>,);
        // macro. We will discuss what this does later. The
        // parameter(s) are a lambda capture of the function 
        // parameter. Only these parameters can be used inside
        // the "coroutine".
        //
        // Here we are "lambda capturing" message by copy
        // and socket by reference. 
        MC_BEGIN(coproto::task<>,message, &socket);
        std::cout << "sending... " << std::endl;

        // send the size of the message and the message 
        // itself. Instead of using the c++20 co_await keyword
        // we will use the MC_AWAIT macro to achieve a similar
        // result. This will pause the function at this point
        // until the operation has completed.
        //MC_AWAIT(socket.send(message.size()));
        MC_AWAIT(socket.send(message));

        // wait for the server to respond.
        //MC_AWAIT(socket.recv(message));
        //std::cout << "echo client received: " << message << std::endl;

        // finally, we end the coroutine with MC_END.
        MC_END();
    }

    coproto::task<> echoServer(coproto::Socket& s, volePSI::u64 n,std::vector<volePSI::block> &message)
    {
        // again, the parameters to CP_BEGIN represent
        // the lambda capture for this coroutine. All
        // local variables must be declared here.
        //
        // we are again capturing socket by reference.
        // Additionally, we make two local variables,
        // size ith type size_t, and message with type std::string.
        MC_BEGIN(coproto::task<>,
            &socket = s,
            size = n,
            &message 
        );

        //MC_AWAIT(socket.recv(size));
        message.resize(size);

        // the size of the received message must match.
        // if not as error will occur.
        MC_AWAIT(socket.recv(message));

        std::cout << "echo server received: "  << std::endl;

        // send the result back.
        //MC_AWAIT(socket.send(message));

        MC_END();

    }



}