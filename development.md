## Development Notes

### Building the project
The project uses CMake and is intended to be build for multiple platforms; Mac, Linux and Windows.

### Environment

A number of tools are required.

#### Docker
Make sure that docker is installed and that the current user has sufficient permissions.

Use the provided docker files for cross compilation.

```bash
# darwin - ?

# linux: ubuntu:20.04
docker build -t linux-build --no-cache -f Dockerfile-linux-build .    
docker run --rm -v $(pwd)/volume:/usr/src/fmu-rabbitmq/build linux-build

# windows - TBD
```
The results of the build are in the volume folder.
#### Preparing dependencies
To compile the dependencies first make sure that the checkout contains submodules:

```bash
git submodule update --init
```

To compile the dependencies first prepare the docker 

```bash
mkdir -p build

# run cmake
./<platform>-dockcross cmake -Bbuild/<platform> -H.

# compile
./<platform>-dockcross make -Cbuild/<platform> -j8
```

#### Tests

In order to test the functionality without hooking it up with an actual external simulator, three scripts are included in the server/ folder to be executed with rabbitmq-main
(which can be found in the /build/<build-distribution>/rabbitmq-fmu/ folder, after building the project). The three scripts should be executed before running the fmu.

Run each of the following in a different terminal.

consume.py --> gets data from the rabbitMQ FMU
```bash
python3 consume.py
```
playback_gazebo_data.py --> feeds robot data (content data) to the rabbitMQ
```bash
python3 playback_gazebo_data-test.py
```
consume-systemHealthData.py --> gets the system health data from the rabbitMQ FMU. Everytime it gets new data, it replies with the current time on the external system.
```bash
python3 consume-systemHealthData.py
```
Finally, on a fourth terminal run:
```bash
./build/darwin-x64/rabbitmq-fmu/it-test-rabbitmq
```
The ```modelDescription.xml```used by this test is located under ```rabbitmq-fmu/xmls-for-tests```.

Should the consume-systemHealthData.py crash and stop sending data to the rabbitmq fmu, simply restart the script.
### Local development

1. First run the compliation script for your platform to get the external libraries compiled. This is located in the scripts directory. Example: `./scripts/darwin64_build.sh`, this will use docker. Alternatively, if on mac, simply run: 
 ```bash
  ./build_locally_darwin.sh 
 ```
2. Second run the following command matching the platform to the one just build:

```bash
cmake . -DTHIRD_PARTY_LIBRARIES_ROOT=`readlink -f build/external/darwin-x86_64`
```

#### Procedure for additions and release of new features

1. Do a single feature development in a branch that is NOT development and NOT master.
2. Once the feature is ready, merge it into the development branch.
3. When it is decided that a release is due, do final fixes (if any) in development, and create a TAG.
4. Finally to release it, switch to the master branch and merge with the TAG.

Note that: the master branch always contains the latest release, whereas the development branch is always stable. Github actions are triggered on push to master, and development.

#### Development on windows

This is currently a problem for local development, as the project does not build. However, it succeeds on actions. In order to run locally on windows the unit tests, the following need to be installed first, from a msys2 shell:

```bash
$ pacman -S --noconfirm mingw-w64-x86_64-gcc-libs
$ pacman -S openssl
```
