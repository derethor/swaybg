#include <assert.h>
#include <stdlib.h>
#include <wayland-client.h>
#include "state.h"
#include "swaybg.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"

void render_frame(struct swaybg_output *output)
{
	int buffer_width = output->width * output->scale,
			buffer_height = output->height * output->scale;
	output->current_buffer = get_next_buffer(output->state->shm,
																					 output->buffers, buffer_width, buffer_height);
	if (!output->current_buffer)
	{
		return;
	}
	cairo_t *cairo = output->current_buffer->cairo;
	cairo_save(cairo);
	cairo_set_operator(cairo, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cairo);
	cairo_restore(cairo);
	if (output->config->mode == BACKGROUND_MODE_SOLID_COLOR)
	{
		cairo_set_source_u32(cairo, output->config->color);
		cairo_paint(cairo);
	}
	else
	{
		if (output->config->color)
		{
			cairo_set_source_u32(cairo, output->config->color);
			cairo_paint(cairo);
		}
		render_background_image(cairo, output->config->image,
														output->config->mode, buffer_width, buffer_height);
	}

	wl_surface_set_buffer_scale(output->surface, output->scale);
	wl_surface_attach(output->surface, output->current_buffer->buffer, 0, 0);
	wl_surface_damage_buffer(output->surface, 0, 0, INT32_MAX, INT32_MAX);
	wl_surface_commit(output->surface);
}

void destroy_swaybg_output(struct swaybg_output *output)
{
	if (!output)
		return;

	wl_list_remove(&output->link);
	if (output->layer_surface != NULL)
	{
		zwlr_layer_surface_v1_destroy(output->layer_surface);
	}
	if (output->surface != NULL)
	{
		wl_surface_destroy(output->surface);
	}
	zxdg_output_v1_destroy(output->xdg_output);
	wl_output_destroy(output->wl_output);
	destroy_buffer(&output->buffers[0]);
	destroy_buffer(&output->buffers[1]);
	free(output->name);
	free(output->identifier);
	free(output);
}

void destroy_all_swaybg_output(struct swaybg_state *state)
{
	assert(state != NULL);

	struct swaybg_output *output;
	struct swaybg_output *tmp_output;

	wl_list_for_each_safe(output, tmp_output, &(state->outputs), link)
	{
		destroy_swaybg_output(output);
	}
}
