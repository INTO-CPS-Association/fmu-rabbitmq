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

To Be Continued

Message Format
---------------
To Be Continued
