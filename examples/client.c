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

int main(int argc, char **argv) {
  int ret;
  binder_ctx *ctx;
  buf_t *read_buf;

  if (argc != 2) {
    LOG("Usage: %s message", argv[0]);
    return 0;
  }

  read_buf = buf_alloc();
  if (!read_buf) {
    return 1;
  }

  ctx = binder_open("/dev/binder");
  if (!ctx) {
    return 1;
  }

  binder_enter_looper(ctx);

  ret = binder_send_raw_txn(ctx, 0, 0, TF_ONE_WAY, argv[1],
                      strlen(argv[1]), false, false);
  if (ret < 0) {
    ERR("Failed to send a transaction %d", ret);
    return 1;
  }

  // Read `BR_TRANSACTION_COMPLETE` from Binder to prevent undelivered error
  buf_init_read(read_buf);
  binder_recv(ctx, read_buf);

  buf_free(read_buf);
  binder_close(ctx);
  return 0;
}
