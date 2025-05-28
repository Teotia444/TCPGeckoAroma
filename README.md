# TCPGecko

A plugin that allows for basic peek / poke instruction. Comes with other set of commands that are mostly for show.
The plugin is accessible with a TCP socket (hence, the name) at port 7332 and allows for the following commands:

* `peek -t (type: u8, u16, u32, f32) -a (address: 0xXXXXXXXX)` : peeks an address and interprets as requested type
* `poke -t (type: u8, u16, u32, f32) -a (address: 0xXXXXXXXX) -v (value: 0xXX)` : pokes an address interpreted as the specified type with the specified value
* `peekmultiple -t -a -a -a... ` : peeks multiple values using a single request. They will be separated using the pipe (`|`) operator

* `pokemultiple -t -a -v -a -v -a -v... ` : pokes multiple values using a single request.

* `find -s (step:first, next, list) -v (value: 0xFF)` : searches ram for a value in a Cheat Engine style.

* `pause`: pauses the main thread's execution

* `advance`: advances the main thread's execution

* `resume`: resumes the main thread's execution

* `call -a (address: 0xXXXXXXXX)`: branches to an address interpreted as assembly.

* `drawtext -text (quoted text: "example") -r (value: 0xFF) -g (value: 0xFF) -b (value: 0xFF) -a (value: 0xFF)` : displays text in the top left of the screen with the specified colors.
 

## Installation

(`[ENVIRONMENT]` is a placeholder for the actual environment name.)

1. Copy the file `TCPGecko.wps` into `sd:/wiiu/environments/[ENVIRONMENT]/plugins`.
2. Requires the [WiiUPluginLoaderBackend](https://github.com/wiiu-env/WiiUPluginLoaderBackend) in `sd:/wiiu/environments/[ENVIRONMENT]/modules`.

Start the environment (e.g Aroma) and the backend should load the plugin.

## Building

For building you need:

- [wups](https://github.com/Maschell/WiiUPluginSystem)
- [wut](https://github.com/devkitpro/wut)

Install them (in this order) according to their README's. Don't forget the dependencies of the libs itself.

Then you should be able to compile via `make` (with no logging) or `make DEBUG=1` (with logging).

## Buildflags

### Logging

Building via `make` only logs errors (via OSReport). To enable logging via the [LoggingModule](https://github.com/wiiu-env/LoggingModule) set `DEBUG` to `1` or `VERBOSE`.

`make` Logs errors only (via OSReport).  
`make DEBUG=1` Enables information and error logging via [LoggingModule](https://github.com/wiiu-env/LoggingModule).  
`make DEBUG=VERBOSE` Enables verbose information and error logging via [LoggingModule](https://github.com/wiiu-env/LoggingModule).

If the [LoggingModule](https://github.com/wiiu-env/LoggingModule) is not present, it'll fallback to UDP (Port 4405) and [CafeOS](https://github.com/wiiu-env/USBSerialLoggingModule) logging.

## Building using the Dockerfile

It's possible to use a docker image for building. This way you don't need anything installed on your host system.

```
# Build docker image (only needed once)
docker build . -t example-plugin-builder

# make 
docker run -it --rm -v ${PWD}:/project example-plugin-builder make DEBUG=1

# make clean
docker run -it --rm -v ${PWD}:/project example-plugin-builder make clean
```

## Format the code via docker

`docker run --rm -v ${PWD}:/src ghcr.io/wiiu-env/clang-format:13.0.0-2 -r ./src -i`