## OpenDLV Microservice to encode images in I420 format into VP8 or VP9 for network broadcast

This repository provides source code to encode images in I420 format that are accessible
via a shared memory area into VP8 or VP9 frames for the OpenDLV software ecosystem.

[![License: GPLv3](https://img.shields.io/badge/license-GPL--3-blue.svg
)](https://www.gnu.org/licenses/gpl-3.0.txt)


## Table of Contents
* [Dependencies](#dependencies)
* [Usage](#usage)
* [Build from sources on the example of Ubuntu 16.04 LTS](#build-from-sources-on-the-example-of-ubuntu-1604-lts)
* [License](#license)


## Dependencies
You need a C++14-compliant compiler to compile this project.

The following dependency is part of the source distribution:
* [libcluon](https://github.com/chrberger/libcluon) - [![License: GPLv3](https://img.shields.io/badge/license-GPL--3-blue.svg
)](https://www.gnu.org/licenses/gpl-3.0.txt)

The following dependencies are downloaded and installed during the Docker-ized build:
* [libvpx 1.7.0](https://github.com/webmproject/libvpx/releases/tag/v1.7.0) - [![License: BSD 3-Clause](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause) - [Google Patent License Conditions](https://raw.githubusercontent.com/webmproject/libvpx/f80be22a1099b2a431c2796f529bb261064ec6b4/PATENTS)
* [libyuv](https://chromium.googlesource.com/libyuv/libyuv/+/master) - [![License: BSD 3-Clause](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause) - [Google Patent License Conditions](https://chromium.googlesource.com/libyuv/libyuv/+/master/PATENTS)


## Usage
To run this microservice using `docker-compose`, you can simply add the following
section to your `docker-compose.yml`:

```yml
version: '2' # Must be present exactly once at the beginning of the docker-compose.yml file
services:    # Must be present exactly once at the beginning of the docker-compose.yml file
    video-vpx-encoder:
        image: chalmersrevere/opendlv-video-vpx-encoder-multi:v0.0.8
        restart: on-failure
        network_mode: "host"
        ipc: "host"
        volumes:
        - /tmp:/tmp
        command: "--cid=111 --name=video0.i420 --width=640 --height=480 --vp8"
```

As this microservice is connecting to another video frame-providing microservice
via a shared memory area using SysV IPC, the `docker-compose.yml` file specifies
the use of `ipc:host`. The parameter `network_mode: "host"` is necessary to
broadcast the resulting frames into an `OD4Session` for OpenDLV. The folder
`/tmp` is shared into the Docker container to attach to the shared memory area.
The parameters to the application are:

* `--cid=111`: Identifier of the OD4Session to broadcast the VP8 or VP9 frames to
* `--id=2`: Optional identifier to set the senderStamp in broadcasted VP8 or VP9 frames in case of multiple instances of this microservice
* `--name=XYZ`: Name of the shared memory area to attach to
* `--width=W`: Width of the image in the shared memory area
* `--height=H`: Height of the image in the shared memory area
* `--bitrate=B`: desired bitrate (default: 800,000)
* `--gop=G`: desired length of group of pictures (default: 10)
* `--vp8`: use VP8 for encoding the frames
* `--vp9`: use VP8 for encoding the frames


## Build from sources on the example of Ubuntu 16.04 LTS
To build this software, you need cmake, C++14 or newer, libyuv, libvpx, and make.
Having these preconditions, just run `cmake` and `make` as follows:

```
mkdir build && cd build
cmake -D CMAKE_BUILD_TYPE=Release ..
make && make test && make install
```


## License

* This project is released under the terms of the GNU GPLv3 License

