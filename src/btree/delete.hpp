#ifndef __BTREE_DELETE_HPP__
#define __BTREE_DELETE_HPP__

#include "store.hpp"
#include "btree/node.hpp"
#include "btree/slice.hpp"

delete_result_t btree_delete(const store_key_t &key, bool dont_put_in_delete_queue, btree_slice_t *slice, repli_timestamp timestamp);

#endif // __BTREE_DELETE_HPP__
