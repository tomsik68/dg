#include <cstdarg>
#include <cstdio>
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "ADT/Queue.h"

using namespace dg::ADT;

namespace dg {
namespace tests {

SCENARIO( "LIFO" ) {
    GIVEN( "empty LIFO" ) {
        QueueLIFO<int> queue;
        THEN( "is empty" ) {
            REQUIRE( queue.empty() );
        }

        WHEN( "elements are pushed" ) {
            queue.push(1);
            queue.push(13);
            queue.push(4);
            queue.push(2);
            queue.push(2);

            WHEN( "elements are popped" ) {
                REQUIRE(queue.pop() == 2);
                REQUIRE(queue.pop() == 2);
                REQUIRE(queue.pop() == 4);
                REQUIRE(queue.pop() == 13);
                REQUIRE(queue.pop() == 1);

                THEN( "queue is empty" ) {
                    REQUIRE(queue.empty());
                }
            }
        }

    }
}

TEST_CASE( "FIFO test" ) {
    QueueFIFO<int> queue;

    SECTION( "empty" ) {
        REQUIRE(queue.empty());
    }

    SECTION( "push and pop" ) {
        queue.push(1);
        queue.push(13);
        queue.push(4);
        queue.push(4);
        queue.push(2);

        SECTION( "pop order" ) {
            REQUIRE(queue.pop() == 1);
            REQUIRE(queue.pop() == 13);
            REQUIRE(queue.pop() == 4);
            REQUIRE(queue.pop() == 4);
            REQUIRE(queue.pop() == 2);
            REQUIRE(queue.empty());
        }
    }
}

struct mycomp
{
    bool operator()(int a, int b) const
    {
        return a > b;
    }
};

SCENARIO( "PrioritySet" ) {
    GIVEN( "empty PrioritySet" ) {
        PrioritySet<int, mycomp> queue;
        THEN( "it must be empty" ) {
            REQUIRE(queue.empty());
        }
        WHEN( "elements are pushed in queue" ) {
            queue.push(1);
            queue.push(13);
            queue.push(4);
            queue.push(4);
            queue.push(2);
            THEN( "set size must be 4" ) {
                REQUIRE(queue.size() == 4);
            }
            WHEN( "elements are popped" ) {
                REQUIRE(queue.pop() == 13);
                REQUIRE(queue.pop() == 4);
                REQUIRE(queue.pop() == 2);
                REQUIRE(queue.pop() == 1);
                THEN( "queue must be empty" ) {
                    REQUIRE(queue.empty());
                }
            }
        }
    }
}

}; // namespace tests
}; // namespace dg

