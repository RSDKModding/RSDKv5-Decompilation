#include <switch.h>

/**
 * @brief Initialize address space manager
 *
 * This should be called early in crt0 and should not be called by
 * any applications or utility functions before using address space
 * management functions.
 */
Result as_init();

/**
 * @brief Finalize address space manager
 *
 * Called by crt0 and should not be called by applications or
 * utility functions.
 */
void as_finalize();

/**
 * @brief Finds and reserves an unmapped region of address space
 *
 * @param len The length of address space to reserve
 */
void *as_reserve(size_t len);

/**
 * @brief Frees a region of address space reserved by \ref as_reserve
 *
 * The address and size must exactly match an entire memory region
 * reserved by \ref as_reserve.
 *
 * @param addr Base of the reserved address space
 * @param len Length of the reserved address space
 */
void as_release(void *addr, size_t len);
