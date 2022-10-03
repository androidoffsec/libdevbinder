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

#include <sys/types.h>
#include <linux/android/binder.h>
#include <string.h>
#include <unistd.h>

#include "binder.h"
#include "util.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static void process_buf(binder_ctx *ctx, buf_t *in_b);

int main() {
  int ret;
  binder_ctx *ctx;
  buf_t *read_buf;

  read_buf = buf_alloc();
  if (!read_buf) {
    return 1;
  }

  ctx = binder_open("/dev/binder");
  if (!ctx) {
    return 1;
  }

  ret = binder_set_context_manager(ctx);
  if (ret < 0) {
    return 1;
  }

  binder_enter_looper(ctx);

  LOG("Listening...");
  do {
    buf_init_read(read_buf);

    ret = binder_recv(ctx, read_buf);
    if (ret < 0)
      break;

    process_buf(ctx, read_buf);
  } while (1);

  buf_free(read_buf);
  binder_close(ctx);
  return 0;
}

static void process_buf(binder_ctx *ctx, buf_t *in_b) {
  uint32_t cmd;

  while (!buf_is_empty(in_b)) {
    cmd = buf_read_u32(in_b);
    switch (cmd) {
      case BR_ERROR: {
        buf_pop(in_b, sizeof(int));  // int error
        LOG("BR_ERROR");
      } break;
      case BR_OK:
        LOG("BR_OK");
        break;
      case BR_TRANSACTION:
      case BR_REPLY: {
        char out_buffer[256] = {0};
        struct binder_transaction_data txn;

        char *cmd_str = cmd == BR_TRANSACTION ? "BR_TRANSACTION" : "BR_REPLY";

        buf_read_transaction_data(in_b, &txn);

        if (txn.flags & TF_ONE_WAY) {
          LOG("%s (TF_ONE_WAY)", cmd_str);
        } else {
          LOG("%s ", cmd_str);
        }

        memcpy(out_buffer, (void *)txn.data.ptr.buffer,
               MIN(txn.data_size, sizeof(out_buffer)));
        LOG("\t%s", out_buffer);

        binder_free_buffer(ctx, (binder_uintptr_t)txn.data.ptr.buffer);

        break;
      }
      case BR_DEAD_REPLY:
        LOG("BR_DEAD_REPLY\n");
        break;
      case BR_TRANSACTION_COMPLETE:
        LOG("BR_TRANSACTION_COMPLETE\n");
        break;
      case BR_INCREFS: {
        buf_pop(in_b, sizeof(void *));  // void *ptr
        buf_pop(in_b, sizeof(void *));  // void *cookie
        LOG("BR_INCREFS");
      } break;
      case BR_ACQUIRE: {
        buf_pop(in_b, sizeof(void *));  // void *ptr
        buf_pop(in_b, sizeof(void *));  // void *cookie
        LOG("BR_ACQUIRE");
      } break;
      case BR_RELEASE: {
        buf_pop(in_b, sizeof(void *));  // void *ptr
        buf_pop(in_b, sizeof(void *));  // void *cookie
        LOG("BR_RELEASE");
      } break;
      case BR_DECREFS: {
        buf_pop(in_b, sizeof(void *));  // void *ptr
        buf_pop(in_b, sizeof(void *));  // void *cookie
        LOG("BR_DECREFS");
      } break;
      case BR_NOOP:
        break;
      case BR_SPAWN_LOOPER:
        LOG("BR_SPAWN_LOOPER");
        break;
      case BR_DEAD_BINDER: {
        buf_pop(in_b, sizeof(void *));  // void *cookie
        LOG("BR_DEAD_BINDER");
      } break;
      case BR_CLEAR_DEATH_NOTIFICATION_DONE: {
        buf_pop(in_b, sizeof(void *));  // void *cookie
        LOG("BR_CLEAR_DEATH_NOTIFICATION_DONE");
      } break;
      case BR_FAILED_REPLY:
        LOG("BR_FAILED_REPLY\n");
        break;
      default:
        LOG("WARN: Unknown returned command\n");
        break;
    }
  }
}
