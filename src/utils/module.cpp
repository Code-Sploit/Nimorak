#include <utils/module.hpp>
#include <algorithm> // for std::find_if
#include <string>

namespace Module {
    void ModuleList::init(int capacity) {
        this->items.reserve(capacity);
    }

    void ModuleList::free() {
        this->items.clear();
        this->items.shrink_to_fit();
    }

    void ModuleList::add(Module::ModuleFunction function, void* arg, const std::string& identifier) {
        this->items.push_back({function, arg, identifier});
    }

    void ModuleList::pop() {
        if (!this->items.empty()) {
            this->items.pop_back();
        }
    }

    void ModuleList::del(const std::string& identifier) {
        auto it = std::find_if(
            this->items.begin(),
            this->items.end(),
            [&](const ModuleCall& call) {
                return call.identifier == identifier;
            });

        if (it != this->items.end()) {
            this->items.erase(it);
        }
    }

    void ModuleList::run() {
        for (auto& call : this->items) {
            if (call.function) {
                call.function(call.arg);
            }
        }
    }
} // namespace Module