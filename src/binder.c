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

#include "binder.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/android/binder.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include "buf.h"
#include "util.h"

binder_ctx *binder_open(const char *device) {
  if (!device)
    return NULL;

  binder_ctx *ctx = malloc(sizeof(*ctx));
  if (!ctx)
    return NULL;

  ctx->fd = open(device, O_RDWR, 0);
  if (ctx->fd == -1) {
    ERR("Failed to open binder device: %s", device);
    goto err_open;
  }

  ctx->map_size = BINDER_VM_SIZE;
  ctx->map_ptr = mmap(NULL, BINDER_VM_SIZE, PROT_READ, MAP_PRIVATE, ctx->fd, 0);
  if (ctx->map_ptr == MAP_FAILED) {
    ERR("Failed to mmap binder device");
    goto err_mmap;
  }

  return ctx;
err_mmap:
  close(ctx->fd);
err_open:
  free(ctx);
  return NULL;
}

void binder_close(binder_ctx *ctx) {
  if (ctx) {
    munmap(ctx->map_ptr, ctx->map_size);
    close(ctx->fd);
    free(ctx);
  }
}

int binder_check_version(binder_ctx *ctx) {
  int ret;
  struct binder_version version = {0};

  ret = ioctl(ctx->fd, BINDER_VERSION, &version);
  if (ret < 0)
    return ret;

  if (version.protocol_version != BINDER_CURRENT_PROTOCOL_VERSION) {
    ERR("Binder version mismatched: %u", version.protocol_version);
    return -1;
  }

  return 0;
}

int binder_set_context_manager(binder_ctx *ctx) {
  int ret;

  ret = ioctl(ctx->fd, BINDER_SET_CONTEXT_MGR, 0);
  if (ret < 0)
    ERR("BINDER_SET_CONTEXT_MGR ioctl failed: %d", errno);

  return ret;
}

int binder_thread_exit(binder_ctx *ctx) {
  int ret;

  ret = ioctl(ctx->fd, BINDER_THREAD_EXIT, 0);
  if (ret < 0)
    ERR("BINDER_THREAD_EXIT ioctl failed: %d", errno);

  return ret;
}

int binder_send(binder_ctx *ctx, buf_t *b) {
  int ret;

  struct binder_write_read bwr = {.write_size = b->size,
                                  .write_buffer = (binder_uintptr_t)b->buffer,
                                  .write_consumed = 0};

  ret = ioctl(ctx->fd, BINDER_WRITE_READ, &bwr);

  if (ret < 0) {
    ERR("BINDER_WRITE_READ ioctl failed: %d", errno);
    return ret;
  }

  return bwr.write_consumed;
}

int binder_recv(binder_ctx *ctx, buf_t *b) {
  int ret;

  struct binder_write_read bwr = {.read_size = b->size,
                                  .read_buffer = (binder_uintptr_t)b->buffer,
                                  .read_consumed = 0};

  ret = ioctl(ctx->fd, BINDER_WRITE_READ, &bwr);
  if (ret < 0) {
    ERR("BINDER_WRITE_READ ioctl failed: %d", errno);
    return ret;
  }

  b->size = bwr.read_consumed;
  return bwr.read_consumed;
}

int binder_send_cmd(binder_ctx *ctx, uint32_t cmd, uint8_t *data,
                    size_t data_size) {
  int ret;

  buf_t *b = buf_alloc();
  if (!b)
    return -1;
  buf_init_write(b);

  buf_write_u32(b, cmd);
  if (data && data_size > 0) {
    buf_write(b, data, data_size);
  }
  ret = binder_send(ctx, b);

  buf_free(b);
  if (ret < 0) {
    ERR("BINDER_WRITE_READ ioctl failed: %d", errno);
    return ret;
  }
  return 0;
}

int binder_enter_looper(binder_ctx *ctx) {
  return binder_send_cmd(ctx, BC_ENTER_LOOPER, NULL, 0);
}

int binder_free_buffer(binder_ctx *ctx, binder_uintptr_t ptr) {
  return binder_send_cmd(ctx, BC_FREE_BUFFER, (uint8_t *)&ptr, sizeof(ptr));
}

int binder_handle_acquire(binder_ctx *ctx, int32_t handle) {
  return binder_send_cmd(ctx, BC_ACQUIRE, (uint8_t *)&handle, sizeof(handle));
}

int binder_handle_release(binder_ctx *ctx, int32_t handle) {
  return binder_send_cmd(ctx, BC_RELEASE, (uint8_t *)&handle, sizeof(handle));
}

int binder_send_txn(binder_ctx *ctx, int32_t handle, uint32_t code,
                    uint32_t flags, const translation_data_t *trdata,
                    bool reply, bool sg) {
  uint32_t cmd;
  struct binder_transaction_data tr = {0};
  struct binder_transaction_data_sg tr_sg = {0};

  tr.target.handle = handle;
  tr.code = code;
  tr.flags = flags;

  tr.data_size = trdata->data_ptr - trdata->data;
  tr.data.ptr.buffer = (binder_uintptr_t)trdata->data;

  tr.offsets_size = (trdata->offs_ptr - trdata->offs) * sizeof(binder_size_t);
  tr.data.ptr.offsets = (binder_uintptr_t)trdata->offs;

  if (sg) {
    tr_sg.transaction_data = tr;
    tr_sg.buffers_size = trdata->buffers_size;
    cmd = reply ? BC_REPLY_SG : BC_TRANSACTION_SG;
    return binder_send_cmd(ctx, cmd, (uint8_t *)&tr_sg, sizeof(tr_sg));
  } else {
    cmd = reply ? BC_REPLY : BC_TRANSACTION;
    return binder_send_cmd(ctx, cmd, (uint8_t *)&tr, sizeof(tr));
  }
}

int binder_send_raw_txn(binder_ctx *ctx, int32_t handle, uint32_t code,
                        uint32_t flags, void *data, size_t data_size,
                        bool reply, bool sg) {
  translation_data_t trdata = {0};
  trdata_init(&trdata);
  trdata_put_bytes(&trdata, data, data_size);

  return binder_send_txn(ctx, handle, code, flags, &trdata, reply, sg);
}

static int binder_skip_cmds(binder_ctx *ctx, buf_t *buf,
                            translated_data_t *txnin) {
  uint32_t cmd;
  uint8_t cmd_data[2048];

  while (buf->ptr != (buf->buffer + buf->size)) {
    cmd = buf_read_u32(buf);
    buf_read(buf, cmd_data, _IOC_SIZE(cmd));
    switch (cmd) {
      case BR_ACQUIRE:
      case BR_INCREFS: {
        buf_t buf;
        struct binder_ptr_cookie *bpc = (struct binder_ptr_cookie *)cmd_data;
        buf_init_write(&buf);
        buf_write_u32(&buf,
                      cmd == BR_ACQUIRE ? BC_ACQUIRE_DONE : BC_INCREFS_DONE);
        buf_write_uintptr(&buf, bpc->ptr);
        buf_write_uintptr(&buf, bpc->cookie);
        binder_send(ctx, &buf);
      } break;
      case BR_DEAD_BINDER: {
        buf_t buf;
        binder_uintptr_t cookie = *(binder_uintptr_t *)cmd_data;
        buf_init_write(&buf);
        buf_write_u32(&buf, BC_DEAD_BINDER_DONE);
        buf_write_uintptr(&buf, cookie);
        binder_send(ctx, &buf);
      } break;
      case BR_TRANSACTION:
      case BR_REPLY:
        txnin_init(txnin, (struct binder_transaction_data *)cmd_data);
        return 1;
        break;
      default:
        break;
    }
  }
  return 0;
}

int binder_recv_txn(binder_ctx *ctx, translated_data_t *txnin) {
  buf_t buf;
  binder_size_t read_consumed;

  while (1) {
    buf_init_read(&buf);
    read_consumed = binder_recv(ctx, &buf);
    if (read_consumed < 0)
      return read_consumed;

    if (binder_skip_cmds(ctx, &buf, txnin))
      break;
  }

  return 0;
}
