# Rabbitmq FMU

## Prerequisites
Concepts of FMI/FMU, co-simulation, multi-model, digital twin (DT), physical twin (PT)

## Purpose
This project builds an FMU which uses a rabbitmq server to feed live/log data into a co-simulation, as well as send data to an external entity (e.g. Gazebo simulation) from the co-simulation.
In more detail, this is useful to transform an FMI-based co-simulation into a DT by connecting it to the physical twin/system for which the DT itself is built.
The rabbitMQ FMU, or RMQFMU can be included in any co-simulation multi-model, with its inputs and outputs connected to the other models included in the multi-model as needed. 

The rest of this guide covers the following topics:
* An example showing how to include the RMQFMU into a co-simulation, how to run the latter (through the Maestro co-orchestration engine), how to setup a rabbitMQ server on localhost, and how to publish/receive data from the server. 
* An overview of how the RMQFMU can be configured through its parameters, as well as how to specify inputs and outputs, all three in the model description file.
* How to use `rabbitmq_fmu_configure.py` to add the desired inputs and outputs without decompressing the FMU, and dealing the model description file.
* Set up a dockerised rabbitMQ server at localhost.
* Notes on how the RMQFMU creates connections to the rabbitMQ server.
* Development notes for those who wish to contribute to the RMQFMU project.


<!-- Types of data in the RMQFMU
Two types of data are considered, content data, and system health data.
The rabbitMQ steps once it has valid content data. In case any of its inputs changes between two consecutive timesteps, the fmu will send only the changed inputs to the entity outside the co-sim. 
System health data are auxilliary, that is the fmu will publish it's current time-step (formatted to system time) to a topic, and will consume from a topic if the 'real' time of the outside entity is published. Note that the real time data should be coupled with the simulation time data sent by the rabbitmq FMU (a more detailed description can be found at https://into-cps-rabbitmq-fmu.readthedocs.io/en/latest/user-manual.html#). If the latter information is available the fmu will calculate whether the co-sim is ahead or behind the external entity. Note that the simulation will just continue as usual if this data is not available.
-->

## Running a simple example

In the following [README](rmqfmu-example/README.md) you will be shown how to setup a co-simulation with two FMUS, of which one is the RMQFMU, and how to feed data to this co-simulation through a python publisher script.
All files for running the example can be found in ```rmqfmu-example```.

## Configuring the RMQFMU

### Parameters

The RabbitMQ FMU can be configured by setting the following parameters (note, value references from 0-20 are reserved in the RMQFMU and thus CANNOT be used for inputs and outputs): 

```xml
<ScalarVariable name="config.hostname" valueReference="0" variability="fixed" causality="parameter" description="the hostname for the rabbitMQ server">
    <String start="localhost"/>
</ScalarVariable>
<ScalarVariable name="config.port" valueReference="1" variability="fixed" causality="parameter" description="the port for the rabbitMQ server">
    <Integer start="5672"/>
</ScalarVariable>
<ScalarVariable name="config.username" valueReference="2" variability="fixed" causality="parameter">
    <String start="guest"/>
</ScalarVariable>
<ScalarVariable name="config.password" valueReference="3" variability="fixed" causality="parameter">
    <String start="guest"/>
</ScalarVariable>
<ScalarVariable name="config.routingkey" valueReference="4" variability="fixed" causality="parameter" description="routing key for data fetched by the RMQFMU">
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
<ScalarVariable name="config.exchangename" valueReference="9" variability="fixed" causality="parameter" initial="exact" description="Exchange name for the routing key with valref=4">
    <String start="fmi_digital_twin_cd"/>
</ScalarVariable>
<ScalarVariable name="config.exchangetype" valueReference="10" variability="fixed" causality="parameter" initial="exact">
    <String start="direct"/>
</ScalarVariable>
<ScalarVariable name="config.healthdata.exchangename" valueReference="11" variability="fixed" causality="parameter" initial="exact" description="Used for auxiallary health data">
    <String start="fmi_digital_twin_sh"/>
</ScalarVariable>
<ScalarVariable name="config.healthdata.exchangetype" valueReference="12" variability="fixed" causality="parameter" initial="exact" description="Used for auxiallary health data">
    <String start="direct"/>
</ScalarVariable>
<ScalarVariable name="config.routingkey.from_cosim" valueReference="13" variability="fixed" causality="parameter" initial="exact" description="routing key for data sent by the RMQFMU">
    <String start="linefollower.data.from_cosim"/>
</ScalarVariable>
<ScalarVariable name="config.ssl" valueReference="16" variability="fixed" causality="parameter" initial="exact" description="allows a user to configure an ssl connection to the rabbitMQ server.  This parameter can be removed if not needed, it will default to false, that is connection without ssl.">
    <Boolean start="true"/>
</ScalarVariable>
<ScalarVariable name="config.queueupperbound" valueReference="17" variability="fixed" causality="parameter" initial="exact" description="bounds the size of the incoming queue in the RMQFMU. If the limit is reached then RMQFMU will not consume from the server. This parameter can be removed if not needed, it will default to 100">
    <Integer start="100"/>
</ScalarVariable>
```

### Adding outputs manually

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
    <InitialUnknowns>
    <Unknown index="1"/>
    <Unknown index="2"/>
    <Unknown index="3"/>
    <Unknown index="4"/>
    <Unknown index="5"/>
  </InitialUnknowns>
</ModelStructure>
```
Remember to add the outputs before the configuration variables, for the indexing to match the snippet above.
If outputs `time_discrepancy` and `simtime_discrepancy` are given, and there is system health data provided, the rabbitmq fmu will set these values. If the outputs are not given, the rabbitmq fmu will proceed as usual.

OPTIONAL output:
* Value reference `14` is reserved for output `seqno`, that refers to the sequence number of the message.  This output can be removed if not needed.

### Adding inputs manually

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

OPTIONAL input:
* Value reference `15` is reserved for input `enable send input`, that allows a user to send a control signal to RMQFMU to enable/disable its ability to send messages outside of the co-sim.  This input can be removed if not needed.

### Placing the `modelDescription.xml` 

Note that this file should be present in the zip at both the root and `resources` folders.

### Automate configuration for inputs/outputs 

It is possible to automate the configuration of inputs and outputs through the script `rabbitmq_fmu_configure.py`, without manually modyfing the model description file:

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

__NOTE that the script doesn't check for the validity of the modelDescription file, use vdmCheck scripts for that purpose.

__NOTE the model description file will automatically be added to both folders.

## Dockerized RabbitMq server

To launch a Rabbitmq server at localhost for your own tests the following can be used:

```bash
cd server
docker-compose up -d
```

This will launch it at localhost `5672` for TCP communication and http://localhost:15672 will serve the management interface. The default login is username: `guest` and password: `guest`


## Notes on connections created to the rabbitMQ server

* If the RMQFMU is configured to build with the threaded option on (the default behaviour, and what you get with the release package), then for each type of data, two connections are created, one for publishing to the server, and one for consuming from the server. This is to ensure that there is no bottleneck at the socket level. 

* Otherwise the fmu creates two connections with which the rabbitmq communicates with an external entity, for the content data and system health data respecitvely. Note that the variables with value reference=4 and 13 mean that the same routing keys are created for both connecetions.

* The connection for content data is configured through: `config.exchangename`, `config.exchangetype`, `config.routingkey`, `config.routingkey.from_cosim`.
* The connection for health data is configured through: `config.healthdata.exchangename`, `config.healthdata.exchangetype`, `config.routingkey`, `config.routingkey.from_cosim`.



## Development Notes

Please follow the instructions given [here](development.md) should you wish to contribute to this project.