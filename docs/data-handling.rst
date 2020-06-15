Data Handling
==============

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

SimulatinoTime
    End time of a step (currentCommunicationPoint + communicationStepSize). Internal time in RabbitMQ FMU is milliseconds.



Functions
---------
check
    Returns false if any of the following cases occur for a message\: msg in currentData:
    - Missing in currentData
    - CurrentData holds a future value\: msg.time > simulationTime
    - CurrentData holds an expired value\: (msg.time + maxAge) < simulationTime

ProcessIncoming
    - Move lookahead amount of messages per message type from IncomingUnprocessed to IncomingLookahead
    - Sort lookahead according to time
    - Delete messages in IncomingUnprocessed

ProcessLookahead
    - Move value from incomingLookahead to currentData if <= simulationTime. Otherwise keep in IncomingLookahead

.. uml::
    title DoStep operation
    hide footbox

    boundary "Co-simulation Master" as Master
    participant RabbitMQFMUInterface as FMUI
    participant RabbitMQFMUCore as FMUC

    Master -> FMUI: doStep(currentCommunicationTime, communicationStepSize)
    FMUI -> FMUI: simulationTime = applyPrecision(currentCommunicationTime+communicationStepSize)
    FMUI -> FMUC: process(simulationTime)
    FMUC -> FMUC: check()
    FMUC -> FMUC: ProcessIncoming()
    FMUC -> FMUC: ProcessLookahead()
    FMUC -> FMUC: processResult = check()
    FMUC --> FMUI: processResult
    FMUI -> FMUI: StartTime = Time Now
        loop TimeNow - StartTime < communicationTimeOut
            FMUI -> Server: ConsumeSingleMessage(&msg)
            alt There is a message
                Server -> Server: msg = message
                Server --> FMUI: True
                FMUI -> FMUC: AddToIncomingUnprocessed(msg)
            else There are no messages
                Server --> FMUI: False
            FMUI -> FMUC: processResult = Process() // Described above
            alt processResult == True
            FMUI -> Master: True
    FMUI -> Master: False
