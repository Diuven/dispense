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

void websocketDistributedMul(Matrix &A, Matrix &B, Matrix &C)
{

    // open servers with &C, task_count

    Vector rows[A.rows]; // A.
    Vector cols[B.cols]; // B.

    for (int i = 0; i < A.rows; i++)
    {
        rows[i] = A.getRow(i);
    }
    for (int i = 0; i < B.cols; i++)
    {
        cols[i] = B.getCol(i);
    }

    auto start = std::chrono::high_resolution_clock::now();

    int cur_client_idx = 0;

    for (int i = 0; i < A.rows; i++)
    {
        for (int j = 0; j < B.cols; j++)
        {
            int action_id = i * A.cols + j;

            int client_idx = cur_client_idx % CLIENT_COUNT;
            cur_client_idx = (cur_client_idx + 1) % CLIENT_COUNT;
            // int client_id = client_ids[client_idx];

            // auto server = servers[client_idx];
            // std::string message;

            // message = std::to_string(client_id) + ";;ASSIGN_ACTION;;" + std::to_string(action_id);
            // server->publish("broadcast", message, uWS::OpCode::TEXT, message.length() < 16 * 1024);

            // message = std::to_string(client_id) + ";;VECTOR_A;;" + rows[i].serialize();
            // server->publish("broadcast", message, uWS::OpCode::TEXT, message.length() < 16 * 1024);

            // message = std::to_string(client_id) + ";;VECTOR_B;;" + cols[j].serialize();
            // server->publish("broadcast", message, uWS::OpCode::TEXT, message.length() < 16 * 1024);

            // message = std::to_string(client_id) + ";;CALCULATE;;";
            // server->publish("broadcast", message, uWS::OpCode::TEXT, message.length() < 16 * 1024);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
}

int main()
{
    srand(time(NULL));

    auto [entry_server_handler, entry_server_app] = get_entry_server();

    std::thread check_thread = std::thread(
        [&entry_server_app]()
        {
            for (int i = 0; i < 60; i++)
            {
                std::cout << "Main loop iteration " << i << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
            entry_server_app.close();
        });
    entry_server_app.run();

    check_thread.join();

    return 0;

    // std::cout << "All " + std::to_string(CLIENT_COUNT) + " clients connected." << std::endl;

    // std::cout << "Generate random matrix A and B\n";
    // Matrix A = randomMatrix(SIZE, SIZE);
    // Matrix B = randomMatrix(SIZE, SIZE);
    // Matrix R(SIZE, SIZE);

    // std::cout << "Start\n";
    // websocketDistributedMul(A, B, R);
}
