
## smart_pointer

### Basic setup

Add bincrafters remote to conan:

```sh
conan remote add bincrafters https://api.bintray.com/conan/bincrafters/public-conan
```

You will first need the cairo recipe or package with `xcb` enable:
```sh
git clone https://github.com/chreniuc/conan-cairo.git
cd conan-cairo
conan create . chreniuc/stable -o *:shared=False
```

After that go in the project directory:

```sh
mkdir build && cd build
conan install ..
conan build ..
./bin/smart_pointer
```

**Notice:** This project links statically with almost all dependencies of `cairo`,
including `cairo` itself, excluding `xcb` and it's dependencies: `xcb`,
`xcb-shm`, `xcb-render`.
