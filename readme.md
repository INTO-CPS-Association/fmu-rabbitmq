# Rabbitmq FMU

This project builds a FMU which uses a rabbitmq server to feed live or log data into a simulation, as well as send data to an external entity (e.g. Gazebo simulation).

Two types of data are considered, content data, and system health data.
The rabbitMQ steps once it has valid content data. In case any of its inputs changes between two consecutive timesteps, the fmu will send only the changed inputs to the entity outside the co-sim. 
System health data are auxilliary, that is the fmu will publish it's current time-step (formatted to system time) to a topic, and will consume from a topic if the 'real' time of the outside entity is published. Note that the real time data should be coupled with the simulation time data sent by the rabbitmq FMU (a more detailed description can be found at https://into-cps-rabbitmq-fmu.readthedocs.io/en/latest/user-manual.html#). If the latter information is available the fmu will calculate whether the co-sim is ahead or behind the external entity. Note that the simulation will just continue as usual if this data is not available.

The FMU is configured using a script TBD for the output variables that are model specific or manually by (Value References from 0-20 are reserved for development.):
* adding all model outputs as:
```xml
<ModelVariables>
  <ScalarVariable name="level" valueReference="20" variability="continuous" causality="output">
    <Real />
  </ScalarVariable>  
  <ScalarVariable name="level-2" valueReference="21" variability="continuous" causality="output">
    <Real />
  </ScalarVariable>
</ModelVariables>
<ModelStructure>
  <Outputs>
    <Unknown index="1"/>
    <Unknown index="2"/>
  </Outputs>
</ModelStructure>
```
remember to add the outputs before the configuration variables
* add the `modelDescription.xml` file to the zip at both the root and `resources` folder.

* adding all model inputs as:
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

It can be configured by setting the following parameters:

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
    <String start="linefollower"/>
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
<ScalarVariable name="config.routingkeySystemHealth" valueReference="9" variability="fixed" causality="parameter" initial="exact">
    <String start="system_health"/>
</ScalarVariable>
```

In total the fmu creates two connections with which the rabbitmq communicates with an external entity, for the content data and system health data respecitvely. There are two channels for each connection, one channel that handles the publishing and the other that handles the consuming. Note that the variable with value reference=4 serves as a base for the configuration of the connection for the content data, whereas the variable with value reference=9 does the same for the connection for the system health data.

The fmu configures the name of the channels as follows:
${routing key base}+"_from_cosim" for publishing, which would result in "linefollower_from_cosim" and "system_health_from_cosim" given the values in the above example. Data sent from the
rabbitMQ can be consumed from these topics.
${routing key base}+"_to_cosim" for consuming, which would result in "linefollower_to_cosim" and "system_health_to_cosim" given the values in the above example. Data to be sent
to the rabbitMQ should be published to these topics.

## Dockerized RabbitMq
To launch a Rabbitmq server the following can be used:

```bash
cd server
docker-compose up -d
```bash
This will launch it at localhost `5672` for TCP communication and http://localhost:15672 will serve the management interface. The default login is username: `guest` and password: `guest`


# Building the project
The project uses CMake and is intended to be build for multiple platforms; Mac, Linux and Windows.

# Environment

A number of tools are required.

## Docker and dockcross

Make sure that docker is installed and that the current user has sufficient permissions.

Prepare dockcross helper scripts
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

## Preparing dependencies
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

## Tests

In order to test the functionality without hooking it up with an actual external simulator, three scripts are included in the server/ folder to be executed with rabbitmq-main
(which can be found in the /build/<build-distribution>/rabbitmq-fmu/ folder, after building the project). The three scripts should be executed before running the fmu.

Run each of the following in a different terminal.

consume.py --> gets data from the rabbitMQ FMU
```bash
python3 consume.py
```
playback_gazebo_data.py --> feeds robot data (content data) to the rabbitMQ
```bash
python3 playback_gazebo_data.py
```
consume-systemHealthData.py --> gets the system health data from the rabbitMQ FMU. Everytime it gets new data, it replies with the current time on the external system.
```bash
python3 consume-systemHealthData.py
```
Finally, on a fourth terminal run:
```bash
./build/darwin-x64/rabbitmq-fmu/rabbitmq-main
```

Should the consume-systemHealthData.py crash and stop sending data to the rabbitmq fmu, simply restart the script.
# Local development

1. First run the compliation script for your platform to get the external libraries compiled. This is located in the scripts directory. Example: `./scripts/darwin64_build.sh`
2. Second run the following command matching the platform to the one just build:

```bash
cmake . -DTHIRD_PARTY_LIBRARIES_ROOT=`readlink -f build/external/darwin-x86_64`
```

# Procedure for additions and release of new features

1. Do a single feature development in a branch that is NOT development and NOT master.
2. Once the feature is ready, merge it into the development branch.
3. When it is decided that a release is due, do final fixes (if any) in development, and create a TAG.
4. Finally to release it, switch to the master branch and merge with the TAG.

Note that: the master branch always contains the latest release, whereas the development branch is always stable.  
