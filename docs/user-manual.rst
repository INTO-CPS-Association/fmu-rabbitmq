User Manual
===========
This page covers how to use RabbitMQ FMU (RMQFMU) but does not describe its internal workings.
The two parts described are: The structure of a message and how to configure RMQFMU via the modelDescription file.

Model Description File
----------------------
The first 0-8 `valueReference` of the model description file are used for configuring RMQFMU, but it is adviced **not** to use `valueReference` 0-19, as these might be used for future updates.
Below is a description of the configuration via scalar variables of the model description file:

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
    Precision if the number of decimals to consider after converting the addition of currentCommunicationpoint and communicationStepSize passed in :code:`doStep` to milliseconds.
    This has proven important in relation to imprecision of real numbers.
    The calculation is: :code:`std::round(((currentCommunicationPoint + communicationStepSize) * 1000 ) * precision) / precision`

ValueReference 7 - Max Age
    The maximum age of variable values expressed in milliseconds.
    This is a notion of when the value of a given variable is too old for RMQFMU to continue.

ValueReference 8 - Look Ahead
    The maximum number of queue messages that should be considered on each processing.
    Does not cause blocking behaviour if less messages are available.



To Be Continued

Message Format
---------------
To Be Continued
