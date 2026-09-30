#include "../traversability/cost_map.hpp"

#include <cassert>
#include <iostream>

int main() {
    ugv::CostMap map(10, 8);

    assert(map.width() == 10);
    assert(map.height() == 8);

    map.setCost(3, 4, 75);
    assert(map.getCost(3, 4) == 75);

    map.reset();
    assert(map.getCost(3, 4) == 0);

    std::cout << "CostMap tests: PASS\n";

    return 0;
}
