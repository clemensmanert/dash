#include <libdash.h>
#include <gtest/gtest.h>
#include "TestBase.h"
#include "TransformTest.h"

#include <array>

TEST_F(TransformTest, GlobalPlusLocalIntBlocking) {
  // Add local range to every block in global range
  const size_t num_elem_local = 5;
  size_t num_elem_total = _dash_size * num_elem_local;
  dash::Array<int> array_dest(num_elem_total, dash::BLOCKED);
  std::array<int, num_elem_local> local;

  // Initialize result array: [ 100, 100, ... | 200, 200, ... ]
  for (auto l_it = array_dest.lbegin(); l_it != array_dest.lend(); ++l_it) {
    *l_it = (dash::myid() + 1) * 100;
  }

  // Every unit adds a local range of elements to every block in a global
  // array.

  // Initialize local values, e.g. unit 2: [ 2000, 2001, 2002, ... ]
  for (auto l_idx = 0; l_idx < num_elem_local; ++l_idx) {
    local[l_idx] = ((dash::myid() + 1) * 1000) + (l_idx + 1);
  }

  // Accumulate local range to every block in the array:
  for (auto block_idx = 0; block_idx < _dash_size; ++block_idx) {
    auto block_offset = block_idx * num_elem_local;
    dash::transform<int>(&(*local.begin()), &(*local.end()), // A
                         array_dest.begin() + block_offset,  // B
                         array_dest.begin() + block_offset,  // C = op(A, B)
                         dash::plus<int>());                 // op
  }

  dash::barrier();
  
  // Verify values in local partition of array:
  
  // Gaussian sum of all local values accumulated = 1100 + 1200 + ...
  int global_acc = ((dash::myid() + 1) * 100) +
                   ((_dash_size + 1) * _dash_size * 1000) / 2;
  for (auto l_idx = 0; l_idx < num_elem_local; ++l_idx) {
    int expected = global_acc + ((l_idx + 1) * _dash_size);
    LOG_MESSAGE("array_dest.local[%d]: %p", l_idx, &array_dest.local[l_idx]);
    ASSERT_EQ_U(expected, array_dest.local[l_idx]);
  }
}

TEST_F(TransformTest, GlobalPlusGlobalIntBlocking) {
  // Add values in global range to values in other global range
  const size_t num_elem_local = 100;
  size_t num_elem_total = _dash_size * num_elem_local;
  dash::Array<int> array_dest(num_elem_total, dash::BLOCKED);
  dash::Array<int> array_values(num_elem_total, dash::BLOCKED);

  // Initialize result array: [ 100, 100, ... | 200, 200, ... ]
  for (auto l_it = array_dest.lbegin(); l_it != array_dest.lend(); ++l_it) {
    *l_it = (dash::myid() + 1) * 100;
  }

  // Every unit adds a local range of elements to every block in a global
  // array.

  // Initialize values to add, e.g. unit 2: [ 2000, 2001, 2002, ... ]
  for (auto l_idx = 0; l_idx < num_elem_local; ++l_idx) {
    array_values.local[l_idx] = ((dash::myid() + 1) * 1000) + (l_idx + 1);
  }

  // Accumulate local range to every block in the array:
  dash::transform<int>(array_values.begin(), array_values.end(), // A
                       array_dest.begin(),                       // B
                       array_dest.begin(),                       // = A op B
                       dash::plus<int>());                       // op

  dash::barrier();
  
  // Verify values in local partition of array:
  
  for (auto l_idx = 0; l_idx < num_elem_local; ++l_idx) {
    int expected = (dash::myid() + 1) * 100 +               // initial value
                   (dash::myid() + 1) * 1000 + (l_idx + 1); // added value
    ASSERT_EQ_U(expected, array_dest.local[l_idx]);
  }
}