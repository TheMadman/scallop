#ifndef SCALLOP_UTIL_H
#define SCALLOP_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <csalt/stores.h>

int receive_split_for_set_(csalt_store *store, void *value)
{
	struct {
		void *buffer;
		ssize_t element_size;
	} *params = value;

	ssize_t write_result = csalt_store_write(
		store,
		params->buffer,
		params->element_size
	);

	return !(write_result < params->element_size);
}

/**
 * This function assumes the store contains an array
 * of fixed-length elements, of size `element_size`,
 * and attempts an insert into the array at the
 * `index`th position.
 *
 * It returns 1 if the write succeeded and 0 if the
 * write failed.
 */
char store_set_element(
	csalt_store *store,
	void *buffer,
	ssize_t index,
	ssize_t element_size
)
{
	/* candidate for moving into ceasoning? */
	struct {
		void *buffer;
		ssize_t element_size;
	} params = { buffer, element_size };
	ssize_t
		begin_byte = index * element_size,
		end_byte = (index + 1) * element_size;

	return (char)csalt_store_split(
		store,
		begin_byte,
		end_byte,
		receive_split_for_set_,
		&params
	);
}

int receive_split_for_get_(csalt_store *store, void *value)
{
	struct {
		void *buffer;
		ssize_t element_size;
	} *params = value;

	ssize_t read_result = csalt_store_read(
		store,
		params->buffer,
		params->element_size
	);

	return !(read_result < params->element_size);
}

/**
 * This function assumes the store contains an array of
 * fixed-length elements, of size `element_size`, and
 * attempts a read from the array at the `index`th position.
 *
 * It returns 1 if the read succeeded and 0 if the read failed.
 */
char store_get_element(
	csalt_store *store,
	void *buffer,
	ssize_t index,
	ssize_t element_size
)
{
	/* candidate for moving into ceasoning? */
	struct {
		void *buffer;
		ssize_t element_size;
	} params = { buffer, element_size };

	ssize_t
		begin_byte = index * element_size,
		end_byte = (index + 1) * element_size;

	return (char)csalt_store_split(
		store,
		begin_byte,
		end_byte,
		receive_split_for_get_,
		&params
	);
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SCALLOP_UTIL_H
