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

#ifndef BINDER_H
#define BINDER_H

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "buf.h"
#include "transaction.h"

#define BINDER_VM_SIZE 1 * 1024 * 1024

/**
 * Represents a Binder context.
 *
 * @fd: The file descriptor associated with the opened Binder device.
 * @map_ptr: A pointer to the memory-mapped region used for Binder.
 * @map_size: The size of the memory-mapped region in bytes.
 */
typedef struct {
  int fd;
  void *map_ptr;
  size_t map_size;
} binder_ctx;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Binder open and close operations
 */

/**
 * Opens a Binder device and returns a context for further operations.
 *
 * @param driver The name of the Binder driver (e.g., "/dev/binder").
 * @return A pointer to a `binder_ctx` structure on success, or NULL on failure.
 */
binder_ctx *binder_open(const char *driver);

/**
 * Closes a Binder device and frees the associated context.
 *
 * @param ctx A pointer to the `binder_ctx` structure.
 */
void binder_close(binder_ctx *ctx);

/*
 * IOCTL operations
 */

/**
 * Checks the version of the Binder driver. (BINDER_VERSION)
 *
 * @param ctx A pointer to the `binder_ctx` structure.
 * @return 0 on success, or a negative error code on failure.
 */
int binder_check_version(binder_ctx *ctx);

/**
 * Sets the context manager. (BINDER_SET_CONTEXT_MGR)
 *
 * @param ctx A pointer to the `binder_ctx` structure.
 * @return 0 on success, or a negative error code on failure.
 */
int binder_set_context_manager(binder_ctx *ctx);

/**
 * Signals the Binder thread to exit. (BINDER_THREAD_EXIT)
 *
 * @param ctx A pointer to the `binder_ctx` structure.
 * @return 0 on success, or a negative error code on failure.
 */
int binder_thread_exit(binder_ctx *ctx);

/*
 * IOCTL BINDER_WRITE_READ operations
 */

/**
 * Sends raw data in the `write_buf`.
 *
 * @param ctx A pointer to the `binder_ctx` structure.
 * @param b A pointer to a `buf_t` structure containing the data to be sent.
 * @return 0 on success, or a negative error code on failure.
 */
int binder_send(binder_ctx *ctx, buf_t *b);

/**
 * Sends a command and raw data in the `write_buf`.
 *
 * @param ctx A pointer to the `binder_ctx` structure.
 * @param cmd The command code.
 * @param data A pointer to the data to be sent after the command.
 * @param data_size The size of the data in bytes.
 * @return 0 on success, or a negative error code on failure.
 */
int binder_send_cmd(binder_ctx *ctx, uint32_t cmd, uint8_t *data,
                    size_t data_size);

/**
 * Sends a `BC_TRANSACTION`/`BC_TRANSACTION_SG` or `BC_REPLY`/`BC_REPLY_SG`
 * command along with the transaction data in the `write_buf`.
 *
 * @param ctx A pointer to the `binder_ctx` structure.
 * @param handle The handle of the target recipient.
 * @param code The transaction code.
 * @param flags The transaction flags.
 * @param trdata A pointer to transaction data.
 * @param reply Whether it is a BC_TRANSACTION* or `BC_REPLY*` transaction.
 * @param sg Whether it is a `BC_*` or `BC_*_SG` transaction.
 * @return 0 on success, or a negative error code on failure.
 */
int binder_send_txn(binder_ctx *ctx, int32_t handle, uint32_t code,
                    uint32_t flags, const translation_data_t *trdata,
                    bool reply, bool sg);

/**
 * Sends a raw transaction to a Binder service.
 *
 * @param ctx A pointer to the `binder_ctx` structure.
 * @param handle The handle of the target recipient.
 * @param code The transaction code.
 * @param flags The transaction flags.
 * @param data A pointer to the raw transaction data.
 * @param data_size The size of the raw transaction data in bytes.
 * @param reply Whether it is a BC_TRANSACTION* or `BC_REPLY*` transaction.
 * @param sg Whether it is a `BC_*` or `BC_*_SG` transaction.
 * @return 0 on success, or a negative error code on failure.
 */
int binder_send_raw_txn(binder_ctx *ctx, int32_t handle, uint32_t code,
                        uint32_t flags, void *data, size_t data_size,
                        bool reply, bool sg);

/**
 * Reads raw data from the `read_buf`.
 *
 * @param ctx A pointer to the `binder_ctx` structure.
 * @param b A pointer to a `buf_t` structure to store the received data.
 * @return 0 on success, or a negative error code on failure.
 */
int binder_recv(binder_ctx *ctx, buf_t *b);

/**
 * Reads a `BC_TRANSACTION`/`BC_TRANSACTION_SG` or `BC_REPLY`/`BC_REPLY_SG`
 * transaction, skipping other received commands.
 *
 * @param ctx A pointer to the `binder_ctx` structure.
 * @param txnin A pointer to a `translated_data_t` structure to store the
 *              received transaction data.
 * @return 0 on success, or a negative error code on failure.
 */
int binder_recv_txn(binder_ctx *ctx, translated_data_t *txnin);

/**
 * Send the `BC_ENTER_LOOPER` command.
 *
 * @param ctx A pointer to the `binder_ctx` structure.
 * @return 0 on success, or a negative error code on failure.
 */
int binder_enter_looper(binder_ctx *ctx);

/**
 * Sends the `BC_ACQUIRE` command to acquire a handle
 *
 * @param ctx A pointer to the `binder_ctx` structure.
 * @param handle The handle to acquire.
 * @return 0 on success, or a negative error code on failure.
 */
int binder_handle_acquire(binder_ctx *ctx, int32_t handle);

/**
 * Sends the `BC_RELEASE` command to release a handle
 *
 * @param ctx A pointer to the `binder_ctx` structure.
 * @param handle The handle to release.
 * @return 0 on success, or a negative error code on failure.
 */
int binder_handle_release(binder_ctx *ctx, int32_t handle);

/**
 * Sends the `BC_FREE_BUFFER` to free a transaction buffer.
 *
 * @param ctx A pointer to the `binder_ctx` structure representing the Binder
 *            context.
 * @param ptr The pointer to the transaction buffer in the memory-mapped region
 *            to be freed.
 * @return 0 on success, or a negative error code on failure.
 */
int binder_free_buffer(binder_ctx *ctx, binder_uintptr_t ptr);

#ifdef __cplusplus
}
#endif

#endif  // BINDER_H_
