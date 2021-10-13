#pragma once

#include <pigeon/util.h>
#include <pigeon/array_list.h>

PIGEON_ERR_RET add_to_ptr_list(PigeonArrayList ** alp, void* x);
void remove_from_ptr_list(PigeonArrayList ** alp, void* x);
