# GroundLift

An AirDrop alternative.

## Compilation Requirements

In order to compile this project you're required to install a couple of
packages. In case you're using a Debian-based system just run the following
command and you'll be all set:

```bash
sudo apt install build-essential cmake
```

## Compiling

Compiling this project is a very simple task thanks to the
[CMake](https://cmake.org/) build system. All you have to do is this:

```bash
mkdir build
cd build
cmake ..
make
```

Now you should have the command line executable `build/cli/gl` that you can use
to test the project out.

## License

This project is licensed under the **MIT License**.

