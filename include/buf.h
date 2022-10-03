/*
 * Copyright 2024 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef BUF_H
#define BUF_H

#include <sys/types.h>
#include <linux/android/binder.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

/**
 * A buffer used for Binder write/read operations.
 *
 * @buffer: The buffer.
 * @ptr: Current position for writing or reading within the buffer.
 * @size: The number of bytes currently used in the buffer.
 * @max_size: The maximum capacity of the buffer.
 */
typedef struct {
  unsigned char buffer[0x200];
  unsigned char *ptr;
  size_t size;
  size_t max_size;
} buf_t;

#ifdef __cplusplus
extern "C" {
#endif

buf_t *buf_alloc();
void buf_free(buf_t *b);
void buf_init_write(buf_t *b);
void buf_init_read(buf_t *b);

void *buf_push(buf_t *b, size_t size);
void *buf_pop(buf_t *b, size_t size);
int buf_is_empty(buf_t *b);

uint32_t buf_read_u32(buf_t *b);
void buf_read(buf_t *b, void *out_data, size_t size);
void buf_read_transaction_data(buf_t *b, struct binder_transaction_data *tr);

void buf_write_u32(buf_t *b, uint32_t value);
void buf_write_uintptr(buf_t *b, binder_uintptr_t value);
void buf_write(buf_t *b, void *in_data, size_t size);

#ifdef __cplusplus
}
#endif

#endif  // BUF_H_
