#include <iostream>
#include <map>
#include "App.h"
#include "entryServer.hpp"

#ifndef DATA_MODEL_HPP
#include "utils/dataModel.hpp"
#endif

#ifndef MATHLIB_HPP
#include "utils/mathlib.hpp"
#endif

int main()
{
    srand(time(NULL));

    auto handler_ptr = new EntryServerHandler();
    uWS::App app =
        uWS::App()
            .get("/hello/:name",
                 [&handler_ptr](auto *res, auto *req)
                 { handler_ptr->get_stat_handler(res, req); })
            .ws<WebSocketData>(
                "/*",
                {
                    /* Settings */
                    .compression = uWS::CompressOptions(uWS::DEDICATED_COMPRESSOR_4KB | uWS::DEDICATED_DECOMPRESSOR),
                    .maxPayloadLength = 100 * 1024 * 1024,
                    .idleTimeout = 16,
                    .maxBackpressure = 100 * 1024 * 1024,
                    .closeOnBackpressureLimit = false,
                    .resetIdleTimeoutOnSend = false,
                    .sendPingsAutomatically = true,
                    /* Handlers */
                    .upgrade = nullptr,
                    .open = [](auto * /*ws*/) {},
                    .message = [&handler_ptr](auto *ws, std::string_view message, uWS::OpCode opCode)
                    { handler_ptr->message_handler(ws, message, opCode); },
                    .dropped = [](auto * /*ws*/, std::string_view /*message*/, uWS::OpCode /*opCode*/)
                    { std::cout << "Dropped message" << std::endl; },
                    .drain = [](auto * /*ws*/) {},
                    .ping = [](auto * /*ws*/, std::string_view)
                    { std::cout << "Ping received." << std::endl; },
                    .pong = [](auto * /*ws*/, std::string_view)
                    { std::cout << "Pong received." << std::endl; },
                    .close = [&handler_ptr](auto *ws, int code, std::string_view message)
                    { handler_ptr->close_handler(ws, code, message); },
                });

    app.listen(ENTRY_SERVER_PORT, [](auto *listen_socket)
               {if (listen_socket) {std::cout << "Listening on port " << ENTRY_SERVER_PORT << std::endl;} });

    std::cout << "Generate random matrix A and B\n";
    Matrix A = randomMatrix(VECTOR_SIZE, VECTOR_SIZE);
    Matrix B = randomMatrix(VECTOR_SIZE, VECTOR_SIZE);
    Matrix R(VECTOR_SIZE, VECTOR_SIZE);

    auto start = std::chrono::high_resolution_clock::now();
    auto end = std::chrono::high_resolution_clock::now();

    int BOOTUP_SECONDS = 5;
    std::thread bootup_thread = std::thread(
        [&start, handler_ptr, BOOTUP_SECONDS, &A, &B, &R]()
        {
            std::cout << "Booting up..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(BOOTUP_SECONDS));
            std::cout << "Booted up. Starting initialization." << std::endl;

            handler_ptr->set_input_data(A, B, R);

            std::cout << "Start!\n";
            handler_ptr->booting_up = false;
            start = std::chrono::high_resolution_clock::now();
        });

    std::thread cleanup_thread = std::thread(
        [&start, &end, handler_ptr, &app, &A, &B, &R]()
        {
            while (handler_ptr->booting_up)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            while (!handler_ptr->done)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                while (handler_ptr->task_queue.size() == 0 && handler_ptr->action_ids.size() > 0)
                {
                    auto it = handler_ptr->action_ids.begin();
                    if ((*it).second <= 0)
                        handler_ptr->action_ids.erase(it);
                }
                if (handler_ptr->task_queue.size() == 0 && handler_ptr->action_ids.size() == 0)
                {
                    std::cout << "All tasks done. Cleaning up..." << std::endl;
                    handler_ptr->done = true;
                }
            }
            end = std::chrono::high_resolution_clock::now();
            app.close();

            // check answers
            bool was_wrong = false;
            for (int i = 0; i < VECTOR_SIZE; i++)
                for (int j = 0; j < VECTOR_SIZE; j++)
                {
                    long long now = A.getRow(i).dot(B.getCol(j));
                    if (R.get(i, j) != now)
                    {
                        std::cout << "Error at " << i << " " << j << " : " << R.get(i, j) << " != " << now << std::endl;
                        was_wrong = true;
                    }
                }
            std::cout << "Done. " << (was_wrong ? "Error" : "Correct!!!") << std::endl;
            std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
        });

    app.run();
    bootup_thread.join();
    cleanup_thread.join();

    return 0;
}
