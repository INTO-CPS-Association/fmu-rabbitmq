Data Handling
==============

This page gives a more detailed description of initialisation and doStep of RMQFMU. The description is mainly carried out via diagrams, where some functions are referred to by name. Such functions are described in the section *Functions*.

Initialization
______________
The initialization process of RabbitMQ FMU is peculiar in the sense that it needs to have an output for all messages. For this reason, the initialization has to occur when :code:`fmi2EnterInitializationMode` is invoked on RabbitMQ FMU.

Functions
^^^^^^^^^
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
    FMUI -> FMUI: configure()
    FMUI -> server: Publish Ready message
    FMUI -> FMUI: initializeCoreState()
    FMUI -> FMUI: set StartTime = Time Now
        loop TimeNow - StartTime < communicationTimeOut
            FMUI -> server: ConsumeSingleMessage(&msg)
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
This section describes the dostep operation of RabbitMQ FMU and provides valuable insights into the operation of RabbitMQ FMU and the meaning of the Quality Attributes. First a few functions are explained before presenting the flow of a DoStep operation.

Functions
^^^^^^^^^^
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
                FMUI -> FMUC: processResult = Process() // Described above
                alt processResult == True
                    FMUI -> Master: True
                end
            else There are no messages
                server --> FMUI: False
            end
        end
    FMUI -> Master: False







  KJWAFAKJW

This section concerns data handling occurring within :code:`fmi2DoStep` of RabbitMQ FMU.

The process is the following:


All values from messages with :code:`timestampInSimulationTime` within the time interval: :code:`]currentSimlationTime,currentSimulationTime+simulationStepSize]`
are considered in a step from :code:`currentSimlationTime` to :code:`currentSimulationTime+simulationStepSize`. If a value from a message has a newer timestamp than the value stored in the RabbitMQ FMU Output state, then the output state is updated, thus using *zero-order hold*.

.. uml::

    title Time Handling for a Single Message
    hide footbox

    actor User
    database "RabbitMQ Server" as server
    boundary "Execution of Co-Simulation" as exec
    participant RabbitMQFMU as FMU

    User -> exec: Start

    == Initialization ==
    exec -> FMU: EnterInitializationMode()"
    FMU -> server: createTopicExchangeBindingAndQueue
    ... RabbitMQ FMU Blocks execution until data is available in the topic ...
    server -> FMU: "Publish message with timestamp of WCT1"
    note over FMU: Rabbit FMU unblocks and sets Simulation Time 0 = WCT1.

    == Simulation Loop until endtime ==
    exec -> FMU: Fmi2DoStep(currentSimulationTime, simulaionStepSize)
        loop FMU process messages until messageTimeStampInSimulationTime >= currentSimulationTime + simulationStepSize
        server -> FMU: Message
        alt messageTimeStampInSimulationTime <= currentSimulationTime+simulationStepSize
            FMU -> FMU: Update output values with values from message
        else messageTimeStampInSimulationTime > currentSimulationTime + simulationStepSize
            FMU -> FMU: Store message for usage in subsequent DoStep operation.

IncomingUnprocessed (map from ScalarVariableId -> List of TimeStampedMessages)
    When a message is read by RabbitMQ FMU it goes into IncomingUnprocessed

IncomingLookahead (map from ScalarVariableId -> List of TimeStampedMessages)
    IncomingLookahead contains x messages moved from IncomingUnprocessed, where x is according to Lookahead. IncomingLookahead is always sorted according to time.

Lookahead
    Lookahead determines the maximum amount of messages per message type that should be read from IncomingUnprocessed at each DoStep.

CurrentData (map from ScalarVariableId -> TimeStampedMessage)
    Holds the current outputs

MaxAge
    A measure of how long time a message is valid in milliseconds.

StartOffsetTime

Precision
    The parameter precision is related to the communication step size passed to the function :code:`fmi2DoStep` of RabbitMQ FMU.
    It describes the number of decimals to consider.

Communication Timeout

SimulationTime
    End time of a step (currentCommunicationPoint + communicationStepSize). Internal time in RabbitMQ FMU is milliseconds.

Ready Message

