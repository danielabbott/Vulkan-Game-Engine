#include <glad/glad.h>
#include <pigeon/assert.h>
#include <pigeon/misc.h>
#include <pigeon/wgi/opengl/timer_query.h>
#include <stdlib.h>

PIGEON_ERR_RET pigeon_opengl_create_timer_query_group(PigeonOpenGLTimerQueryGroup* g, unsigned int n)
{
	ASSERT_R1(g && n);

	g->n = n;
	g->ids = calloc(4, n);
	ASSERT_R1(g->ids);

	glGenQueries(n, g->ids);

	bool fail = false;
	for (unsigned int i = 0; i < n; i++) {
		if (!g->ids[i]) {
			fail = true;
			break;
		}
	}

	if (fail) {
		glDeleteQueries(n, g->ids);
		free(g->ids);
		ASSERT_R1(false);
	}

	return 0;
}

void pigeon_opengl_set_timer_query_value(PigeonOpenGLTimerQueryGroup* g, unsigned int i)
{
	assert(g && g->ids && i < g->n);
	glQueryCounter(g->ids[i], GL_TIMESTAMP);
}

double pigeon_opengl_get_timer_query_result(PigeonOpenGLTimerQueryGroup* g, unsigned int i)
{
	assert(g && g->ids && i < g->n);

	uint64_t x = 0;
	glGetQueryObjectui64v(g->ids[i], GL_QUERY_RESULT, &x);
	return (double)x / 1000000.0;
}

void pigeon_opengl_get_timer_query_results(PigeonOpenGLTimerQueryGroup* g, double* times)
{
	assert(g && g->ids);

	for (unsigned int i = 0; i < g->n; i++) {
		uint64_t x = 0;
		glGetQueryObjectui64v(g->ids[i], GL_QUERY_RESULT, &x);
		times[i] = (double)x / 1000000.0;
	}
}

void pigeon_opengl_destroy_timer_query_group(PigeonOpenGLTimerQueryGroup* g)
{
	assert(g);

	if (g->ids) {
		glDeleteQueries(g->n, g->ids);
		free2(g->ids);
	}
}
