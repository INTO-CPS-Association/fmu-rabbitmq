User Manual
===========
This page covers how to use RabbitMQ FMU (RMQFMU), whereas the internal workins are described in Developer Manual.
The two parts described are: The structure of a message and how to configure properties of RMQFMU via the modelDescription file.

Message Format
---------------
Messages are to be in JSON format with timestamps in ISO 8061 with UTC offset - i.e.: :code:`2019-01-04T16:41:24+02:00`.
A message shall contain a timestamp and one or more values. The example below contains two values.

.. code-block:: json

    {
        "time":"2019-01-04T16:41:24+02:00",
        "level":0,
        "other_variable":10.0
    }


Model Description File
----------------------
:code:`ScalarVariables` within the Model Description File are used to both configure properties of RMQFMU and mapping message data to FMU outputs.
The first 0-8 `valueReference` of the model description file are used for configuring RMQFMU. It is adviced **not** to use `valueReference` 0-19, as these might be used for future updates.
Below is a description of the configuration of RMQFMU via scalar variables of the model description file:

ValueReference 0 - hostname
    Defines the host, i.e. `localhost`

ValueReference 1 - port
    Defines the port, i.e. 5672

ValueReference 2 - username
    Defines the username for the RabbitMQ Server

ValueReference 3 - Password
    Defines the password for the RabbitMQ Server

ValueReference 4 - Routing Key
    Defines the Routing Key for the messages

ValueReference 5 - Communication Timeout
    Defines when to time out if the desired state cannot be reached.

ValueReference 6 - Precision
    Precision is the number of decimals to consider after converting the addition of :code:`currentCommunicationpoint` and :code:`communicationStepSize` passed in :code:`doStep` to milliseconds.
    This has proven important in relation to imprecision of real numbers.
    The calculation is: :code:`std::round(((currentCommunicationPoint + communicationStepSize) * 1000 ) * precision) / precision`

ValueReference 7 - Max Age
    The maximum age of variable values expressed in milliseconds.
    This is a notion of when the value of a given variable is too old for RMQFMU to continue.

ValueReference 8 - Look Ahead
    The maximum number of queue messages that should be considered on each processing.
    Does not cause blocking behaviour if less messages are available.

A mapping of message data to FMU output is carried out via the name property of a :code:`ScalarVariable`. For example: :code:`<ScalarVariable name="level" valueReference="20" variability="continuous" causality="output"><Real /></ScalarVariable>` maps the value of the key :code:`level` within a message to the output with :code:`valueReference 20`.

