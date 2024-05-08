#include <iostream>
#include <queue>
#include <algorithm>
#include <random>
#include "App.h"

#ifndef DATA_MODEL_HPP
#include "utils/dataModel.hpp"
#endif
#ifndef MATHLIB_HPP
#include "utils/mathlib.hpp"
#endif
#include <fstream>

std::string serialize_vector_for_web(const Vector &v)
{
    std::string s = "";
    for (int i = 0; i < v.size; i++)
    {
        s += std::to_string(v.data[i]) + " ";
    }
    return s;
}

int find_from_map(const std::unordered_map<int, int> &m, int val)
{
    auto it = m.find(val);
    if (it != m.end())
        return it->second;
    return -1;
}

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
        std::string s = std::to_string(booting_up) + "::" +
                        std::to_string(inserting_data) + "::" +
                        std::to_string(done) + "::" +
                        std::to_string(remaining_task_count) + "::" +
                        std::to_string(client_count) + "::" +
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
    std::unordered_map<int, int> action_ids; // node_id -> action_id
    std::unordered_map<int, int> task_info;  // action_id -> task_info
    std::deque<int> task_queue;

    Vector random_masks[SHUFFLE_SIZE][2];                    // mask x, y
    long long random_mask_prods[SHUFFLE_SIZE][SHUFFLE_SIZE]; // x dot y
    int mask_indices[VECTOR_SIZE];
    Vector a_rows[VECTOR_SIZE][2];
    Vector b_cols[VECTOR_SIZE][2];
    Matrix result;

    std::random_device rd;
    std::mt19937 g;

    std::uniform_int_distribution<int> random_dist;

    EntryServerHandler()
    {
        booting_up = true;
        inserting_data = false;
        done = false;

        auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        g = std::mt19937(seed);
        random_dist = std::uniform_int_distribution<int>(1, 1e9);

        // Prepare random masks for Beaver triples
        for (int i = 0; i < SHUFFLE_SIZE; i++)
        {
            random_masks[i][0] = Vector(VECTOR_SIZE);
            randomVector(random_masks[i][0]);
            random_masks[i][1] = Vector(VECTOR_SIZE);
            randomVector(random_masks[i][1]);
        }
        // offset for Beaver triples
        for (int i = 0; i < SHUFFLE_SIZE; i++)
            for (int j = 0; j < SHUFFLE_SIZE; j++)
                random_mask_prods[i][j] = random_masks[i][0].dot(random_masks[j][1]);

        // Allocate random indices for each row/col vectors
        for (int i = 0; i < VECTOR_SIZE; i++)
        {
            mask_indices[i] = random_dist(g) % SHUFFLE_SIZE;
        }
    }

    int make_task_id(int row_idx, int col_idx, int sign)
    {
        return (row_idx * VECTOR_SIZE + col_idx) * 2 + sign + 1;
    }
    std::tuple<int, int, int> unpack_action_id(int action_id)
    {
        int zeroed_task_id = find_from_map(task_info, action_id) - 1;
        if (zeroed_task_id < 0)
        {
            std::cerr << "Task ID not found for action ID " << action_id << "!!!" << std::endl;
            return std::make_tuple(0, 0, 0);
        }
        int sign = zeroed_task_id % 2;
        zeroed_task_id /= 2;
        int col_idx = zeroed_task_id % VECTOR_SIZE;
        int row_idx = zeroed_task_id / VECTOR_SIZE;
        return std::make_tuple(row_idx, col_idx, sign);
    }

    void set_input_data(Matrix &A, Matrix &B, Matrix &C)
    {
        // apply shuffles to each vectors
        for (int i = 0; i < A.rows; i++)
        {
            int mask_idx = mask_indices[i];
            a_rows[i][0] = A.getRow(i) + random_masks[mask_idx][0];
            a_rows[i][1] = A.getRow(i) - random_masks[mask_idx][0];
        }
        for (int i = 0; i < B.cols; i++)
        {
            int mask_idx = mask_indices[i];
            b_cols[i][0] = B.getCol(i) + random_masks[mask_idx][1];
            b_cols[i][1] = B.getCol(i) - random_masks[mask_idx][1];
        }
        result = C;

        // fill task queue
        for (int i = 0; i < A.rows; i++)
        {
            for (int j = 0; j < B.cols; j++)
            {
                result.set(i, j, -2 * random_mask_prods[mask_indices[i]][mask_indices[j]]);
                task_queue.push_back(make_task_id(i, j, 0));
                task_queue.push_back(make_task_id(i, j, 1));
            }
        }

        // shuffle task queue
        std::shuffle(task_queue.begin(), task_queue.end(), g);

        if (DEBUG)
            std::cout << "Task queue size: " << task_queue.size() << std::endl;
    }

    int assign_single_task(int node_id)
    {
        if (task_queue.size() == 0)
            return -1;

        // while (task_queue.front() == 0)
        // {
        //     std::cerr << "Task ID 0 is found in the queue!!!" << std::endl;
        //     std::cerr << "Task queue size: " << task_queue.size() << std::endl;
        //     task_queue.pop_front();
        // }

        int task_id = task_queue.front();
        task_queue.pop_front();
        int action_id = random_dist(g);

        action_ids[node_id] = action_id;
        task_info[action_id] = task_id;
        // std::cout << "Assigned task " << action_ids[node_id] << " - " << task_info[action_id] << " to client " << node_id << std::endl;

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

        int ongoing_action_id = find_from_map(action_ids, node_id);
        if (ongoing_action_id > 0)
        {
            task_queue.push_back(ongoing_action_id);
            task_info.erase(ongoing_action_id);
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
        auto [row_idx, col_idx, sign] = unpack_action_id(find_from_map(action_ids, node_id));
        bool swap = (row_idx + col_idx) % 2; // should be pre-determined random
        std::string serialized_a = serialize_vector_for_web(swap ? b_cols[col_idx][sign] : a_rows[row_idx][sign]);
        return std::make_tuple("GET_A_RESP", serialized_a);
    }

    std::tuple<std::string, std::string> handle_get_b(int node_id, std::string_view &data)
    {
        auto [row_idx, col_idx, sign] = unpack_action_id(find_from_map(action_ids, node_id));
        bool swap = (row_idx + col_idx) % 2; // should be pre-determined random
        std::string serialized_b = serialize_vector_for_web(swap ? a_rows[row_idx][sign] : b_cols[col_idx][sign]);
        return std::make_tuple("GET_B_RESP", serialized_b);
    }

    std::tuple<std::string, std::string> handle_return(int node_id, std::string_view &data)
    {
        int action_id = find_from_map(action_ids, node_id);
        auto [row_idx, col_idx, sign] = unpack_action_id(action_id);
        long long got_result = std::stoll(std::string(data));
        // long long did_result = a_rows[row_idx][sign].dot(b_cols[col_idx][sign]); // expected result

        result.add(row_idx, col_idx, got_result); // result[row][col] = (a-x)(b-y) + (a+y)(b+x) - 2xy
        if (action_id == -1)
        {
            std::cerr << "Action ID not found for client " << node_id << "!!!" << std::endl;
            return std::make_tuple("STOP", "");
        }
        task_info.erase(action_id);
        action_ids.erase(node_id);

        int new_action_id = assign_single_task(node_id);
        if (new_action_id == -1)
        {
            return std::make_tuple("STOP", "");
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

        if (got_action_id > 0 && got_action_id != find_from_map(action_ids, node_id))
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

        int action_id = find_from_map(action_ids, node_id);

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
        float avg_throughput = (stat.elapsed_time > 0) ? ((2 * VECTOR_SIZE * VECTOR_SIZE - stat.remaining_task_count) * 1000.0 / stat.elapsed_time) : 0;
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

        if (task_queue.size() > 0 && false)
        {
            document += "<p>Task status: ";
            auto [row_idx, col_idx, sign] = unpack_action_id(task_queue.front());
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
