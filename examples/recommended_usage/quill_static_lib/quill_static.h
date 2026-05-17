#pragma once

/**
 * Minimal wrapper-library API used by the recommended-usage examples.
 *
 * This example keeps the API intentionally small. Real projects can expose multiple setup
 * functions, logger getters, or richer logging/bootstrap objects instead of globals.
 */
void setup_quill(char const* log_file);
