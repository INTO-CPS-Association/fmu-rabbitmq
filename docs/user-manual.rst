User Manual
===========
The RabbitMQ FMU (RMQFMU) allows to publish and consume two types of data to/from a rabbitMQ server. 
The two types of data are: content data, and system health data. For each type of data, a separate connection is used to the rabbitMQ server, with two channels, one for publishing and one for consuming respectively. 

The rest of this page covers how to use RMQFMU, whereas the internal workings are described in Developer Manual (Data Handling).
The two parts described are: The structure of a message and how to configure properties of RMQFMU via the modelDescription file.

**NOTE: As the Model Description file itself is parsed by RMQFMU, a copy has to be placed inside the resources folder.**

Message Format - Content Data
------------------------------
Messages to be consumed by the RMQFMU are to be in JSON format with timestamps in ISO 8061 with UTC offset - i.e.: :code:`2019-01-04T16:41:24+02:00`.
A message shall contain a timestamp and one or more values. The example below contains two values.

.. code-block:: json

    {
        "time":"2019-01-04T16:41:24+02:00",
        "level":0,
        "other_variable":10.0
    }

Messages published by the RMQFMU are also packaged in the json format, and will contain a timestep together with one or more values of the included inputs. Only those inputs the value of which has changed between two consecutive timesteps are included in the message. If no input has changed, then no message is sent from the RMQFMU. An example is given below.

.. code-block:: json

    {
        "timestep":"2019-01-04T16:41:24+02:00",
        "input_bool_stop":1,
        "input_int_speed":5,
        "other_string_input":"hello rbMQ"
    }
    
Message Format - System Health Data
------------------------------------
Messages to be consumed by the RMQFMU are to be in JSON format with two fields as shown in the example below. The key :code:`rtime` refers to the current timestep as perceived from the system outside of the co-simuluation sending a message to RMQFMU, whereas the key :code:`cosimtime` to the latest timestep sent by the RMQFMU. Note that the time should be in accordance with the ISO 8061 with UTC offset - i.e.: :code:`2019-01-04T16:41:24+02:00`.

.. code-block:: json

    {
        "rtime":"2019-01-04T16:41:24+02:00",
        "cosimtime":"2019-01-04T16:41:24+02:00",
    }

Messages published by the RMQFMU are also packaged in the json format, and contain the current timestep in which the RMQFMU is operating. Note that the time is to be in accordance with the ISO 8061 with UTC offset - i.e.: :code:`2019-01-04T16:41:24+02:00`.

.. code-block:: json

    {
        "simAtTime":"2019-01-04T16:41:24+02:00"
    }
    
Note that, the RMQFMU will calculate the time discrepancy between the co-sim and outside world based on ``rtime`` and ``cosimtime``. Initially these values will be transformed into internal co-simulation time, afterwards the difference ``cosimtime``-``rtime`` will be used to set the ``time_discrepancy`` output of RMQFMU. Furthermore, the difference between the current simulation time and `cosimtime` will be used to set the ``simtime_discrepancy`` output of RMQFMU. Normally this should be zero, as the RMQFMU will set the output based on the latest system health message. 
Finally, consuming system health data is not blocking. If no data is available when a :code:`consume()` is performed, the co-simulation will proceed to the next step, using the previously calculated values if the outputs ``simtime_discrepancy`` and ``time_discrepancy`` are given in the modelDescription.xml.

Model Description File
----------------------
:code:`ScalarVariables` within the Model Description File are used to configure properties of RMQFMU, mapping message data to FMU outputs, as well as FMU inputs to message data.
The first 0-9 `valueReference` of the model description file are used for configuring RMQFMU. It is adviced **not** to use `valueReference` 0-19, as these might be used for future updates.
Below is a description of the configuration of RMQFMU via scalar variables of the model description file:

ValueReference 0 - hostname
    Defines the host, i.e. `localhost`

ValueReference 1 - port
    Defines the port, i.e. 5672

ValueReference 2 - username
    Defines the username for the RabbitMQ Server

ValueReference 3 - Password
    Defines the password for the RabbitMQ Server

ValueReference 4 - Routing Key - content data
    Defines the Routing Key for the content data messages

ValueReference 5 - Communication Timeout
    Defines when to time out if the desired state cannot be reached.

ValueReference 6 - Precision
    Precision is the number of decimals to consider after converting the addition of :code:`currentCommunicationpoint` and :code:`communicationStepSize` passed in :code:`doStep` to milliseconds.
    This has proven important in relation to imprecision of real numbers.
    The calculation is: :code:`precision = std::pow(10, precisionDecimalPlaces); simulationTime = std::round(simulationTime * precision) / precision;`

ValueReference 7 - Max Age
    The maximum age of variable values expressed in milliseconds.
    This is a notion of when the value of a given variable is too old for RMQFMU to continue.

ValueReference 8 - Look Ahead
    The maximum number of queue messages that should be considered on each processing.
    Does not cause blocking behaviour if less messages are available.

The parameter with value reference 4 is used as a base to configure two different connections to the rabbitMQ server, with two channels each. 
Based on the ``routingKey`` the fmu configures the name of the channels as follows:
:code:`${routing key base}+".{data|system_health}."+"from_cosim"` for publishing, which would result in ``linefollower.data.from_cosim`` and ``linefollower.system_health.from_cosim`` given the values in the above example. Data sent from the
rabbitMQ can be consumed from these topics.
:code:`${routing key base}+".{data|system_health}."+"to_cosim"` for consuming, which would result in ``linefollower.data.to_cosim`` and ``linefollower.system_health.to_cosim`` given the values in the above example. Data to be sent
to the rabbitMQ should be published to these topics.

**NOTE: If no system health data is published to RMQFMU then the operation of the fmu will continue normally, however no information regarding system health will be outputted from RMQFMU.**

A mapping of message data to FMU output is carried out via the name property of a :code:`ScalarVariable`. For example: :code:`<ScalarVariable name="level" valueReference="20" variability="continuous" causality="output"><Real /></ScalarVariable>` maps the value of the key :code:`level` within a message to the output with :code:`valueReference 20`.

Remember, when adding an additional output this also has to be added to outputs in modelstructure. Note, that it uses index and not valuereference! Index is related to the order of the respective scalarvariable. I.e. the topmost scalar variable within ``ModelVariables`` has index 1. Example of adding two indices to ``ModelStructure/Outputs``:

.. code-block:: xml

    <ModelStructure>
        <Outputs>
            <Unknown index="1"/>
            <Unknown index="2"/>
        </Outputs>
    </ModelStructure>
    
A mapping of an FMU input to a message is carried out via the name property of a :code:`ScalarVariable`. For example: :code:`<ScalarVariable name="feedback" valueReference="21" variability="continuous" causality="input"><Real /></ScalarVariable>` maps the value of the input with :code:`valueReference 21` to the key :code:`feedback` within a message.
