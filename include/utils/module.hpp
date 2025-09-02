#pragma once

#include <string>
#include <vector>
#include <cstddef> // for size_t

namespace Module {
    using ModuleFunction = void (*)(void*);

    struct ModuleCall {
        ModuleFunction function = nullptr;
        void* arg = nullptr;
        std::string identifier;
    };

    class ModuleList {
        public:
            std::vector<ModuleCall> items;

            void init(int capacity);
            void free();

            void add(ModuleFunction function, void* arg, const std::string& identifier);
            void pop(); // removes the last item by default
            void del(const std::string& identifier);

            // ----------------------------
            // Run all modules
            // ----------------------------
            void run();
    };

} // namespace Module