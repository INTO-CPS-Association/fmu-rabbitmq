# Rabbitmq FMU

This project builds a FMU which uses a rabbitmq server to feed live or log data into a simulation, as well as send data to an external entity (e.g. Gazebo simulation).

Two types of data are considered, content data, and system health data.
The rabbitMQ steps once it has valid content data. In case any of its inputs changes between two consecutive timesteps, the fmu will send only the changed inputs to the entity outside the co-sim. 
System health data are auxilliary, that is the fmu will publish it's current time-step (formatted to system time) to a topic, and will consume from a topic if the 'real' time of the outside entity is published. Note that the real time data should be coupled with the simulation time data sent by the rabbitmq FMU (a more detailed description can be found at https://into-cps-rabbitmq-fmu.readthedocs.io/en/latest/user-manual.html#). If the latter information is available the fmu will calculate whether the co-sim is ahead or behind the external entity. Note that the simulation will just continue as usual if this data is not available.

## Running a simple example

In this [example](rmqfmu-example/README.md) you will be shown how to setup a co-simulation with two FMUS, of which one is the RMQFMU, and how to feed data to this co-simulation through a python publisher script.
All files for running the example can be found in ```rmqfmu-example```.

## Usage Notes

### How to Setup

The FMU is configured using a script `rabbitmq_fmu_configure.py` for the input/output variables that are model specific or manually by (Value References from 0-20 are reserved for development.):
* adding all model outputs manually as:
```xml
<ModelVariables>
  <ScalarVariable name="seqno" valueReference="14" variability="continuous" causality="output">
    <Integer />
  </ScalarVariable> 
  <ScalarVariable name="level" valueReference="20" variability="continuous" causality="output">
    <Real />
  </ScalarVariable>  
  <ScalarVariable name="level-2" valueReference="21" variability="continuous" causality="output">
    <Real />
  </ScalarVariable>
    
  <ScalarVariable name="simtime_discrepancy" valueReference="22" variability="continuous" causality="output">
    <Real />
  </ScalarVariable>
 <ScalarVariable name="time_discrepancy" valueReference="23" variability="continuous" causality="output">
    <Real />
</ScalarVariable>
</ModelVariables>
<ModelStructure>
  <Outputs>
    <Unknown index="1"/>
    <Unknown index="2"/>
    <Unknown index="3"/>
    <Unknown index="4"/>
    <Unknown index="5"/>
  </Outputs>
</ModelStructure>
```
Remember to add the outputs before the configuration variables.
If outputs `time_discrepancy` and `simtime_discrepancy` are given, and there is system health data provided, the rabbitmq fmu will set these values. If the outputs are not given, the rabbitmq fmu will proceed as usual.

* adding all model inputs manually as:
```xml
<ModelVariables>
  <ScalarVariable name="command_stop" valueReference="22" variability="discrete" causality="input">
      <Boolean start="false" />
  </ScalarVariable>
  <ScalarVariable name="command2" valueReference="23" variability="continuous" causality="input">
      <Real start="3.2" />
  </ScalarVariable>
  <ScalarVariable name="command3" valueReference="24" variability="continuous" causality="input">
      <Integer start="2" />
  </ScalarVariable>
  <ScalarVariable name="command4" valueReference="25" variability="discrete" causality="input">
      <String start="hello rabbitmq" />
  </ScalarVariable>
```

* configuring the inputs/outputs through the script `rabbitmq_fmu_configure.py`:

To add outputs and specify type and variability:
```bash
$ python3 rabbitmq_fmu_configure.py -fmu in.fmu -output greet=Real,continuous -dest=out.fmu 
```
The command will add an output with name ```greet```, of type ```Real```, and variability ```continuous```.

Note that the length of the lists: output, output types and output variabilities must be the same.

Similarly for inputs. A complete command looks like:
```bash
$ python3 rabbitmq_fmu_configure.py -fmu in.fmu -output greet=Real,continuous -dest=out.fmu -input chat=String,discrete stop=Boolean,discrete beatles=Integer,discrete
```
The command will add an output with name ```greet```, of type ```Real```, and variability ```continuous```, as well three inputs (i) with name ```chat```, of type ```String```, and variability ```discrete```, (ii) with name ```stop```, of type ```Boolean```, and variability ```discrete```, and (iii) with name ```beatles```, of type ```Integer```, and variability ```discrete```.

__NOTE that the script doesn't check for the validity of the modelDescription file, use the vdmCheck scripts for that purpose.

* add the `modelDescription.xml` file to the zip at both the root and `resources` folder.

The RabbitMQ FMU can be configured by setting the following parameters:

```xml
<ScalarVariable name="config.hostname" valueReference="0" variability="fixed" causality="parameter">
    <String start="localhost"/>
</ScalarVariable>
<ScalarVariable name="config.port" valueReference="1" variability="fixed" causality="parameter">
    <Integer start="5672"/>
</ScalarVariable>
<ScalarVariable name="config.username" valueReference="2" variability="fixed" causality="parameter">
    <String start="guest"/>
</ScalarVariable>
<ScalarVariable name="config.password" valueReference="3" variability="fixed" causality="parameter">
    <String start="guest"/>
</ScalarVariable>
<ScalarVariable name="config.routingkey" valueReference="4" variability="fixed" causality="parameter">
    <String start="linefollower.data.to_cosim"/>
</ScalarVariable>
<ScalarVariable name="config.communicationtimeout" valueReference="5" variability="fixed" causality="parameter" description="Network read time out in seconds" initial="exact">
    <Integer start="60"/>
</ScalarVariable>
<ScalarVariable name="config.precision" valueReference="6" variability="fixed" causality="parameter" description="Communication step comparison precision. Number of decimals to consider" initial="exact">
    <Integer start="10"/>
</ScalarVariable>
<ScalarVariable name="config.maxage" valueReference="7" variability="fixed" causality="parameter" description="The max age of a value specified in ms," initial="exact">
    <Integer start="100"/>
</ScalarVariable>
<ScalarVariable name="config.lookahead" valueReference="8" variability="fixed" causality="parameter" description="The number of queue messages that should be considered on each processing. Value must be greater than 0" initial="exact">
    <Integer start="1"/>
</ScalarVariable> 
<ScalarVariable name="config.exchangename" valueReference="9" variability="fixed" causality="parameter" initial="exact">
    <String start="fmi_digital_twin_cd"/>
</ScalarVariable>
<ScalarVariable name="config.exchangetype" valueReference="10" variability="fixed" causality="parameter" initial="exact">
    <String start="direct"/>
</ScalarVariable>
<ScalarVariable name="config.healthdata.exchangename" valueReference="11" variability="fixed" causality="parameter" initial="exact">
    <String start="fmi_digital_twin_sh"/>
</ScalarVariable>
<ScalarVariable name="config.healthdata.exchangetype" valueReference="12" variability="fixed" causality="parameter" initial="exact">
    <String start="direct"/>
</ScalarVariable>
<ScalarVariable name="config.routingkey.from_cosim" valueReference="13" variability="fixed" causality="parameter" initial="exact">
    <String start="linefollower.data.from_cosim"/>
</ScalarVariable>
<ScalarVariable name="config.ssl" valueReference="16" variability="fixed" causality="parameter" initial="exact">
    <Boolean start="true"/>
</ScalarVariable>
<ScalarVariable name="config.queueupperbound" valueReference="17" variability="fixed" causality="parameter" initial="exact">
    <Integer start="100"/>
</ScalarVariable>
```

Note that the value reference `14` is reserved for output `seqno`, that refers to the sequence number of the message.  This output can be removed if not needed.
Note that the value reference `15` is reserved for input `enable send input`, that allows a user to send a control signal to RMQFMU to enable/disable its ability to send messages outside of the co-sim.  This input can be removed if not needed.
Note that the value reference `16` is reserved for parameter `ssl`, which allows a user to configure an ssl connection to the rabbitMQ server.  This parameter can be removed if not needed, it will default to false, that is connection without ssl.
Note that the value reference `17` is reserved for parameter `queue upper bound`, that bounds the size of the incoming queue in the RMQFMU. If the limit is reached then RMQFMU will not consume from the server. This parameter can be removed if not needed, it will default to 100.

### Notes on connections created to the rabbitMQ server

If the RMQFMU is configured to build with the threaded option on (the default behaviour, and what you get with the release package), then for each type of data, two connections are created, one for publishing to the server, and one for consuming from the server. This is to ensure that there is no bottleneck at the socket level. 

Otherwise the fmu creates two connections with which the rabbitmq communicates with an external entity, for the content data and system health data respecitvely. Note that the variables with value reference=4 and 13 mean that the same routing keys are created for both connecetions.

The connection for content data is configured through: `config.exchangename`, `config.exchangetype`, `config.routingkey`, `config.routingkey.from_cosim`.
The connection for health data is configured through: `config.healthdata.exchangename`, `config.healthdata.exchangetype`, `config.routingkey`, `config.routingkey.from_cosim`.

### Dockerized RabbitMq
To launch a Rabbitmq server the following can be used:

```bash
cd server
docker-compose up -d
```

This will launch it at localhost `5672` for TCP communication and http://localhost:15672 will serve the management interface. The default login is username: `guest` and password: `guest`


## Development Notes

### Building the project
The project uses CMake and is intended to be build for multiple platforms; Mac, Linux and Windows.

### Environment

A number of tools are required.

#### Docker and dockcross

Make sure that docker is installed and that the current user has sufficient permissions.

Prepare dockcross helper scripts, for building across the three platforms locally

```bash
# darwin
docker run --rm docker.sweng.au.dk/dockcross-darwin-x64-clang:latest > ./darwin-x64-dockcross
chmod +x ./darwin-x64-dockcross

# linux
docker run --rm dockcross/linux-x64:latest > ./linux-x64-dockcross
chmod +x ./linux-x64-dockcross

# windows
docker run --rm dockcross/windows-static-x64:latest > ./win-x64-dockcross
chmod +x ./win-x64-dockcross
```

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
