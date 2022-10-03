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

#include "buf.h"

#include <stdlib.h>
#include <string.h>

#include "util.h"

void buf_init(buf_t *b) {
  memset(b->buffer, 0, sizeof(b->buffer));
  b->ptr = b->buffer;
  b->size = 0;
  b->max_size = sizeof(b->buffer);
}

void buf_init_write(buf_t *b) {
  buf_init(b);
  b->size = 0;
}

void buf_init_read(buf_t *b) {
  buf_init(b);
  b->size = sizeof(b->buffer);
}

buf_t *buf_alloc() {
  buf_t *b = malloc(sizeof(buf_t));
  return b;
}

void buf_free(buf_t *b) { free(b); }

void *buf_push(buf_t *b, size_t size) {
  void *ptr = b->ptr;

  if (b->max_size - b->size < size) {
    ERR("Buffer is out of capacity");
    return NULL;
  }

  b->ptr += size;
  b->size += size;

  return ptr;
}

void *buf_pop(buf_t *b, size_t size) {
  void *ptr = b->ptr;

  if (b->buffer + b->size < b->ptr + size) {
    ERR("Buffer is out of capacity");
    return NULL;
  }

  b->ptr += size;

  return ptr;
}

int buf_is_empty(buf_t *b) {
  if (b->buffer + b->size == b->ptr)
    return 1;
  return 0;
}

void buf_write_u32(buf_t *b, uint32_t value) {
  uint32_t *ptr = buf_push(b, sizeof(uint32_t));
  *ptr = value;
}

void buf_write_uintptr(buf_t *b, binder_uintptr_t value) {
  binder_uintptr_t *ptr = buf_push(b, sizeof(binder_uintptr_t));
  *ptr = value;
}

void buf_write(buf_t *b, void *in_data, size_t size) {
  void *ptr = buf_push(b, size);
  memcpy(ptr, in_data, size);
}

uint32_t buf_read_u32(buf_t *b) {
  uint32_t *ptr = buf_pop(b, sizeof(uint32_t));
  return *ptr;
}

void buf_read(buf_t *b, void *out_data, size_t size) {
  void *ptr = buf_pop(b, size);
  memcpy(out_data, ptr, size);
}

void buf_read_transaction_data(buf_t *b, struct binder_transaction_data *tr) {
  buf_read(b, tr, sizeof(*tr));
}
