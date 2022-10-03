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

#include "transaction.h"

#include <string.h>

#define PAD_SIZE_UNSAFE(s) (((s) + 3) & ~3UL)

void trdata_init(translation_data_t *trdata) {
  trdata->data_ptr = trdata->data;
  trdata->data_avail = sizeof(trdata->data);
  trdata->offs_ptr = trdata->offs;
  trdata->offs_avail = sizeof(trdata->offs) / sizeof(binder_size_t);
  trdata->buffers_size = 0;
}

void *trdata_alloc(translation_data_t *trdata, size_t size, bool obj) {
  void *ptr;

  size = PAD_SIZE_UNSAFE(size);
  if (size > trdata->data_avail)
    return NULL;

  ptr = trdata->data_ptr;
  trdata->data_ptr += size;
  trdata->data_avail -= size;

  if (obj) {
    *trdata->offs_ptr++ = ((uint8_t *)ptr) - ((uint8_t *)trdata->data);
    trdata->offs_avail--;
  }

  return ptr;
}

struct flat_binder_object *trdata_alloc_fbo(translation_data_t *trdata) {
  return trdata_alloc(trdata, sizeof(struct flat_binder_object), true);
}

struct binder_buffer_object *trdata_alloc_bbo(translation_data_t *trdata,
                                              binder_size_t length) {
  struct binder_buffer_object *bbo =
      trdata_alloc(trdata, sizeof(struct binder_buffer_object), true);

  if (!bbo)
    return NULL;

  trdata->buffers_size += length;

  return bbo;
}

void trdata_put_u32(translation_data_t *trdata, uint32_t n) {
  uint32_t *ptr = trdata_alloc(trdata, sizeof(n), false);
  if (ptr)
    *ptr = n;
}

void trdata_put_bytes(translation_data_t *trdata, const char *data,
                      size_t len) {
  uint8_t *ptr;

  ptr = trdata_alloc(trdata, len, false);
  if (!ptr)
    return;

  memcpy(ptr, data, len);
}

void trdata_put_str(translation_data_t *trdata, const char *str) {
  size_t i, len;
  uint8_t *ptr;

  len = strlen(str);
  ptr = trdata_alloc(trdata, len + 1, false);
  if (!ptr)
    return;

  for (i = 0; i < len; i++) {
    ptr[i] = str[i];
  }
  ptr[len] = '\0';
}

void trdata_put_str16(translation_data_t *trdata, const char *str) {
  size_t i, len;

  len = strlen(str);
  trdata_put_u32(trdata, len);
  uint16_t *ptr = trdata_alloc(trdata, (len + 1) * 2, false);
  if (!ptr)
    return;

  for (i = 0; i < len; i++) {
    ptr[i] = str[i];
  }
  ptr[len] = '\0';
}

void trdata_put_buffer(translation_data_t *trdata, binder_uintptr_t buffer,
                       binder_size_t length, binder_size_t parent,
                       binder_size_t parent_offset, bool has_parent) {
  struct binder_buffer_object *bbo;

  bbo = trdata_alloc_bbo(trdata, length);
  if (!bbo)
    return;

  bbo->hdr.type = BINDER_TYPE_PTR;
  bbo->flags = has_parent ? BINDER_BUFFER_FLAG_HAS_PARENT : 0;
  bbo->buffer = buffer;
  bbo->length = length;
  bbo->parent = parent;
  bbo->parent_offset = parent_offset;
}

void trdata_put_binder(translation_data_t *trdata, binder_uintptr_t ptr,
                       bool strong) {
  struct flat_binder_object *fbo;

  fbo = trdata_alloc_fbo(trdata);
  if (!fbo)
    return;

  fbo->hdr.type = strong ? BINDER_TYPE_BINDER : BINDER_TYPE_WEAK_BINDER;
  fbo->flags = 0;
  fbo->binder = ptr;
  fbo->cookie = 0;
}

void trdata_put_handle(translation_data_t *trdata, uint32_t handle,
                       bool strong) {
  struct flat_binder_object *fbo;

  fbo = trdata_alloc_fbo(trdata);
  if (!fbo)
    return;

  fbo->hdr.type = strong ? BINDER_TYPE_HANDLE : BINDER_TYPE_WEAK_HANDLE;
  fbo->flags = 0;
  fbo->handle = handle;
  fbo->cookie = 0;
}

void txnin_init(translated_data_t *txnin, struct binder_transaction_data *tr) {
  txnin->data = (uint8_t *)tr->data.ptr.buffer;
  txnin->data_ptr = txnin->data;
  txnin->data_avail = tr->data_size;
  txnin->code = tr->code;
  txnin->target = tr->target.ptr;
  txnin->cookie = tr->cookie;
}

void *txnin_pop(translated_data_t *txnin, size_t size) {
  void *ptr;

  size = PAD_SIZE_UNSAFE(size);
  if (size > txnin->data_avail)
    return NULL;

  ptr = txnin->data_ptr;
  txnin->data_ptr += size;
  txnin->data_avail -= size;

  return ptr;
}

uint32_t txnin_pop_u32(translated_data_t *txnin) {
  return *(uint32_t *)txnin_pop(txnin, sizeof(uint32_t));
}

int32_t txnin_pop_i32(translated_data_t *txnin) {
  return *(int32_t *)txnin_pop(txnin, sizeof(int32_t));
}

uint32_t txnin_pop_handle(translated_data_t *txnin) {
  struct flat_binder_object *fbo;

  fbo = txnin_pop(txnin, sizeof(*fbo));
  if (!fbo)
    return 0;

  return fbo->handle;
}

void *txnin_pop_buffer(translated_data_t *txnin) {
  struct binder_buffer_object *bbo;

  bbo = txnin_pop(txnin, sizeof(*bbo));
  if (!bbo)
    return NULL;

  return (void *)bbo->buffer;
}
