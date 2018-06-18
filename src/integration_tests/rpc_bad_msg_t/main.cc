// stdlib++
#include <iostream>

// smf headers
#include <smf/log.h>
#include <smf/rpc_generated.h>

// smfc generated headers
#include "integration_tests/bad_svc.smf.fb.h"

using test_msg_t = smf::rpc_typed_envelope<bad::msg::test>;

int main(int argc, char **argv) {
    test_msg_t test;
    test.data->test_node->name = "TEST";
    return 0;
}