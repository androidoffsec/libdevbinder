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

#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <sys/types.h>
#include <linux/android/binder.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint8_t data[0x10000];
  uint8_t *data_ptr;
  size_t data_avail;
  binder_size_t offs[0x1000];
  binder_size_t *offs_ptr;
  size_t offs_avail;
  size_t buffers_size;
} translation_data_t;

typedef struct {
  uint8_t *data;
  uint8_t *data_ptr;
  size_t data_avail;

  binder_uintptr_t target;
  binder_uintptr_t cookie;
  uint32_t code;
} translated_data_t;

#ifdef __cplusplus
extern "C" {
#endif

void trdata_init(translation_data_t *trdata);
void *trdata_alloc(translation_data_t *trdata, size_t size, bool obj);
struct flat_binder_object *trdata_alloc_fbo(translation_data_t *trdata);
struct binder_buffer_object *trdata_alloc_bbo(translation_data_t *trdata,
                                              binder_size_t length);
void trdata_put_u32(translation_data_t *trdata, uint32_t n);
void trdata_put_bytes(translation_data_t *trdata, const char *data, size_t len);
void trdata_put_str(translation_data_t *trdata, const char *str);
void trdata_put_str16(translation_data_t *trdata, const char *str);
void trdata_put_buffer(translation_data_t *trdata, binder_uintptr_t buffer,
                       binder_size_t length, binder_size_t parent,
                       binder_size_t parent_offset, bool has_parent);
void trdata_put_binder(translation_data_t *trdata, binder_uintptr_t ptr,
                       bool strong);
void trdata_put_handle(translation_data_t *trdata, uint32_t handle,
                       bool strong);

void txnin_init(translated_data_t *txnin, struct binder_transaction_data *tr);
void *txnin_pop(translated_data_t *txnin, size_t size);
uint32_t txnin_pop_u32(translated_data_t *txnin);
int32_t txnin_pop_i32(translated_data_t *txnin);
uint32_t txnin_pop_handle(translated_data_t *txnin);
void *txnin_pop_buffer(translated_data_t *txnin);

#ifdef __cplusplus
}
#endif

#endif  // TRANSACTION_H
