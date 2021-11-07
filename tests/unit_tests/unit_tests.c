#include <config_parser_test.h>
#include <pigeon/assert.h>
#include <pigeon/util.h>
#include <pigeon/array_list.h>
#include <pigeon/object_pool.h>
#include <stdint.h>

static PIGEON_ERR_RET pigeon_test_array_list(void)
{
    PigeonArrayList al = {0};


    ASSERT_R1(!pigeon_create_array_list2(&al, 2, 2));
    ASSERT_R1(al.capacity >= 2 && al.size == 0 && al.element_size == 2 && al.elements);

    void * new = pigeon_array_list_add(&al, 4);
    ASSERT_R1(new == al.elements && al.size == 4 && al.capacity >= 4 && al.elements);
    new = pigeon_array_list_add(&al, 1);
    ASSERT_R1((uintptr_t)new == (uintptr_t)al.elements + 2*4 && al.elements);
    ASSERT_R1(al.size == 5 && al.capacity >= 5);

    uint16_t * e = al.elements;
    for(unsigned int i = 0; i < 5; i++) e[i] = (uint16_t)i;


    pigeon_array_list_remove_preserve_order(&al, 1, 2);
    e = al.elements;
    ASSERT_R1(al.elements && al.size == 3 && al.capacity >= 3 && e[0] == 0 && e[1] == 3 && e[2] == 4);


    pigeon_array_list_remove(&al, 0, 2);
    e = al.elements;
    ASSERT_R1(al.elements && al.size == 1 && al.capacity >= 1 && e[0] == 4);

    pigeon_destroy_array_list(&al);


    memset(&al, 0, sizeof al);

    pigeon_create_array_list(&al, 12);
    ASSERT_R1(al.capacity == 0 && al.size == 0 && !al.elements && al.element_size == 12);


    ASSERT_R1(!pigeon_array_list_resize(&al, 1000));
    ASSERT_R1(al.size == 1000 && al.capacity >= 1000 && al.elements);

    pigeon_array_list_zero(&al);
    ASSERT_R1(al.size == 1000 && al.capacity >= 1000 && al.elements);

    uint32_t * x = al.elements;
    for(unsigned int i = 0; i < 1000*3; i++) { ASSERT_R1(!x[i]); }

    pigeon_destroy_array_list(&al);


    return 0;
}

static PIGEON_ERR_RET pigeon_test_object_pool(void)
{
    PigeonObjectPool pool = {0};
    pigeon_create_object_pool(&pool, 4, true);
    ASSERT_R1(!pool.allocated_obj_count && pool.object_size == 4);
    pigeon_destroy_object_pool(&pool);


    memset(&pool, 0, sizeof pool);

    ASSERT_R1(!pigeon_create_object_pool2(&pool, 1, 500, true));
    ASSERT_R1(!pool.allocated_obj_count && pool.object_size == 1);

    uint8_t * obj = pigeon_object_pool_allocate(&pool);
    ASSERT_R1(obj && *obj == 0);
    
    pigeon_object_pool_free(&pool, obj);
    ASSERT_R1(pool.allocated_obj_count == 0);

    for(unsigned int i = 0; i < 100000; i++) {
        uint8_t * x = pigeon_object_pool_allocate(&pool);
        ASSERT_R1(!*x);
    }
    ASSERT_R1(pool.allocated_obj_count == 100000);

    pigeon_destroy_object_pool(&pool);


    return 0;
}

int main(void)
{
	ASSERT_R1(!pigeon_test_config_parser());
	ASSERT_R1(!pigeon_test_array_list());
	ASSERT_R1(!pigeon_test_object_pool());
    return 0;
}