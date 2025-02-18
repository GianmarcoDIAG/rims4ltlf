#include"Utils.h"

namespace Syft {

    std::vector<int> Utils::to_bits(int i, std::size_t size) {
        std::vector<int> bin;
            if (i == 0) bin.push_back(0);
            else {
                while (i) {
                    int r = i%2;
                    bin.push_back(r);
                    i /= 2;
                }
            }
        while (bin.size() < size) bin.push_back(0);
        return bin;
    }
}