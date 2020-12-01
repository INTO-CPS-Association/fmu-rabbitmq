Data Handling
==============

This page gives a more detailed description of initialisation and doStep of RMQFMU. The description is mainly carried out via diagrams, where some functions are referred to by name. Such functions are described in the section *Functions*.

Initialization
--------------
The initialization process of RabbitMQ FMU is peculiar in the sense that it needs to have an output for all messages. For this reason, the initialization has to occur when :code:`fmi2EnterInitializationMode` is invoked on RabbitMQ FMU.

.. _initialisation-functions:

Functions
^^^^^^^^^
Some functions are described in this seciton that are used in the diagram in the subsequent section.

RabbitMQFMU.configure()
    - Configure the RMQFMU based on the configuration within the model description file.

RabbitMQFMUCore.ProcessIncoming
    - Move lookahead amount of messages per message type from IncomingUnprocessed to IncomingLookahead
    - Sort lookahead according to time

RabbitMQFMUCore.ProcessLookahead
    - Move message from incomingLookahead to currentData if newer.

RabbitMQFMUCore.check
    Returns false if any of the following cases occur for a message\: msg in currentData\:

    - Missing in currentData
    - CurrentData holds a future value\: msg.time > simulationTime
    - CurrentData holds an expired value\: (msg.time + maxAge) < simulationTime


Flow of EnterInitializationMode
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. uml::

    title EnterInitializationMode
    hide footbox

    boundary "Co-simulation Master" as Master
    participant RabbitMQFMUInterface as FMUI
    participant RabbitMQFMUCore as FMUC
    database "RabbitMQ Server" as server


    Master -> FMUI: EnterInitializationMode
    FMUI -> FMUI: configure() connection for content data (CCD)
    FMUI -> server: Publish Ready message on connection for content data
    FMUI -> FMUI: configure() connection for system health data (CSHD)
    FMUI -> FMUI: initializeCoreState()
    FMUI -> FMUI: set StartTime = Time Now
        loop TimeNow - StartTime < communicationTimeOut
            FMUI -> server: ConsumeSingleMessage(&msg) | (CCD)
            alt There is a message
                server --> FMUI: msg = message; return True
                FMUI -> FMUC: AddToIncomingUnprocessed(msg)
                FMUI -> FMUC: initializeResult = Initialize()
                group initialize function
                    FMUC -> FMUC: processIncoming()
                    FMUC -> FMUC: processLookahead()
                    FMUC -> FMUC: startOffsetTime = calculateStartTime()
                    FMUC -> FMUC: initializeResult = check(0)
                    FMUC --> FMUI: initializeResult
                end
                alt initializeResult is True
                    FMUI --> Master: True
                end
            else There are no messages
                server --> FMUI: False
            end
        end

DoStep
-------
This section describes the doStep operation of RabbitMQ FMU.

.. _dostep-functions:

Functions
^^^^^^^^^^

Some functions are are described in this seciton that  are used in the diagram in the subsequent section.


RabbitMQFMUCore.check
    Described in functions section of the initialization section.

RabbitMQFMUCore.ProcessIncoming
    - Move lookahead amount of messages per message type from IncomingUnprocessed to IncomingLookahead
    - Sort lookahead according to time

RabbitMQFMUCore.ProcessLookahead
    - Move value from incomingLookahead to currentData if <= simulationTime and newer than the value in currentData. Otherwise keep in IncomingLookahead

Flow of DoStep Operation
^^^^^^^^^^^^^^^^^^^^^^^^^

.. uml::

    title DoStep operation
    hide footbox

    boundary "Co-simulation Master" as Master
    participant RabbitMQFMUInterface as FMUI
    participant RabbitMQFMUCore as FMUC
    database "RabbitMQ Server" as server


    Master -> FMUI: doStep(currentCommunicationTime, communicationStepSize)
    FMUI -> FMUI: simulationTime = applyPrecision(\ncurrentCommunicationTime+communicationStepSize)
    FMUI -> FMUC: Publish system health data | (CSHD)
    FMUI -> FMUC: process(simulationTime)
    group process function
        FMUC -> FMUC: check()
        FMUC -> FMUC: ProcessIncoming()
        FMUC -> FMUC: ProcessLookahead()
        FMUC -> FMUC: processResult = check()
        FMUC --> FMUI: processResult
    end
    FMUI -> FMUI: StartTime = Time Now
        loop TimeNow - StartTime < communicationTimeOut
            FMUI -> server: ConsumeSingleMessage(&msg)
            alt There is a message
                server --> FMUI: msg = message; return True
                FMUI -> FMUC: AddToIncomingUnprocessed(msg)
                alt There is change of inputs
                    FMUI -> FMUC: Package json message
                    FMUI -> server: Send message with changed inputs
                end
                FMUI -> server: consumeSingleMessage(&msgSH)
                alt There is system health data
                   server --> FMUI: msgSH = messageSH; return True
                   FMUI -> FMUI: Calculate time discrepancy
                end
                FMUI -> FMUC: processResult = Process() // Described above
                alt processResult == True
                    FMUI -> Master: True
                end
            else There are no messages
                server --> FMUI: False
            end
        end
    FMUI -> Master: False
