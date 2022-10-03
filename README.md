# libdevbinder

`libdevbinder` is a C library that simplifies interaction with binder (Android
IPC subsystem). It abstracts the complexities of ioctl calls into a familiar
send/receive socket-like interface, making it easier to develop programs that
perform IPC via Binder. It supports both Linux and Android platforms on x86_64
and arm64 architectures.

This is not an officially supported Google product

## Clone and Build

Clone the repository

```bash
git clone https://github.com/androidoffsec/libdevbinder.git
```

Run `make` to build `libdevbinder.a` and other examples.

```bash
make
```

The default target architecture is x86_64. To cross compile for arm64:

```bash
ARCH=arm64 make
```

To compile for Android using Android NDK:

```bash
export NDK_ROOT=/path/to/android-ndk
ANDROID=1 make
```

### Android Studio Project

To include `libdevbinder` as a dependency in your Android application, follow
these steps:

1. Create a new Native C++ project
1. Clone the repository under `src/main/cpp`.

```bash
cd /path/to/app/src/main/cpp
git clone https://github.com/androidoffsec/libdevbinder.git
```

In `src/main/cpp/CMakeLists.txt`,

3. Include `libdevbinder` as a submodule.

```cmake
add_subdirectory(libdevbinder)
```

4. Link the target object with `devbinder` or `devbinder_static`.

```cmake
# Link the shared library
target_link_libraries(${CMAKE_PROJECT_NAME} ... devbinder)

# Link the static library
target_link_libraries(${CMAKE_PROJECT_NAME} ... devbinder_static)
```

## Examples

By default, all examples connect to the `/dev/binder` device.

> Note: Running these examples directly on an Android device might not work as
> expected. This is because the context manager is already assigned to the
> ServiceManager process.

On a rooted Android device, you can create a new binder device node by mounting
a new binderfs file system:

```bash
mkdir /dev/mybinder
mount -t binder binder /dev/mybinder
```

Then, modify the example code to connect to `/dev/mybinder/binder` instead of
`/dev/binder`.

### Server

Run the server to listen for incoming transactions and display them. The server
sets itself as the context manager.

```bash
./server
```

### Client

Run the client to send a hello world message to the context manager
([server](#server)).

```bash
./client "hello world"
```

## Contributing

Contributions are welcome; see [CONTRIBUTING.md](CONTRIBUTING.md).

## License

This project is licensed under the terms of Apache license 2.0; see
[`LICENSE`](LICENSE).

## Security

This library is intended for educational purposes only. Please submit a PR for
security fixes, but do not open an issue, request a CVE, or submit the issue to
any bug bounty programs.
