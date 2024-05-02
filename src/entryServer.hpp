#include <iostream>
#include <queue>
#include "App.h"

#ifndef DATA_MODEL_HPP
#include "utils/dataModel.hpp"
#endif
#ifndef MATHLIB_HPP
#include "utils/mathlib.hpp"
#endif
#include <fstream>

struct StatData
{
    bool booting_up;
    bool inserting_data;
    bool done;

    int remaining_task_count;
    int client_count;
    long long elapsed_time;

    StatData(bool booting_up, bool inserting_data, bool done, int remaining_task_count, int client_count, long long elapsed_time)
    {
        this->booting_up = booting_up;
        this->inserting_data = inserting_data;
        this->done = done;
        this->remaining_task_count = remaining_task_count;
        this->client_count = client_count;
        this->elapsed_time = elapsed_time;
    }

    std::string serialize()
    {
        std::string s = std::to_string(booting_up) + ";;" +
                        std::to_string(inserting_data) + ";;" +
                        std::to_string(done) + ";;" +
                        std::to_string(remaining_task_count) + ";;" +
                        std::to_string(client_count) + ";;" +
                        std::to_string(elapsed_time);
        return s;
    }
};

class EntryServerHandler
{
public:
    bool booting_up = true;
    bool inserting_data = false; // TODO, to keep the queue exploding
    bool done = false;
    std::chrono::time_point<std::chrono::high_resolution_clock> start;

    std::unordered_set<int> node_ids;
    std::unordered_map<int, int> action_ids;
    Vector a_rows[VECTOR_SIZE];
    Vector b_cols[VECTOR_SIZE];
    std::queue<int> task_queue;
    Matrix result;

    std::unordered_map<int, int> task_status;

    EntryServerHandler()
    {
        booting_up = true;
        inserting_data = false;
        done = false;
    }

    int make_action_id(int row_idx, int col_idx)
    {
        return row_idx * VECTOR_SIZE + col_idx + 1;
    }
    std::tuple<int, int> unpack_action_id(int action_id)
    {
        action_id -= 1;
        return {action_id / VECTOR_SIZE, action_id % VECTOR_SIZE};
    }
    int make_random_id()
    {
        return rand() % 100 + 1;
    }

    void set_input_data(Matrix &A, Matrix &B, Matrix &C)
    {
        for (int i = 0; i < A.rows; i++)
        {
            a_rows[i] = A.getRow(i);
        }
        for (int i = 0; i < B.cols; i++)
        {
            b_cols[i] = B.getCol(i);
        }
        result = C;

        for (int i = 0; i < A.rows; i++)
        {
            for (int j = 0; j < B.cols; j++)
            {
                task_queue.push(make_action_id(i, j));
            }
        }
        if (DEBUG)
            std::cout << "Task queue size: " << task_queue.size() << std::endl;
    }

    int assign_single_task(int node_id)
    {
        if (task_queue.front() == 0)
            task_queue.pop();
        if (task_queue.size() == 0)
        {
            return -1;
        }
        int action_id = task_queue.front();
        task_queue.pop();

        action_ids[node_id] = action_id;

        return action_id;
    }

    std::tuple<std::string, std::string> handle_enter(int node_id, std::string_view &data)
    {
        if (node_ids.count(node_id) > 0)
        {
            std::cout << "Client id " + std::to_string(node_id) + " is already taken.\n";
            return std::make_tuple("ENTER_RESP", "0");
        }

        node_ids.insert(node_id);

        std::cout << "Client id " + std::to_string(node_id) + " is connected.\n";

        if (booting_up)
        {
            std::cout << "Currently booting up!\n";
            action_ids[node_id] = -1;
            return std::make_tuple("ENTER_RESP", "1");
        }
        else
        {
            int action_id = assign_single_task(node_id);
            if (DEBUG)
                std::cout << "Assigned task " << action_id << " to client " << node_id << std::endl;
            if (action_id == -1)
            {
                return std::make_tuple("ENTER_RESP", "1");
            }
            return std::make_tuple("ASSIGN_ACTION", "1");
        }
    }

    std::tuple<std::string, std::string> handle_close(int node_id, std::string_view &data)
    {
        if (node_ids.count(node_id) == 0)
        {
            if (DEBUG)
                std::cout << "Client id " + std::to_string(node_id) + " is not found.\n";
            return std::make_tuple("ENTER_RESP", "0");
        }

        if (action_ids.count(node_id) > 0 && action_ids[node_id] > 0)
        {
            task_queue.push(action_ids[node_id]);
        }
        action_ids.erase(node_id);
        node_ids.erase(node_id);
        return std::make_tuple("ENTER_RESP", "1");
    }

    std::tuple<std::string, std::string> handle_nudge(int node_id, std::string_view &data)
    {
        if (booting_up)
        {
            if (DEBUG)
                std::cout << "Still booting up!\n";
            return std::make_tuple("NUDGE_RESP", "1");
        }
        else
        {
            int action_id = assign_single_task(node_id);
            if (DEBUG)
                std::cout << "Assigned task " << action_id << " to client " << node_id << std::endl;
            if (action_id == -1)
            {
                return std::make_tuple("NUDGE_RESP", "1");
            }
            return std::make_tuple("ASSIGN_ACTION", "1");
        }
    }

    std::tuple<std::string, std::string> handle_get_a(int node_id, std::string_view &data)
    {
        auto [a_row_idx, _] = unpack_action_id(action_ids[node_id]);
        std::string serialized_a = a_rows[a_row_idx].serialize();
        return std::make_tuple("GET_A_RESP", serialized_a);
    }

    std::tuple<std::string, std::string> handle_get_b(int node_id, std::string_view &data)
    {
        auto [_, b_col_idx] = unpack_action_id(action_ids[node_id]);
        std::string serialized_b = b_cols[b_col_idx].serialize();
        return std::make_tuple("GET_B_RESP", serialized_b);
    }

    std::tuple<std::string, std::string> handle_return(int node_id, std::string_view &data)
    {
        int action_id = action_ids[node_id];
        auto [row_idx, col_idx] = unpack_action_id(action_id);
        // long long result = a_rows[row_idx].dot(b_cols[col_idx]);
        long long got_result = std::stoll(std::string(data));
        result.set(row_idx, col_idx, got_result);
        action_ids.erase(node_id);

        int new_action_id = assign_single_task(node_id);
        if (new_action_id == -1)
        {
            if (task_queue.size() == 0 && action_ids.size() == 0)
            {
                std::cout << "All tasks are done!" << std::endl;
                done = true;
                return std::make_tuple("STOP", "");
            }
            else
            {
                return std::make_tuple("STOP", "");
            }
        }
        else
        {
            return std::make_tuple("ASSIGN_ACTION", "1");
        }
    }

    StatData get_stat()
    {
        int remaining_task_count = task_queue.size();
        int client_count = node_ids.size();
        long long elapsed_time = 0;
        if (!booting_up)
        {
            auto now = std::chrono::high_resolution_clock::now();
            elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
        }
        return StatData(booting_up, inserting_data, done, remaining_task_count, client_count, elapsed_time);
    }

    std::tuple<std::string, std::string> handle_stat(int node_id, std::string_view &data)
    {
        StatData stat = get_stat();
        std::string serialized_data = stat.serialize();
        return std::make_tuple("STAT_RESP", serialized_data);
    }

    // websocket handlers
    void message_handler(
        uWS::WebSocket<false, true, WebSocketData> *ws,
        std::string_view message,
        uWS::OpCode opCode)
    {
        auto [node_id_str, op_type, action_id_str, data] = split_message(message);
        int node_id = std::stoi(std::string(node_id_str));
        int got_action_id = (action_id_str.size() > 0) ? std::stoi(std::string(action_id_str)) : -1;
        if (DEBUG)
            std::cout << "Received operation: " << op_type << " of action id " << got_action_id << " with data length of : " << data.size() << " from client " << node_id << std::endl;

        if (got_action_id > 0 && got_action_id != action_ids[node_id])
        {
            std::cerr << "Action ID mismatch for client " << node_id << "!!!" << std::endl;
            return;
        }

        std::string res_op_type, res_data;

        if (op_type == "ENTER")
            std::tie(res_op_type, res_data) = handle_enter(node_id, data);
        else if (op_type == "CLOSE")
            std::tie(res_op_type, res_data) = handle_close(node_id, data);
        else if (op_type == "NUDGE")
            std::tie(res_op_type, res_data) = handle_nudge(node_id, data);
        else if (op_type == "GET_A")
            std::tie(res_op_type, res_data) = handle_get_a(node_id, data);
        else if (op_type == "GET_B")
            std::tie(res_op_type, res_data) = handle_get_b(node_id, data);
        else if (op_type == "RETURN")
            std::tie(res_op_type, res_data) = handle_return(node_id, data);
        else if (op_type == "STAT")
            std::tie(res_op_type, res_data) = handle_stat(node_id, data);
        else
        {
            res_data = "Invalid operation";
            std::cerr << "Invalid operation " << op_type << " from client " << node_id << std::endl;
            return;
        }

        int action_id = action_ids[node_id];

        if (DEBUG)
            std::cout << "Sending response " << res_op_type << " of action id " << action_id << " with data length of : " << res_data.size() << " to client " << node_id << std::endl;
        std::string response = format_message(node_id, res_op_type, action_id, res_data);
        ws->send(response, uWS::OpCode::TEXT, response.length() < 16 * 1024);
    }

    void close_handler(
        uWS::WebSocket<false, true, WebSocketData> *ws,
        int code,
        std::string_view message)
    {
        std::cout << "Client disconnected. " << code << " " << message << std::endl;
    }

    // http handlers
    void get_stat_handler(uWS::HttpResponse<false> *res, uWS::HttpRequest *req)
    {
        StatData stat = get_stat();
        float avg_throughput = (stat.elapsed_time > 0) ? ((VECTOR_SIZE * VECTOR_SIZE - stat.remaining_task_count) * 1000.0 / stat.elapsed_time) : 0;
        std::string document = "<h1>Current status</h1>";

        document += "<p>Booting up: " + std::to_string(stat.booting_up) + "</p>";
        document += "<p>Inserting data: " + std::to_string(stat.inserting_data) + "</p>";
        document += "<p>Done: " + std::to_string(stat.done) + "</p>";
        document += "<br/>";

        document += "<p>Remaining task count: " + std::to_string(stat.remaining_task_count) + "</p>";
        document += "<p>Client count: " + std::to_string(stat.client_count) + "</p>";
        document += "<p>Elapsed time: " + std::to_string(stat.elapsed_time) + "</p>";
        document += "<p>Average throughput: " + std::to_string(avg_throughput) + " tasks/sec</p>";
        document += "<br/>";

        if (task_queue.size() > 0)
        {
            document += "<p>Task status: ";
            auto [row_idx, col_idx] = unpack_action_id(task_queue.front());
            document += "(" + std::to_string(row_idx) + ", " + std::to_string(col_idx) + ") ";
            // for (auto x : task_queue)
            // {
            //     auto [row_idx, col_idx] = unpack_action_id(x);
            //     document += "(" + std::to_string(row_idx) + ", " + std::to_string(col_idx) + ") ";
            //     count += 1;
            //     if (count > 10)
            //         break;
            // }
            document += "</p>";
        }
        res->end(document);
    }
};
