#include "trace_loader.h"
#include <fstream>
#include <stdexcept>

std::vector<Request> TraceLoader::load(const std::string& filepath, KeyMapper& mapper) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open trace file: " + filepath);
    }

    std::vector<Request> requests;
    std::string op_str, key;

    while (file >> op_str >> key) {
        Request req;

        if      (op_str == "R") req.op = Request::Op::READ;
        else if (op_str == "W") req.op = Request::Op::WRITE;
        else continue; // skip malformed lines silently

        req.id = mapper.normalize(key);
        requests.push_back(req);
    }

    return requests;
}
